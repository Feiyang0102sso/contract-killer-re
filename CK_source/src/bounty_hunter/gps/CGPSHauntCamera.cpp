#include "CGPSHauntCamera.h"
#include "CGPSInformantMisson.h"

#include "App.h"
#include "CBH_GPSMap.h"
#include "CBH_Player.h"
#include "CGPSBounty.h"
#include "CGPSInstantInformant.h"
#include "CDH_Weapon.h"
#include "CDH_WeaponsManager.h"
#include "CDH_SoundManager.h"
#include "CFont.h"
#include "CFontMgr.h"
#include "CGameAnalytics.h"
#include "CGPSMission.h"
#include "CGPSSubContract.h"
#include "CRandGen.h"
#include "CTutorialManager.h"
#include "CSubContractManager.h"
#include "CXmlHelper.h"
#include "DGHelper.h"
#include "ResMap.h"
// Header statics of the original unit (static initializer: banner tables, then a class id).
#include "AdBannerSizes.h"
#include "CNGSOfferManager.h"

CGPSHauntCamera::CGPSHauntCamera(XString& hauntName, TiXmlNode* node)
    : m_ambientSound(NULL), m_haunt(hauntName), m_locked(true), m_minWeaponRange(0),
      m_field6c(false), m_blueBounty(false), m_tempMission(false), m_tempTargettingMission(false),
      m_targettingMission(false), m_gatedMissionId(-1), m_field78(false), m_gatedMission(false),
      m_listener(NULL), m_temporaryItem(NULL), m_item(NULL), m_horizontalLimit(90.0f), m_verticalLimit(45.0f),
      m_introTimeMs(-1), m_durationMs(-1), m_additionalMinionCount(0),
      m_subContract(NULL), m_spawnedSubContracts(0), m_state(0)
{
    ParseNode(node);
    m_font = CFontMgr::GetInstance()->GetFont(GAME_FONT_DEFAULT);
    XString format = Window::ResString("IDS_GPS_CAMERA_LEVEL");
    m_levelText = XString::Format(format.c_str(), m_unlockLevel);
    m_levelTextWidth = 0;
    if (m_font != NULL) {
        m_levelTextWidth = m_font->MeasureTextWidth(m_levelText.c_str(), -1, -1, NULL);
    }
    SetBlinkingImage("IDB_GPS_SQUARE_ICON_ANIM_TUTORIAL");
    SetPressImage("IDB_GPS_SQUARE_ICON_ANIM");
    UpdateState();
}

CGPSHauntCamera::~CGPSHauntCamera()
{
    delete m_subContract;
    delete m_temporaryItem;
    delete m_item;
}

void CGPSHauntCamera::ParseNode(TiXmlNode* node)
{
    m_id = CXmlHelper::GetAttributeValue(node, "id").ToInt();
    ParseInfo(node->FirstChild("info"));
}

void CGPSHauntCamera::ParseInfo(TiXmlNode* node)
{
    m_unlockLevel = CXmlHelper::GetAttributeValue(node, "unlock_level").ToInt();
    m_location = CXmlHelper::GetAttributeValue(node, "location_name");
    App* app = static_cast<App*>(WindowApp::m_instance);
    m_ambientSound = app->InitResMap()->getResourceID(CXmlHelper::GetAttributeValue(node, "ambientSFX"));
    m_defaultImage = CXmlHelper::GetAttributeValue(node, "image");
    SetImage(XString::AnsiString(m_defaultImage.c_str()));
    int x = CXmlHelper::GetAttributeValue(node, "posX").ToInt();
    int y = CXmlHelper::GetAttributeValue(node, "posY").ToInt();
    SetPos(x, y);
    m_cameraIndex = CXmlHelper::GetAttributeValue(node, "camera").ToInt();
    m_horizontalLimit = CXmlHelper::GetAttributeValueOrUseDefault(node, "maxCameraHorzAngle", XString("45.0")).ToFloat();
    m_verticalLimit = CXmlHelper::GetAttributeValueOrUseDefault(node, "maxCameraVertAngle", XString("45.0")).ToFloat();
    m_introTimeMs = CXmlHelper::GetAttributeValueOrUseDefault(node, "movingCameraIntroTimeMS", XString("-1")).ToInt();
    m_durationMs = CXmlHelper::GetAttributeValueOrUseDefault(node, "movingCameraTotalTimeMS", XString("-1")).ToInt();
    m_minWeaponRange = CXmlHelper::GetAttributeValueOptional(node, "minimalWeaponRange").ToFloat();
    m_hudModel = CXmlHelper::GetAttributeValueOptional(node, "cameraHUDModel3D");
    m_vehicle = CXmlHelper::GetAttributeValueOptional(node, "vehicleName");
    m_additionalMinionCount = CXmlHelper::GetAttributeValueOptional(node, "additionalMinionCount").ToInt();
    ParseWeaponGatedInfo(node);
}

