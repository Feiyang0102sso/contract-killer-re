#include "CGPSHauntCamera.h"
#include "CGPSInformantMisson.h"

#include "App.h"
#include "CBH_GPSMap.h"
#include "CBH_Player.h"
#include "CGPSBounty.h"
#include "CGPSInstantInformant.h"
#include "CDH_Weapon.h"
#include "CDH_WeaponsManager.h"
#include "CFont.h"
#include "CFontMgr.h"
#include "CGameAnalytics.h"
#include "CNGS.h"
#include "CNGSLocalUser.h"
#include "CNGSRemoteUser.h"
#include "CNGSRemoteUserList.h"
#include "CNGSSocialInterface.h"
#include "CNGS_Platform.h"
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
      m_blueBounty(false), m_tempMission(false), m_tempTargettingMission(false),
      m_targettingMission(false), m_gatedMissionId(-1), m_field78(false), m_gatedMission(false),
      m_temporaryItem(NULL), m_item(NULL), m_horizontalLimit(90.0f), m_verticalLimit(45.0f),
      m_introTimeMs(-1), m_durationMs(-1), m_additionalMinionCount(0),
      m_subContract(NULL), m_spawnedSubContracts(0), m_state(0)
{
    // Original 0x128174: the level text width comes from DGHelper::GetTextWidth.
    ParseNode(node);
    m_font = CFontMgr::GetInstance()->GetFont(GAME_FONT_DEFAULT);
    m_levelText = XString::Format(Window::ResString("IDS_GPS_CAMERA_LEVEL").c_str(), m_unlockLevel);
    m_levelTextWidth = static_cast<App*>(WindowApp::m_instance)->m_helper->GetTextWidth(
        XString::AnsiString(m_levelText.c_str()), m_font);
    SetBlinkingImage("IDB_GPS_SQUARE_ICON_ANIM_TUTORIAL");
    SetPressImage("IDB_GPS_SQUARE_ICON_ANIM");
    UpdateState();
}

CGPSHauntCamera::~CGPSHauntCamera()
{
    if (m_item != NULL) {
        delete m_item;
        m_item = NULL;
    }
    if (m_temporaryItem != NULL) {
        delete m_temporaryItem;
        m_temporaryItem = NULL;
    }
    if (m_subContract != NULL) {
        delete m_subContract;
        m_subContract = NULL;
    }
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
    XString ambientSound = CXmlHelper::GetAttributeValue(node, "ambientSFX");
    m_ambientSound = static_cast<App*>(WindowApp::m_instance)->InitResMap()->getResourceID(ambientSound);
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
    m_minWeaponRange = CXmlHelper::GetAttributeValueOrUseDefault(node, "minimalWeaponRange", XString("0.0")).ToFloat();
    m_hudModel = CXmlHelper::GetAttributeValueOrUseDefault(node, "cameraHUDModel3D", XString(""));
    m_vehicle = CXmlHelper::GetAttributeValueOrUseDefault(node, "vehicleName", XString(""));
    m_additionalMinionCount = CXmlHelper::GetAttributeValueOrUseDefault(node, "additionalMinionCount", XString("0")).ToInt();
    ParseWeaponGatedInfo(node);
}

void CGPSHauntCamera::ParseWeaponGatedInfo(TiXmlNode* node)
{
    m_gatedWeaponCategory = CXmlHelper::GetAttributeValueOrUseDefault(node, "weaponGatedCategory", XString("handgun"));
    XString gatedIds = CXmlHelper::GetAttributeValueOrUseDefault(node, "gatedWeaponID", XString("-1"));
    Vector<XString> gatedTokens;
    gatedIds.Split(gatedTokens, L",");
    int gatedCount = gatedTokens.size();
    if (gatedCount > 0) {
        m_gatedWeapons.ensureCapacity(gatedCount);
        for (int index = 0; index < gatedCount; ++index) {
            XString value = gatedTokens[index].Trim(true, true);
            if (value.ByteSize() != 0) {
                m_gatedWeapons.addElement(value.ToInt());
            }
        }
    }

    XString randomIds = CXmlHelper::GetAttributeValueOrUseDefault(node, "randomWeapons", XString(""));
    Vector<XString> randomTokens;
    randomIds.Split(randomTokens, L",");
    for (int index = 0; index < randomTokens.size(); ++index) {
        XString value = randomTokens[index].Trim(true, true);
        if (value.ByteSize() != 0) {
            m_randomWeapons.addElement(value.ToInt());
        }
    }
    if (m_randomWeapons.size() == 0) {
        m_randomWeapons.addElement(37);
        m_randomWeapons.addElement(42);
        m_randomWeapons.addElement(35);
        m_randomWeapons.addElement(34);
        m_randomWeapons.addElement(25);
        m_randomWeapons.addElement(16);
    }
}

