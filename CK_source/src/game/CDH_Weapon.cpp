// CDH_Weapon.cpp - CK weapon data, inventory and loaded magazine.
// Function order follows the original debug map (0x161c80..0x165554).
#include "CDH_Weapon.h"

#include "App.h"
#include "AdBannerSizes.h"
#include "AppSettings.h"
#include "CAmmo.h"
#include "CAmmoManager.h"
#include "CBH_Player.h"
#include "CBH_SurfaceManager.h"
#include "CDH_Animation.h"
#include "CDH_BasicGameData.h"
#include "CDH_Cheats.h"
#include "CDH_ResourceManager.h"
#include "CDH_SoundManager.h"
#include "CGameAnalytics.h"
#include "CHuntingInfo.h"
#include "CSwerveGame.h"
#include "CXmlHelper.h"
#include "CSaveManager.h"
#include "DGHelper.h"
#include "ResMap.h"
#include "SwerveHelper.h"
#include <string.h>

typedef char CDH_Weapon_ARMv7_Size[(sizeof(CDH_Weapon) == 0x20c) ? 1 : -1];

namespace {
    App* GameApp()
    {
        return static_cast<App*>(WindowApp::m_instance);
    }
}

// Shell-by-shell reload split of the reload time. The original evaluates these
// fractions in double precision (float literals widened: 0x3fe6666660000000 ...).
static const double RELOAD_PROCESS_PART = 0.7f;
static const double RELOAD_EDGE_PART = 0.15f;
static const double RELOAD_FIXED_PART = 0.3f;

int CDH_Weapon::g_availabilityChangesCount;
Vector<const CAmmo*> CDH_Weapon::m_universalAmmo;

CDH_Weapon::CDH_Weapon(int id)
    : CItem(id)
    , m_modelRefCount(0)
    , m_shellReloadTime(-1)
    , m_shellsRemaining(-1)
    , m_shellsTotal(-1)
    , m_projectileType(0)
    , m_projectileOffsetX(0.0f)
    , m_projectileOffsetY(0.0f)
    , m_state(1)
    , m_action(0)
    , m_actionTimeLeft(0)
    , m_reloadPending(false)
    , m_fetched(false)
    , m_cyberScopeEquipped(false)
    , m_damageUpgrade(0)
    , m_rangeUpgrade(0)
    , m_wanderUpgrade(0)
    , m_activeAmmoIndex(0)
    , m_universalAmmoChoice(NULL)
    , m_firstNamedAmmo(NULL)
    , m_scopeTime(0)
    , m_spinUpTime(0)
    , m_spinEndTime(0)
    , m_autoShotsLeft(0)
    , m_autoShot(false)
    , m_spread(false)
    , m_zoomFovMin(52.5f)
    , m_zoomFovMax(52.5f)
    , m_tranquillizer(false)
    , m_scopeAttached(false)
    , m_zoomWithoutScope(false)
    , m_originallySilent(false)
    , m_silencerAvailable(false)
    , m_silencerActive(false)
    , m_autoAim(false)
    , m_priceless(false)
    , m_tranquillizingHPSpeed(0)
    , m_countInQueue(0)
    , m_spriteScopeArch(-1)
    , m_shootAnimId(-1)
    , m_shootLeftAnimId(-1)
    , m_shootRightAnimId(-1)
    , m_fetchAnimId(-1)
    , m_reloadAnimId(-1)
    , m_reloadStartAnimId(-1)
    , m_reloadProcessAnimId(-1)
    , m_reloadEndAnimId(-1)
    , m_startShootAnimId(-1)
    , m_endShootAnimId(-1)
    , m_scopeAnimId(-1)
    , m_scopeOutAnimId(-1)
    , m_asyncShotTime(0)
    , m_field200(0)
    , m_field204(0)
    , m_skipShotAnimation(false)
    , m_pendingShot(true)
    , m_field20a(false)
{
    m_field160[3] = NULL;
    m_field160[2] = NULL;
    m_field160[1] = NULL;
    m_field160[0] = NULL;
    m_animation = NULL;
    m_secondaryAnimation = NULL;
    m_available = false;
}

CDH_Weapon::~CDH_Weapon()
{
    m_compatibleAmmo.removeAllElements();
    for (int index = 0; index < m_itemsGroups.size(); ++index) {
        if (m_itemsGroups[index] != NULL) {
            delete m_itemsGroups[index];
            m_itemsGroups[index] = NULL;
        }
    }
    releaseResources();
}

void CDH_Weapon::init()
{
    if (m_compatibleAmmo.size() != 0) {
        if (CBH_Player::GetInstance()->GetAvailableAmmo(GetActiveAmmo(), true) < 1) {
            SetActiveAmmo(GetDefaultAmmo());
        }
        const CAmmo* ammo = GetActiveAmmo();
        int available = CBH_Player::GetInstance()->GetAvailableAmmo(ammo, true);
        if (available < m_clipSize) {
            m_ammo = available;
        } else {
            m_ammo = m_clipSize;
        }
    } else {
        m_ammo = 1;
    }
    m_state = 1;
    m_action = 0;
    m_actionTimeLeft = 0;
    m_reloadPending = false;
}

const CAmmo* CDH_Weapon::GetActiveAmmo() const
{
    if (m_compatibleAmmo.size() == 0) {
        return NULL;
    }
    if (m_activeAmmoIndex < 0 || m_activeAmmoIndex >= m_compatibleAmmo.size()) {
        return NULL;
    }
    if (!m_tranquillizer && m_universalAmmoChoice != NULL) {
        return m_universalAmmoChoice;
    }
    return m_compatibleAmmo[m_activeAmmoIndex];
}

void CDH_Weapon::SetActiveAmmo(const CAmmo* ammo)
{
    if (!CanUseAmmo(ammo)) {
        return;
    }
    m_activeAmmoIndex = m_compatibleAmmo.indexOf(ammo);
    const CAmmo* active = GetActiveAmmo();
    int available = CBH_Player::GetInstance()->GetAvailableAmmo(active, true);
    if (available < m_clipSize) {
        m_ammo = available;
    } else {
        m_ammo = m_clipSize;
    }
}

const CAmmo* CDH_Weapon::GetDefaultAmmo() const
{
    if (m_compatibleAmmo.size() != 0) {
        return m_compatibleAmmo[0];
    }
    return NULL;
}

void CDH_Weapon::init(const TiXmlElement& element)
{
    CItem::init(element);
    Load();
}