void CGPSHauntCamera::ParseWeaponGatedInfo(TiXmlNode* node)
{
    m_gatedWeaponCategory = CXmlHelper::GetAttributeValueOrUseDefault(node, "weaponGatedCategory", XString("handgun"));
    Vector<XString> tokens;
    CXmlHelper::GetAttributeValueOrUseDefault(node, "gatedWeaponID", XString("-1")).Split(tokens, L",");
    m_gatedWeapons.ensureCapacity(tokens.size());
    for (int index = 0; index < tokens.size(); ++index) {
        XString value = tokens[index].Trim(true, true);
        if (value.Length() != 0) {
            m_gatedWeapons.addElement(value.ToInt());
        }
    }
    tokens.removeAllElements();
    CXmlHelper::GetAttributeValueOptional(node, "randomWeapons").Split(tokens, L",");
    for (int index = 0; index < tokens.size(); ++index) {
        XString value = tokens[index].Trim(true, true);
        if (value.Length() != 0) {
            m_randomWeapons.addElement(value.ToInt());
        }
    }
    if (m_randomWeapons.size() == 0) {
        const int defaults[] = {37, 42, 35, 34, 25, 16};
        for (unsigned int index = 0; index < sizeof(defaults) / sizeof(defaults[0]); ++index) {
            m_randomWeapons.addElement(defaults[index]);
        }
    }
}

int CGPSHauntCamera::GetGatedWeaponID() const
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    int level = CBH_Player::GetInstance()->GetLevel();
    Vector<int> candidates;
    candidates.ensureCapacity(m_gatedWeapons.size());
    for (int index = 0; index < m_gatedWeapons.size(); ++index) {
        int id = m_gatedWeapons[index];
        CDH_Weapon* weapon = app->InitWeaponManager()->GetWeaponById(id);
        if (weapon != NULL && weapon->m_unlockLevel <= level) {
            candidates.addElement(id);
        }
    }
    if (candidates.size() == 0) {
        return -1;
    }
    if (candidates.size() == 1) {
        return candidates[0];
    }
    int index = CRandGen::GetInstance()->GetRandRange(0, candidates.size() - 1);
    return candidates[index];
}

void CGPSHauntCamera::SetCameraListener(CGPSCameraListener* listener)
{
    m_listener = listener;
}

void CGPSHauntCamera::OnClick()
{
    if (m_listener != NULL) {
        m_listener->OnCameraSelected(this);
    }
}

bool CGPSHauntCamera::IsAvailable()
{
    return !m_locked && m_item == NULL;
}

bool CGPSHauntCamera::IsLockedAvailable()
{
    return m_locked && m_item == NULL;
}

bool CGPSHauntCamera::HasMission()
{
    return m_item != NULL && m_item->GetType() == 0;
}

bool CGPSHauntCamera::HasBlueBounty()
{
    if (m_item == NULL) {
        return false;
    }
    return m_item->GetType() == 1 && m_blueBounty;
}

bool CGPSHauntCamera::HasTempMission()
{
    return m_temporaryItem != NULL;
}

bool CGPSHauntCamera::HasTargettingMission()
{
    if (m_tempMission) {
        return m_tempTargettingMission;
    }
    return m_targettingMission;
}