int CGPSHauntCamera::GetGatedWeaponID() const
{
    int count = m_gatedWeapons.size();
    if (count < 1) {
        return -1;
    }
    Vector<int> candidates;
    candidates.ensureCapacity(count);
    for (int index = 0; index < count; ++index) {
        int id = m_gatedWeapons[index];
        CDH_Weapon* weapon = static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->GetWeaponById(id);
        if (weapon != NULL) {
            int unlockLevel = weapon->m_unlockLevel;
            if (unlockLevel <= CBH_Player::GetInstance()->GetLevel()) {
                candidates.addElement(id);
            }
        }
    }
    if (candidates.size() < 1) {
        return -1;
    }
    if (candidates.size() < 2) {
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
    // the original returns the temporary-mission flag
    return m_tempMission;
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

// Original 0x128d44: the old item is deleted without stopping the sub contracts; the mission is
// logged to analytics.
void CGPSHauntCamera::SetMissionOnCamera(int missionId, CGPSMissionInformant* informant)
{
    if (m_item != NULL) {
        delete m_item;
        m_item = NULL;
    }
    m_item = new CGPSMission(this, missionId);
    if (informant != NULL) {
        m_activeImage = informant->m_cameraImage;
    } else {
        m_activeImage = XString("IDB_CAMERA_STORY_MISSION");
    }
    UpdateState();
    int source = 0;
    if (informant != NULL) {
        source = informant->m_id;
    }
    StartNextSubContract();
    CGameAnalytics::logMissionReceived(
        XString::Format(L"%02d-%03d-%02d", CBH_Player::GetInstance()->GetLevel(), m_item->GetAAId(), m_id), source);
}

// Original 0x128ecc.
void CGPSHauntCamera::SetBountyOnCamera(CGPSInstantInformant* informant, bool replace, bool temporary)
{
    int bountyType = informant->GetRandomBountyType();
    CGPSBounty* bounty;
    if (temporary) {
        // the old temporary item is not deleted (original)
        bounty = new CGPSBounty(this, bountyType, informant);
        m_temporaryItem = bounty;
        m_tempMission = true;
    } else {
        if (m_item != NULL && !replace) {
            return;
        }
        if (m_item != NULL) {
            delete m_item;
            m_item = NULL;
        }
        bounty = new CGPSBounty(this, bountyType, informant);
        m_item = bounty;
        if (bounty != NULL && !bounty->m_weaponGated) {
            if (m_durationMs < 1 &&
                static_cast<unsigned int>(CRandGen::GetInstance()->GetRandRange(0, 99)) < 15) {
                // Draw without replacement until an unowned weapon ID is found.
                Vector<CDH_Weapon*> available;
                static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->GetAvailableWeapons(available);
                Vector<int> candidates(m_randomWeapons);
                while (candidates.size() != 0) {
                    int index = CRandGen::GetInstance()->GetRand(candidates.size());
                    int id = candidates[index];
                    candidates.removeElementAt(index);
                    int ownedIndex = 0;
                    for (; ownedIndex < available.size(); ++ownedIndex) {
                        if (available[ownedIndex]->GetId() == id) {
                            break;
                        }
                    }
                    if (ownedIndex >= available.size() && id != -1) {
                        bounty->m_randomWeapon = id;
                        bounty->m_messageType = BountyMessageRandomWeapon;
                        bounty->m_bossFirst = false;
                        bounty->m_bossWins = false;
                        bounty->m_tranquillize = false;
                        bounty->m_bossSpawnsLast = false;
                        bounty->m_silentKill = false;
                        break;
                    }
                }
            }
        }
        if (bounty != NULL && !bounty->m_weaponGated && !bounty->m_silentKill &&
            CRandGen::GetInstance()->GetRandRange(0, 3) == 1) {
            Vector<int> nonPlayers;
            CNGSSocialInterface* facebook = CNGS_Platform::GetInstance()->getSocialInterface(SOCIAL_INTERFACE_FACEBOOK);
            CNGSRemoteUserList* players = CNGS::GetInstance()->GetLocalUser()->m_remoteUserLists[0];
            for (int index = 0; index < facebook->getFriendsCount(); ++index) {
                CStrWChar friendName = facebook->getFriendNameFromSortedListAtIndex(index);
                CStrWChar friendId = facebook->getFriendIDFromSortedListAtIndex(index);
                int playerIndex = 0;
                for (; playerIndex < players->m_users.Size(); ++playerIndex) {
                    CNGSRemoteUser* player = players->m_users[playerIndex];
                    if (player->IsFromSocialNetwork(SOCIAL_INTERFACE_FACEBOOK) &&
                        friendId == player->GetIDForSocialNetwork(SOCIAL_NETWORK_FACEBOOK)) {
                        break;
                    }
                }
                if (playerIndex >= players->m_users.Size()) {
                    nonPlayers.addElement(index);
                }
            }
            if (nonPlayers.size() > 0) {
                int index = CRandGen::GetInstance()->GetRandRange(0, nonPlayers.size() - 1);
                CStrWChar friendId = facebook->getFriendIDFromSortedListAtIndex(nonPlayers[index]);
                bounty->SetTargetFBUser(friendId);
                CGameAnalytics::logRandomFriendTargetSet(XString(friendId.GetWCharPtr()), bountyType);
            }
        }
    }
    m_blueBounty = informant->m_type == CGPSInstantInformant::Blue;
    int source = 0;
    if (informant != NULL) {
        m_activeImage = informant->m_cameraImage;
        UpdateState();
        StartNextSubContract();
        source = informant->m_id;
    } else {
        UpdateState();
        StartNextSubContract();
    }
    int level = CBH_Player::GetInstance()->GetLevel();
    int bountyId = static_cast<CGPSHuntingItem*>(bounty)->GetAAId();
    CGameAnalytics::logMissionReceived(XString::Format(L"%02d-%03d-%02d", level, bountyId, m_id), source);
}

void CGPSHauntCamera::SetGatedMissionOnCamera(CGPSInstantInformant* informant, int missionId, bool)
{
    m_gatedMissionId = missionId;
    m_gatedMission = true;
    SetBountyOnCamera(informant, false, false);
}

// Original 0x129d94: an item whose friend cannot be set is deleted (sub contracts and state are
// left alone).
void CGPSHauntCamera::SetTargettingMissionOnCamera(CGPSInstantInformant* informant, int friendId, bool revenge, bool temporary)
{
    SetBountyOnCamera(informant, false, temporary);
    CGPSHuntingItem** slot;
    CGPSHuntingItem* item;
    if (temporary != true) {
        goto targetPermanentItem;
    }
    slot = &m_temporaryItem;
    if (m_temporaryItem->SetTargetFriendId(friendId) != true) {
        goto discardItem;
    }
    m_tempTargettingMission = true;
    item = m_temporaryItem;

logTarget:
    item->m_revenge = revenge;
    {
        CBountyManager::SBountyGeneralInfo info =
            *static_cast<App*>(WindowApp::m_instance)->m_bountyManager->GetInfo(static_cast<CGPSBounty*>(*slot)->m_bountyType);
        XString name = info.m_name;
        if (revenge) {
            CGameAnalytics::logFriendTargetRevenge(friendId, name);
        } else {
            CGameAnalytics::logFriendTargetMission(friendId, name);
        }
    }
    return;

discardItem:
    if (*slot != NULL) {
        delete *slot;
        *slot = NULL;
    }
    return;

targetPermanentItem:
    slot = &m_item;
    if (m_item->SetTargetFriendId(friendId) != true) {
        goto discardItem;
    }
    m_targettingMission = true;
    item = m_item;
    goto logTarget;
}

void CGPSHauntCamera::OnCameraBountyComplete()
{
    // Original 0x128464: decided by the temp mission flag; m_item is not NULL checked.
    if (m_tempMission) {
        if (m_temporaryItem != NULL) {
            delete m_temporaryItem;
            m_temporaryItem = NULL;
        }
        m_tempMission = false;
        m_targettingMission = false;
    } else {
        StopSubContracts();
        if (m_item->GetType() == 0) {
            m_locked = false;
        }
        if (m_item != NULL) {
            delete m_item;
            m_item = NULL;
        }
        m_item = NULL;
        m_blueBounty = false;
        m_tempMission = false;
        m_targettingMission = false;
        m_gatedMission = false;
    }
    UpdateState();
}

void CGPSHauntCamera::Update(long elapsed)
{
    // Original 0x1283e4
    if (m_subContract != NULL) {
        m_subContract->Update(elapsed);
    }
    if (m_item == NULL) {
        if (m_subContract == NULL) {
            return;
        }
        DisappearSubContract();
        if (m_item == NULL) {
            return;
        }
    }
    if (m_item->IsNeedToBeDeleted() == true) {
        if (CBH_GPSMap::m_instance != NULL) {
            CBH_GPSMap::m_instance->CancelInstantInformantWithCamera(m_id);
        }
        OnCameraBountyComplete();
    }
}

void CGPSHauntCamera::StartNextSubContract()
{
    // Original 0x128e0c: no item / listener checks.
    if (m_subContract != NULL) {
        delete m_subContract;
        m_subContract = NULL;
    }
    if (m_spawnedSubContracts < static_cast<unsigned int>(static_cast<App*>(WindowApp::m_instance)->m_subContractManager->m_contracts.size())) {
        m_subContract = new CGPSSubContract(this, m_spawnedSubContracts);
        const SSubContractInfo& info = static_cast<App*>(WindowApp::m_instance)->m_subContractManager->m_contracts[m_spawnedSubContracts];
        int appearSeconds = info.m_appearSeconds;
        int availableSeconds = info.m_availableSeconds;
        float rewardFactor = info.m_rewardFactor;
        unsigned int activates = App::GetTime() + appearSeconds;
        m_subContract->Init(activates, activates + availableSeconds, rewardFactor);
        m_listener->PlaceSubContract(m_subContract, static_cast<App*>(WindowApp::m_instance)->m_subContractManager->m_appearRadius);
        ++m_spawnedSubContracts;
    }
    m_listener->NeedSave();
}

void CGPSHauntCamera::DisappearSubContract()
{
    if (m_subContract != NULL) {
        delete m_subContract;
        m_subContract = NULL;
        // the original does not check the listener
        m_listener->NeedSave();
    }
}

void CGPSHauntCamera::StopSubContracts()
{
    if (m_subContract != NULL) {
        delete m_subContract;
        m_subContract = NULL;
    }
    m_spawnedSubContracts = 0;
    // the original does not check the listener
    m_listener->NeedSave();
}

/// Original 0x129bd8
void CGPSHauntCamera::UpdateIcon()
{
    switch (m_state) {
    case 0:
    case 1:
    case 2:
        SetImage("IDB_CAMERA_UNLOCKED");
        break;
    case 5:
        SetImage("IDB_CAMERA_OPEN");
        break;
    case 3:
    case 4:
        if (m_activeImage.ByteSize() != 0) {
            SetImage(XString::AnsiString(m_activeImage.c_str()));
        } else {
            SetImage("IDB_CAMERA_STORY_MISSION");
        }
        break;
    }
}

// Original 0x129c50: a bounty item gives state 3, a mission item state 4 (other item types keep
// the state); locked cameras far below the player level are hidden. The temporary item is not
// looked at here.
void CGPSHauntCamera::UpdateState()
{
    if (static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial() == true && m_locked
        && !static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsCameraVisible(m_id)) {
        m_state = 0;
        m_visible = false;
        return;
    }
    m_visible = true;
    SetBlinking(false);
    CBH_Player* player = CBH_Player::GetInstance();
    if (m_item == NULL) {
        if (m_locked) {
            int levelDiff = player->GetLevel() - m_unlockLevel;
            if (levelDiff >= 0) {
                m_state = 2;
            } else if (levelDiff >= -static_cast<App*>(WindowApp::m_instance)->m_helper->m_config.GetIntegerValueByToken(
                           "LEVEL_DIFF_CAMERA_BECOME_VISIBLE", true)) {
                m_state = 1;
            } else {
                m_state = 0;
                m_visible = false;
            }
        } else {
            m_state = 5;
        }
    } else if (m_item->GetType() == 1) {
        if (static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial() == true) {
            SetBlinking(true);
        }
        m_state = 3;
    } else if (m_item->GetType() == 0) {
        if (static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial() == true) {
            SetBlinking(true);
        }
        m_state = 4;
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
    // the original only asks the sub-contract itself
    if (m_subContract != NULL && m_subContract->CheckHit(x, y, item)) {
        return true;
    }
    return CGPSItem::CheckHit(x, y, item);
}

void CGPSHauntCamera::Paint(ICGraphics2d& graphics, int mapX, int mapY)
{
    // Original 0x12862c: hidden cameras (state 0) draw nothing; the sub contract is drawn before
    // the blink overlay; the level text (state 1) is centered on the bottom edge of the icon.
    if (m_state != 0) {
        static_cast<App*>(WindowApp::m_instance)->m_helper->DrawImage(
            GetImage(), GetX() + mapX, GetY() + mapY, 1.0f, 0, 0, 0.0f, 0);
        if (m_subContract != NULL) {
            m_subContract->Paint(graphics, mapX, mapY);
        }
        CGPSItem::Paint(graphics, mapX, mapY);
        if (m_state == 1) {
            m_font->PaintText(m_levelText.c_str(), -1,
                              GetX() + mapX + GetWidth() / 2 - m_levelTextWidth / 2,
                              GetY() + mapY + GetHeight() - m_font->GetFontHeight() / 2, -1, -1);
            CRectangle label(GetX() + GetWidth() / 2 - m_levelTextWidth / 2,
                             GetY() + GetHeight() - m_font->GetFontHeight() / 2,
                             m_levelTextWidth, m_font->GetFontHeight());
            setSubItem(0, label, mapX, mapY);
        }
    }
}

// Original 0x129498: a fresh camera element (with a fresh tempMission child) is copied into the
// parent and deleted (CBH_GPSMap::Save clears the map element first).
void CGPSHauntCamera::Save(TiXmlElement* parent)
{
    TiXmlElement* camera = new TiXmlElement("camera");
    camera->SetAttribute("id", m_id);
    camera->SetAttribute("locked", m_locked);
    camera->SetAttribute("isTempMission", m_tempMission);
    camera->SetAttribute("isBlueBounty", m_blueBounty);
    camera->SetAttribute("isTargettingMission", m_targettingMission);
    camera->SetAttribute("isTempTargettingMission", m_tempTargettingMission);
    camera->SetAttribute("isGatedMission", m_gatedMission);
    camera->SetAttribute("gatedMissionId", m_gatedMissionId);
    camera->SetAttribute("cameraImageRes", XString::AnsiString(m_activeImage.c_str()));
    camera->SetAttribute("weaponGatedCategory", XString::AnsiString(m_gatedWeaponCategory.c_str()));
    if (m_temporaryItem != NULL) {
        TiXmlElement* temporary = new TiXmlElement("tempMission");
        temporary->SetAttribute("targetUserId", m_temporaryItem->m_targetFriendId);
        temporary->SetAttribute("isRevengeMode", m_temporaryItem->m_revenge);
        m_temporaryItem->Save(temporary);
        camera->InsertEndChild(*temporary);
        delete temporary;
    } else if (m_item != NULL) {
        m_item->Save(camera);
    }
    SaveSubContracts(camera);
    parent->InsertEndChild(*camera);
    delete camera;
}

// Original 0x1296c4: every matching camera element is read (no break); the old items are not
// deleted; a temporary mission whose friend cannot be set is dropped.
void CGPSHauntCamera::Load(TiXmlElement* parent)
{
    for (TiXmlElement* camera = parent->FirstChildElement("camera"); camera != NULL;
         camera = camera->NextSiblingElement("camera")) {
        int id = CXmlHelper::GetAttributeValue(camera, "id").ToInt();
        if (id == m_id) {
            m_locked = CXmlHelper::GetAttributeValue(camera, "locked").ToBool();
            m_blueBounty = CXmlHelper::GetAttributeValueOrUseDefault(camera, "isBlueBounty", XString("0")).ToBool();
            m_activeImage = CXmlHelper::GetAttributeValueOrUseDefault(camera, "cameraImageRes", XString("IDB_CAMERA_RANDOM_MISSION"));
            m_tempMission = CXmlHelper::GetAttributeValueOrUseDefault(camera, "isTempMission", XString("0")).ToBool();
            m_targettingMission = CXmlHelper::GetAttributeValueOrUseDefault(camera, "isTargettingMission", XString("0")).ToBool();
            m_tempTargettingMission = CXmlHelper::GetAttributeValueOrUseDefault(camera, "isTempTargettingMission", XString("0")).ToBool();
            m_gatedMission = CXmlHelper::GetAttributeValueOrUseDefault(camera, "isGatedMission", XString("0")).ToBool();
            m_gatedMissionId = CXmlHelper::GetAttributeValueOrUseDefault(camera, "gatedMissionId", XString("-1")).ToInt();
            if (CXmlHelper::HasAttribute(camera, "weaponGatedCategory") == true) {
                m_gatedWeaponCategory = CXmlHelper::GetAttributeValue(camera, "weaponGatedCategory");
            }
            TiXmlElement* item = camera->FirstChildElement("huntingItem");
            if (item != NULL) {
                int type = CXmlHelper::GetAttributeValue(item, "itemType").ToInt();
                if (type == 0) {
                    m_item = new CGPSMission(this, item);
                } else if (type == 1) {
                    m_item = new CGPSBounty(this, item);
                }
            } else {
                m_item = NULL;
            }
            TiXmlElement* temporary = camera->FirstChildElement("tempMission");
            if (temporary != NULL) {
                TiXmlElement* temporaryItem = temporary->FirstChildElement("huntingItem");
                if (temporaryItem != NULL) {
                    m_temporaryItem = new CGPSBounty(this, temporaryItem);
                    int friendId = CXmlHelper::GetAttributeValue(temporary, "targetUserId").ToInt();
                    if (m_temporaryItem->SetTargetFriendId(friendId) == true) {
                        bool revenge = CXmlHelper::GetAttributeValueOrUseDefault(temporary, "isRevengeMode", XString("0")).ToBool();
                        m_temporaryItem->m_revenge = revenge;
                    } else if (m_temporaryItem != NULL) {
                        delete m_temporaryItem;
                        m_temporaryItem = NULL;
                    }
                }
            }
            LoadSubContracts(camera);
        }
    }
    UpdateState();
}

void CGPSHauntCamera::SaveSubContracts(TiXmlElement* element)
{
    element->SetAttribute("spawnedSubContractsCount", m_spawnedSubContracts);
    // Original 0x129664: a fresh copy is appended (InsertEndChild + delete); the camera element
    // itself is fresh too (see Save).
    if (m_subContract != NULL) {
        TiXmlElement* saved = new TiXmlElement("subcontract");
        m_subContract->Save(saved);
        element->InsertEndChild(*saved);
        delete saved;
    }
}

void CGPSHauntCamera::LoadSubContracts(TiXmlElement* element)
{
    // the original neither drops the current sub-contract nor checks the camera item
    m_spawnedSubContracts = CXmlHelper::GetAttributeValueOptional(element, "spawnedSubContractsCount").ToInt();
    TiXmlElement* saved = element->FirstChildElement("subcontract");
    if (saved != NULL) {
        m_subContract = new CGPSSubContract(this);
        m_subContract->Init();
        m_subContract->Load(saved);
    }
}

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AssertGPSHauntCameraSize[sizeof(CGPSHauntCamera) == 0xe4 ? 1 : -1];
#endif