void CDH_Weapon::Load()
{
    TiXmlElement* root = GameApp()->m_saveManager->GetRootElement()->ToElement();
    TiXmlElement* weapons = CXmlHelper::GetElement(root, XString("weapons"));
    if (weapons == NULL) {
        return;
    }
    bool legacy;
    TiXmlElement* element = CXmlHelper::GetElement(weapons, XString(XString::Format(L"w%u", m_id).c_str()));
    if (element == NULL) {
        // Saves of older versions keyed the weapon by its name.
        legacy = true;
        element = CXmlHelper::GetElement(weapons, m_name.Replace(L" ", L"_", true));
        if (element == NULL) {
            if (m_id != 0x26) {
                return;
            }
            for (TiXmlNode* old = weapons->FirstChild(); old != NULL; old = old->NextSibling()) {
                if (XString(old->Value()).Find(L".50_Desert_Hawk", 0) >= 0) {
                    m_available = true;
                    Save(true);
                    return;
                }
            }
            return;
        }
    } else {
        legacy = false;
    }
    m_available = CXmlHelper::GetAttributeValue(element, "available").ToInt() == 1;
    ++g_availabilityChangesCount;
    m_wanderUpgrade = CXmlHelper::GetAttributeValue(element, "wanderAmountUpgrade").ToInt();
    m_rangeUpgrade = CXmlHelper::GetAttributeValue(element, "effectiveRangeUpgrade").ToInt();
    m_damageUpgrade = CXmlHelper::GetAttributeValue(element, "damageUpgrade").ToInt();
    bool silent = CXmlHelper::GetAttributeValueOrUseDefault(element, "isSilent", XString("0")).ToBool();
    m_silencerActive = silent;
    m_cyberScopeEquipped = CXmlHelper::GetAttributeValueOrUseDefault(element, "cyberScopeEquiped", XString("0")).ToBool();
    int ammoId = CXmlHelper::GetAttributeValueOrUseDefault(element, "activeAmmoId", XString("-1")).ToInt();
    if (ammoId >= 0) {
        const CAmmo* ammo = static_cast<const CAmmo*>(GameApp()->InitAmmoManager()->getItemById(ammoId));
        if (CanUseAmmo(ammo)) {
            m_activeAmmoIndex = m_compatibleAmmo.indexOf(ammo);
        }
    }
    if (legacy) {
        Save(true);
    }
}

void CDH_Weapon::checkForScopeAttachment()
{
    ItemsGroup* group = getGroup(XString("scope"));
    group->setActiveID(1);
}

CDH_Weapon::ItemsGroup* CDH_Weapon::getGroup(const XString& type)
{
    for (int index = 0; index < m_itemsGroups.size(); ++index) {
        if (m_itemsGroups[index]->m_type == type) {
            return m_itemsGroups[index];
        }
    }
    return NULL;
}

void CDH_Weapon::ItemsGroup::setActiveID(int id)
{
    m_activeId = id;
    for (int index = 0; index < m_items.size(); ++index) {
        ItemDesc* item = m_items[index];
        bool visible = index == m_activeId;
        for (int nodeIndex = 0; nodeIndex < item->m_nodes.size(); ++nodeIndex) {
            if (item->m_nodes[nodeIndex] != NULL) {
                item->m_nodes[nodeIndex].setRenderingEnable(visible);
            }
        }
    }
}

void CDH_Weapon::restoreAmmo()
{
    UpdateUniversalAmmo();
    init();
}

void CDH_Weapon::UpdateUniversalAmmo()
{
    m_universalAmmoChoice = NULL;
    int count = m_universalAmmo.size();
    for (int index = 0; index < count; ++index) {
        if (m_universalAmmo[index] != NULL &&
            CBH_Player::GetInstance()->GetAvailableAmmo(m_universalAmmo[index], true) > 0) {
            m_universalAmmoChoice = m_universalAmmo[index];
            return;
        }
    }
}

bool CDH_Weapon::isSilent() const
{
    return m_originallySilent || m_silencerActive;
}

bool CDH_Weapon::CanUseAmmo(const CAmmo* ammo)
{
    if (ammo == NULL && m_compatibleAmmo.size() == 0) {
        return true;
    }
    for (int index = 0; index < m_compatibleAmmo.size(); ++index) {
        if (m_compatibleAmmo[index] == ammo) {
            return true;
        }
    }
    return false;
}

void CDH_Weapon::update(unsigned int elapsed)
{
    if (GameApp()->m_helper->m_swerveGame->m_slowMode) {
        return;
    }
    if (App::m_platformType != 0) {
        animateWeaponModel();
    }
    if (m_action == 9 && m_spinEndTime >= 0 && ++m_asyncShotTime == 2) {
        PlaySpinEndAnimation();
    }
    UpdateUniversalAmmo();
    UpdateAsyncShot(elapsed);
    UpdateState(elapsed);
}

void CDH_Weapon::animateWeaponModel()
{
    int step = m_shellReloadTime;
    if (m_reloadStartAnimId >= 0 && m_action == 3 && (step == 0 || step > 1)) {
        bool advance;
        if (step == 0) {
            advance = m_reloadDuration - m_actionTimeLeft > m_reloadTime * 0.15f;
        } else {
            advance = m_actionTimeLeft <
                      (m_shellsTotal * RELOAD_PROCESS_PART / m_shellsRemaining + RELOAD_EDGE_PART) * m_reloadDuration;
        }
        if (advance) {
            int shells = m_shellsTotal - 1;
            m_shellsTotal = shells;
            int next = 1;
            if (shells > 0) {
                next = step + 1;
                if (step == 0) {
                    next = 3;
                }
            }
            m_shellReloadTime = next;
            if (m_animation != NULL) {
                m_animation->stopAllAnimation();
                m_animation->setCurrentController(shells > 0 ? m_reloadProcessAnimId : m_reloadEndAnimId, false);
            }
            if (m_reloadInSteps) {
                GameApp()->m_soundManager->PlaySound(shells > 0 ? m_reloadProcessSound : m_reloadEndSound, false,
                                                    false);
            }
        }
    }
    // The original animates the primary controller without a NULL test.
    m_animation->animate();
    if (m_secondaryAnimation != NULL) {
        m_secondaryAnimation->animate();
    }
}

void CDH_Weapon::PlaySpinEndAnimation()
{
    if (m_secondaryAnimation != NULL) {
        m_secondaryAnimation->stopAllAnimation();
    }
    if (m_animation != NULL) {
        m_animation->setCurrentController(m_endShootAnimId, false);
    }
    m_pendingShot = false;
}

void CDH_Weapon::UpdateAsyncShot(int elapsed)
{
    if (m_skipShotAnimation && m_state == 4 && m_pendingShot) {
        m_field204 -= elapsed;
        if (m_field204 <= 0) {
            m_field204 = m_field200;
            PlayShotAnimation();
        }
    }
}