bool CGPSHauntCamera::HasHuntingItem()
{
    return m_item != NULL || m_temporaryItem != NULL;
}

CGPSHuntingItem* CGPSHauntCamera::GetGPSHuntingItem()
{
    return m_item;
}

void CGPSHauntCamera::SetMissionOnCamera(int missionId, CGPSMissionInformant* informant)
{
    StopSubContracts();
    delete m_item;
    m_item = new CGPSMission(this, missionId);
    m_activeImage = XString("IDB_CAMERA_STORY_MISSION");
    if (informant != NULL && informant->m_cameraImage.Length() != 0) {
        m_activeImage = informant->m_cameraImage;
    }
    UpdateState();
    StartNextSubContract();
}

void CGPSHauntCamera::SetBountyOnCamera(CGPSInstantInformant* informant, bool replace, bool temporary)
{
    if (informant == NULL || (!temporary && !replace && m_item != NULL)) {
        return;
    }
    CGPSBounty* bounty = new CGPSBounty(this, informant->GetRandomBountyType(), informant);
    if (bounty->m_boss == NULL) {
        delete bounty;
        return;
    }
    if (temporary) {
        delete m_temporaryItem;
        m_temporaryItem = bounty;
        m_tempMission = true;
    } else {
        StopSubContracts();
        delete m_item;
        m_item = bounty;
        m_blueBounty = informant->m_type == CGPSInstantInformant::Blue;
        if (!bounty->m_weaponGated && m_durationMs < 1 && CRandGen::GetInstance()->GetRandRange(0, 99) < 15) {
            App* app = static_cast<App*>(WindowApp::m_instance);
            Vector<int> candidates;
            for (int index = 0; index < m_randomWeapons.size(); ++index) {
                int id = m_randomWeapons[index];
                CDH_Weapon* weapon = app->InitWeaponManager()->GetWeaponById(id);
                if (weapon != NULL && !weapon->isAvailable()) {
                    candidates.addElement(id);
                }
            }
            if (candidates.size() > 0) {
                bounty->m_randomWeapon = candidates[CRandGen::GetInstance()->GetRand(candidates.size())];
                bounty->m_messageType = BountyMessageRandomWeapon;
                bounty->m_bossFirst = false;
                bounty->m_bossWins = false;
                bounty->m_tranquillize = false;
                bounty->m_bossSpawnsLast = false;
                bounty->m_silentKill = false;
            }
        }
    }
    m_activeImage = informant->m_cameraImage;
    UpdateState();
    if (!temporary) {
        StartNextSubContract();
    }
}

void CGPSHauntCamera::SetGatedMissionOnCamera(CGPSInstantInformant* informant, int missionId, bool)
{
    m_gatedMissionId = missionId;
    m_gatedMission = true;
    SetBountyOnCamera(informant, false, false);
}

void CGPSHauntCamera::SetTargettingMissionOnCamera(CGPSInstantInformant* informant, int friendId, bool revenge, bool temporary)
{
    CGPSHuntingItem** slot = &m_item;
    if (temporary) {
        slot = &m_temporaryItem;
    }
    if (*slot != NULL) {
        return;
    }
    SetBountyOnCamera(informant, false, temporary);
    CGPSHuntingItem* item = *slot;
    if (item == NULL) {
        return;
    }
    if (!item->SetTargetFriendId(friendId)) {
        if (temporary) {
            m_tempMission = false;
        } else {
            StopSubContracts();
        }
        delete item;
        *slot = NULL;
        UpdateState();
        return;
    }
    if (temporary) {
        m_tempTargettingMission = true;
    } else {
        m_targettingMission = true;
    }
    item->m_revenge = revenge;
    App* app = static_cast<App*>(WindowApp::m_instance);
    int bountyType = static_cast<CGPSBounty*>(item)->m_bountyType;
    const CBountyManager::SBountyGeneralInfo* bounty = app->m_bountyManager->GetInfo(bountyType);
    if (bounty != NULL) {
        if (revenge) {
            CGameAnalytics::logFriendTargetRevenge(friendId, bounty->m_name);
        } else {
            CGameAnalytics::logFriendTargetMission(friendId, bounty->m_name);
        }
    }
}