void CDH_Weapon::UpdateState(int elapsed)
{
    if (m_actionTimeLeft != 0) {
        m_actionTimeLeft -= elapsed;
        if (m_actionTimeLeft <= 0) {
            m_actionTimeLeft = 0;
            m_state = m_action;
            m_action = 0;
            AfterStateChanged();
        }
    }
}

void CDH_Weapon::hideWeaponModel()
{
    if (m_secondaryAnimation != NULL) {
        m_secondaryAnimation->stopAllAnimation();
    }
    if (m_animation != NULL) {
        m_animation->setCurrentController(m_fetchAnimId, false);
        m_animation->animate();
        m_animation->setCurrentController(-1, false);
    }
}

void CDH_Weapon::fetchWeaponModel()
{
    if (m_secondaryAnimation != NULL) {
        m_secondaryAnimation->stopAllAnimation();
    }
    if (m_animation != NULL) {
        m_animation->setCurrentController(m_fetchAnimId, false);
    }
    const CAmmo* ammo = GetActiveAmmo();
    int available = CBH_Player::GetInstance()->GetAvailableAmmo(ammo, true);
    m_ammo = available < m_clipSize ? available : m_clipSize;
    CheckForOutOfAmmo();
}

void CDH_Weapon::CheckForOutOfAmmo()
{
    if (m_ammo > 0) {
        return;
    }
    if (CBH_Player::GetInstance()->GetAvailableAmmo(GetActiveAmmo(), true) < 1) {
        SetActiveAmmo(GetDefaultAmmo());
    }
    if (m_ammo > 0) {
        Reload();
    }
}

void CDH_Weapon::unfetchWeaponModel()
{
    if (m_secondaryAnimation != NULL) {
        m_secondaryAnimation->stopAllAnimation();
    }
    if (m_animation != NULL) {
        m_animation->stopAllAnimation();
        m_animation->setCurrentController(m_fetchAnimId, true);
    }
}

void CDH_Weapon::scopeReset()
{
    if (m_animation != NULL) {
        m_animation->stopAllAnimation();
    }
    if (m_secondaryAnimation != NULL) {
        m_secondaryAnimation->setCurrentController(m_scopeAnimId, false);
        m_secondaryAnimation->animate();
        m_secondaryAnimation->setCurrentController(-1, false);
    }
    m_fetched = false;
}

int CDH_Weapon::isModelAnimationComplete() const
{
    // No primary animation counts as complete.
    return m_animation == NULL || m_animation->isAnimationComplete();
}

void CDH_Weapon::paintWeaponModel(Graphics3D& graphics)
{
    if (CDH_Cheats::CHEAT_SHOW_WEAPON) {
        App::RenderWorld(graphics, m_world);
    }
}

void CDH_Weapon::playFireSound() const
{
    if (m_silencerActive) {
        GameApp()->m_soundManager->PlaySound("IDM_FIRE_SILENCER_SHOT", false, false);
    } else if (CBH_Player::GetInstance()->IsSpeedMode() == true && m_category != Category_Exotic) {
        GameApp()->m_soundManager->PlaySound("IDM_SLOW_GUNSHOT", false, false);
    } else {
        GameApp()->m_soundManager->PlaySound(m_shootSound, false, false);
    }
}

void CDH_Weapon::loadWeaponModel(bool forMenu)
{
    m_skipShotAnimation = !forMenu;
    if (App::m_platformType == 0) {
        m_scopeTime = 1000;
        return;
    }
    if (m_modelRefCount++ > 0) {
        return;
    }
    m_world = World(App::LoadObject3D(m_modelId));
    m_world.getBackground().setColorClearEnable(false);
    m_eyeCamera = Camera(m_world.find(DGHelper::getSwerveID("eyecam")));
    // Pull the eye camera slightly back along X (original constant 0xb727c5ac).
    Transform transform(CSwerve::GetInstance()->m_swerve);
    m_eyeCamera.getCompositeTransform(transform);
    float offset[3];
    offset[0] = -1.0e-5f;
    offset[1] = 0.0f;
    offset[2] = 0.0f;
    transform.postTranslate(offset);
    m_eyeCamera.setTransform(transform);
    m_menuCamera = Camera(m_world.find(DGHelper::getSwerveID("menu_cam")));
    m_world.setActiveCamera(m_eyeCamera);
    m_controller = Transformable(m_world.find(DGHelper::getSwerveID("weaponControllerTransform")));
    m_animation = new CDH_Animation(m_controller);
    if (CDH_Animation::HaveController(m_controller, "shoot")) {
        m_shootAnimId = m_animation->addController(XString("shoot"));
        if (CDH_Animation::HaveController(m_controller, "start_shoot")) {
            m_startShootAnimId = m_animation->addController(XString("start_shoot"));
            m_spinUpTime = m_animation->getControllerBaseDuration(m_startShootAnimId);
        }
        if (CDH_Animation::HaveController(m_controller, "end_shoot")) {
            m_endShootAnimId = m_animation->addController(XString("end_shoot"));
            m_spinEndTime = m_animation->getControllerBaseDuration(m_endShootAnimId);
        }
    } else {
        m_shootLeftAnimId = m_animation->addController(XString("shoot_l"));
        m_shootRightAnimId = m_animation->addController(XString("shoot_r"));
    }
    m_fetchAnimId = m_animation->addController(XString("fetch"));
    bool reloadInSteps = CDH_Animation::HaveController(m_eyeCamera.getParent(), "reload_start") &&
                         CDH_Animation::HaveController(m_eyeCamera.getParent(), "reload_process") &&
                         CDH_Animation::HaveController(m_eyeCamera.getParent(), "reload_end");
    if (reloadInSteps) {
        m_reloadStartAnimId = m_animation->addController(XString("reload_start"));
        m_reloadProcessAnimId = m_animation->addController(XString("reload_process"));
        m_reloadEndAnimId = m_animation->addController(XString("reload_end"));
    } else {
        m_reloadAnimId = m_animation->addController(XString("reload"));
    }
    if (CDH_Animation::HaveController(m_eyeCamera.getParent(), "scope")) {
        m_secondaryAnimation = new CDH_Animation(m_eyeCamera.getParent());
        m_scopeAnimId = m_secondaryAnimation->addController(XString("scope"));
        m_scopeTime = m_secondaryAnimation->getControllerBaseDuration(m_scopeAnimId);
        if (CDH_Animation::HaveController(m_eyeCamera.getParent(), "scope_out")) {
            m_scopeOutAnimId = m_secondaryAnimation->addController(XString("scope_out"));
        }
    }
    if (m_shootAnimId < 0) {
        m_field200 = m_animation->getControllerBaseDuration(m_shootLeftAnimId);
        if (m_field200 > m_rateOfFire) {
            m_animation->setControllerDuration(m_shootLeftAnimId, m_rateOfFire);
            m_animation->setControllerDuration(m_shootRightAnimId, m_rateOfFire);
        }
    } else {
        m_field200 = m_animation->getControllerBaseDuration(m_shootAnimId);
        if (m_field200 > m_rateOfFire && forMenu) {
            m_animation->setControllerDuration(m_shootAnimId, m_rateOfFire);
        }
    }
    if (m_reloadStartAnimId >= 0) {
        int available = CBH_Player::GetInstance()->GetAvailableAmmo(GetActiveAmmo(), true);
        m_animation->setControllerDuration(m_reloadStartAnimId, (int)(m_reloadTime * 0.15f));
        if (available > m_clipSize) {
            available = m_clipSize;
        }
        m_animation->setControllerDuration(m_reloadProcessAnimId, (int)(m_reloadTime * RELOAD_PROCESS_PART / (float)available));
        m_animation->setControllerDuration(m_reloadEndAnimId, (int)(m_reloadTime * 0.15f));
    } else {
        int duration = m_animation->getControllerBaseDuration(m_reloadAnimId);
        if (m_reloadTime < duration) {
            m_reloadTime = duration;
        }
        m_animation->setControllerDuration(m_reloadAnimId, m_reloadTime);
    }
    for (int groupIndex = 0; groupIndex < m_itemsGroups.size(); ++groupIndex) {
        ItemsGroup* group = m_itemsGroups[groupIndex];
        for (int itemIndex = 0; itemIndex < group->m_items.size(); ++itemIndex) {
            ItemDesc* item = group->m_items[itemIndex];
            item->m_nodes.removeAllElements();
            for (int nodeIndex = 0; nodeIndex < item->m_nodeNames.size(); ++nodeIndex) {
                XString::AnsiString name(item->m_nodeNames[nodeIndex].c_str());
                item->m_nodes.addElement(Node(m_world.find(DGHelper::getSwerveID(name))));
            }
            item->m_image = static_cast<ICRenderSurface*>(
                GameApp()->m_resourceManager->getResourceData(item->m_resourceId, false));
        }
        group->m_items[0]->m_available = true;
        group->setActiveID(0);
    }
    hideWeaponModel();
    if (m_secondaryAnimation != NULL) {
        scopeReset();
    }
    checkForScopeAttachment();
}

void CDH_Weapon::loadResources()
{
    init();
}

void CDH_Weapon::Parse(const TiXmlElement& element)
{
    ParseCategory(element);
    CItem::Parse(element);
    const TiXmlElement* sounds = element.FirstChildElement("sounds");
    ParseSounds(*sounds);
    const TiXmlElement* parts = element.FirstChildElement("parts");
    if (parts != NULL) {
        ParseParts(*parts);
    }
    const TiXmlElement* upgrades = element.FirstChildElement("upgrades");
    const TiXmlElement* child = upgrades->FirstChildElement("damage");
    if (child != NULL) {
        ParseUpgrades(*child, m_damageLevels);
    }
    child = upgrades->FirstChildElement("wander_amount");
    if (child != NULL) {
        ParseUpgrades(*child, m_wanderLevels);
    }
    child = upgrades->FirstChildElement("effective_range");
    if (child != NULL) {
        ParseUpgrades(*child, m_rangeLevels);
    }
    ParseCompatibleAmmo(element);
}

void CDH_Weapon::ParseCategory(const TiXmlElement& element)
{
    XString category = CXmlHelper::GetAttributeValue(element, "category").Lower();
    m_category = GetWeaponCategoryType(category);
}

CDH_Weapon::EWeaponCategory CDH_Weapon::GetWeaponCategoryType(const XString& category)
{
    if (XString("handgun") == category) {
        return Category_Handgun;
    }
    if (XString("automatic") == category) {
        return Category_Automatic;
    }
    if (XString("sniper") == category) {
        return Category_Sniper;
    }
    if (XString("shotgun") == category) {
        return Category_Shotgun;
    }
    if (XString("exotic") == category) {
        return Category_Exotic;
    }
    return Category_Handgun;
}

void CDH_Weapon::ParseTexts(const TiXmlElement& element)
{
    CItem::ParseTexts(element);
    m_statsText = CXmlHelper::GetAttributeValue(element, "stats");
}

void CDH_Weapon::ParseStats(const TiXmlElement& element)
{
    CItem::ParseStats(element);
    m_shootType = CXmlHelper::GetAttributeValue(element, "shoot_type").ToInt();
    m_clipSize = CXmlHelper::GetAttributeValue(element, "num_of_shots").ToInt();
    m_rateOfFire = CXmlHelper::GetAttributeValue(element, "rate_of_fire_ms").ToInt();
    m_reloadTime = CXmlHelper::GetAttributeValue(element, "reloading_time_ms").ToInt();
    m_wanderTimeX = CXmlHelper::GetAttributeValue(element, "wander_time_x_ms").ToInt();
    m_wanderTimeY = CXmlHelper::GetAttributeValue(element, "wander_time_y_ms").ToInt();
    m_bonusXp = CXmlHelper::GetAttributeValue(element, "bonus_xp").ToInt();
    m_silencerAvailable = CXmlHelper::GetAttributeValueOrUseDefault(element, "silencerAvailable", XString("0")).ToBool();
    if (CXmlHelper::GetAttributeValue(element, "isSpread").ToInt() == 1) {
        m_spread = true;
    }
    if (CXmlHelper::GetAttributeValue(element, "tranquillize").ToInt() == 1) {
        m_tranquillizer = true;
    }
    m_originallySilent = CXmlHelper::GetAttributeValue(element, "isOriginallySilent").ToBool();
    m_autoAim = CXmlHelper::GetAttributeValue(element, "isAutoAim").ToBool();
    m_priceless = CXmlHelper::GetAttributeValue(element, "isPriceless").ToBool();
    m_zoomWithoutScope =
        CXmlHelper::GetAttributeValueOrUseDefault(element, "isRequiredZoomWithoutScope", XString("0")).ToBool();
    m_scopeAttached = CXmlHelper::GetAttributeValue(element, "scopeAttached").ToBool();
    if (m_zoomWithoutScope || m_scopeAttached) {
        m_zoomFovMin = CXmlHelper::GetAttributeValue(element, "zoom_fov_min").ToFloat();
        m_zoomFovMax = CXmlHelper::GetAttributeValue(element, "zoom_fov_max").ToFloat();
    }
    m_tranquillizingHPSpeed = CXmlHelper::GetAttributeValue(element, "tranquillizingHPSpeed").ToInt();
    m_scopeHudModel = CXmlHelper::GetAttributeValue(element, "scope_hud_model");
    m_countInQueue = CXmlHelper::GetAttributeValue(element, "count_in_queue").ToInt();
    m_projectileType = CXmlHelper::GetAttributeValue(element, "projectile_type").ToInt();
    m_projectileOffsetX =
        CXmlHelper::GetAttributeValueOrUseDefault(element, "projectile_start_offset_x", XString(L"0.0")).ToFloat();
    m_projectileOffsetY =
        CXmlHelper::GetAttributeValueOrUseDefault(element, "projectile_start_offset_y", XString(L"0.0")).ToFloat();
}