void CGPSHauntCamera::OnCameraBountyComplete()
{
    if (m_temporaryItem != NULL) {
        delete m_temporaryItem;
        m_temporaryItem = NULL;
        m_tempMission = false;
        m_tempTargettingMission = false;
    } else if (m_item != NULL) {
        StopSubContracts();
        if (m_item->GetType() == 0) {
            m_locked = false;
        }
        delete m_item;
        m_item = NULL;
        m_blueBounty = false;
        m_gatedMission = false;
    }
    m_targettingMission = false;
    UpdateState();
}

void CGPSHauntCamera::Update(long elapsed)
{
    if (m_subContract == NULL) {
        return;
    }
    m_subContract->Update(elapsed);
    if (m_subContract->m_flying || m_subContract->GetPressed()) {
        return;
    }
    unsigned int disappear = m_subContract->GetDisappearTime();
    if (disappear == 0) {
        App* app = static_cast<App*>(WindowApp::m_instance);
        app->m_soundManager->PlaySound("IDM_SUBCONTRACTING_COLLECT", false, false);
        StartNextSubContract();
    } else if (App::GetTime() > disappear) {
        DisappearSubContract();
    } else {
        return;
    }
    if (CBH_GPSMap::m_instance != NULL) {
        CBH_GPSMap::m_instance->Save();
    }
}

void CGPSHauntCamera::StartNextSubContract()
{
    DisappearSubContract();
    App* app = static_cast<App*>(WindowApp::m_instance);
    CSubContractManager* manager = app->InitSubContractManager();
    if (m_item == NULL || m_spawnedSubContracts >= static_cast<unsigned int>(manager->m_contracts.size())) {
        return;
    }
    const SSubContractInfo& info = manager->m_contracts[m_spawnedSubContracts];
    unsigned int activates = App::GetTime() + info.m_appearSeconds;
    m_subContract = new CGPSSubContract(this, m_spawnedSubContracts);
    m_subContract->Init(activates, activates + info.m_availableSeconds, info.m_rewardFactor);
    if (m_listener != NULL) {
        m_listener->PlaceSubContract(m_subContract, manager->m_appearRadius);
    } else {
        m_subContract->SetScaledPos(GetX() + GetWidth(), GetY());
    }
    ++m_spawnedSubContracts;
}

void CGPSHauntCamera::DisappearSubContract()
{
    delete m_subContract;
    m_subContract = NULL;
}

void CGPSHauntCamera::StopSubContracts()
{
    DisappearSubContract();
    m_spawnedSubContracts = 0;
}

void CGPSHauntCamera::UpdateIcon()
{
    if (m_state == 5) {
        SetImage("IDB_CAMERA_OPEN");
    } else if (m_state == 3 || m_state == 4) {
        if (m_activeImage.Length() != 0) {
            SetImage(XString::AnsiString(m_activeImage.c_str()));
        } else {
            SetImage("IDB_CAMERA_STORY_MISSION");
        }
    } else if (m_state <= 2) {
        SetImage("IDB_CAMERA_UNLOCKED");
    }
}

void CGPSHauntCamera::UpdateState()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    int level = CBH_Player::GetInstance()->GetLevel();
    m_visible = true;
    SetBlinking(false);
    if (m_temporaryItem != NULL) {
        m_state = 3;
    } else if (m_item != NULL) {
        m_state = 4;
        if (m_item->GetType() == 1) {
            m_state = 3;
        }
        SetBlinking(CTutorialManager::IsPlayerInTutorial());
    } else if (!m_locked) {
        m_state = 5;
    } else if (level >= m_unlockLevel) {
        m_state = 2;
    } else {
        int threshold = app->m_helper->m_config.GetIntegerValueByToken("LEVEL_DIFF_CAMERA_BECOME_VISIBLE", true);
        m_visible = m_unlockLevel - level <= threshold;
        m_state = 1;
        if (!m_visible) {
            m_state = 0;
        }
    }
    UpdateIcon();
}