void CDH_Weapon::ParseGraphics(const TiXmlElement& element)
{
    XString model = CXmlHelper::GetAttributeValue(element, "model");
    XString shopImage = CXmlHelper::GetAttributeValue(element, "image_shop");
    XString bigImage = CXmlHelper::GetAttributeValue(element, "image_big");
    XString gameImage = CXmlHelper::GetAttributeValue(element, "image_game");
    XString menuImage = CXmlHelper::GetAttributeValue(element, "image_menu");
    XString bulletOnImage = CXmlHelper::GetAttributeValue(element, "image_bullet_on");
    XString bulletOffImage = CXmlHelper::GetAttributeValue(element, "image_bullet_off");
    m_spriteScopeArch = CXmlHelper::GetAttributeValue(element, "sprite_scope_arch").ToInt();
    m_modelId = GameApp()->InitResMap()->getResourceID(model);
    setShopImageResId(GameApp()->InitResMap()->getResourceID(shopImage));
    setBigImageResId(GameApp()->InitResMap()->getResourceID(bigImage));
    m_gameImageId = GameApp()->InitResMap()->getResourceID(gameImage);
    m_menuImageId = GameApp()->InitResMap()->getResourceID(menuImage);
    m_bulletOnImageId = GameApp()->InitResMap()->getResourceID(bulletOnImage);
    m_bulletOffImageId = GameApp()->InitResMap()->getResourceID(bulletOffImage);
}

void CDH_Weapon::ParseSounds(const TiXmlElement& element)
{
    XString shoot = CXmlHelper::GetAttributeValue(element, "shoot");
    XString noAmmo = CXmlHelper::GetAttributeValue(element, "no_ammo");
    XString reload = CXmlHelper::GetAttributeValue(element, "reload");
    XString reloadStart = CXmlHelper::GetAttributeValueOptional(element, "reload_start");
    XString reloadProcess = CXmlHelper::GetAttributeValueOptional(element, "reload_process");
    XString reloadEnd = CXmlHelper::GetAttributeValueOptional(element, "reload_end");
    m_shootSound = GameApp()->InitResMap()->getResourceID(shoot);
    m_noAmmoSound = GameApp()->InitResMap()->getResourceID(noAmmo);
    m_reloadSound = GameApp()->InitResMap()->getResourceID(reload);
    m_reloadInSteps = false;
    if (!(reloadStart == XString(""))) {
        m_reloadInSteps = true;
        m_reloadStartSound = GameApp()->InitResMap()->getResourceID(reloadStart);
        m_reloadProcessSound = GameApp()->InitResMap()->getResourceID(reloadProcess);
        m_reloadEndSound = GameApp()->InitResMap()->getResourceID(reloadEnd);
    }
}

void CDH_Weapon::ParseParts(const TiXmlElement& element)
{
    for (const TiXmlNode* groupNode = element.FirstChild(); groupNode != NULL;
         groupNode = groupNode->NextSibling()) {
        const char* type = groupNode->Value();
        const char* groupName = groupNode->ToElement()->Attribute("name");
        ItemsGroup* group = new ItemsGroup(XString(type), XString(groupName));
        for (const TiXmlNode* item = groupNode->FirstChild("item"); item != NULL;
             item = item->NextSibling("item")) {
            const TiXmlElement* itemElement = item->ToElement();
            XString name = CXmlHelper::GetAttributeValue(itemElement, "desc");
            XString image = CXmlHelper::GetAttributeValue(itemElement, "image");
            int credits = CXmlHelper::GetAttributeValue(itemElement, "price_credits").ToInt();
            int points = CXmlHelper::GetAttributeValue(itemElement, "price_points").ToInt();
            Vector<XString> names;
            for (const TiXmlNode* object = item->FirstChild("object"); object != NULL;
                 object = object->NextSibling("object")) {
                names.addElement(XString(object->ToElement()->Attribute("id")));
            }
            ItemDesc* entry = new ItemDesc(name, names, GameApp()->InitResMap()->getResourceID(image), credits,
                                           points, points == 0 && credits == 0);
            group->m_items.addElement(entry);
        }
        m_itemsGroups.addElement(group);
    }
}

void CDH_Weapon::ParseUpgrades(const TiXmlElement& element, Vector<Upgrade>& upgrades)
{
    for (const TiXmlNode* child = element.FirstChild(); child != NULL; child = child->NextSibling()) {
        Upgrade upgrade;
        upgrade.m_pricePoints = 0;
        upgrade.m_priceCredits = 0;
        const TiXmlElement* upgradeElement = child->ToElement();
        upgrade.m_value = CXmlHelper::GetAttributeValue(upgradeElement, "value").ToInt();
        const char* points = upgradeElement->Attribute("price_points");
        if (points != NULL) {
            upgrade.m_pricePoints = XString(points).ToInt();
        }
        const char* credits = upgradeElement->Attribute("price_credits");
        if (credits != NULL) {
            upgrade.m_priceCredits = XString(credits).ToInt();
        }
        upgrades.addElement(upgrade);
    }
}

void CDH_Weapon::ParseCompatibleAmmo(const TiXmlElement& element)
{
    for (const TiXmlElement* child = element.FirstChildElement("ammo"); child != NULL;
         child = child->NextSiblingElement("ammo")) {
        XString type = CXmlHelper::GetAttributeValue(*child, "type");
        if (XString("infinite_ammo") == type) {
            const CAmmo* infinite = NULL;
            m_compatibleAmmo.addElement(infinite);
        } else {
            const CAmmo* ammo = GameApp()->InitAmmoManager()->GetAmmoByType(type);
            if (ammo != NULL) {
                if (m_firstNamedAmmo == NULL) {
                    m_firstNamedAmmo = ammo;
                }
                m_compatibleAmmo.addElement(ammo);
            }
        }
    }
}

XString CDH_Weapon::getUIDString() const
{
    return XString::Format(L"weapon_%d", m_id);
}

CDH_Weapon::ItemDesc::ItemDesc(const XString& name, const Vector<XString>& names, const char* resource,
                              int priceCredits, int pricePoints, bool available)
{
    m_name = name;
    m_nodeNames = names;
    m_resourceId = resource;
    m_priceCredits = priceCredits;
    m_pricePoints = pricePoints;
    m_available = available;
}

CDH_Weapon::ItemDesc::~ItemDesc()
{
    GameApp()->m_resourceManager->releaseResource(m_resourceId);
}

CDH_Weapon::ItemsGroup::ItemsGroup(const XString& type, const XString& name)
{
    m_type = type;
    m_name = name;
}

CDH_Weapon::ItemsGroup::~ItemsGroup()
{
    for (int index = 0; index < m_items.size(); ++index) {
        if (m_items[index] != NULL) {
            delete m_items[index];
            m_items[index] = NULL;
        }
    }
}

bool CDH_Weapon::IsPriceless() const
{
    return m_priceless;
}

float CDH_Weapon::effectiveRange() const
{
    return (float)m_rangeLevels[m_rangeUpgrade].m_value;
}

float CDH_Weapon::wanderAmount() const
{
    return (float)m_wanderLevels[m_wanderUpgrade].m_value;
}

float CDH_Weapon::wanderAmountMax() const
{
    if (m_wanderLevels.size() == 0) {
        return 0.0f;
    }
    return (float)m_wanderLevels[0].m_value;
}

int CDH_Weapon::damage() const
{
    if (m_damageUpgrade < 0 || m_damageUpgrade >= m_damageLevels.size()) {
        return 0;
    }
    int value = m_damageLevels[m_damageUpgrade].m_value;
    const CAmmo* ammo = GetActiveAmmo();
    int extra = 0;
    if (ammo != NULL) {
        extra = ammo->m_extraDamage;
    }
    return extra + value;
}

void CDH_Weapon::setAvailable(bool available)
{
    m_available = available;
    ++g_availabilityChangesCount;
    if (GameApp()->InitAmmoManager()->IsAllAmmoTypesOpen()) {
        CBH_Player::GetInstance()->m_gameStats.SetAllAmmoTypesOpen(true);
    }
    Save(true);
}

void CDH_Weapon::Save(bool queueSave)
{
    TiXmlElement* root = GameApp()->m_saveManager->GetRootElement()->ToElement();
    TiXmlElement* weapons = CXmlHelper::ObtainElement(root, XString("weapons"));
    TiXmlElement* old = CXmlHelper::GetElement(weapons, m_name.Replace(L" ", L"_", true));
    if (old != NULL) {
        weapons->RemoveChild(old);
    }
    TiXmlElement* element = CXmlHelper::ObtainElement(weapons, XString(XString::Format(L"w%u", m_id).c_str()));
    element->SetAttribute("available", isAvailable());
    element->SetAttribute("damageUpgrade", m_damageUpgrade);
    element->SetAttribute("effectiveRangeUpgrade", m_rangeUpgrade);
    element->SetAttribute("wanderAmountUpgrade", m_wanderUpgrade);
    element->SetAttribute("isSilent", m_silencerActive);
    element->SetAttribute("cyberScopeEquiped", m_cyberScopeEquipped);
    const CAmmo* ammo = GetActiveAmmo();
    if (ammo != NULL) {
        element->SetAttribute("activeAmmoId", ammo->m_id);
    }
    if (queueSave) {
        GameApp()->m_saveManager->SaveOnNextUpdate(false);
    }
}

void CDH_Weapon::releaseWeaponModel()
{
    if (App::m_platformType == 0) {
        return;
    }
    if (--m_modelRefCount == 0) {
        if (m_secondaryAnimation != NULL) {
            delete m_secondaryAnimation;
            m_secondaryAnimation = NULL;
        }
        if (m_animation != NULL) {
            delete m_animation;
            m_animation = NULL;
        }
        m_controller.Clear();
        m_eyeCamera.Clear();
        m_menuCamera.Clear();
        m_world.Clear();
    }
    if (m_modelRefCount < 0) {
        m_modelRefCount = 0;
    }
}

void CDH_Weapon::FlushAllUpgrades()
{
    m_damageUpgrade = 0;
    m_rangeUpgrade = 0;
    m_wanderUpgrade = 0;
}

void CDH_Weapon::releaseResources()
{
    GameApp()->m_surfaceManager->ReleaseSurface(m_gameImageId);
    GameApp()->m_surfaceManager->ReleaseSurface(m_menuImageId);
    GameApp()->m_surfaceManager->ReleaseSurface(m_bulletOnImageId);
    GameApp()->m_surfaceManager->ReleaseSurface(m_bulletOffImageId);
    GameApp()->m_surfaceManager->ReleaseSurface(getShopImageResId());
    GameApp()->m_surfaceManager->ReleaseSurface(getBigImageResId());
}

void CDH_Weapon::InitUniversalAmmo()
{
    if (m_universalAmmo.size() > 0) {
        m_universalAmmo.removeAllElements();
    }
    int count = GameApp()->InitAmmoManager()->getItemsCount();
    for (int id = 0; id < count; ++id) {
        const CAmmo* ammo = static_cast<const CAmmo*>(GameApp()->InitAmmoManager()->getItemById(id));
        if (ammo != NULL && ammo->m_universal) {
            m_universalAmmo.addElement(ammo);
        }
    }
}

ICRenderSurface* CDH_Weapon::getGameImage() const
{
    ICRenderSurface* surface = GameApp()->m_surfaceManager->GetFromCache(m_gameImageId);
    if (surface != NULL) {
        return surface;
    }
    return GameApp()->m_surfaceManager->CreateSurface(m_gameImageId, true);
}

ICRenderSurface* CDH_Weapon::getMenuImage() const
{
    ICRenderSurface* surface = GameApp()->m_surfaceManager->GetFromCache(m_menuImageId);
    if (surface != NULL) {
        return surface;
    }
    return GameApp()->m_surfaceManager->CreateSurface(m_menuImageId, true);
}

ICRenderSurface* CDH_Weapon::getBulletOnImage() const
{
    ICRenderSurface* surface = GameApp()->m_surfaceManager->GetFromCache(m_bulletOnImageId);
    if (surface != NULL) {
        return surface;
    }
    return GameApp()->m_surfaceManager->CreateSurface(m_bulletOnImageId, true);
}