void CGPSHauntCamera::SetBlinking(bool blinking)
{
    if (m_blinking != blinking) {
        m_blinking = blinking;
    }
}

void CGPSHauntCamera::SetPressed(bool pressed)
{
    if (!pressed || static_cast<unsigned int>(m_state - 3) < 2) {
        CGPSItem::SetPressed(pressed);
    }
}

CRectangle CGPSHauntCamera::GetRect() const
{
    return CGPSItem::GetRect();
}

bool CGPSHauntCamera::CheckHit(int x, int y, CGPSItem** item)
{
    if (m_visible && m_subContract != NULL && m_subContract->CheckHit(x, y, item)) {
        return true;
    }
    return CGPSItem::CheckHit(x, y, item);
}

void CGPSHauntCamera::Paint(ICGraphics2d& graphics, int mapX, int mapY)
{
    if (!m_visible) {
        return;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    int x = mapX + GetX();
    int y = mapY + GetY();
    if (m_image != NULL) {
        app->m_helper->DrawImage(m_image, x, y, 1.0f, 0, 0, 0.0f, 0);
    }
    if (m_locked && m_font != NULL && m_state == 1) {
        m_font->PaintText(m_levelText.c_str(), -1, x + (m_width - m_levelTextWidth) / 2,
                          y + m_height, m_levelTextWidth, m_font->GetFontHeight());
    }
    CGPSItem::Paint(graphics, mapX, mapY);
    if (m_subContract != NULL) {
        m_subContract->Paint(graphics, mapX, mapY);
    }
}

void CGPSHauntCamera::Save(TiXmlElement* parent)
{
    TiXmlElement* camera = parent->FirstChildElement("camera");
    while (camera != NULL && CXmlHelper::GetAttributeValue(camera, "id").ToInt() != m_id) {
        camera = camera->NextSiblingElement("camera");
    }
    if (camera == NULL) {
        camera = new TiXmlElement("camera");
        camera->SetAttribute("id", m_id);
        parent->LinkEndChild(camera);
    }
    camera->SetAttribute("locked", m_locked);
    camera->SetAttribute("isTempMission", m_tempMission);
    camera->SetAttribute("isBlueBounty", m_blueBounty);
    camera->SetAttribute("isTargettingMission", m_targettingMission);
    camera->SetAttribute("isTempTargettingMission", m_tempTargettingMission);
    camera->SetAttribute("isGatedMission", m_gatedMission);
    camera->SetAttribute("gatedMissionId", m_gatedMissionId);
    camera->SetAttribute("cameraImageRes", XString::AnsiString(m_activeImage.c_str()));
    camera->SetAttribute("weaponGatedCategory", XString::AnsiString(m_gatedWeaponCategory.c_str()));
    TiXmlElement* savedItem = camera->FirstChildElement("huntingItem");
    if (savedItem != NULL) {
        int type = CXmlHelper::GetIntAttributeValueOrUseDefault(savedItem, "itemType", -1);
        if (type == 0 || type == 1) {
            camera->RemoveChild(savedItem);
            savedItem = NULL;
        }
    }
    TiXmlElement* temporary = camera->FirstChildElement("tempMission");
    if (m_temporaryItem != NULL) {
        if (temporary == NULL) {
            temporary = new TiXmlElement("tempMission");
            camera->LinkEndChild(temporary);
        }
        temporary->SetAttribute("targetUserId", m_temporaryItem->m_targetFriendId);
        temporary->SetAttribute("isRevengeMode", m_temporaryItem->m_revenge);
        TiXmlElement* previous = temporary->FirstChildElement("huntingItem");
        if (previous != NULL) {
            temporary->RemoveChild(previous);
        }
        m_temporaryItem->Save(temporary);
    } else if (temporary != NULL && temporary->FirstChildElement("huntingItem") != NULL) {
        camera->RemoveChild(temporary);
    }
    if (m_item != NULL && savedItem == NULL) {
        m_item->Save(camera);
    }
    SaveSubContracts(camera);
}

void CGPSHauntCamera::Load(TiXmlElement* parent)
{
    for (TiXmlElement* camera = parent->FirstChildElement("camera"); camera != NULL;
         camera = camera->NextSiblingElement("camera")) {
        if (CXmlHelper::GetAttributeValue(camera, "id").ToInt() == m_id) {
            m_locked = CXmlHelper::GetAttributeValue(camera, "locked").ToBool();
            m_tempMission = CXmlHelper::GetBoolAttributeValueOrUseDefault(camera, "isTempMission", false);
            m_blueBounty = CXmlHelper::GetBoolAttributeValueOrUseDefault(camera, "isBlueBounty", false);
            m_targettingMission = CXmlHelper::GetBoolAttributeValueOrUseDefault(camera, "isTargettingMission", false);
            m_tempTargettingMission = CXmlHelper::GetBoolAttributeValueOrUseDefault(camera, "isTempTargettingMission", false);
            m_gatedMission = CXmlHelper::GetBoolAttributeValueOrUseDefault(camera, "isGatedMission", false);
            m_gatedMissionId = CXmlHelper::GetIntAttributeValueOrUseDefault(camera, "gatedMissionId", -1);
            m_activeImage = CXmlHelper::GetAttributeValueOrUseDefault(camera, "cameraImageRes", XString("IDB_CAMERA_RANDOM_MISSION"));
            if (CXmlHelper::HasAttribute(camera, "weaponGatedCategory")) {
                m_gatedWeaponCategory = CXmlHelper::GetAttributeValue(camera, "weaponGatedCategory");
            }
            TiXmlElement* item = camera->FirstChildElement("huntingItem");
            delete m_item;
            m_item = NULL;
            if (item != NULL) {
                int type = CXmlHelper::GetIntAttributeValueOrUseDefault(item, "itemType", -1);
                if (type == 0) {
                    m_item = new CGPSMission(this, item);
                } else if (type == 1) {
                    m_item = new CGPSBounty(this, item);
                }
            }
            TiXmlElement* temporary = camera->FirstChildElement("tempMission");
            delete m_temporaryItem;
            m_temporaryItem = NULL;
            if (temporary != NULL && temporary->FirstChildElement("huntingItem") != NULL) {
                m_temporaryItem = new CGPSBounty(this, temporary->FirstChildElement("huntingItem"));
                m_temporaryItem->m_targetFriendId = CXmlHelper::GetIntAttributeValueOrUseDefault(temporary, "targetUserId", -1);
                m_temporaryItem->m_revenge = CXmlHelper::GetBoolAttributeValueOrUseDefault(temporary, "isRevengeMode", false);
            }
            LoadSubContracts(camera);
            break;
        }
    }
    UpdateState();
}

void CGPSHauntCamera::SaveSubContracts(TiXmlElement* element)
{
    element->SetAttribute("spawnedSubContractsCount", m_spawnedSubContracts);
    TiXmlElement* saved = element->FirstChildElement("subcontract");
    if (m_subContract != NULL) {
        if (saved == NULL) {
            saved = new TiXmlElement("subcontract");
            element->LinkEndChild(saved);
        }
        m_subContract->Save(saved);
    } else if (saved != NULL) {
        element->RemoveChild(saved);
    }
}

void CGPSHauntCamera::LoadSubContracts(TiXmlElement* element)
{
    DisappearSubContract();
    m_spawnedSubContracts = CXmlHelper::GetAttributeValueOptional(element, "spawnedSubContractsCount").ToInt();
    TiXmlElement* saved = element->FirstChildElement("subcontract");
    if (saved != NULL && m_item != NULL) {
        m_subContract = new CGPSSubContract(this);
        m_subContract->Init();
        m_subContract->Load(saved);
    }
}

typedef char AssertGPSHauntCameraSize[sizeof(CGPSHauntCamera) == 0xe4 ? 1 : -1];