ICRenderSurface* CDH_Weapon::getShopImage() const
{
    ICRenderSurface* surface = GameApp()->m_surfaceManager->GetFromCache(getShopImageResId());
    if (surface != NULL) {
        return surface;
    }
    return GameApp()->m_surfaceManager->CreateSurface(getShopImageResId(), true);
}

void CDH_Weapon::PlayShotAnimation()
{
    if (m_secondaryAnimation != NULL) {
        m_secondaryAnimation->stopAllAnimation();
    }
    if (m_animation == NULL) {
        return;
    }
    if (m_shootAnimId >= 0) {
        m_animation->setCurrentController(m_shootAnimId, false);
        return;
    }
    if (m_field20a) {
        m_animation->setCurrentController(m_shootLeftAnimId, false);
    } else {
        m_animation->setCurrentController(m_shootRightAnimId, false);
    }
    m_field20a = !m_field20a;
}

void CDH_Weapon::AfterStateChanged()
{
    switch (m_state) {
    case 3:
        AfterReloading();
        if (m_reloadPending && !CBH_Player::GetInstance()->IsInHide()) {
            ScopeIn();
            return;
        }
        m_state = 1;
        return;
    case 5:
        m_state = 2;
        CheckForReload(false);
        CheckForOutOfAmmo();
        return;
    case 6:
        m_state = 2;
        GameApp()->m_helper->m_swerveGame->onSwitchToScope();
        return;
    case 7:
        if (m_reloadPending) {
            StopAllAnimation();
            Reload();
        } else {
            m_state = 1;
        }
        GameApp()->m_helper->m_swerveGame->onSwitchFromScope();
        return;
    case 4:
    case 8:
        EndSpin();
        return;
    case 9:
        if (m_skipShotAnimation) {
            ResetModelAnimation();
        }
        m_state = 1;
        CheckForReload(false);
        CheckForOutOfAmmo();
        return;
    default:
        return;
    }
}

void CDH_Weapon::AfterReloading()
{
    if (m_compatibleAmmo.size() > 0) {
        const CAmmo* ammo = GetActiveAmmo();
        if (ammo == NULL) {
            return;
        }
        int available = CBH_Player::GetInstance()->GetAvailableAmmo(ammo, true);
        m_ammo = available < m_clipSize ? available : m_clipSize;
        ResetAutoShot();
    }
}

void CDH_Weapon::ScopeIn()
{
    if (m_state != 1 && m_state != 3) {
        return;
    }
    m_fetched = true;
    PlayScopeAnimation(false, false);
    App* app = GameApp();
    CGameAnalytics::logScopeActivated(app->m_huntingInfo->m_assignedData.m_missionName,
                                      app->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon()->m_id);
    m_actionTimeLeft = m_scopeTime;
    m_action = 6;
    if (m_scopeAttached) {
        GameApp()->m_soundManager->PlaySound("IDM_SCOPE_ON", false, false);
    }
    if (CSwerveGame::m_pInstance != NULL) {
        CSwerveGame::m_pInstance->onScopeInStart();
    }
    if (m_reloadPending) {
        m_reloadPending = false;
    }
}

void CDH_Weapon::EndSpin()
{
    if (m_endShootAnimId >= 0) {
        m_asyncShotTime = 0;
        m_action = 9;
        m_actionTimeLeft = m_spinEndTime;
        return;
    }
    m_state = 9;
    m_action = 0;
    AfterStateChanged();
}

bool CDH_Weapon::CheckForReload(bool force)
{
    if (m_compatibleAmmo.size() == 0) {
        return true;
    }
    if (!force && m_ammo >= 1) {
        return true;
    }
    const CAmmo* ammo = GetActiveAmmo();
    if (CBH_Player::GetInstance()->GetAvailableAmmo(ammo, true) <= 0) {
        return false;
    }
    if (m_state == 2) {
        m_reloadPending = true;
        ScopeOut();
    } else {
        Reload();
    }
    return true;
}

void CDH_Weapon::StopAllAnimation()
{
    if (m_secondaryAnimation != NULL) {
        m_secondaryAnimation->stopAllAnimation();
    }
    if (m_animation != NULL) {
        m_animation->stopAllAnimation();
    }
}

void CDH_Weapon::Reload()
{
    if (m_compatibleAmmo.size() == 0) {
        return;
    }
    if (m_activeAmmoIndex < 0) {
        return;
    }
    if (m_activeAmmoIndex >= m_compatibleAmmo.size()) {
        return;
    }
    m_shellReloadTime = 0;
    int target = CBH_Player::GetInstance()->GetAvailableAmmo(GetActiveAmmo(), true);
    if (m_clipSize < target) {
        target = m_clipSize;
    }
    m_reloadTarget = target;
    m_shellsRemaining = target - m_ammo;
    m_shellsTotal = target - m_ammo;
    PlayReloadAnimation();
    m_actionTimeLeft = m_reloadTime;
    m_action = 3;
    if (m_reloadStartAnimId >= 0) {
        m_reloadDuration = m_actionTimeLeft =
            (int)(m_reloadTime * (m_shellsTotal * RELOAD_PROCESS_PART / (float)m_reloadTarget + RELOAD_FIXED_PART));
        if (m_reloadInSteps) {
            GameApp()->m_soundManager->PlaySound(m_reloadStartSound, false, false);
            return;
        }
    }
    GameApp()->m_soundManager->PlaySound(m_reloadSound, false, false);
}

void CDH_Weapon::ResetModelAnimation()
{
    if (m_animation != NULL) {
        m_animation->setCurrentController(m_shootAnimId, false);
        m_animation->animate();
        m_animation->setCurrentController(-1, false);
    }
}

bool CDH_Weapon::IsReadyForAutoShot()
{
    return m_shootType == 2 && IsReadyForShot() && m_autoShot;
}

bool CDH_Weapon::IsReadyForShot()
{
    return IsReadyForUse() || m_action == 9;
}

bool CDH_Weapon::IsReadyForUse()
{
    return (m_state == 1 || m_state == 2) && m_action == 0;
}

bool CDH_Weapon::IsReloading()
{
    return m_reloadPending || m_action == 3;
}

bool CDH_Weapon::DoesShowReloadingMessage()
{
    return m_action == 3;
}

bool CDH_Weapon::MakeShot()
{
    if (m_state != 1 && m_state != 2 && m_action != 9) {
        return false;
    }
    if (m_ammo < 1) {
        GameApp()->m_soundManager->PlaySound("IDM_FIRE_EMPTY", false, false);
        ResetAutoShot();
        CheckForReload(false);
        if (m_ammo < 1) {
            return false;
        }
    }
    if (!GameApp()->m_settings->IsFireButtonEnabled()) {
        HandleAutoShot();
    }
    if (!m_tranquillizer && !isSilent()) {
        // A loud shot alerts the scene.
        GameApp()->m_helper->m_swerveGame->m_peaceTime = false;
        GameApp()->m_helper->m_swerveGame->m_missionAlert = true;
    }
    if (m_compatibleAmmo.size() > 0) {
        --m_ammo;
        const CAmmo* ammo = GetActiveAmmo();
        CBH_Player::GetInstance()->UseAmmo(ammo);
    }
    if (!m_originallySilent && m_silencerActive) {
        CBH_Player::GetInstance()->ReduceSilentShotsCount(1);
        if (CBH_Player::GetInstance()->m_silentShotsCount == 0) {
            m_silencerActive = false;
        }
    }
    m_pendingShot = true;
    if (m_action == 9) {
        MakeShotImpl();
        return true;
    }
    return StartSpin();
}

void CDH_Weapon::ResetAutoShot()
{
    m_autoShotsLeft = 0;
    m_autoShot = false;
}

void CDH_Weapon::HandleAutoShot()
{
    if (m_shootType == 2) {
        if (!m_autoShot) {
            m_autoShot = true;
            m_autoShotsLeft = m_countInQueue;
        }
        if (m_autoShot) {
            --m_autoShotsLeft;
            if (m_autoShotsLeft < 1) {
                ResetAutoShot();
            }
        }
    }
}

void CDH_Weapon::MakeShotImpl()
{
    if (!m_skipShotAnimation) {
        PlayShotAnimation();
    }
    playFireSound();
    m_actionTimeLeft = m_rateOfFire;
    int action = 4;
    if (m_state == 2) {
        action = 5;
    }
    m_action = action;
}

bool CDH_Weapon::StartSpin()
{
    if (m_startShootAnimId >= 0) {
        PlaySpinStartAnimation();
        m_action = 8;
        m_actionTimeLeft = m_spinUpTime;
        return false;
    }
    MakeShotImpl();
    return true;
}

void CDH_Weapon::PlayScopeAnimation(bool out, bool reset)
{
    ResetModelAnimation();
    if (m_animation != NULL) {
        m_animation->stopAllAnimation();
    }
    if (reset && m_scopeOutAnimId >= 0) {
        // The original does not test the secondary animation on this path.
        m_secondaryAnimation->setCurrentController(m_scopeOutAnimId, false);
    } else if (m_secondaryAnimation != NULL) {
        m_secondaryAnimation->setCurrentController(m_scopeAnimId, out);
    }
}

void CDH_Weapon::ScopeOut()
{
    if (m_state != 2) {
        return;
    }
    if (m_action == 7) {
        return;
    }
    m_fetched = false;
    PlayScopeAnimation(true, m_reloadPending);
    m_actionTimeLeft = m_scopeTime;
    m_action = 7;
    if (m_scopeAttached) {
        GameApp()->m_soundManager->PlaySound("IDM_SCOPE_OFF", false, false);
    }
    if (CSwerveGame::m_pInstance != NULL) {
        CSwerveGame::m_pInstance->onScopeOutStart();
    }
}

void CDH_Weapon::ResetInScropeRoutine()
{
    if (m_state == 1 && m_reloadPending) {
        m_reloadPending = false;
        GameApp()->m_helper->m_swerveGame->ResetInScopeRoutine();
    }
}

void CDH_Weapon::PlayReloadAnimation()
{
    if (m_secondaryAnimation != NULL) {
        m_secondaryAnimation->stopAllAnimation();
    }
    if (m_animation != NULL) {
        int id = m_reloadStartAnimId;
        if (id < 0) {
            id = m_reloadAnimId;
        }
        m_animation->setCurrentController(id, false);
    }
}

void CDH_Weapon::PlaySpinStartAnimation()
{
    if (m_secondaryAnimation != NULL) {
        m_secondaryAnimation->stopAllAnimation();
    }
    if (m_animation != NULL) {
        m_animation->setCurrentController(m_startShootAnimId, false);
    }
    GameApp()->m_soundManager->PlaySound("IDM_MINI_GUN_ROTATE_START", false, false);
}

XString CDH_Weapon::GetStatsString() const
{
    XString scope = m_scopeAttached ? Window::ResString("IDS_SHOP_WEAPON_STATS_YES")
                                    : Window::ResString("IDS_SHOP_WEAPON_STATS_NO");
    XString reload;
    if (m_reloadTime < 1500) {
        reload = Window::ResString("IDS_SHOP_WEAPON_STATS_HIGH");
    } else if (m_reloadTime <= 2500) {
        reload = Window::ResString("IDS_SHOP_WEAPON_STATS_MEDIUM");
    } else {
        reload = Window::ResString("IDS_SHOP_WEAPON_STATS_LOW");
    }
    XString accuracy;
    if (wanderAmount() < 5.0f) {
        accuracy = Window::ResString("IDS_SHOP_WEAPON_STATS_HIGH");
    } else if (m_reloadTime >= 6 && m_reloadTime <= 10) {
        accuracy = Window::ResString("IDS_SHOP_WEAPON_STATS_MEDIUM");
    } else {
        accuracy = Window::ResString("IDS_SHOP_WEAPON_STATS_LOW");
    }
    XString magazine;
    if (GetActiveAmmo() == NULL) {
        magazine = L"\x221e";  // infinity sign
    } else {
        magazine = XString(m_clipSize);
    }
    XString result;
    if (m_bonusXp > 0) {
        result.Append(XString::Format(Window::ResString("IDS_SHOP_WEAPON_STATS_BONUS_XP").c_str(), m_bonusXp) + L"\n");
    }
    result.Append(XString::Format(Window::ResString("IDS_SHOP_WEAPON_STATS").c_str(), (int)effectiveRange(),
                                  scope.c_str(), magazine.c_str(), reload.c_str(), accuracy.c_str()));
    return result;
}

int CDH_Weapon::getRPM() const
{
    return 60000 / m_rateOfFire;
}

XString CDH_Weapon::GetWeaponCategoryName(EWeaponCategory category)
{
    switch (category) {
    case Category_Handgun:
        return Window::ResString("IDS_FILTER_HANDGUN");
    case Category_Automatic:
        return Window::ResString("IDS_FILTER_AUTOMATIC");
    case Category_Sniper:
        return Window::ResString("IDS_FILTER_SNIPER");
    case Category_Shotgun:
        return Window::ResString("IDS_FILTER_SHOTGUN");
    case Category_Exotic:
        return Window::ResString("IDS_FILTER_EXOTIC");
    }
    return XString("");
}

void CDH_Weapon::SetCyberScopeEquiped(bool equipped)
{
    m_cyberScopeEquipped = equipped;
}
