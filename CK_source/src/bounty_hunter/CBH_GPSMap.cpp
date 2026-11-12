#include "CBH_GPSMap.h"
#include "CBH_Math.h"

#include "App.h"
#include "CAchievementManager.h"
#include "CBH_GPSHud.h"
#include "CBH_InformantsHud.h"
#include "CBH_MenuButton.h"
#include "CBH_NetworkWindow.h"
#include "CBH_PersonType.h"
#include "CBH_PersonTypeManager.h"
#include "CBH_Player.h"
#include "CBH_SurfaceManager.h"
#include "CChallenge.h"
#include "CChallengeManager.h"
#include "CChallengesWindow.h"
#include "CDailyDealManager.h"
#include "CDailyDealWindow.h"
#include "CDH_SoundManager.h"
#include "CDH_Weapon.h"
#include "CDH_WeaponsManager.h"
#include "CDialogWindow.h"
#include "CFactionType.h"
#include "CFactionTypeManager.h"
#include "CFiveDaysBonus.h"
#include "CFiveDaysBonusDialog.h"
#include "CFiveDaysIncentiveBonus.h"
#include "CFriendGatedMission.h"
#include "CFriendGatedMissionManager.h"
#include "CFriendsWindow.h"
#include "CGameAnalytics.h"
#include "CGServeHandler.h"
#include "CGServeHelper.h"
#include "CHuntingInfo.h"
#include "CIncentivizedWindow.h"
#include "CIntroduceItemWindow.h"
#include "CLocation.h"
#include "CLocationManager.h"
#include "CNationalityType.h"
#include "CMissionManager.h"
#include "CNewsFeedManager.h"
#include "CNetworkAvailability.h"
#include "CNGS.h"
#include "GameCenter_Facade.h"
#include "CNGSSocialInterface.h"
#include "CNGS_Platform.h"
#include "CNGSLocalUser.h"
#include "LocalPushManager.h"
#include "QuickSort.h"
#include "CBH_GPSScan.h"
#include "CTimerWindow.h"
#include "CIAPCurrencyManager.h"
#include "CAmmoManager.h"
#include "CMiscItemManager.h"
#include "CEnergyManager.h"
#include "CArmorManager.h"
#include "CGoPillManager.h"
#include "CMedicantManager.h"
#include "CAmmoPackManager.h"
#include "CSubContractManager.h"
#include "CGPSSubContract.h"
#include "AppSettings.h"
#include "CNGSLoginFlow.h"
#include "CNGSRemoteUser.h"
#include "CNGSRemoteUserList.h"
#include "CNotificationHandler.h"
#include "CPHInterface.h"
#include "CProgressSystem.h"
#include "CRandGen.h"
#include "CRegularShop.h"
#include "CSaveManager.h"
#include "CScriptManager.h"
#include "CShopItem.h"
#include "CSocialGiftsManager.h"
#include "CSquadTypeManager.h"
#include "CStdUtil_Cocoa.h"
#include "CTutorialManager.h"
#include "CTjManager.h"
#include "CVisualCharacterType.h"
#include "CWindowQueue.h"
#include "CXmlHelper.h"
#include "DGHelper.h"
#include "ICSystemFont.h"
#include "MathLib.h"
#include "ResMap.h"
#include "SScreenAdaptingConstant.h"
#include "TapjoyInterface.h"
#include "tinyxml.h"
#include "gps/CGPSMission.h"
#include "gps/CGPSBounty.h"
#include "gps/CGPSSubContract.h"

namespace {
// CK __MergedGlobals order (__GLOBAL__I_a): 0x00, 0x10, 0x20, 0x30.
SScreenAdaptingConstant ScrollEdgeConstant(0x140, 0x280, 0x300);  // used by UpdateScroll (not mapped yet)
SScreenAdaptingConstant SubContractBottomInset(50, 100, 90);
SScreenAdaptingConstant MapWidthInset(20, 40);
SScreenAdaptingConstant MapHeightInset(20, 40);

}  // namespace

// CK: these headers' ClassId statics are initialized after the map constants (__GLOBAL__I_a order).
#include "CNGSOfferManager.h"
#include "ICInAppPurchase.h"

namespace {
SScreenAdaptingConstant ZeroConstant(0, 0);  // __MergedGlobals+0x40; user not mapped yet
const int AcceptMissionDialog = 0x255fe8ff;
const int CancelMissionDialog = -0x17cc439d;
const int ViewNewChallenges = 0x79399e3c;
const int LowMoneyGetCommand = -0x17b4f1c;  // ShowLowMoneyDialog second button
const int ConfirmInformant = 0x43a30531;
const int CancelInformant = 0x43a30532;
const int OpenMissionAmmoShop = 0x43a30533;
const int OpenMissionWeaponShop = 0x43a30534;
const int OpenMissionLoadout = 0x43a30535;
const int AcknowledgeWhatsNew = 0x137fed7f;
const int RocketLauncherId = 28;
const int ChooseTargetFriend = static_cast<int>(0x93e847d9u);
const int OfferFriendGatedMission = static_cast<int>(0xdc4219a8u);

int ClampMapCoordinate(int position, int minimum, int maximum)
{
    if (position < minimum) {
        return minimum;
    }
    if (position > maximum) {
        return maximum;
    }
    return position;
}

float LimitScrollStep(float distance, float limit)
{
    if (distance > limit) {
        return limit;
    }
    if (distance < -limit) {
        return -limit;
    }
    return distance;
}

float RestingScrollPosition(float position, int screenSize, int mapSize, int inset)
{
    int minPosition = screenSize - mapSize;
    int maxPosition = -inset;
    if (minPosition > maxPosition) {
        return (screenSize - mapSize) * 0.5f;
    }
    if (position < minPosition) {
        return minPosition;
    }
    if (position > maxPosition) {
        return maxPosition;
    }
    return position;
}

float SlowScroll(float velocity, float distanceToEdge, int elapsed, int scale)
{
    float resistance = 2000.0f * scale;
    float brake = 0.0008f * scale * elapsed * (resistance + distanceToEdge * distanceToEdge) / resistance;
    velocity -= LimitScrollStep(velocity, brake);
    if (MathLib::Fabs(velocity) < 0.01f) {
        return 0.0f;
    }
    return velocity;
}

void ShowEquipmentWarning(Window* parent, const XString& message, int action, int itemId, bool canContinue)
{
    CDialogWindow* dialog = new CDialogWindow(true);
    dialog->SetTitleText("IDS_DIALOG_TITLE_WARNING");
    dialog->SetMessageText(message);
    CDialogWindow::EDialogButton button = CDialogWindow::BUTTON_NONE;
    if (action == OpenMissionAmmoShop) {
        button = CDialogWindow::BUTTON_AMMO;
    } else if (action == OpenMissionWeaponShop) {
        button = CDialogWindow::BUTTON_WEAPON;
    } else if (action == OpenMissionLoadout) {
        button = CDialogWindow::BUTTON_EQUIP;
    }
    if (canContinue) {
        dialog->SetButtons(CDialogWindow::BUTTON_OK, CDialogWindow::BUTTON_IGNORE, button);
        dialog->SetButtonsCommands(CancelMissionDialog, AcceptMissionDialog, action);
        dialog->SetButtonsCommandFirstParam(0, 0, itemId);
    } else {
        dialog->SetButtons(CDialogWindow::BUTTON_OK, button);
        dialog->SetButtonsCommands(CancelMissionDialog, action, -1);
        dialog->SetButtonsCommandFirstParam(0, itemId, 0);
    }
    parent->AddModal(dialog);
}

CDH_Weapon* FindGatedWeapon(CDH_BasicGameData* data, int id, CDH_Weapon::EWeaponCategory category)
{
    CDH_Weapon* weapons[] = {data->GetWeapon01(true), data->GetWeapon02(true)};
    for (int index = 0; index < 2; ++index) {
        CDH_Weapon* weapon = weapons[index];
        if (weapon != NULL && ((id >= 0 && weapon->m_id == id) || (id < 0 && weapon->m_category == category))) {
            return weapon;
        }
    }
    return NULL;
}
}

class CChallengesDialog : public CTextDialog {
public:
    CChallengesDialog();
    void SetChallengeText(const XString& text);
};

CChallengesDialog::CChallengesDialog()
{
    SetTitleText("IDS_CHALLENGE_NEW_CHALLENGES_TITLE");
    SetButtons(BUTTON_OK, BUTTON_IGNORE);
    SetButtonsCommands(ViewNewChallenges, -1, -1);
    m_buttons[0]->SetCaption("IDS_VIEW");
    m_buttons[1]->setImage("IDB_BUTTON_SET_BOUNTY");
    m_buttons[1]->setPressedImage("IDB_BUTTON_SET_BOUNTY_ACTIVE");
    m_buttons[1]->SetCaption("IDS_CONTINUE");
    m_content->SetPercentWidth(70, 0, 0);
}

void CChallengesDialog::SetChallengeText(const XString& text)
{
    XString message = ResString("IDS_CHALLENGE_NEW_CHALLENGES_TEXT");
    message.Append(XString(L"\n\n"));
    message.Append(text);
    SetMessageText(message, CFontMgr::GetInstance()->GetFont(15));
}

bool CBH_GPSMap::m_isFirstLoading = true;
CBH_GPSMap* CBH_GPSMap::m_instance;
bool CBH_GPSMap::shopIsPainting;
bool CBH_GPSMap::weaponIsPainting;
bool CBH_GPSMap::safehouseIsPainting;

CBH_GPSMap::CBH_GPSMap()
    : CBaseScreen(false, false, false, true, false, true), m_startDialogs(NULL), m_showFeaturedInstall(true),
      m_backgroundCount(0), m_currentChallengeDay(-1), m_mapWidth(0), m_mapHeight(0), m_columns(0), m_rows(0),
      m_mapX(0), m_mapY(0), m_pendingCommand(0), m_lastDailyDealDisplay(CStdUtil_Cocoa::GetTimeSeconds()),
      m_field144(false), m_showRocketLauncherIntro(true), m_draggingMap(false), m_field150(false),
      m_field180(false), m_field184(0), m_field188(0), m_field18d(false), m_field194(0), m_field198(0),
      m_field19c(0), m_gpsScan(NULL), m_field1a4(false), m_scrollState(0), m_scrollVelocityX(0.0f), m_scrollVelocityY(0.0f),
      m_scrollSpeedX(0.0f), m_scrollSpeedY(0.0f), m_scrollTargetX(0.0f), m_scrollTargetY(0.0f), m_scrollX(0.0f),
      m_scrollY(0.0f), m_previousScrollX(0.0f), m_previousScrollY(0.0f),
      m_recordedMapDrag(true), m_shortcuts(NULL), m_cooperation(NULL), m_bar(NULL), m_shop(NULL), m_network(NULL), m_needSave(false),
      m_prefetchIndex(0), m_socialNetworkMode(1),
      m_hud(NULL), m_selectedCamera(NULL),
      m_selectedInformant(NULL), m_priorDialogIndex(0), m_advanceMissionDialog(false),
      m_showingHuntConfirmation(false), m_acceptInformant(false), m_showTutorialFinish(false),
      m_dailyDealShown(false), m_nextGiftSaveAttempt(0), m_zombiesPromo(NULL),
      m_nextChallengeSaveAttempt(0), m_mapIdleElapsed(0), m_bonusTeaserElapsed(0),
      m_scrollToItem(false), m_openTargetPickerAfterLogin(false),
      m_friendGatedMission(NULL), m_friendGatedBoss(NULL)
{
    SetFlags(0x40);
    LoadTiles();
    m_gpsScan = new CBH_GPSScan();
    if (m_scrollState != 1) {
        m_scrollState = 1;
        m_scrollX = static_cast<float>(m_mapX);
        m_scrollY = static_cast<float>(m_mapY);
    }
    m_instance = this;
    m_startDialogs = new CWindowQueue();
    AddToFront(m_startDialogs);
    InitPrefetchedManager();
    shopIsPainting = false;
    safehouseIsPainting = false;
    weaponIsPainting = false;
}

CBH_GPSMap::~CBH_GPSMap()
{
    Save();
    SAFE_DELETE_VECTOR(m_tiles);
    if (m_gpsScan != NULL) {
        delete m_gpsScan;
        m_gpsScan = NULL;
    }
    // Every map item (haunts, cameras' owners, informants, icons) is owned by m_items.
    SAFE_DELETE_VECTOR(m_items);
    ReleasePrefetchedManagerResource();
    m_instance = NULL;
}

void CBH_GPSMap::LoadTiles()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    ResMap* resources = app->InitResMap();
    char* xml = resources->loadText(resources->getFilename("GPS_MAP_DESCRIPTION"));
    TiXmlDocument document;
    document.Parse(xml);
    delete[] xml;

    const TiXmlElement* maps = document.FirstChildElement("maps");
    if (maps == NULL) {
        return;
    }
    const TiXmlElement* map = maps->FirstChildElement("map");
    if (map != NULL) {
        SetTileLayout(*map);
    }
}

void CBH_GPSMap::InitPrefetchedManager()
{
    if (m_prefetchedManagers.size() != 0) {
        return;
    }
    m_prefetchedManagers.ensureCapacity(10);
    App* app = static_cast<App*>(WindowApp::m_instance);
    m_prefetchedManagers.addElement(app->InitWeaponManager());
    m_prefetchedManagers.addElement(app->InitAmmoManager());
    m_prefetchedManagers.addElement(app->m_ammoPackManager);
    m_prefetchedManagers.addElement(app->InitIAPCurrencyManager());
    m_prefetchedManagers.addElement(app->m_medicantManager);
    m_prefetchedManagers.addElement(app->m_goPillManager);
    m_prefetchedManagers.addElement(app->m_armorManager);
    m_prefetchedManagers.addElement(app->m_energyManager);
    m_prefetchedManagers.addElement(app->InitMiscItemManager());
}

void CBH_GPSMap::Init()
{
    CreateBar();
    CreateShop();
    CreateNetwork();
    CreateInstantInformants();
    CreateHaunts();
    CreateInformantsImpl(&m_missionInformants[0], L"res/common/configs/informants.xml");
    CreateAlternativeStoryLineInformants();
    CreateZombiesPromoIcon();
    CreateCooperationIcon();
    m_shortcuts = new CBH_InformantsHud();
    AddToFront(m_shortcuts);
    Load();

    App* app = static_cast<App*>(WindowApp::m_instance);
    if (!CTutorialManager::IsPlayerInTutorial()) {
        CheckForNewChallenges();
    }
    if (CBH_Player::GetInstance()->m_fieldb4 > 0 && CBH_Player::GetInstance()->m_energy < 3) {
        CBH_Player::GetInstance()->m_gameStats.AddOutOfEnergyTimes(1);
    }

    // A hunt result is pending when the last hunt item is still set.
    bool huntPending = false;
    if (CBH_Player::GetInstance()->m_lastHuntResult != 0 && CBH_Player::GetInstance()->m_lastHuntItem >= 0) {
        huntPending = true;
    }
    bool weaponGated = app->m_huntingInfo->m_fieldc0;

    // Decide which shop page (if any) opens on top of the map.
    if (CTutorialManager::IsShowShop() == true) {
        ShowShop(static_cast<CShopWindow::ETab>(CTutorialManager::GetCurrentShopTab()), false);
        CGameAnalytics::logStoreEnter(static_cast<CGameAnalytics::EShopEnterFrom>(0));
    } else if (CTutorialManager::IsShowHome() == true) {
        CBH_NetworkWindow::GetInstance()->BringToFrontModal(app->m_mainLayer);
    } else if (!huntPending && app->m_huntingInfo->m_assignedData.m_weaponGated
               && app->m_huntingInfo->m_assignedData.m_requiredWeaponId == -1) {
        CDH_Weapon* weapon = GetMostExpensiveWeaponInCategory(app->m_huntingInfo->m_assignedData.m_weaponCategory);
        if (weapon == NULL) {
            return;
        }
        if (!weapon->isAvailable()) {
            ShowShopWithWeapon(*weapon);
        }
    } else if (CBH_Player::GetInstance()->m_lastHuntItem >= 0
               && GetCameraById(CBH_Player::GetInstance()->m_lastHuntItem)->GetGPSHuntingItem()->m_randomWeapon != -1) {
        CGPSHuntingItem* item = GetCameraById(CBH_Player::GetInstance()->m_lastHuntItem)->GetGPSHuntingItem();
        ShowShopWithUnlockedWeapon(item->m_randomWeapon, CShopItemInfoWindow::SOURCE_RANDOM_WEAPON_MISSION);
        CGameAnalytics::logStoreEnter(static_cast<CGameAnalytics::EShopEnterFrom>(0));
    } else if (weaponGated) {
        Vector<int> unlocked;
        app->InitWeaponManager()->GetUnlockedWeaponsIdOnLevel(CBH_Player::GetInstance()->m_level, unlocked);
        if (unlocked.size() > 0) {
            ShowShopWithUnlockedWeapon(GetMostExpensiveWeaponId(unlocked), static_cast<CShopItemInfoWindow::ESource>(1));
            CGameAnalytics::logStoreEnter(static_cast<CGameAnalytics::EShopEnterFrom>(0));
        }
    } else if (CBH_Player::GetInstance()->m_lastHuntItem >= 0 && !CTutorialManager::IsPlayerInTutorial()
               && app->m_helper->GetGameData()->GetWeapon01(true)->GetActiveAmmo() != NULL
               && app->m_helper->GetGameData()->GetWeapon02(true)->GetActiveAmmo() != NULL) {
        ShowShop(CShopWindow::TAB_AMMO, false);
        CGameAnalytics::logStoreEnter(static_cast<CGameAnalytics::EShopEnterFrom>(0));
    }

    // Hunt was aborted: report the failed mission.
    if (CBH_Player::GetInstance()->m_lastHuntResult == 0 && CBH_Player::GetInstance()->m_lastHuntItem >= 0) {
        if (!huntPending) {
            CGPSHuntingItem* item = GetCameraById(CBH_Player::GetInstance()->m_lastHuntItem)->GetGPSHuntingItem();
            XString missionName;
            if (item != NULL) {
                if (item->GetType() == 1) {
                    CBountyManager::SBountyGeneralInfo info =
                        *app->m_bountyManager->GetInfo(static_cast<CGPSBounty*>(item)->m_bountyType);
                    missionName = info.m_name;
                } else {
                    missionName = XString::Format(L"mission_%d", item->GetMissionId());
                }
                CGameAnalytics::logMissionFailed(app->m_huntingInfo->m_assignedData.m_missionId, missionName);
            }
        }
        if (CTutorialManager::IsPlayerInTutorial() == true && !CBH_Player::GetInstance()->HasAnyAmmo()) {
            CBH_Player* player = CBH_Player::GetInstance();
            player->SetAmmoForWeaponId(app->m_helper->GetGameData()->GetWeapon01(true)->m_id, 1);
        }
    }

    // Hunt finished: complete the mission on its camera.
    if (CBH_Player::GetInstance()->m_lastHuntResult != 0 && CBH_Player::GetInstance()->m_lastHuntItem >= 0) {
        int cameraId = CBH_Player::GetInstance()->m_lastHuntItem;
        CGPSHauntCamera* camera = GetCameraById(cameraId);
        CGPSHuntingItem* item = camera->GetGPSHuntingItem();
        if (item != NULL) {
            if (item->GetType() == 0) {
                if (static_cast<unsigned int>(CBH_Player::GetInstance()->m_completedMissions.size() - 4) < 3
                    && !CBH_Player::GetInstance()->m_appRated) {
                    CDialogWindow* dialog = new CDialogWindow(true);
                    dialog->SetButtons(CDialogWindow::BUTTON_OK, CDialogWindow::BUTTON_LATER, CDialogWindow::BUTTON_NONE);
                    dialog->SetTitleText("IDS_RATE_ME_HEADER");
                    dialog->SetMessageText(ResString("IDS_RATE_ME_TEXT"), NULL);
                    dialog->SetButtonsCommands(AcceptMissionDialog, CancelMissionDialog, -1);
                    m_pendingCommand = 0xe;
                    CDialogWindow* queued = dialog;
                    m_dialogsQueue.insertElementAt(queued, 0);
                    ShowNextQueueDialog(false);
                }
                CBH_Player::GetInstance()->OnMissionFinished(item->GetMissionId());
                CGameAnalytics::logCameraUnlocked(camera->m_id);
            } else if (item->GetType() == 1) {
                for (int index = 0; index < m_instantInformants.size(); ++index) {
                    CGPSInstantInformant* informant = m_instantInformants[index];
                    if (informant->m_cameraId == cameraId) {
                        informant->OnBountyCompleted();
                    }
                }
            }
            camera->HasTargettingMission();
            camera->OnCameraBountyComplete();
        }
        if (!CTutorialManager::IsPlayerInTutorial()) {
            CBH_Player::GetInstance()->MissionCompleted();
        }
    }

    CBH_Player* player = CBH_Player::GetInstance();
    player->m_lastHuntItem = -1;
    player->Save();
    if (CTutorialManager::IsAutoStartMission() == true) {
        StartTutorialMission();
    } else {
        m_hud = new CBH_GPSHud(false);
        int titleHeight = GetTitleBarHeight();
        m_hud->SetOrigin(0, titleHeight);
        AddToFront(m_hud);
        CheckGameCenterAccounts();
    }
    UpdateMissionInformants();
    app->m_challengeManager->Save();
    Save();
    if (CTutorialManager::IsPlayerInTutorial() == true) {
        app->m_tutorialManager->ShowStateInitMessage();
    }
    m_openFriendsWindow = false;
    m_field2b5 = false;
}

void CBH_GPSMap::SetTileLayout(const TiXmlElement& map)
{
    map.Attribute("width", &m_mapWidth);
    map.Attribute("height", &m_mapHeight);
    map.Attribute("tiles_count_x", &m_columns);
    map.Attribute("tiles_count_y", &m_rows);
    m_mapWidth -= MapWidthInset();
    m_mapHeight -= MapHeightInset();
    m_mapX = WindowApp::m_instance->m_screenWidth / 2 - m_mapWidth / 2;
    m_mapY = WindowApp::m_instance->m_screenHeight / 2 - m_mapHeight / 2;
    m_previousScrollX = m_scrollX = m_mapX;
    m_previousScrollY = m_scrollY = m_mapY;
}

void CBH_GPSMap::AddMapPatch(ICRenderSurface* surface)
{
    m_tiles.addElement(surface);
}

void CBH_GPSMap::OnHauntSelected(CGPSHaunt* haunt)
{
    CDialogWindow* dialog = new CDialogWindow(false);
    dialog->SetTitleText(haunt->m_screenName);
    dialog->SetMessageText(haunt->m_description);
    dialog->SetButtons(CDialogWindow::BUTTON_OK);
    m_parent->AddModal(dialog);
    ScrollToItem(*haunt, 0.2f);
}

void CBH_GPSMap::OnCameraSelected(CGPSHauntCamera* camera)
{
    CGPSHuntingItem* item = camera->GetGPSHuntingItem();
    if (camera->HasTempMission()) {
        item = camera->m_temporaryItem;
    }
    if (item == NULL) {
        return;
    }
    m_selectedCamera = camera;
    m_priorDialogIndex = 0;
    m_showingHuntConfirmation = false;
    if (!MakePlayerEnergyCheck(item->m_energyCost) || !CheckMissionEquipment(camera, item)) {
        m_selectedCamera = NULL;
        return;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_soundManager->PlaySound("IDM_MENU_WINDOW_OPEN_CLOSE", false, false);
    ShowMissionDialog();
}

bool CBH_GPSMap::MakePlayerEnergyCheck(int cost)
{
    CBH_Player* player = CBH_Player::GetInstance();
    if (!player->IsEnegyEnabled() || player->m_energy >= cost) {
        return true;
    }
    XString message = ResString("IDS_MESSAGE_NO_ENERGY");
    ShowEquipmentWarning(m_parent, XString::Format(message.c_str(), cost), -1, -1, false);
    CGameAnalytics::logNotEnoughEnergy(player->m_energy);
    return false;
}

bool CBH_GPSMap::MakeLowAmmoCheck(const CDH_Weapon* weapon)
{
    if (weapon == NULL || CBH_Player::GetInstance()->GetAvailableAmmo(weapon->GetDefaultAmmo(), true) >= weapon->m_clipSize) {
        return true;
    }
    XString message = ResString("IDS_MESSAGE_LOW_AMMO_IN_MAIN_WEAPON");
    ShowEquipmentWarning(m_parent, XString::Format(message.c_str(), weapon->m_name.c_str()),
                         OpenMissionAmmoShop, -1, true);
    return false;
}

bool CBH_GPSMap::MakePlayerAmmoCheck()
{
    CBH_Player* player = CBH_Player::GetInstance();
    if (!player->HasAnyAmmo()) {
        ShowEquipmentWarning(m_parent, ResString("IDS_MESSAGE_NO_AMMO"), OpenMissionAmmoShop, -1, false);
        return false;
    }
    if (!player->HasAmmoInBothWeapons()) {
        ShowEquipmentWarning(m_parent, ResString("IDS_MESSAGE_NO_AMMO_IN_ONE_WEAPON"), OpenMissionAmmoShop, -1, true);
        return false;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_BasicGameData* data = app->m_helper->GetGameData();
    return MakeLowAmmoCheck(data->GetWeapon01(true)) && MakeLowAmmoCheck(data->GetWeapon02(true));
}

bool CBH_GPSMap::MakePlayerTranqMissionCheck()
{
    CBH_Player* player = CBH_Player::GetInstance();
    if (!player->HasTranqWeapon()) {
        ShowEquipmentWarning(m_parent, ResString("IDS_MESSAGE_NOT_READY_TO_TRANQ_NO_WEAPON"),
                             OpenMissionWeaponShop, -1, false);
        return false;
    }
    if (!player->HasTranqWeaponWithin()) {
        ShowEquipmentWarning(m_parent, ResString("IDS_MESSAGE_NOT_READY_TO_TRANQ_NO_WEAPON_WITHIN"),
                             OpenMissionLoadout, -1, false);
        return false;
    }
    if (!player->HasAmmoForTranqWeaponWithin()) {
        ShowEquipmentWarning(m_parent, ResString("IDS_MESSAGE_NOT_READY_TO_TRANQ_NO_AMMO"),
                             OpenMissionAmmoShop, -1, false);
        return false;
    }
    return true;
}

bool CBH_GPSMap::MakePlayerWeaponRangeCheck(float range)
{
    if (range < 0.0f) {
        return true;
    }
    CBH_Player* player = CBH_Player::GetInstance();
    bool hasWeapon = player->HasWeaponWithRange(range);
    if (hasWeapon && player->HasWeaponWithRangeInHands(range)) {
        return true;
    }
    XString message = ResString("IDS_MESSAGE_NOT_READY_TO_SMALL_RANGE_WEAPON");
    int action = OpenMissionWeaponShop;
    if (hasWeapon) {
        action = OpenMissionLoadout;
    }
    ShowEquipmentWarning(m_parent, XString::Format(message.c_str(), static_cast<int>(range)), action, -1, false);
    return false;
}

bool CBH_GPSMap::MakeSilentWeaponCheck()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_BasicGameData* data = app->m_helper->GetGameData();
    CDH_Weapon* first = data->GetWeapon01(true);
    CDH_Weapon* second = data->GetWeapon02(true);
    if ((first != NULL && first->isSilent() && !first->m_tranquillizer)
        || (second != NULL && second->isSilent() && !second->m_tranquillizer)) {
        return true;
    }
    ShowEquipmentWarning(m_parent, ResString("IDS_MESSAGE_NOT_READY_SILENT_WEAPON"), OpenMissionLoadout, -1, false);
    return false;
}

bool CBH_GPSMap::CheckMissionEquipment(CGPSHauntCamera* camera, CGPSHuntingItem* item)
{
    const bool weaponLoan = item->m_randomWeapon >= 0 || item->GetEstimatedWeaponID() >= 0;
    if (!weaponLoan && item->GetType() == 0 && !MakePlayerWeaponCheck(item->GetMissionId())) {
        return false;
    }
    if (camera->m_durationMs < 0 && item->m_tranquillize && !weaponLoan && !MakePlayerTranqMissionCheck()) {
        return false;
    }
    if (!weaponLoan && !MakePlayerWeaponRangeCheck(camera->m_minWeaponRange)) {
        return false;
    }
    if (camera->m_durationMs <= 0 && item->m_silentKill && !weaponLoan && !MakeSilentWeaponCheck()) {
        return false;
    }
    if (!item->m_weaponGated) {
        return weaponLoan || MakePlayerAmmoCheck();
    }

    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_Weapon::EWeaponCategory category = CDH_Weapon::GetWeaponCategoryType(camera->m_gatedWeaponCategory);
    CDH_BasicGameData* data = app->m_helper->GetGameData();
    CDH_Weapon* weapon = FindGatedWeapon(data, item->m_gatedWeapon, category);
    if (weapon == NULL) {
        if (item->m_gatedWeapon >= 0) {
            return MakePlayerWeaponGatedIDCheck(item->m_gatedWeapon);
        }
        return MakePlayerWeaponGatedCategoryCheck(category);
    }
    if (CBH_Player::GetInstance()->GetAvailableAmmo(weapon->GetActiveAmmo(), true) <= 0) {
        ShowEquipmentWarning(m_parent, ResString("IDS_MESSAGE_NO_AMMO"), OpenMissionAmmoShop, -1, false);
        return false;
    }
    return true;
}

bool CBH_GPSMap::MakePlayerWeaponGatedCategoryCheck(CDH_Weapon::EWeaponCategory category)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (FindGatedWeapon(app->m_helper->GetGameData(), -1, category) != NULL) {
        return true;
    }
    CDH_Weapon* recommended = GetMostExpensiveWeaponInCategory(category);
    int itemId = -1;
    int action = OpenMissionWeaponShop;
    if (recommended != NULL) {
        itemId = recommended->m_id;
        if (recommended->isAvailable()) {
            action = OpenMissionLoadout;
        }
    }
    XString name = CDH_Weapon::GetWeaponCategoryName(category);
    XString message = ResString("IDS_MESSAGE_YOU_NEED_WEAPON");
    ShowEquipmentWarning(m_parent, XString::Format(message.c_str(), name.c_str()), action, itemId, false);
    return false;
}

bool CBH_GPSMap::MakePlayerWeaponGatedIDCheck(int id)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_Weapon* required = app->InitWeaponManager()->GetWeaponById(id);
    if (required == NULL || FindGatedWeapon(app->m_helper->GetGameData(), id, required->m_category) != NULL) {
        return true;
    }
    int action = OpenMissionWeaponShop;
    if (required->isAvailable()) {
        action = OpenMissionLoadout;
    }
    XString message = ResString("IDS_MESSAGE_YOU_NEED_WEAPON");
    ShowEquipmentWarning(m_parent, XString::Format(message.c_str(), required->m_name.c_str()), action, id, false);
    return false;
}

void CBH_GPSMap::ShowShop(CShopWindow::ETab tab, bool openTapJoy)
{
    CRegularShop* shop = CRegularShop::GetInstance(true);
    shop->SetTab(tab);
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_mainLayer->AddModal(shop);
    if (openTapJoy) {
        shop->TryOpenTapJoy();
    }
}

CDH_Weapon* CBH_GPSMap::GetMostExpensiveWeaponInCategory(int category) const
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_WeaponsManager* manager = app->InitWeaponManager();
    CDH_Weapon* best = NULL;
    int bestPrice = -1;
    for (CItem* item = manager->firstItem(); item != NULL; item = manager->nextItem()) {
        CDH_Weapon* weapon = static_cast<CDH_Weapon*>(item);
        if (!weapon->IsPriceless() && weapon->m_category == category && weapon->getPriceCredits() > bestPrice) {
            best = weapon;
            bestPrice = weapon->getPriceCredits();
        }
    }
    return best;
}

void CBH_GPSMap::ShowShopWithWeapon(const CDH_Weapon& weapon)
{
    CRegularShop* shop = CRegularShop::GetInstance(true);
    shop->SetTab(CShopWindow::TAB_WEAPONS);
    shop->SwitchWeaponFilter(weapon.m_category);
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_mainLayer->AddModal(shop);
    CShopItemInfoWindow* details = new CShopItemInfoWindow(app->InitWeaponManager(), 0, weapon.m_id,
                                                           weapon.GetStatsString(), false,
                                                           CShopItemInfoWindow::SOURCE_SHOP);
    shop->AddModal(details);
}

void CBH_GPSMap::ShowShopWithUnlockedWeapon(int weaponId, CShopItemInfoWindow::ESource source)
{
    CRegularShop* shop = CRegularShop::GetInstance(true);
    shop->SetTab(CShopWindow::TAB_WEAPONS);
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_mainLayer->AddModal(shop);
    CItem* weapon = app->InitWeaponManager()->getItemById(weaponId);
    CShopItemInfoWindow* details = new CShopItemInfoWindow(app->InitWeaponManager(), 0, weaponId,
                                                           weapon->GetStatsString(), false, source);
    shop->AddModal(details);
}

int CBH_GPSMap::GetMostExpensiveWeaponId(const Vector<int>& ids)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_WeaponsManager* manager = app->InitWeaponManager();
    int bestCreditPrice = 0;
    int bestCashPrice = -1;
    int creditItem = -1;
    int cashItem = -1;
    for (int index = 0; index < ids.size(); ++index) {
        CDH_Weapon* weapon = manager->GetWeaponById(ids[index]);
        if (weapon == NULL) {
            continue;
        }
        int credits = weapon->getPriceCredits();
        if (credits > bestCreditPrice) {
            bestCreditPrice = credits;
            creditItem = weapon->m_id;
        }
        if (weapon->m_pricePoints > bestCashPrice) {
            bestCashPrice = weapon->m_pricePoints;
            cashItem = weapon->m_id;
        }
    }
    if (creditItem >= 0) {
        return creditItem;
    }
    return cashItem;
}

bool CBH_GPSMap::MakePlayerWeaponCheck(int missionId)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CMission* mission = app->m_missionManager->GetMissionById(missionId);
    if (mission == NULL) {
        return false;
    }
    const Vector<int>& permitted = mission->m_permittedWeapons;
    if (permitted.size() == 0 || mission->m_estimatedWeaponId >= 0) {
        return true;
    }
    CDH_BasicGameData* data = app->m_helper->GetGameData();
    CDH_Weapon* primary = data->GetWeapon01(true);
    CDH_Weapon* secondary = data->GetWeapon02(true);
    for (int index = 0; index < permitted.size(); ++index) {
        int id = permitted[index];
        if ((primary != NULL && primary->m_id == id) || (secondary != NULL && secondary->m_id == id)) {
            return true;
        }
    }
    int action = OpenMissionWeaponShop;
    XString names;
    CDH_WeaponsManager* weapons = app->InitWeaponManager();
    for (int index = 0; index < permitted.size(); ++index) {
        CDH_Weapon* weapon = weapons->GetWeaponById(permitted[index]);
        if (weapon != NULL) {
            if (names.Length() != 0) {
                names.Append(XString(L", "));
            }
            names.Append(weapon->m_name);
            if (weapon->isAvailable()) {
                action = OpenMissionLoadout;
            }
        }
    }
    if (names.Length() != 0) {
        XString message;
        if (permitted.size() == 1) {
            XString format = ResString("IDS_MESSAGE_YOU_NEED_WEAPON");
            message = XString::Format(format.c_str(), names.c_str());
        } else {
            message = ResString("IDS_MESSAGE_YOU_NEED_ONE_OF_WEAPON");
            message.Append(XString(L"\n"));
            message.Append(names);
        }
        ShowEquipmentWarning(m_parent, message, action, GetMostExpensiveWeaponId(permitted), false);
    }
    return false;
}

void CBH_GPSMap::OnCommand(Event& event)
{
    bool handledShortcut = true;
    switch (event.m_id) {
    case OfferFriendGatedMission:
        ShowBeforeFriendsGatedMission();
        break;
    case ChooseTargetFriend: {
        CIncentivizedWindow* picker = reinterpret_cast<CIncentivizedWindow*>(event.m_param1);
        if (picker != NULL) {
            Vector<int> targets = picker->GetSelectedIds();
            if (targets.size() > 0) {
                picker->Close();
                StartTargettingMission(targets[0], false);
            }
        }
        break;
    }
    case AcknowledgeWhatsNew:
        CBH_Player::GetInstance()->ChangeGameVersion();
        break;
    case CFiveDaysBonusDialog::INCENTIVE_BONUS_ACCEPTED:
        ShowFiveDaysInventiveBonusRecomendedItem();
        break;
    case CBH_InformantsHud::Shop:
        OnShopItemSelected(NULL);
        break;
    case CBH_InformantsHud::Bank:
        CRegularShop::Show(CShopWindow::TAB_BANK);
        break;
    case CBH_InformantsHud::SafeHouse:
        OnGPSNetworkSelected(NULL);
        break;
    case CBH_InformantsHud::Friends:
        OnBarItemSelected(m_bar);
        break;
    case CBH_InformantsHud::Target: {
        CGPSInstantInformant* informant = FindInstantInformant(CGPSInstantInformant::Targeting, false);
        if (informant != NULL) {
            OnGPSInstantInformantSelected(informant);
        }
        break;
    }
    case ViewNewChallenges:
    case CBH_InformantsHud::Coop:
        OnCooperationItemSelected(m_cooperation);
        break;
    case CBH_InformantsHud::Weapon:
    case CBH_InformantsHud::Vehicle: {
        CGPSInstantInformant* informant;
        if (event.m_id == CBH_InformantsHud::Weapon) {
            informant = FindInstantInformant(CGPSInstantInformant::Gated, true);
        } else {
            informant = FindInstantInformant(CGPSInstantInformant::Moving, false);
        }
        if (informant != NULL) {
            OnGPSInstantInformantSelected(informant);
        }
        break;
    }
    default:
        handledShortcut = false;
        break;
    }
    if (handledShortcut) {
        event.Clear();
        return;
    }
    if (event.m_id == OpenMissionAmmoShop || event.m_id == OpenMissionWeaponShop || event.m_id == OpenMissionLoadout) {
        m_selectedCamera = NULL;
        if (event.m_id == OpenMissionAmmoShop) {
            CRegularShop::Show(CShopWindow::TAB_AMMO);
        } else if (event.m_id == OpenMissionWeaponShop) {
            if (event.m_param1 >= 0) {
                CRegularShop::ShowItem(CShopWindow::TAB_WEAPONS, event.m_param1);
            } else {
                CRegularShop::Show(CShopWindow::TAB_WEAPONS);
            }
        } else {
            OnGPSNetworkSelected(NULL);
        }
        event.Clear();
        return;
    }
    if (m_selectedInformant != NULL) {
        if (event.m_id == ConfirmInformant) {
            m_acceptInformant = true;
            event.Clear();
        } else if (event.m_id == CancelInformant) {
            m_selectedInformant = NULL;
            event.Clear();
        }
    }
    if (event.m_id == AcceptMissionDialog && m_dialogsQueue.size() != 0) {
        m_dialogsQueue.removeElementAt(m_dialogsQueue.size() - 1);
        if (ShowNextQueueDialog(false)) {
            event.Clear();
            return;
        }
        if (m_pendingCommand == 0xe) {
            m_pendingCommand = 0;
            CBH_Player* player = CBH_Player::GetInstance();
            player->m_appRated = true;
            player->Save();
            CGameAnalytics::logGameRated(true);
            static_cast<App*>(WindowApp::m_instance)->m_helper->LaunchRateMe();
            event.Clear();
            return;
        }
    } else if (event.m_id == CancelMissionDialog && m_dialogsQueue.size() != 0) {
        if (m_pendingCommand == 0xe) {
            CGameAnalytics::logGameRated(false);
        }
        m_pendingCommand = 0;
        FlushDialogsQueue();
    }
    if (m_selectedCamera == NULL && m_friendGatedMission == NULL) {
        return;
    }
    if (event.m_id == AcceptMissionDialog) {
        m_advanceMissionDialog = true;
        event.Clear();
    } else if (event.m_id == CancelMissionDialog) {
        m_selectedCamera = NULL;
        m_friendGatedMission = NULL;
        m_friendGatedBoss = NULL;
        event.Clear();
    }
}

void CBH_GPSMap::ShowMissionDialog()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CGPSHuntingItem* item = m_selectedCamera->GetGPSHuntingItem();
    if (m_selectedCamera->HasTempMission()) {
        item = m_selectedCamera->m_temporaryItem;
    }
    if (item == NULL) {
        m_selectedCamera = NULL;
        return;
    }
    CMission* mission = app->m_missionManager->GetMissionById(item->GetMissionId());
    if (mission == NULL && item->GetType() != 1) {
        m_selectedCamera = NULL;
        return;
    }
    if (m_selectedCamera->HasTargettingMission()) {
        CBeforeTargettingDialog* targetDialog = CreateBeforeTargettingDialog(true);
        m_showingHuntConfirmation = true;
        m_parent->AddModal(targetDialog);
        return;
    }
    if (mission != NULL && m_priorDialogIndex < mission->m_priorDialogs.size()) {
        const CMission::Dialog& prior = mission->m_priorDialogs[m_priorDialogIndex++];
        ShowMessage(prior.m_title, item->ParametrizeText(prior.m_text), prior.m_image, NULL);
        return;
    }
    if (item->GetType() == 1 && m_priorDialogIndex == 0) {
        ++m_priorDialogIndex;
        const CGPSBounty* bounty = static_cast<const CGPSBounty*>(item);
        if (bounty->GetBountyMessage().Length() != 0) {
            ShowBountyPriorDialogs(bounty, m_selectedCamera->m_haunt, NULL);
            return;
        }
    }
    CBeforeHuntDialog* dialog = CreateBeforeHuntDialog(true);
    if (dialog != NULL) {
        m_showingHuntConfirmation = true;
        m_parent->AddModal(dialog);
    }
}

CGPSHaunt* CBH_GPSMap::GetHauntByName(const XString& name)
{
    for (int index = 0; index < m_haunts.size(); ++index) {
        if (m_haunts[index]->m_name == name) {
            return m_haunts[index];
        }
    }
    return NULL;
}

void CBH_GPSMap::ShowBountyPriorDialogs(const CGPSBounty* bounty, const XString& hauntName, CFont* font)
{
    XString message = bounty->GetBountyMessage();
    CGPSHaunt* haunt = GetHauntByName(hauntName);
    if (haunt != NULL) {
        message = message.Replace(L"[LocationName]", haunt->m_screenName.c_str(), true);
    }
    ShowBountyPriorDialogs(message, bounty->GetObjectiveTitle().Upper(), font);
}

void CBH_GPSMap::ShowBountyPriorDialogs(const XString& message, const XString& title, CFont* font)
{
    ShowMessage(title, message, NULL, font);
}

void CBH_GPSMap::ShowMessage(XString title, XString message, const char* image, CFont* font)
{
    CDialogWindow* dialog = new CDialogWindow(true);
    if (image != NULL) {
        dialog->SetExtraImageFromImageID(image);
    }
    dialog->SetTitleText(title);
    dialog->SetMessageText(message, font);
    dialog->SetButtons(CDialogWindow::BUTTON_OK);
    dialog->SetButtonsCommands(AcceptMissionDialog, CancelMissionDialog, -1);
    m_parent->AddModal(dialog);
}

CBeforeHuntDialog* CBH_GPSMap::CreateBeforeHuntDialog(bool allowLater)
{
    CGPSHuntingItem* item = m_selectedCamera->GetGPSHuntingItem();
    if (m_selectedCamera->HasTempMission()) {
        item = m_selectedCamera->m_temporaryItem;
    }
    if (item == NULL) {
        return NULL;
    }
    PrepareHuntBrief(*m_selectedCamera, *item);
    CBH_Player* player = CBH_Player::GetInstance();
    CBeforeHuntDialog* dialog = new CBeforeHuntDialog();
    dialog->SetExtraImageFromImageID(player->m_missionBriefImage);
    dialog->SetTitleText(player->m_missionBriefTitle);
    dialog->SetMessageText(player->m_missionBriefText);
    dialog->SetMissionRewards(item->m_moneyReward, item->m_experienceReward, item->m_energyCost, false);
    CDialogWindow::EDialogButton later = CDialogWindow::BUTTON_NONE;
    if (allowLater) {
        later = CDialogWindow::BUTTON_LATER;
    }
    dialog->SetButtons(CDialogWindow::BUTTON_OK, later);
    dialog->SetButtonsCommands(AcceptMissionDialog, CancelMissionDialog, -1);
    return dialog;
}

void CBH_GPSMap::PrepareHuntBrief(const CGPSHauntCamera& camera, const CGPSHuntingItem& item)
{
    CBH_Player* player = CBH_Player::GetInstance();
    player->m_missionBriefImage = "IDB_DIALOG_NOTEBOOK";
    if (item.m_boss != NULL && item.m_boss->m_visualCharacter != NULL
        && item.m_boss->m_visualCharacter->m_screenImage != NULL) {
        player->m_missionBriefImage = item.m_boss->m_visualCharacter->m_screenImage;
    }
    player->m_missionBriefTitle = item.GetObjectiveTitle();
    XString goal = item.GetObjectiveGoal();
    CGPSHaunt* haunt = GetHauntByName(camera.m_haunt);
    if (haunt != NULL) {
        goal = goal.Replace(L"[LocationName]", haunt->m_screenName.c_str(), true);
    }
    XString nameLabel = ResString("IDS_DIALOG_NAME");
    XString goalLabel = ResString("IDS_DIALOG_GOAL");
    player->m_missionBriefText = XString::Format(L"%ls: %ls\n%ls: %ls", nameLabel.c_str(),
        item.GetObjectiveBossScreenName().c_str(), goalLabel.c_str(), goal.c_str());
}

void CBH_GPSMap::ShowBeforeFriendsGatedMission()
{
    if (m_selectedCamera != NULL || m_friendGatedMission != NULL) {
        return;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    CBH_Player* player = CBH_Player::GetInstance();
    const CFriendGatedMission* mission = app->m_friendMissionManager->GetMissionById(player->GetCurrentFriendGatedMissionID());
    if (mission == NULL) {
        return;
    }
    const CBountyManager::SBountyGeneralInfo* bounty = app->m_bountyManager->GetInfo(mission->m_bountyType);
    const CFactionType* faction = app->InitFactionTypeManager()->GetRandomFaction();
    if (bounty == NULL || faction == NULL) {
        return;
    }
    CBH_PersonType* boss = app->InitPersonTypeManager()->GetType(faction->GetBoss());
    if (boss == NULL || boss->m_visualCharacter == NULL || boss->m_nationality == NULL) {
        return;
    }
    const CBountyManager::SMessage* brief = app->m_bountyManager->GetRandomTargettingMessage();
    if (brief == NULL) {
        return;
    }

    m_friendGatedMission = mission;
    m_friendGatedBoss = boss;
    m_friendGatedBossName = boss->m_nationality->GetRandomFullName(boss->m_visualCharacter->m_isMan);
    WindowApp::HandleTunnelCommand(0x62b2a2ce, 0, 0, 0);

    CBeforeHuntDialog* dialog = new CBeforeHuntDialog();
    player->m_missionBriefImage = boss->m_visualCharacter->m_screenImage;
    if (player->m_missionBriefImage == NULL) {
        player->m_missionBriefImage = "IDB_DIALOG_NOTEBOOK";
    }
    player->m_missionBriefTitle = brief->m_title;
    XString nameLabel = ResString("IDS_DIALOG_NAME");
    XString goalLabel = ResString("IDS_DIALOG_GOAL");
    player->m_missionBriefText = XString::Format(L"%ls: %ls\n%ls: %ls", nameLabel.c_str(),
        m_friendGatedBossName.c_str(), goalLabel.c_str(), brief->m_text.c_str());
    dialog->SetExtraImageFromImageID(player->m_missionBriefImage);
    dialog->SetTitleText(player->m_missionBriefTitle);
    dialog->SetMessageText(player->m_missionBriefText);
    dialog->SetMissionRewards(mission->m_rewardMoney, mission->m_rewardXp, bounty->m_energy, false);
    dialog->SetButtons(CDialogWindow::BUTTON_OK, CDialogWindow::BUTTON_LATER);
    dialog->SetButtonsCommands(AcceptMissionDialog, CancelMissionDialog, -1);
    m_parent->AddModal(dialog);
}

void CBH_GPSMap::StartNextFriendGatedMission()
{
    const CFriendGatedMission* mission = m_friendGatedMission;
    CBH_PersonType* boss = m_friendGatedBoss;
    m_friendGatedMission = NULL;
    m_friendGatedBoss = NULL;
    App* app = static_cast<App*>(WindowApp::m_instance);
    const CBountyManager::SBountyGeneralInfo* bounty = app->m_bountyManager->GetInfo(mission->m_bountyType);
    if (bounty == NULL || !MakePlayerEnergyCheck(bounty->m_energy)) {
        return;
    }
    const CGPSHauntCamera* camera = GetRandomCamera(false, false);
    if (camera == NULL) {
        ShowEquipmentWarning(m_parent, ResString("IDS_MESSAGE_NO_FREE_CAMERAS"), -1, -1, false);
        return;
    }
    const CLocation* location = app->m_locationManager->GetLocation(camera->m_location);
    CSquadType* squad = app->InitSquadTypeManager()->GetType(boss->m_name);
    if (location == NULL || squad == NULL) {
        return;
    }

    CBH_Player* player = CBH_Player::GetInstance();
    CHuntingInfo::SAssignedData data;
    data.m_bossName = m_friendGatedBossName;
    data.m_bossType = boss;
    data.m_squadType = squad;
    unsigned int seed = CRandGen::GetInstance()->GetRand(0xfffffff);
    XString script = CBountyManager::GetRandomScriptWithSeed(*bounty, mission->m_bountyType,
        location->m_name, camera->m_id, player->GetLevel(), seed);
    if (script.Length() != 0) {
        data.m_scriptType = app->m_scriptManager->GetType(script);
    }
    data.m_civilianCount = app->InitProgressSystem()->GetCivilianCount(player->GetLevel());
    data.m_experienceReward = mission->m_rewardXp;
    data.m_moneyReward = mission->m_rewardMoney;
    data.m_locationName = location->m_name;
    data.m_ambientSound = camera->m_ambientSound;
    data.m_cameraHorizontalLimit = camera->m_horizontalLimit;
    data.m_cameraVerticalLimit = camera->m_verticalLimit;
    data.m_friendGated = true;
    data.m_alertSeconds = bounty->m_alertSeconds;
    data.m_escapeSeconds = bounty->m_escapeSeconds;
    data.m_tripleMultiplierSeconds = app->m_helper->m_config.GetIntegerValueByToken("TIME_XP_MULTI_X3_SEC", true);
    data.m_doubleMultiplierSeconds = app->m_helper->m_config.GetIntegerValueByToken("TIME_XP_MULTI_X2_SEC", true);
    data.m_tranquillize = bounty->m_tranquillize;
    data.m_multiplierCountdown = true;
    data.m_bossFirst = bounty->m_bossFirst;
    data.m_bossWins = bounty->m_bossWins;
    data.m_bossSpawnsLast = bounty->m_bossSpawnsLast;
    data.m_silentKill = bounty->m_silentKill;

    // Borrow the camera's location and limits. This contract does not occupy the camera.
    app->m_huntingInfo->SetHuntingData(data);
    player->m_targetFriendId = -1;
    player->m_missionBriefEnergy = bounty->m_energy;
    player->ReduceEnergy(bounty->m_energy);
    ++player->m_fieldb4;
    player->Save();
    CGameAnalytics::logMissionStarted(data.m_missionName);
    m_parent->SetInterrupt(1);
}

CBeforeTargettingDialog* CBH_GPSMap::CreateBeforeTargettingDialog(bool allowLater)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CGPSHuntingItem* item = m_selectedCamera->GetGPSHuntingItem();
    if (m_selectedCamera->HasTempMission()) {
        item = m_selectedCamera->m_temporaryItem;
    }
    CBeforeTargettingDialog* dialog;
    if (item->m_targetFriendId >= 0) {
        dialog = new CBeforeTargettingDialog(item->m_targetFriendId);
    } else {
        dialog = new CBeforeTargettingDialog(CStrWChar(item->m_targetFBUser.c_str()));
    }
    dialog->SetExtraImageFromImageID("IDB_DIALOG_NOTEBOOK");
    const CBountyManager::SMessage* message = app->m_bountyManager->GetRandomTargettingMessage();
    XString title = item->GetObjectiveTitle();
    XString text = item->GetObjectiveGoal();
    if (message != NULL) {
        title = message->m_title;
        text = item->ParametrizeText(message->m_text);
    }
    text = XString::Format(L"%ls: %ls\n%ls", ResString("IDS_DIALOG_NAME").c_str(), item->m_bossName.c_str(), text.c_str());
    CBH_Player* player = CBH_Player::GetInstance();
    player->m_missionBriefImage = "IDB_DIALOG_NOTEBOOK";
    player->m_missionBriefTitle = title;
    player->m_missionBriefText = text;
    dialog->SetTitleText(title);
    dialog->SetMessageText(text);
    dialog->SetMissionRewards(item->m_moneyReward, item->m_experienceReward, item->m_energyCost, true);
    CDialogWindow::EDialogButton later = CDialogWindow::BUTTON_NONE;
    if (allowLater) {
        later = CDialogWindow::BUTTON_LATER;
    }
    dialog->SetButtons(CDialogWindow::BUTTON_OK, later);
    dialog->SetButtonsCommands(AcceptMissionDialog, CancelMissionDialog, -1);
    return dialog;
}

bool CBH_GPSMap::StartTargettingMission(int friendId, bool revenge)
{
    CGPSInstantInformant* targeting = NULL;
    for (int index = 0; index < m_instantInformants.size(); ++index) {
        if (m_instantInformants[index]->m_type == CGPSInstantInformant::Targeting) {
            targeting = m_instantInformants[index];
            break;
        }
    }
    CGPSHauntCamera* camera = GetRandomCamera(true, false);
    if (camera == NULL || targeting == NULL) {
        CDialogWindow* warning = new CDialogWindow(true);
        warning->SetTitleText("IDS_DIALOG_TITLE_WARNING");
        warning->SetMessageText(ResString("IDS_MESSAGE_NO_FREE_CAMERAS"));
        warning->SetButtons(CDialogWindow::BUTTON_OK);
        m_parent->AddModal(warning);
        return false;
    }
    camera->SetTargettingMissionOnCamera(targeting, friendId, revenge, false);
    if (!camera->HasTargettingMission()) {
        return false;
    }
    targeting->OnBountyGiven(camera->m_id);
    Save();
    OnCameraSelected(camera);
    ScrollToItem(*camera, 1.1f);
    return true;
}

void CBH_GPSMap::RequestTargetPicker()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (app->m_networkAvailability == NULL || !app->m_networkAvailability->IsAvailable()) {
        app->m_gserveHandler->ShowOfflineModeDialog();
        return;
    }
    CNGSLocalUser* local = CNGS::GetInstance()->GetLocalUser();
    if (!local->m_remoteUserLists[0]->isValid()) {
        m_openTargetPickerAfterLogin = true;
        if (!CNGSLoginFlow::GetInstance()->IsRunning()) {
            app->StartLogin(App::LoginBothNetworks, true);
        }
        return;
    }
    ShowSelectFriendDialog();
}

void CBH_GPSMap::ShowSelectFriendDialog()
{
    CNGSRemoteUserList* friends = CNGS::GetInstance()->GetLocalUser()->m_remoteUserLists[0];
    CIncentivizedWindow* picker = NULL;
    for (int index = 0; index < friends->m_users.Size(); ++index) {
        CNGSRemoteUser* user = friends->m_users[index];
        int friendId = user->GetClientID();
        CStrWChar nickname = user->GetNickName();
        if (friendId <= 0 || nickname.GetLength() == 0) {
            continue;
        }
        if (picker == NULL) {
            picker = new CIncentivizedWindow(ResString("IDS_FRIENDS_TARGET"));
            picker->SetTitleText("IDS_FRIENDS_TARGET_TITLE");
            picker->SetSingleSelection();
            picker->SetCommand(ChooseTargetFriend);
            picker->SetEventReciever(this);
        }
        picker->AddFriend(nickname.GetWCharPtr(), friendId);
    }
    if (picker != NULL) {
        AddModal(picker);
    } else {
        CTextDialog* dialog = new CTextDialog();
        dialog->SetTitleText("IDS_FRIENDS_NO_FRIENDS_TO_TARGET_TITLE");
        dialog->SetMessageText(ResString("IDS_FRIENDS_NO_FRIENDS_TO_TARGET_TEXT"));
        dialog->SetButtons(CDialogWindow::BUTTON_OK);
        AddModal(dialog);
    }
}

void CBH_GPSMap::CreateHaunts()
{
    TiXmlDocument document = CXmlHelper::Parse(L"res/common/configs/haunts.xml");
    for (TiXmlNode* node = document.FirstChild("haunt"); node != NULL; node = node->NextSiblingElement("haunt")) {
        CGPSHaunt* haunt = new CGPSHaunt(node);
        haunt->SetHauntListener(this);
        m_items.addElement(haunt);
        m_haunts.addElement(haunt);
        m_clickableItems.addElement(haunt);
        for (int index = 0; index < haunt->m_cameras.size(); ++index) {
            haunt->m_cameras[index]->SetCameraListener(this);
            m_clickableItems.addElement(haunt->m_cameras[index]);
        }
    }
}

void CBH_GPSMap::Load()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    TiXmlElement* map = app->m_saveManager->GetRootElement()->FirstChildElement("gps_map");
    if (map != NULL) {
        m_showFeaturedInstall = CXmlHelper::GetAttributeValueOrUseDefault(map, "show_featured_install", XString("1")).ToBool();
        m_backgroundCount = CXmlHelper::GetAttributeValueOrUseDefault(map, "FI_minimizes_count", XString("0")).ToInt();
        m_currentChallengeDay = CXmlHelper::GetAttributeValueOrUseDefault(map, "currentChallengeDay", XString("-1")).ToInt();
        m_showRocketLauncherIntro = CXmlHelper::GetAttributeValueOrUseDefault(map,
            "needToShowRocketLauncherIntro", XString("1")).ToBool();
        if (m_isFirstLoading) {
            m_backgroundCount++;
        }
        for (int index = 0; index < m_items.size(); ++index) {
            m_items[index]->Load(map);
        }
    }
    m_isFirstLoading = false;
}

void CBH_GPSMap::CreateBar()
{
    m_bar = new CGPSBar();
    m_bar->SetPos(280, 130);
    m_bar->m_listener = this;
    m_items.addElement(m_bar);
}

void CBH_GPSMap::CreateZombiesPromoIcon()
{
    m_zombiesPromo = new CGPSZombiesPromo();
    m_zombiesPromo->SetPos(500 + MapWidthInset(), 350 + MapHeightInset());
    m_zombiesPromo->m_listener = this;
    m_items.addElement(m_zombiesPromo);
}

void CBH_GPSMap::CreateCooperationIcon()
{
    m_cooperation = new CGPSCooperation();
    m_cooperation->SetPos(290 + MapWidthInset(), 270 + MapHeightInset());
    m_cooperation->SetListener(this);
    m_items.addElement(m_cooperation);
}

void CBH_GPSMap::CreateShop()
{
    m_shop = new CGPSShop();
    m_shop->SetPos(400, 350);
    m_shop->SetListener(this);
    m_items.addElement(m_shop);
}

void CBH_GPSMap::CreateNetwork()
{
    m_network = new CGPSNetwork();
    m_network->SetPos(465, 285);
    m_network->SetListener(this);
    m_network->SetVisible(CTutorialManager::IsHomeVisible());
    m_items.addElement(m_network);
}

void CBH_GPSMap::CreateInstantInformants()
{
    TiXmlDocument document = CXmlHelper::Parse(L"res/common/configs/instant_informant.xml");
    for (TiXmlNode* node = document.FirstChild("instant_informant"); node != NULL;
         node = node->NextSiblingElement("instant_informant")) {
        CGPSInstantInformant* informant = new CGPSInstantInformant(node);
        informant->SetListener(this);
        m_items.addElement(informant);
        m_instantInformants.addElement(informant);
        m_clickableItems.addElement(informant);
        if (informant->m_type == 4) {
            m_informantType4 = informant;
        }
        if (informant->IsWeaponGated() == true) {
            m_weaponGatedInformant = informant;
        }
        if (informant->m_type == 5) {
            m_informantType5 = informant;
        }
    }
}

CGPSInstantInformant* CBH_GPSMap::FindInstantInformant(CGPSInstantInformant::Type type, bool weaponGated) const
{
    for (int index = 0; index < m_instantInformants.size(); ++index) {
        CGPSInstantInformant* informant = m_instantInformants[index];
        if (informant->m_visible && ((weaponGated && informant->IsWeaponGated()) || informant->m_type == type)) {
            return informant;
        }
    }
    return NULL;
}

CGPSHauntCamera* CBH_GPSMap::GetCameraForBlueInformant()
{
    CGPSHauntCamera* camera = GetRandomCamera(false, false);
    if (camera == NULL) {
        camera = GetRandomCamera();
    }
    return camera;
}

CGPSHauntCamera* CBH_GPSMap::GetRandomCamera(bool unlocked, bool movingOnly)
{
    Vector<CGPSHauntCamera*> available;
    for (int index = 0; index < m_haunts.size(); ++index) {
        CGPSHaunt* haunt = m_haunts[index];
        for (int number = 0; number < haunt->m_cameras.size(); ++number) {
            CGPSHauntCamera* camera = haunt->m_cameras[number];
            if ((camera->m_durationMs > 0) != movingOnly || camera->HasHuntingItem()) {
                continue;
            }
            if ((unlocked && camera->IsAvailable()) || (!unlocked && camera->IsLockedAvailable())) {
                available.addElement(camera);
            }
        }
    }
    if (available.size() > 0) {
        return available[CRandGen::GetInstance()->GetRandRange(0, available.size() - 1)];
    }
    return NULL;
}

CGPSHauntCamera* CBH_GPSMap::GetRandomCamera()
{
    Vector<CGPSHauntCamera*> available;
    for (int index = 0; index < m_haunts.size(); ++index) {
        CGPSHaunt* haunt = m_haunts[index];
        for (int number = 0; number < haunt->m_cameras.size(); ++number) {
            CGPSHauntCamera* camera = haunt->m_cameras[number];
            // Any free camera that is not a moving one.
            if ((camera == NULL || camera->m_durationMs <= 0) && !camera->HasHuntingItem()) {
                available.addElement(camera);
            }
        }
    }
    if (available.size() != 0) {
        return available[CRandGen::GetInstance()->GetRandRange(0, available.size() - 1)];
    }
    return NULL;
}

CGPSHauntCamera* CBH_GPSMap::GetMovingRandomCamera() const
{
    Vector<CGPSHauntCamera*> available;
    for (int index = 0; index < m_haunts.size(); ++index) {
        CGPSHaunt* haunt = m_haunts[index];
        for (int number = 0; number < haunt->m_cameras.size(); ++number) {
            CGPSHauntCamera* camera = haunt->m_cameras[number];
            if (camera != NULL && camera->IsAvailable() == true && camera->m_durationMs > 0) {
                available.addElement(camera);
            }
        }
    }
    if (available.size() != 0) {
        return available[CRandGen::GetInstance()->GetRandRange(0, available.size() - 1)];
    }
    return NULL;
}

void CBH_GPSMap::ShowBuyInformantDialog(CGPSInstantInformant* informant)
{
    CDialogWindow* dialog = new CDialogWindow(true);
    dialog->SetTitleText(informant->m_name);
    XString text = informant->m_startText;
    text.Append(XString(L"\n"));
    text.Append(ResString("IDS_DIALOG_IT_WILL_COST"));
    if (informant->m_cashCost > 0) {
        text.Append(XString::Format(L" $%d", informant->m_cashCost));
    }
    if (informant->m_creditsCost > 0) {
        text.Append(XString::Format(L" %d %ls", informant->m_creditsCost, ResString("IDS_DIALOG_CREDITS").c_str()));
    }
    dialog->SetMessageText(text);
    dialog->SetButtons(CDialogWindow::BUTTON_HIRE, CDialogWindow::BUTTON_LATER);
    dialog->SetButtonsCommands(ConfirmInformant, CancelInformant, -1);
    m_selectedInformant = informant;
    m_parent->AddModal(dialog);
}

void CBH_GPSMap::ShowInformantIsSearchingDialog(CGPSInstantInformant* informant)
{
    CDialogWindow* dialog = new CDialogWindow(true);
    dialog->SetTitleText(informant->m_name);
    XString text = informant->m_comeLaterText;
    text.Append(XString::Format(L"\n%ls\n%d %ls", DGHelper::ClockStringFromSeconds(informant->GetCooldownTimeLeft()).c_str(),
                               informant->m_resetCost, ResString("IDS_DIALOG_CREDITS").c_str()));
    dialog->SetMessageText(text);
    dialog->SetButtons(CDialogWindow::BUTTON_BUY, CDialogWindow::BUTTON_LATER);
    dialog->SetButtonsCommands(ConfirmInformant, CancelInformant, -1);
    m_selectedInformant = informant;
    m_parent->AddModal(dialog);
}

void CBH_GPSMap::ResolveInformantPurchase(CGPSInstantInformant* informant)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    bool bought = informant->m_bought;
    bool success;
    if (bought) {
        success = informant->BuyInfoFaster();
    } else {
        success = informant->Buy();
    }
    if (!success) {
        CDialogWindow* dialog = new CDialogWindow(true);
        dialog->SetTitleText("IDS_DIALOG_TITLE_WARNING");
        dialog->SetMessageText(ResString("IDS_MESSAGE_NO_MONEY"));
        if (!bought && CTutorialManager::IsPlayerInTutorial() && app->m_tutorialManager->IsTutorialInstantInformant(informant->m_id)) {
            success = informant->BuyFree();
            dialog->SetTitleText("IDS_TUTORIAL_TITLE");
            dialog->SetMessageText(ResString("IDS_TUTORIAL_INFORMANT_NO_MONEY"));
        }
        dialog->SetButtons(CDialogWindow::BUTTON_OK);
        m_startDialogs->Add(dialog);
        m_startDialogs->Start();
    }
    if (success) {
        if (!bought) {
            WindowApp::HandleTunnelCommand(-0x72ce08c6, 0, 0, 0);
            m_showTutorialFinish = CBH_Player::GetInstance()->GetTutorialState() == 12;
        }
        OnGPSInstantInformantSelected(informant);
        Save();
        CBH_Player::GetInstance()->Save();
    }
}

void CBH_GPSMap::OnGPSInstantInformantSelected(CGPSInstantInformant* informant)
{
    if (!informant->m_bought && informant->m_type != CGPSInstantInformant::Moving) {
        ShowBuyInformantDialog(informant);
        return;
    }
    if (informant->IsSearching()) {
        ShowInformantIsSearchingDialog(informant);
        return;
    }

    CGPSHauntCamera* camera = GetCameraById(informant->m_cameraId);
    if (camera != NULL && camera->GetGPSHuntingItem() != NULL && camera->GetGPSHuntingItem()->GetType() == 1) {
        OnCameraSelected(camera);
        return;
    }
    if (informant->m_cameraId >= 0) {
        informant->OnBountyCanceled();
    }
    if (!informant->IsBountyAvailable()) {
        return;
    }
    if (informant->m_type == CGPSInstantInformant::Targeting) {
        RequestTargetPicker();
        return;
    }
    if (informant->m_type == CGPSInstantInformant::Blue) {
        camera = GetRandomCamera(false, false);
        if (camera == NULL) {
            camera = GetRandomCamera(true, false);
        }
    } else {
        camera = GetRandomCamera(true, informant->m_type == CGPSInstantInformant::Moving);
    }
    if (camera != NULL) {
        if (informant->m_type == CGPSInstantInformant::Gated) {
            camera->SetGatedMissionOnCamera(informant, 0, false);
        } else {
            camera->SetBountyOnCamera(informant, false, false);
        }
        if (camera->GetGPSHuntingItem() == NULL) {
            camera->m_gatedMission = false;
            camera->m_gatedMissionId = -1;
            camera = NULL;
        }
    }
    if (camera == NULL) {
        CDialogWindow* dialog = new CDialogWindow(true);
        dialog->SetTitleText("IDS_DIALOG_TITLE_WARNING");
        dialog->SetMessageText(ResString("IDS_MESSAGE_NO_FREE_CAMERAS"));
        dialog->SetButtons(CDialogWindow::BUTTON_OK);
        m_startDialogs->Add(dialog);
        m_startDialogs->Start();
        return;
    }
    informant->OnBountyGiven(camera->m_id);
    ScrollToItem(*camera, 1.1f);
    Save();
    OnCameraSelected(camera);
}

void CBH_GPSMap::Shutdown()
{
    SetupLocalPushNotifications();
}

void CBH_GPSMap::SetupLocalPushNotifications()
{
    LocalPushManager::CancelLocalPushNotifications(false);
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (app->m_settings->IsLocalPushEnabled() != true) {
        return;
    }

    // Find the informant that becomes ready first, and the last type-1 informant on cooldown.
    CGPSInstantInformant* typeOne = NULL;
    CGPSInstantInformant* soonest = NULL;
    unsigned int minCooldown = static_cast<unsigned int>(-1);
    for (int index = 0; index < m_instantInformants.size(); ++index) {
        CGPSInstantInformant* informant = m_instantInformants[index];
        if (informant == NULL) {
            continue;
        }
        if (informant->GetCooldownTimeLeft() != 0 &&
            static_cast<unsigned int>(informant->GetCooldownTimeLeft()) < minCooldown) {
            soonest = informant;
            minCooldown = informant->GetCooldownTimeLeft();
        }
        if (informant->m_type == 1 && informant->GetCooldownTimeLeft() != 0) {
            typeOne = informant;
        }
    }

    unsigned int energyTime = CBH_Player::GetInstance()->GetEnergyCooldownTimeLeft();
    Vector<SLocalPush> pushes;
    if (soonest != NULL) {
        XString text = XString::Format(ResString("IDS_PUSH_INFORMANT_READY").c_str(), soonest->m_name.c_str());
        pushes.addElement(SLocalPush(text, minCooldown));
    }
    if (typeOne != NULL && soonest != typeOne) {
        XString text = XString::Format(ResString("IDS_PUSH_INFORMANT_READY").c_str(), typeOne->m_name.c_str());
        pushes.addElement(SLocalPush(text, typeOne->GetCooldownTimeLeft()));
    }
    if (energyTime > 10 && CBH_Player::GetInstance()->IsEnegyEnabled() == true) {
        pushes.addElement(SLocalPush(ResString("IDS_PUSH_ENERGY_REFILLED"), energyTime));
    }
    CollectSubContractsNotifications(pushes);

    int count = pushes.size();
    if (count != 0) {
        SLocalPush* items = pushes.m_items;
        QuickSort::Run(items, count, sizeof(SLocalPush), SLocalPush::cmp);
        for (int index = 0; index < count;) {
            XString::AnsiString text(items[index].m_text.c_str());
            int time = items[index].m_time;
            ++index;
            LocalPushManager::SetLocalPushNotification(text, "To game", time, index);
        }
    }
    pushes.free_all_items();
}

void CBH_GPSMap::CollectSubContractsNotifications(Vector<SLocalPush>& pushes)
{
    unsigned int now = App::GetTime();
    if (m_haunts.size() == 0) {
        return;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    unsigned int disappearTime = static_cast<unsigned int>(-1);
    unsigned int appearTime = static_cast<unsigned int>(-1);
    unsigned int limit = now + 10;
    int notifyTimeLeft = app->m_subContractManager->m_disappearNotifyTimeLeft;
    for (unsigned int hauntIndex = 0; hauntIndex < static_cast<unsigned int>(m_haunts.size()); ++hauntIndex) {
        CGPSHaunt* haunt = m_haunts[hauntIndex];
        for (unsigned int cameraIndex = 0; cameraIndex < static_cast<unsigned int>(haunt->m_cameras.size());
             ++cameraIndex) {
            CGPSSubContract* contract = haunt->m_cameras[cameraIndex]->m_subContract;
            if (contract == NULL) {
                continue;
            }
            unsigned int activate = contract->GetActivateTime();
            // Original combines both tests without short-circuit (tst).
            if ((activate < appearTime) & (limit < activate)) {
                appearTime = activate;
            }
            unsigned int disappear = contract->GetDisappearTime() - notifyTimeLeft;
            if (limit < disappear && disappear < disappearTime) {
                disappearTime = disappear;
            }
        }
    }
    if (appearTime != static_cast<unsigned int>(-1)) {
        pushes.addElement(SLocalPush(XString::Format(ResString("IDS_PUSH_SUB_CONTRACT_APPEARED").c_str()),
                                     appearTime - now));
    }
    if (disappearTime != static_cast<unsigned int>(-1)) {
        pushes.addElement(SLocalPush(XString::Format(ResString("IDS_PUSH_SUB_CONTRACT_WILL_DISAPPEAR").c_str()),
                                     disappearTime - now));
    }
}

void CBH_GPSMap::OnPause()
{
    SetupLocalPushNotifications();
}

void CBH_GPSMap::OnEnterBackground()
{
    m_backgroundCount++;
}

void CBH_GPSMap::OnGPSSocialNetworkSelected()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (!app->GetNetworkAvailability()->IsAvailable()) {
        app->m_gserveHandler->ShowOfflineModeDialog();
        return;
    }
    CNGSLocalUser* local = CNGS::GetInstance()->GetLocalUser();
    if (!local->isAuthenticated(2) ||
        (!local->isAuthenticated(1) && GameCenter_Facade::GetInstance()->isGameCenterSupported() == true)) {
        app->StartLogin(App::LoginBothNetworks, true);
        return;
    }
    if (m_openFriendsWindow) {
        CFriendsWindow* window = new CFriendsWindow();
        window->SetTab(m_socialNetworkMode);
        AddModal(window);
    }
    m_openFriendsWindow = false;
}

bool CBH_GPSMap::LoadPrefetchedManagerResource()
{
    if (m_prefetchIndex >= m_prefetchedManagers.size()) {
        return false;
    }
    m_prefetchedManagers[m_prefetchIndex]->loadResources();
    m_prefetchIndex++;
    return true;
}

void CBH_GPSMap::CancelInstantInformantWithCamera(int cameraId)
{
    for (int index = 0; index < m_instantInformants.size(); ++index) {
        CGPSInstantInformant* informant = m_instantInformants[index];
        if (informant != NULL && informant->m_cameraId == cameraId) {
            informant->OnBountyCanceled();
            return;
        }
    }
}

void CBH_GPSMap::OnGPSNetworkSelected(CGPSNetwork*)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CBH_NetworkWindow::GetInstance()->BringToFrontModal(app->m_mainLayer);
    WindowApp::HandleTunnelCommand(-0x4e79c923, 0, 0, 0);
}

void CBH_GPSMap::NeedSave()
{
    m_needSave = true;
}

void CBH_GPSMap::OnShopItemSelected(CGPSShop*)
{
    CShopWindow::ETab tab = CShopWindow::TAB_WEAPONS;
    if (CTutorialManager::IsPlayerInTutorial()) {
        tab = static_cast<CShopWindow::ETab>(CTutorialManager::GetCurrentShopTab());
    }
    CRegularShop::Show(tab);
}

void CBH_GPSMap::OnBarItemSelected(CGPSBar*)
{
    if (!HasModalChild()) {
        App* app = static_cast<App*>(WindowApp::m_instance);
        app->m_mainLayer->AddModal(new CFriendsWindow());
    }
}

void CBH_GPSMap::OnBarVisible()
{
    if (m_shortcuts != NULL) {
        m_shortcuts->ShowFriendsButton();
    }
}

void CBH_GPSMap::OnZombiesItemSelected(CGPSZombiesPromo*)
{
    CGameAnalytics::logZombiesPromoPressed();
    CPHInterface::startPublisherContentRequest("ck_zombies", false, true);
}

void CBH_GPSMap::OnCooperationItemSelected(CGPSCooperation*)
{
    if (HasModalChild()) {
        return;
    }
    if (CChallengeManager::GetTimeSec() == 0) {
        static_cast<App*>(WindowApp::m_instance)->m_gserveHandler->ShowOfflineModeDialog();
    } else {
        App* app = static_cast<App*>(WindowApp::m_instance);
        app->m_mainLayer->AddModal(new CChallengesWindow());
    }
}

void CBH_GPSMap::OnCoopVisible()
{
    if (m_shortcuts != NULL) {
        m_shortcuts->ShowCoopButton();
        m_shortcuts->ShowWeaponGatedButton();
    }
}

void CBH_GPSMap::OnTargetMissionsVisible()
{
    if (m_shortcuts != NULL) {
        m_shortcuts->ShowTargetButton();
    }
}

void CBH_GPSMap::OnVehicleMissionsVisible()
{
    if (m_shortcuts != NULL) {
        m_shortcuts->ShowVehicleButton();
    }
}

void CBH_GPSMap::CreateInformantsImpl(Vector<CGPSMissionInformant*>* informants, const wchar_t* path)
{
    TiXmlDocument document = CXmlHelper::Parse(path);
    for (TiXmlNode* node = document.FirstChild("informant"); node != NULL; node = node->NextSiblingElement("informant")) {
        CGPSMissionInformant* informant = new CGPSMissionInformant(node);
        informant->SetListener(this);
        m_items.addElement(informant);
        informants->addElement(informant);
        m_clickableItems.addElement(informant);
    }
}

void CBH_GPSMap::CreateAlternativeStoryLineInformants()
{
    CreateInformantsImpl(&m_missionInformants[1], L"res/common/configs/informants_bridge.xml");
    CreateInformantsImpl(&m_missionInformants[2], L"res/common/configs/informants_club.xml");
    CreateInformantsImpl(&m_missionInformants[3], L"res/common/configs/informants_moving.xml");
    CreateInformantsImpl(&m_missionInformants[4], L"res/common/configs/informants_gas_station.xml");
}

void CBH_GPSMap::UpdateMissionInformants()
{
    for (int group = 0; group < 5; ++group) {
        HideAllInformantsImpl(&m_missionInformants[group]);
        if (group == 0 || !CTutorialManager::IsPlayerInTutorial()) {
            UnhideInformantImpl(&m_missionInformants[group]);
        }
    }
}

void CBH_GPSMap::HideAllInformantsImpl(Vector<CGPSMissionInformant*>* informants)
{
    for (int index = 0; index < informants->size(); ++index) {
        (*informants)[index]->SetVisible(false);
    }
}

void CBH_GPSMap::UnhideInformantImpl(Vector<CGPSMissionInformant*>* informants)
{
    CGPSMissionInformant* first = NULL;
    for (int index = 0; index < informants->size(); ++index) {
        CGPSMissionInformant* informant = (*informants)[index];
        if (informant->IsAvailable() && (first == NULL || informant->m_missionId < first->m_missionId)) {
            first = informant;
        }
    }
    if (first != NULL) {
        first->SetVisible(true);
    }
}

void CBH_GPSMap::OnGpsMissionInformantSelected(CGPSMissionInformant* informant)
{
    if (informant == NULL || !informant->IsAvailable()) {
        return;
    }
    if (!MakePlayerWeaponCheck(informant->m_missionId)) {
        return;
    }
    CGPSHauntCamera* camera = GetCameraById(informant->m_cameraId);
    if (camera == NULL) {
        return;
    }
    CGPSHuntingItem* item = camera->GetGPSHuntingItem();
    if (item != NULL && item->GetMissionId() != informant->m_missionId) {
        CDialogWindow* warning = new CDialogWindow(true);
        warning->SetTitleText("IDS_DIALOG_TITLE_WARNING");
        warning->SetMessageText(ResString("IDS_MESSAGE_CAN_NOT_TAKE_RED_MISSION_BECAUSE_OF_BLUE"));
        warning->SetButtons(CDialogWindow::BUTTON_OK);
        m_parent->AddModal(warning);
        ScrollToItem(*camera, 1.1f);
        return;
    }
    if (item == NULL) {
        camera->SetMissionOnCamera(informant->m_missionId, informant);
        WindowApp::HandleTunnelCommand(-0x2051ee21, 0, 0, 0);
        Save();
    }
    ShowMissionStartDialogs(informant->m_missionId);
    ScrollToItem(*informant, 0.2f);
}

void CBH_GPSMap::PlaceSubContract(CGPSSubContract* contract, unsigned int radius)
{
    const CGPSHauntCamera* camera = contract->GetHauntCamera();
    const int width = contract->GetWidth();
    const int height = contract->GetHeight();
    const int centerX = camera->GetX() + camera->GetWidth() / 2 - width / 2;
    const int centerY = camera->GetY() + camera->GetHeight() / 2 - height / 2;
    const int mapBottom = m_mapHeight - SubContractBottomInset();
    const int mapTop = SScreenAdaptingConstant(40, 80)() + MapHeightInset();
    const int stepX = static_cast<int>(contract->m_scaleX);
    const int stepY = static_cast<int>(contract->m_scaleY);
    const int minimumRadius = (width + camera->GetWidth()) / 2;
    int searchRadius = static_cast<int>(radius);
    if (searchRadius < width * 3) {
        searchRadius = width * 3;
    }
    if (searchRadius < minimumRadius) {
        searchRadius = minimumRadius;
    }
    CRandGen* random = CRandGen::GetInstance();
    for (int expansion = 0; expansion < 5; ++expansion, searchRadius *= 2) {
        int minX = centerX - searchRadius - width / 2;
        int minY = centerY - searchRadius - height / 2;
        int maxX = centerX + searchRadius + width / 2;
        int maxY = centerY + searchRadius + height / 2;
        if (minX < 0) {
            minX = 0;
        }
        if (minY < mapTop) {
            minY = mapTop;
        }
        if (maxX > m_mapWidth - width) {
            maxX = m_mapWidth - width;
        }
        if (maxY > mapBottom - height) {
            maxY = mapBottom - height;
        }
        if (minX > maxX || minY > maxY) {
            continue;
        }
        // SetScaledPos stores unscaled coordinates. Check the position it will display.
        const int firstX = (minX + stepX - 1) / stepX;
        const int firstY = (minY + stepY - 1) / stepY;
        const int lastX = maxX / stepX;
        const int lastY = maxY / stepY;
        if (firstX > lastX || firstY > lastY) {
            continue;
        }

        Vector<const CGPSItem*> items;
        FilterRegionItems(minX, minY, maxX + width, maxY + height, items);
        for (int index = items.size() - 1; index >= 0; --index) {
            if (items[index] == contract) {
                items.removeElementAt(index);
            }
        }

        for (int attempt = 0; attempt < 1000; ++attempt) {
            int distance = random->GetRandRange(minimumRadius, searchRadius);
            float angle = static_cast<float>(random->GetRand(360));
            int x = (centerX + static_cast<int>(MathLib::Sin(angle) * distance)) / stepX;
            int y = (centerY + static_cast<int>(MathLib::Cos(angle) * distance)) / stepY;
            x = ClampMapCoordinate(x, firstX, lastX) * stepX;
            y = ClampMapCoordinate(y, firstY, lastY) * stepY;
            if (IsRegionFree(x, y, x + width, y + height, items)) {
                contract->SetScaledPos(x, y);
                return;
            }
        }
        for (int x = firstX * stepX; x <= maxX; x += stepX) {
            for (int y = firstY * stepY; y <= maxY; y += stepY) {
                if (IsRegionFree(x, y, x + width, y + height, items)) {
                    contract->SetScaledPos(x, y);
                    return;
                }
            }
        }
        if (minX == 0 && minY == mapTop && maxX == m_mapWidth - width && maxY == mapBottom - height) {
            break;
        }
    }

    int x = ClampMapCoordinate(camera->GetX() + camera->GetWidth() + stepX, 0, m_mapWidth - width);
    int y = ClampMapCoordinate(centerY, 0, mapBottom - height);
    contract->SetScaledPos(x, y);
}

bool CBH_GPSMap::IsItemIntersects(const CGPSItem& item, int left, int top, int right, int bottom)
{
    short x = item.GetX();
    short y = item.GetY();
    short width = item.GetWidth();
    short height = item.GetHeight();
    if (x <= right && y <= bottom && x + width >= left && y + height >= top) {
        return true;
    }
    for (int index = 0; index < item.m_subItems.size(); ++index) {
        const CRectangle& rect = item.m_subItems[index];
        if (rect.m_x <= right && rect.m_y <= bottom
            && rect.m_x + rect.m_width >= left && rect.m_y + rect.m_height >= top) {
            return true;
        }
    }
    return false;
}

bool CBH_GPSMap::IsRegionFree(int left, int top, int right, int bottom,
                             const Vector<const CGPSItem*>& items)
{
    for (int index = 0; index < items.size(); ++index) {
        if (IsItemIntersects(*items[index], left, top, right, bottom)) {
            return false;
        }
    }
    return true;
}

void CBH_GPSMap::FilterRegionItems(int left, int top, int right, int bottom,
                                  Vector<const CGPSItem*>& items) const
{
    const CGPSItem* fixedItems[] = {m_shop, m_network, m_bar, m_cooperation, m_zombiesPromo};
    for (unsigned int index = 0; index < sizeof(fixedItems) / sizeof(fixedItems[0]); ++index) {
        const CGPSItem* item = fixedItems[index];
        if (item != NULL && IsItemIntersects(*item, left, top, right, bottom)) {
            items.addElement(item);
        }
    }
    for (int group = 0; group < 5; ++group) {
        for (int index = 0; index < m_missionInformants[group].size(); ++index) {
            const CGPSItem* item = m_missionInformants[group][index];
            if (IsItemIntersects(*item, left, top, right, bottom)) {
                items.addElement(item);
            }
        }
    }
    for (int index = 0; index < m_instantInformants.size(); ++index) {
        const CGPSItem* item = m_instantInformants[index];
        if (IsItemIntersects(*item, left, top, right, bottom)) {
            items.addElement(item);
        }
    }
    for (int haunt = 0; haunt < m_haunts.size(); ++haunt) {
        const Vector<CGPSHauntCamera*>& cameras = m_haunts[haunt]->m_cameras;
        for (int index = 0; index < cameras.size(); ++index) {
            const CGPSHauntCamera* camera = cameras[index];
            if (IsItemIntersects(*camera, left, top, right, bottom)) {
                items.addElement(camera);
            }
            const CGPSSubContract* contract = camera->m_subContract;
            if (contract != NULL && IsItemIntersects(*contract, left, top, right, bottom)) {
                items.addElement(contract);
            }
        }
    }
}

void CBH_GPSMap::FlushDialogsQueue()
{
    for (int index = 0; index < m_dialogsQueue.size(); ++index) {
        if (m_parent->IsChildExisting(m_dialogsQueue[index])) {
            continue;
        }
        m_dialogsQueue[index]->CloseChilds();
        m_dialogsQueue[index]->HandleUpdate(true);
        m_dialogsQueue[index]->HandleUpdate(true);
        m_dialogsQueue[index]->Close();
    }
    m_dialogsQueue.removeAllElements();
}

void CBH_GPSMap::ShowLowMoneyDialog()
{
    m_pendingCommand = 0x1d;
    CDialogWindow* dialog = new CDialogWindow(true);
    dialog->SetButtons(CDialogWindow::BUTTON_BUY, CDialogWindow::BUTTON_GET, CDialogWindow::BUTTON_IGNORE);
    dialog->SetMessageText(ResString("IDS_MENU_LOW_MONEY_DIALOG"), NULL);
    dialog->SetTitleText("IDS_DIALOG_TITLE_WARNING");
    dialog->SetButtonsCommands(AcceptMissionDialog, LowMoneyGetCommand, CancelMissionDialog);
    m_parent->AddModal(dialog);
}

void CBH_GPSMap::ShowCoopWindow()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (!app->GetNetworkAvailability()->IsAvailable()) {
        app->m_gserveHandler->ShowOfflineModeDialog();
        return;
    }
    m_openFriendsWindow = true;
    m_socialNetworkMode = 2;
    OnGPSSocialNetworkSelected();
}

void CBH_GPSMap::FillBeforeTargetHuntInfo(const CGPSBounty* bounty)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    int friendId = bounty->m_targetFriendId;
    CBH_Player::GetInstance()->m_missionBriefImage = "IDB_DIALOG_NOTEBOOK";
    ICRenderSurface* avatar = app->m_surfaceManager->CreateSurface("IDB_AVATAR_01", true);
    CBH_Player::GetInstance()->m_field208 = reinterpret_cast<int>(avatar);
    CBH_Player::GetInstance()->m_missionBriefEnergy = bounty->m_energyCost;
    CBH_Player::GetInstance()->m_field21c = bounty->m_experienceReward;
    CBH_Player::GetInstance()->m_field220 = bounty->m_moneyReward;
    CBH_Player::GetInstance()->m_targetFriendId = friendId;
    CBH_Player::GetInstance()->m_missionBriefTargetFB = "";

    XString bossName;
    bossName = bounty->m_bossName;
    XString text = bounty->GetObjectiveGoal();
    CGPSHaunt* haunt = GetHauntByName(m_selectedCamera->m_haunt);
    XString goal = text.Replace(L"[LocationName]", haunt->m_screenName.c_str(), true);
    CBountyManager::SMessage* message = app->m_bountyManager->GetRandomTargettingMessage();
    XString brief = ResString("IDS_DIALOG_NAME") + XString(": ") + bossName;
    brief.Append(XString("\n"));
    brief.Append(message->m_text + XString(""));
    CBH_Player::GetInstance()->m_missionBriefText = brief;
    CBH_Player::GetInstance()->m_missionBriefTitle = message->m_title;
}

void CBH_GPSMap::ShowShopWithItem(CItemManager* manager, CShopWindow::ETab tab, CItem::EItemTypes, int id)
{
    if (manager == NULL) {
        return;
    }
    CItem* item = manager->getItemById(id);
    if (item == NULL) {
        return;
    }
    CRegularShop* shop = CRegularShop::GetInstance(true);
    shop->SetTab(tab);
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_mainLayer->AddModal(shop);
    CShopItemInfoWindow* details = new CShopItemInfoWindow(manager, tab, id, item->GetStatsString(), false,
                                                           CShopItemInfoWindow::SOURCE_SHOP);
    shop->AddModal(details);
}

void CBH_GPSMap::FillBeforeHuntInfo(const CGPSHuntingItem* item)
{
    const char* image = "IDB_DIALOG_NOTEBOOK";
    if (CStrWChar(item->m_targetFBUser.c_str()).GetLength() < 1 && item->m_targetFriendId < 0) {
        // Boss portrait when the target is not a friend.
        image = item->GetObjectiveBossType()->m_visualCharacter->m_screenImage;
    }
    FillBeforeHuntInfo(item, image, NULL);
}

void CBH_GPSMap::FillBeforeHuntInfo(const CGPSHuntingItem* item, const char* image, ICRenderSurface* surface)
{
    CBH_Player::GetInstance()->m_missionBriefImage = image;
    CBH_Player::GetInstance()->m_field208 = reinterpret_cast<int>(surface);
    CBH_Player::GetInstance()->m_missionBriefTitle = item->GetObjectiveTitle();
    CBH_Player::GetInstance()->m_missionBriefEnergy = item->m_energyCost;
    CBH_Player::GetInstance()->m_field21c = item->m_experienceReward;
    CBH_Player::GetInstance()->m_field220 = item->m_moneyReward;
    CBH_Player::GetInstance()->m_targetFriendId = -1;
    CBH_Player::GetInstance()->m_missionBriefTargetFB = CStrWChar(item->m_targetFBUser.c_str());
    CBH_Player::GetInstance()->m_field238 = item->m_hasBonus;

    XString goal;
    if (item->GetType() == 0) {
        goal = item->GetObjectiveGoal();
    } else {
        XString text = item->GetObjectiveGoal();
        CGPSHaunt* haunt = GetHauntByName(m_selectedCamera->m_haunt);
        goal = text.Replace(L"[LocationName]", haunt->m_screenName.c_str(), true);
    }
    XString brief = ResString("IDS_DIALOG_NAME") + XString(": ") + item->GetObjectiveBossScreenName();
    brief.Append(XString("\n"));
    brief.Append(ResString("IDS_DIALOG_GOAL") + XString(": ") + goal + XString(""));
    CBH_Player::GetInstance()->m_missionBriefText = brief;
}

void CBH_GPSMap::ShowMissionPriorDialogs(int missionId)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    m_pendingCommand = 0xb;
    const CMission* mission = app->m_missionManager->GetMissionById(missionId);
    app->m_soundManager->PlaySound("IDM_MENU_WINDOW_OPEN_CLOSE", false, false);
    for (int index = 0; index < mission->m_priorDialogs.size(); ++index) {
        CMission::Dialog entry = mission->m_priorDialogs[index];
        CDialogWindow* dialog = new CDialogWindow(true);
        dialog->SetExtraImageFromImageID(entry.m_image);
        dialog->SetTitleText(entry.m_title.Upper());
        CBH_PersonType* boss = app->InitPersonTypeManager()->GetType(mission->m_boss);
        XString text = CGPSHuntingItem::ParametrizeTextWithParams(entry.m_text, mission->m_bossName,
                                                                  boss->m_visualCharacter->m_isMan);
        dialog->SetMessageText(text, NULL);
        dialog->SetButtons(CDialogWindow::BUTTON_OK, CDialogWindow::BUTTON_NONE, CDialogWindow::BUTTON_NONE);
        dialog->SetButtonsCommands(AcceptMissionDialog, -1, -1);
        CDialogWindow* queued = dialog;
        m_dialogsQueue.insertElementAt(queued, 0);
    }
    if (m_dialogsQueue.size() == 0) {
        ShowBeforeHuntDialog();
    } else {
        ShowNextQueueDialog(true);
    }
}

void CBH_GPSMap::ShowBeforeHuntDialog()
{
    FillBeforeHuntInfo(m_selectedCamera->GetGPSHuntingItem());
    Window* dialog = NULL;
    if (CBH_Player::GetInstance()->m_missionBriefTargetFB.GetLength() < 1) {
        dialog = CreateBeforeHuntDialog(true);
    } else {
        dialog = CreateBeforeTargettingDialog(true);
    }
    CGPSSubContract* contract = NULL;
    if (m_selectedCamera != NULL) {
        contract = m_selectedCamera->m_subContract;
    }
    if (m_selectedCamera != NULL && contract != NULL) {
        unsigned int activateTime = contract->GetActivateTime();
        if (App::GetTime() + 1 < activateTime) {
            // Countdown to the next sub contract under the dialog.
            TextWindow* label = new TextWindow(ResString("IDS_DIALOG_NEXT_SUB_CONTRACT"),
                                               CFontMgr::GetInstance()->GetFont(15));
            label->SetLayoutType(LAYOUT_SIMPLE);
            label->SetAlign(ALIGN_LEFT | ALIGN_BOTTOM);
            int labelBottom = SScreenAdaptingConstant(0x65, 0xca)();
            int labelLeft = SScreenAdaptingConstant(0x39, 0x72)();
            label->SetOutsetSpacing(0, 0, labelBottom, labelLeft);
            dialog->AddToFront(label);

            CFont* timerFont = CFontMgr::GetInstance()->GetFont(8);
            HorizImageWindow* bar = new HorizImageWindow(ImageRes("SUR_HUD_PROGRESSBAR_LEFT"),
                                                         ImageRes("SUR_HUD_PROGRESSBAR"),
                                                         ImageRes("SUR_HUD_PROGRESSBAR_RIGHT"), false);
            bar->SetLayoutType(LAYOUT_SIMPLE);
            bar->SetAlign(ALIGN_LEFT | ALIGN_BOTTOM);
            int barBottom = SScreenAdaptingConstant(0x53, 0xa6)();
            int barLeft = SScreenAdaptingConstant(0x52, 0xa4)();
            bar->SetOutsetSpacing(0, 0, barBottom, barLeft);
            App* app = static_cast<App*>(WindowApp::m_instance);
            int textWidth = app->m_helper->GetTextWidth("00:00:00", timerFont);
            bar->SetDesiredWidth(SScreenAdaptingConstant(0x10, 0x20)() + textWidth);
            dialog->AddToFront(bar);

            CTimerWindow* timer = new CTimerWindow(activateTime, timerFont);
            timer->SetLayoutType(LAYOUT_SIMPLE);
            timer->SetAlign(ALIGN_LEFT | ALIGN_BOTTOM);
            int timerBottom = SScreenAdaptingConstant(0x53, 0xa6)();
            int timerLeft = SScreenAdaptingConstant(0x5a, 0xb4)();
            timer->SetOutsetSpacing(0, 0, timerBottom, timerLeft);
            dialog->AddToFront(timer);
        }
    }
    m_parent->AddModal(dialog);
    m_pendingCommand = 0xf;
}

void CBH_GPSMap::UpdateMapItems()
{
    UpdateMissionInformants();
    for (int index = 0; index < m_items.size(); ++index) {
        m_items[index]->UpdateState();
    }
    Save();
}

void CBH_GPSMap::ShowMissionStartDialogs(int missionId)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    const CMission* mission = app->m_missionManager->GetMissionById(missionId);
    if (mission == NULL) {
        return;
    }
    for (int group = 0; group < 5; ++group) {
        for (int index = 0; index < m_missionInformants[group].size(); ++index) {
            CGPSMissionInformant* informant = m_missionInformants[group][index];
            if (informant->m_missionId != missionId) {
                continue;
            }
            CGPSHauntCamera* camera = GetCameraById(informant->m_cameraId);
            if (camera == NULL || camera->GetGPSHuntingItem() == NULL) {
                return;
            }
            CGPSHuntingItem* item = camera->GetGPSHuntingItem();
            for (int dialogIndex = 0; dialogIndex < mission->m_startDialogs.size(); ++dialogIndex) {
                const CMission::Dialog& entry = mission->m_startDialogs[dialogIndex];
                CDialogWindow* dialog = new CDialogWindow(true);
                dialog->SetExtraImageFromImageID(entry.m_image);
                dialog->SetTitleText(entry.m_title);
                dialog->SetMessageText(item->ParametrizeText(entry.m_text));
                dialog->SetButtons(CDialogWindow::BUTTON_OK);
                m_startDialogs->Add(dialog);
            }
            m_startDialogs->Start();
            return;
        }
    }
}

void CBH_GPSMap::Save()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    TiXmlElement* map = CXmlHelper::ObtainElement(app->m_saveManager->GetRootElement(), XString("gps_map"));
    // Keep map fields belonging to systems that have not been restored yet.
    map->SetAttribute("currentChallengeDay", m_currentChallengeDay);
    map->SetAttribute("needToShowRocketLauncherIntro", m_showRocketLauncherIntro);
    for (int index = 0; index < m_haunts.size(); ++index) {
        m_haunts[index]->Save(map);
    }
    for (int index = 0; index < m_instantInformants.size(); ++index) {
        m_instantInformants[index]->Save(map);
    }
    app->m_saveManager->SaveOnNextUpdate(false);
}

CGPSHauntCamera* CBH_GPSMap::GetCameraById(int id)
{
    for (int index = 0; index < m_haunts.size(); ++index) {
        CGPSHauntCamera* camera = m_haunts[index]->GetCameraById(id);
        if (camera != NULL) {
            return camera;
        }
    }
    return NULL;
}

bool CBH_GPSMap::IsMovingCameraMissionsFinished() const
{
    const Vector<CGPSMissionInformant*>& informants = m_missionInformants[3];
    for (int index = 0; index < informants.size(); ++index) {
        CGPSMissionInformant* informant = informants[index];
        if (informant == NULL || !CBH_Player::GetInstance()->IsMissionCompleted(informant->m_missionId)) {
            return false;
        }
    }
    return true;
}

bool CBH_GPSMap::ShowNextQueueDialog(bool animate)
{
    int count = m_dialogsQueue.size();
    if (count == 0) {
        return false;
    }
    CDialogWindow* dialog = m_dialogsQueue[count - 1];
    dialog->SetEasyStyle(animate, count == 1);
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_mainLayer->AddModal(dialog);
    return true;
}

void CBH_GPSMap::StartTutorialMission()
{
    CGPSHauntCamera* camera = GetCameraById(4);
    if (camera != NULL) {
        CBH_Player::GetInstance()->SetPlayerStartParams();
        if (!camera->HasHuntingItem()) {
            camera->SetMissionOnCamera(1);
        }
        StartHunt(camera, NULL, true);
    }
}

void CBH_GPSMap::CheckGameCenterAccounts()
{
    if (GameCenter_Facade::GetInstance()->isAuthenticated() != true) {
        return;
    }
    CStrWChar gameCenterId;
    gameCenterId = CNGS_Platform::GetInstance()->getSocialInterface(1)->getPlayerAlias();
    CStrWChar socialId;
    socialId = CNGS::GetInstance()->GetLocalUser()->GetSocialNetworkIdentity(1);
    if (gameCenterId == socialId) {
        return;
    }
    // The Game Center player changed: switch the primary gServe user.
    if (socialId.GetLength() > 0) {
        CNGS::GetInstance()->GetLocalUser()->CNGSHandlePrimaryUserChange(1, "CheckGameCenterAccounts");
    }
}

void CBH_GPSMap::StartHunt(CGPSHauntCamera* camera, const CLocation* location, bool useScript)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CGPSHuntingItem* item = camera->GetGPSHuntingItem();
    const bool temporaryMission = camera->HasTempMission();
    if (temporaryMission) {
        item = camera->m_temporaryItem;
    }
    if (item == NULL) {
        return;
    }
    if (location == NULL) {
        CLocation* const* found = app->m_locationManager->m_locations.Get(camera->m_location);
        if (found != NULL) {
            location = *found;
        }
    }
    if (location == NULL) {
        return;
    }

    CBH_Player* player = CBH_Player::GetInstance();
    CHuntingInfo::SAssignedData data;
    CGPSBounty* ambush = NULL;
    if (!temporaryMission && !item->m_weaponGated && item->GetType() == 1) {
        int firstLevel = app->m_helper->m_config.GetIntegerValueByToken("LEVEL_FROM_AMBUSHES_CAN_APPEAR", true);
        if (player->GetLevel() >= firstLevel) {
            float probability = app->m_helper->m_config.GetFloatValueByToken("AMBUSH_APPEAR_PROB", true);
            if (CBH_Math::GetProbabiliyOutcome(static_cast<int>(probability * 100.0f))) {
                int ambushType = app->m_bountyManager->GetRandomAmbushTypeId();
                ambush = new CGPSBounty(camera, ambushType, NULL);
                if (item->m_targetFriendId >= 0) {
                    ambush->SetTargetFriendId(item->m_targetFriendId);
                }
                if (item->m_targetFBUser.Length() != 0) {
                    ambush->SetTargetFBUser(CStrWChar(item->m_targetFBUser.c_str()));
                }
                ambush->m_revenge = item->m_revenge;
                data.m_ambush = true;
                XString message = ambush->GetBountyMessage();
                CGPSHaunt* haunt = GetHauntByName(camera->m_haunt);
                if (haunt != NULL) {
                    message = message.Replace(L"[LocationName]", haunt->m_screenName.c_str(), true);
                }
                data.m_field6c = message;
                item = ambush;
            }
        }
    }
    CSquadType* squad = app->InitSquadTypeManager()->GetType(item->m_squadName);
    if (squad == NULL) {
        delete ambush;
        return;
    }
    data.m_bossName = item->m_bossName;
    data.m_bossType = item->m_boss;
    data.m_squadType = squad;
    if (useScript) {
        XString scriptName = item->GetScriptName(camera);
        if (scriptName.Length() != 0) {
            data.m_scriptType = app->m_scriptManager->GetType(scriptName);
        }
    }
    data.m_civilianCount = item->m_civilianCount;
    data.m_experienceReward = item->m_experienceReward;
    data.m_moneyReward = item->m_moneyReward;
    data.m_experiencePenalty = item->m_experiencePenalty;
    data.m_moneyPenalty = item->m_moneyPenalty;
    data.m_locationName = location->m_name;
    data.m_ambientSound = camera->m_ambientSound;
    data.m_missionId = item->GetMissionId();
    data.m_field4c = 4;
    if (item->GetType() == 1) {
        data.m_field4c = static_cast<CGPSBounty*>(item)->m_bountyType;
    }
    data.m_cameraIndex = camera->m_cameraIndex;
    data.m_cameraHorizontalLimit = camera->m_horizontalLimit;
    data.m_cameraVerticalLimit = camera->m_verticalLimit;
    data.m_introTimeMs = camera->m_introTimeMs;
    data.m_cameraDurationMs = camera->m_durationMs;
    data.m_cameraHUDModel = camera->m_hudModel;
    data.m_additionalMinionCount = camera->m_additionalMinionCount;
    data.m_hasTarget = item->m_targetFriendId != -1 || item->m_targetFBUser.Length() != 0;
    data.m_revenge = item->m_revenge;
    data.m_weaponGated = item->m_weaponGated && item->m_gatedWeapon < 0;
    if (data.m_weaponGated) {
        data.m_weaponCategory = CDH_Weapon::GetWeaponCategoryType(camera->m_gatedWeaponCategory);
    }
    data.m_requiredWeaponId = item->m_gatedWeapon;
    data.m_randomWeaponId = item->m_randomWeapon;
    data.m_alertSeconds = item->m_alertSeconds;
    data.m_escapeSeconds = item->m_escapeSeconds;
    data.m_tripleMultiplierSeconds = app->m_helper->m_config.GetIntegerValueByToken("TIME_XP_MULTI_X3_SEC", true);
    data.m_doubleMultiplierSeconds = app->m_helper->m_config.GetIntegerValueByToken("TIME_XP_MULTI_X2_SEC", true);
    data.m_tranquillize = item->m_tranquillize;
    data.m_multiplierCountdown = item->m_warTimer;
    data.m_field9c = true;
    data.m_bossFirst = item->m_bossFirst;
    data.m_bossWins = item->m_bossWins;
    data.m_bossSpawnsLast = item->m_bossSpawnsLast;
    data.m_silentKill = item->m_silentKill;
    data.m_missionName = XString::Format(L"%02d-%03d-%02d", player->GetLevel(), item->GetAAId(), camera->m_id);

    CDH_WeaponsManager* weapons = app->InitWeaponManager();
    CDH_Weapon* estimatedWeapon = weapons->GetWeaponById(item->GetEstimatedWeaponID());
    player->SetEstimatedWeapon(estimatedWeapon, item->GetEstimatedAmmonCount());
    if (item->m_weaponGated) {
        if (item->m_gatedWeapon >= 0) {
            SetWeaponWithIDInBothHands(item->m_gatedWeapon);
        } else {
            SetWeaponWithCategoryInBothHands(camera->m_gatedWeaponCategory);
        }
    }
    if (item->m_randomWeapon >= 0) {
        CDH_Weapon* random = weapons->GetWeaponById(item->m_randomWeapon);
        if (random != NULL) {
            player->SetEstimatedWeapon(random, random->m_clipSize * 20);
        }
    }
    CMission* mission = app->m_missionManager->GetMissionById(data.m_missionId);
    if (mission != NULL) {
        data.m_hasFakeBoss = mission->m_fakeBoss.Length() != 0;
        if (mission->m_weaponGiftId >= 0) {
            CDH_Weapon* gift = weapons->GetWeaponById(mission->m_weaponGiftId);
            if (gift != NULL) {
                app->m_helper->m_gamePlayManager.SetWeapon01(gift->m_id);
                gift->setAvailable(true);
                player->SetAmmoForWeaponId(gift->m_id, 2);
            }
        }
    }

    app->InitProgressSystem()->SetMissionDifficulty(item->GenerateDiffRandomizer());
    app->m_huntingInfo->SetHuntingData(data);
    if (!data.m_hasTarget) {
        PrepareHuntBrief(*camera, *item);
    }
    player->m_targetFriendId = item->m_targetFriendId;
    player->m_missionBriefTargetFB = item->m_targetFBUser.c_str();
    player->m_missionBriefEnergy = item->m_energyCost;
    player->m_lastHuntItem = camera->m_id;
    player->m_lastHuntResult = 0;
    player->ReduceEnergy(item->m_energyCost);
    ++player->m_fieldb4;
    player->Save();
    Save();
    CGameAnalytics::logMissionStarted(data.m_missionName);
    delete ambush;
    m_parent->SetInterrupt(1);
}

void CBH_GPSMap::SetWeaponInBothHands(CDH_Weapon* weapon)
{
    if (weapon != NULL) {
        CBH_Player::GetInstance()->SetEstimatedWeapon(weapon, -1);
    }
}

void CBH_GPSMap::SetWeaponWithIDInBothHands(int id)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    SetWeaponInBothHands(app->InitWeaponManager()->GetWeaponById(id));
}

void CBH_GPSMap::SetWeaponWithCategoryInBothHands(const XString& categoryName)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_BasicGameData* gameData = app->m_helper->GetGameData();
    const CDH_Weapon* first = gameData->GetWeapon01(true);
    const CDH_Weapon* second = gameData->GetWeapon02(true);
    if (first != NULL && second != NULL && first->m_category != second->m_category) {
        CDH_Weapon::EWeaponCategory category = CDH_Weapon::GetWeaponCategoryType(categoryName);
        SetWeaponInBothHands(FindGatedWeapon(gameData, -1, category));
    }
}

bool CBH_GPSMap::WillInformantUnlockOnLevelUp()
{
    int nextLevel = CBH_Player::GetInstance()->m_level + 1;
    for (int index = 0; index < m_instantInformants.size(); ++index) {
        if (m_instantInformants[index]->m_unlockLevel == nextLevel) {
            return true;
        }
    }
    return false;
}

void CBH_GPSMap::OnGetFocus()
{
}

void CBH_GPSMap::CheckGiftReceivedNews()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CNewsFeedManager* news = app->m_newsFeedManager;
    if (news == NULL || app->m_notificationHandler == NULL || app->m_saveManager->m_pauseSaving
        || app->m_saveManager->m_pauseGServe != 0 || app->m_field3a0
        || CNGSLoginFlow::GetInstance()->IsRunning() || news->GetNewsCount(NEWS_GIFT) == 0) {
        return;
    }
    unsigned int now = CStdUtil_Cocoa::GetTimeSeconds();
    if (now < m_nextGiftSaveAttempt) {
        return;
    }
    TiXmlElement* root = app->m_saveManager->GetRootElement();
    TiXmlElement* receipts = root->FirstChildElement("received_gifts");
    Vector<const SGiftReceivedMessage*> awaitingAck;
    HashTable<HashTableKey<int>, int> giftCounts;
    HashTable<HashTableKey<int>, int> giftSenders;
    Vector<const SMessage*> gifts;
    news->GetNewsByType(NEWS_GIFT, gifts);
    for (int index = 0; index < gifts.size(); ++index) {
        const SGiftReceivedMessage* received = static_cast<const SGiftReceivedMessage*>(gifts[index]);
        if (received->m_acknowledged || received->m_messageId < 0) {
            continue;
        }
        if (received->m_claimed) {
            awaitingAck.addElement(received);
            continue;
        }
        bool alreadySaved = false;
        if (receipts != NULL) {
            for (const TiXmlElement* receipt = receipts->FirstChildElement("gift"); receipt != NULL;
                 receipt = receipt->NextSiblingElement("gift")) {
                int messageId = -1;
                receipt->QueryIntAttribute("id", &messageId);
                if (messageId == received->m_messageId) {
                    alreadySaved = true;
                    break;
                }
            }
        }
        if (alreadySaved) {
            received->m_claimed = true;
            awaitingAck.addElement(received);
            continue;
        }
        SGift* gift = app->m_socialGiftManager->GetGift(received->m_itemIndex);
        if (gift == NULL) {
            continue;
        }
        CItem* item = gift->GetItem();
        if (item == NULL) {
            continue;
        }
        app->m_socialGiftManager->GiveGift(received->m_itemIndex);
        received->m_claimed = true;
        awaitingAck.addElement(received);
        // Commit the reward and its receipt together before acknowledging the server message.
        if (receipts == NULL) {
            receipts = CXmlHelper::ObtainElement(root, XString("received_gifts"));
        }
        TiXmlElement receipt("gift");
        receipt.SetAttribute("id", received->m_messageId);
        receipts->InsertEndChild(receipt);
        HashTableKey<int> key(received->m_itemIndex);
        int* currentCount = giftCounts.Get(key);
        int count = 1;
        if (currentCount != NULL) {
            count += *currentCount;
        }
        giftCounts.Set(key, count);
        giftSenders.Add(key, received->m_senderId);
    }
    if (awaitingAck.size() == 0) {
        return;
    }
    CBH_Player::GetInstance()->Save();
    if (app->m_saveManager->Save(false) == SAVE_SUCCESS) {
        for (int index = 0; index < awaitingAck.size(); ++index) {
            const SGiftReceivedMessage* received = awaitingAck[index];
            app->m_notificationHandler->RemoveGameSpecificMessage(received->m_messageId);
            received->m_acknowledged = true;
        }
    } else {
        m_nextGiftSaveAttempt = now + 5;
    }
    Vector<HashTableKey<int> > giftIds;
    giftCounts.ToKeys(giftIds);
    for (int index = 0; index < giftIds.size(); ++index) {
        const HashTableKey<int>& key = giftIds[index];
        const int count = *giftCounts.Get(key);
        const int sender = *giftSenders.Get(key);
        const XString nickname = CGServeHelper::GetFriendNickname(sender);
        if (nickname.Length() == 0) {
            return;
        }
        const XString name = app->m_socialGiftManager->GetGift(key.m_value)->GetItem()->GetName();
        const char* title;
        XString text;
        if (count < 2) {
            title = "IDS_GIFT_RECIEVED_TITLE";
            XString format = ResString("IDS_GIFT_RECIEVED_TEXT");
            text = XString::Format(format.c_str(), name.c_str(), nickname.c_str());
        } else {
            title = "IDS_GIFT_RECIEVED_MULTIPLE_TITLE";
            XString format = ResString("IDS_GIFT_RECIEVED_MULTIPLE_TEXT");
            text = XString::Format(format.c_str(), name.c_str(), nickname.c_str(), count - 1);
        }
        ICSystemFont* font = ICSystemFont::CreateInstance();
        font->Initialize(SScreenAdaptingConstant(12, 24)(), NULL, 1);
        CTextDialog* dialog = new CTextDialog();
        dialog->SetButtons(CDialogWindow::BUTTON_OK);
        dialog->SetTitleText(title);
        dialog->SetMessageText(text, font);
        m_startDialogs->Add(dialog);
    }
    if (giftIds.size() != 0) {
        m_startDialogs->Start();
    }
}

void CBH_GPSMap::CheckChallengeNews()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CNewsFeedManager* news = app->m_newsFeedManager;
    if (news == NULL || app->m_notificationHandler == NULL || app->m_saveManager->m_pauseSaving
        || app->m_saveManager->m_pauseGServe != 0 || app->m_field3a0
        || CNGSLoginFlow::GetInstance()->IsRunning()) {
        return;
    }
    unsigned int now = CStdUtil_Cocoa::GetTimeSeconds();
    if (now < m_nextChallengeSaveAttempt) {
        return;
    }
    Vector<const SMissionInviteMessage*> awaitingAck;
    Vector<ENewsType> challengeTypes;
    challengeTypes.addElement(NEWS_CHALLENGE_INVITE);
    challengeTypes.addElement(NEWS_CHALLENGE_COMPLETED);
    bool queuedDialog = false;
    for (int typeIndex = 0; typeIndex < challengeTypes.size(); ++typeIndex) {
        ENewsType type = challengeTypes[typeIndex];
        if (news->GetNewsCount(type) == 0) {
            continue;
        }
        Vector<const SMessage*> messages;
        news->GetNewsByType(type, messages);
        HashTable<HashTableKey<int>, int> challengeCounts;
        HashTable<HashTableKey<int>, int> challengeSenders;
        for (int index = 0; index < messages.size(); ++index) {
            const SMissionInviteMessage* message = static_cast<const SMissionInviteMessage*>(messages[index]);
            if (message->m_acknowledged || message->m_messageId < 0) {
                continue;
            }
            CChallenge* challenge = app->m_challengeManager->GetChallengeById(message->m_missionId);
            if (challenge == NULL) {
                continue;
            }
            if (!message->m_accepted) {
                message->m_accepted = true;
                if (challenge->m_active && message->m_senderId > 0) {
                    HashTableKey<int> key(message->m_missionId);
                    int* currentCount = challengeCounts.Get(key);
                    int count = 1;
                    if (currentCount != NULL) {
                        count += *currentCount;
                    }
                    challengeCounts.Set(key, count);
                    challengeSenders.Add(key, message->m_senderId);
                    challenge->FriendInvited(message->m_senderId);
                    if (type == NEWS_CHALLENGE_COMPLETED) {
                        challenge->FriendCompleteChallenge(message->m_senderId);
                    }
                }
                if (type == NEWS_CHALLENGE_COMPLETED || !challenge->m_active) {
                    message->m_read = true;
                }
            }
            awaitingAck.addElement(message);
        }
        Vector<HashTableKey<int> > challengeIds;
        challengeCounts.ToKeys(challengeIds);
        for (int index = 0; index < challengeIds.size(); ++index) {
            const HashTableKey<int>& key = challengeIds[index];
            CChallenge* challenge = app->m_challengeManager->GetChallengeById(key.m_value);
            const int count = *challengeCounts.Get(key);
            const int sender = *challengeSenders.Get(key);
            if (QueueChallengeNewsDialog(challenge, sender, count, type == NEWS_CHALLENGE_COMPLETED)) {
                queuedDialog = true;
            }
        }
    }
    if (awaitingAck.size() > 0) {
        // Persist the friend IDs and reward flags before acknowledging the server messages.
        if (app->m_saveManager->Save(false) == SAVE_SUCCESS) {
            for (int index = 0; index < awaitingAck.size(); ++index) {
                const SMissionInviteMessage* message = awaitingAck[index];
                app->m_notificationHandler->RemoveGameSpecificMessage(message->m_messageId);
                message->m_acknowledged = true;
            }
        } else {
            m_nextChallengeSaveAttempt = now + 5;
        }
    }
    if (queuedDialog) {
        m_startDialogs->Start();
    }
}

bool CBH_GPSMap::QueueChallengeNewsDialog(CChallenge* challenge, int sender, int count, bool completed)
{
    XString nickname = CGServeHelper::GetFriendNickname(sender);
    if (nickname.Length() == 0) {
        return false;
    }
    const char* title = "IDS_CHALLENGE_INVITE_DIALOG_TITLE";
    const char* body = "IDS_CHALLENGE_INVITE_DIALOG_TEXT";
    const char* multipleTitle = "IDS_CHALLENGE_MULTIPLE_INVITE_DIALOG_TITLE";
    const char* multipleBody = "IDS_CHALLENGE_MULTIPLE_INVITE_DIALOG_TEXT";
    CDialogWindow* dialog;
    if (completed) {
        if (challenge->GetSoloReward()->m_awarded) {
            title = "IDS_CHALLENGE_FRIEND_COMPLETED_POPUP_TITLE";
            body = "IDS_CHALLENGE_FRIEND_COMPLETED_POPUP_MESSAGE";
            multipleTitle = "IDS_CHALLENGE_COMPLETE_MULTIPLE_TITLE";
            multipleBody = "IDS_CHALLENGE_COMPLETE_MULTIPLE_TEXT";
        } else {
            title = "IDS_CHALLENGE_FRIEND_COMPLETED_NO_REWARD_POPUP_TITLE";
            body = "IDS_CHALLENGE_FRIEND_COMPLETED_NO_REWARD_POPUP_MESSAGE";
            multipleTitle = "IDS_CHALLENGE_COMPLETE_NO_REWARD_MULTIPLE_TITLE";
            multipleBody = "IDS_CHALLENGE_COMPLETE_NO_REWARD_MULTIPLE_TEXT";
        }
        dialog = new CChallengeCompletedDialog();
    } else {
        dialog = new CTextDialog();
    }
    XString text;
    if (count < 2) {
        XString rewardText;
        const SChallengeReward* reward = challenge->GetRewardForFriend(sender);
        if (reward != NULL) {
            rewardText = challenge->GetRewardString(reward);
        }
        XString format = ResString(body);
        text = XString::Format(format.c_str(), nickname.c_str(), challenge->m_name.c_str(), rewardText.c_str());
    } else {
        title = multipleTitle;
        XString format = ResString(multipleBody);
        XString firstReward = challenge->GetRewardString(challenge->GetX1Reward());
        XString secondReward = challenge->GetRewardString(challenge->GetX2Reward());
        text = XString::Format(format.c_str(), nickname.c_str(), count - 1, challenge->m_name.c_str(),
                               firstReward.c_str(), secondReward.c_str());
    }
    ICSystemFont* font = ICSystemFont::CreateInstance();
    font->Initialize(SScreenAdaptingConstant(12, 24)(), NULL, 1);
    dialog->SetTitleText(title);
    dialog->SetMessageText(text, font);
    dialog->SetButtons(CDialogWindow::BUTTON_OK);
    dialog->SetSound("IDM_COOP_CHALLENGE");
    m_startDialogs->Add(dialog);
    return true;
}

void CBH_GPSMap::CheckForNewChallenges()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (app->m_mainLayer->HasModalChild() || !m_startDialogs->IsEmpty()
        || CChallengeManager::GetTimeSec() == 0 || !app->GetNetworkAvailability()->IsAvailable()) {
        return;
    }
    CChallengeManager* manager = app->m_challengeManager;
    int day = manager->GetCurrentChallengeDay();
    if (day == m_currentChallengeDay) {
        return;
    }
    Vector<CChallenge*> challenges = manager->GetActiveChallenges();
    if (challenges.size() < 3) {
        return;
    }
    CChallengesDialog* dialog = new CChallengesDialog();
    XString format = ResString("IDS_CHALLENGE_LIST_TEXT");
    dialog->SetChallengeText(XString::Format(format.c_str(), challenges[0]->m_name.c_str(),
        challenges[1]->m_name.c_str(), challenges[2]->m_name.c_str()));
    m_startDialogs->Add(dialog);
    m_startDialogs->Start();
    m_currentChallengeDay = day;
    Save();
}

void CBH_GPSMap::Update()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->InitAchievementManager()->Update();
    CheckGiftReceivedNews();
    if (!CTutorialManager::IsPlayerInTutorial()) {
        app->m_challengeManager->Update();
        CheckForNewChallenges();
        CheckChallengeNews();
        MakeDailyDealCheck();
    }
    if (m_shop != NULL) {
        m_shop->Update(app->m_updateFrameTime);
    }
    if (m_network != NULL) {
        m_network->SetVisible(CTutorialManager::IsHomeVisible());
        m_network->Update(app->m_updateFrameTime);
    }
    if (m_bar != NULL) {
        m_bar->Update(app->m_updateFrameTime);
    }
    if (m_cooperation != NULL) {
        m_cooperation->Update(app->m_updateFrameTime);
    }
    for (int index = 0; index < m_haunts.size(); ++index) {
        m_haunts[index]->Update(app->m_updateFrameTime);
    }
    for (int index = 0; index < m_instantInformants.size(); ++index) {
        m_instantInformants[index]->Update(app->m_updateFrameTime);
    }
    for (int group = 0; group < 5; ++group) {
        for (int index = 0; index < m_missionInformants[group].size(); ++index) {
            m_missionInformants[group][index]->Update(app->m_updateFrameTime);
        }
    }
    CTjManager* featuredOffers = CTjManager::GetInstance();
    featuredOffers->CheckFeaturedAppCondition();
    if (featuredOffers->m_elapsed == 0) {
        featuredOffers->m_queryState = 0;
        if (TapjoyInterface::getFeaturedAppQueryState()) {
            featuredOffers->m_queryState = 1;
        }
    }
    featuredOffers->Update(app->m_updateFrameTime);
    UpdateScroll();
    Window::Update();
    if (m_openTargetPickerAfterLogin && !CNGSLoginFlow::GetInstance()->IsRunning()
        && m_parent != NULL && !m_parent->HasModalChild() && !HasModalChild()) {
        m_openTargetPickerAfterLogin = false;
        ShowSelectFriendDialog();
    }
    if (m_acceptInformant && !m_parent->HasModalChild()) {
        m_acceptInformant = false;
        CGPSInstantInformant* informant = m_selectedInformant;
        m_selectedInformant = NULL;
        if (informant != NULL) {
            ResolveInformantPurchase(informant);
        }
    }
    if (m_showTutorialFinish && !m_parent->HasModalChild() && m_startDialogs->IsEmpty()) {
        m_showTutorialFinish = false;
        WindowApp::HandleTunnelCommand(0x40067282, 0, 0, 0);
    }
    if (m_advanceMissionDialog && !m_parent->HasModalChild()) {
        m_advanceMissionDialog = false;
        if (m_friendGatedMission != NULL) {
            StartNextFriendGatedMission();
        } else if (m_selectedCamera != NULL) {
            if (m_showingHuntConfirmation) {
                CGPSHauntCamera* camera = m_selectedCamera;
                m_selectedCamera = NULL;
                StartHunt(camera, NULL, true);
            } else {
                ShowMissionDialog();
            }
        }
    }
    if (!IsVisible() || m_parent == NULL || m_parent->HasModalChild() || HasModalChild()
        || !m_startDialogs->IsEmpty() || m_selectedCamera != NULL || m_friendGatedMission != NULL
        || m_showTutorialFinish) {
        m_mapIdleElapsed = 0;
    } else {
        const int elapsed = app->m_updateFrameTime;
        if (m_mapIdleElapsed < 2000) {
            m_mapIdleElapsed += elapsed;
        }
        CBH_Player* player = CBH_Player::GetInstance();
        if (m_mapIdleElapsed > 200 && (player->m_newPlayer || player->IsGameVersionNew())) {
            ShowWhatsNewDialog();
            return;
        }
        if (m_mapIdleElapsed >= 2000) {
            HandleBonuses(elapsed);
        }
        if (m_showRocketLauncherIntro && !CTutorialManager::IsPlayerInTutorial()
            && !m_parent->HasModalChild() && m_startDialogs->IsEmpty()) {
            m_showRocketLauncherIntro = false;
            ShowRocketLauncherInroduce();
            Save();
        }
    }
}

void CBH_GPSMap::ShowWhatsNewDialog()
{
    CDialogWithDelayedButtons* dialog = new CDialogWithDelayedButtons(4000);
    dialog->SetTitleText(ResString("IDS_WHATS_NEW_TITLE"));
    dialog->SetMessageText(ResString("IDS_WHATS_NEW_TEXT"));
    dialog->SetButtons(CDialogWindow::BUTTON_OK);
    dialog->SetButtonsCommands(AcknowledgeWhatsNew, -1, -1);
    m_startDialogs->Add(dialog);
    m_startDialogs->Start();
}

void CBH_GPSMap::ShowRocketLauncherInroduce()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_Weapon* launcher = app->InitWeaponManager()->GetWeaponById(RocketLauncherId);
    if (launcher == NULL) {
        return;
    }
    launcher->setAvailable(true);
    const CAmmo* ammo = launcher->GetActiveAmmo();
    CBH_Player* player = CBH_Player::GetInstance();
    if (ammo != NULL && player->GetAvailableAmmo(ammo, true) < 3) {
        player->SetAmmoCount(ammo, 3);
    }
    player->Save();
    CIntroduceItemWindow* dialog = new CIntroduceItemWindow(launcher, XString(""), true, false);
    dialog->SetOnCloseCommand(AcceptMissionDialog);
    m_startDialogs->Add(dialog);
    m_startDialogs->Start();
}

void CBH_GPSMap::HandleBonuses(int elapsed)
{
    CBH_Player* player = CBH_Player::GetInstance();
    bool newDay = false;
    if (player->TryMakeFiveDaysBonus(&newDay)) {
        m_bonusTeaserElapsed = 0;
        ShowFiveDaysBonusMessage();
        if (player->CanShowFiveDaysIncentiveBonusExpirationMessage()) {
            ShowFiveDaysIncentiveBonusExpirationMessage();
            player->RegisterFiveDaysIncentiveBonusExpirationMessageShown();
        }
        m_startDialogs->Start();
    } else if (newDay) {
        m_bonusTeaserElapsed += elapsed;
        if (m_bonusTeaserElapsed > 60000) {
            m_bonusTeaserElapsed = 0;
            ShowFiveDaysBonusTeaser();
        }
    }
    if (player->TryMakeFiveDaysIncentiveBonus()) {
        ShowFiveDaysIncentivisedBonusMessage();
    }
}

void CBH_GPSMap::ShowFiveDaysBonusMessage()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    int day = app->m_fiveDaysBonus->GetDay();
    m_startDialogs->Add(new CFiveDaysBonusDialog(day, CFiveDaysBonusDialog::DAILY_BONUS));
}

void CBH_GPSMap::ShowFiveDaysIncentiveBonusExpirationMessage()
{
    CTextDialog* dialog = new CTextDialog();
    dialog->SetTitleText("IDS_DIALOG_TITLE_WARNING");
    dialog->SetMessageText(ResString("IDS_DAILY_BONUS_EXPIRE"));
    dialog->SetButtons(CDialogWindow::BUTTON_OK);
    m_startDialogs->Add(dialog);
}

void CBH_GPSMap::ShowFiveDaysBonusTeaser()
{
    int day = App::GetCurrentDay();
    CBH_Player* player = CBH_Player::GetInstance();
    if (day <= player->m_lastFiveDaysBonusTeaserDay) {
        return;
    }
    player->m_lastFiveDaysBonusTeaserDay = day;
    CTextDialog* dialog = new CTextDialog();
    dialog->SetTitleText("IDS_FIVE_DAYS_BONUS_OFFLINE_TITLE");
    dialog->SetMessageText(ResString("IDS_FIVE_DAYS_BONUS_OFFLINE_TEXT"));
    dialog->SetButtons(CDialogWindow::BUTTON_OK);
    m_parent->AddModal(dialog);
}

void CBH_GPSMap::ShowFiveDaysIncentivisedBonusMessage()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    int day = app->m_fiveDaysIncentiveBonus->GetDay();
    m_parent->AddModal(new CFiveDaysBonusDialog(day, CFiveDaysBonusDialog::INCENTIVE_BONUS));
}

void CBH_GPSMap::ShowFiveDaysInventiveBonusRecomendedItem()
{
    int day = CBH_Player::GetInstance()->m_activeFiveDaysIncentiveBonusDay;
    int type;
    int itemId = 0;
    switch (day) {
    case 1:
        type = 11;  // Armor
        break;
    case 2:
        type = 9;   // Ammo pack
        break;
    case 3:
        type = 10;  // Go pill
        break;
    case 4:
        type = 9;
        itemId = 2;
        break;
    case 5:
        type = 3;   // Medicant
        itemId = 3;
        break;
    default:
        return;
    }
    CItemManager* manager = App::GetItemManagerByType(type);
    if (manager != NULL) {
        CItem* item = manager->getItemById(itemId);
        if (item != NULL) {
            CRegularShop::ShowDeal(item);
        }
    }
}

void CBH_GPSMap::MakeDailyDealCheck()
{
    if (m_parent == NULL || m_parent->HasModalChild() || HasModalChild()
        || !m_startDialogs->IsEmpty() || m_selectedCamera != NULL || m_showTutorialFinish) {
        return;
    }
    unsigned int now = CStdUtil_Cocoa::GetTimeSeconds();
    bool firstOffer = !m_dailyDealShown && CBH_Player::GetInstance()->m_completedMissionCount == 1;
    if (firstOffer || (now >= m_lastDailyDealDisplay && now - m_lastDailyDealDisplay > 1200)) {
        m_dailyDealShown = true;
        m_lastDailyDealDisplay = now;
        ShowDailyDealWindow();
    }
}

void CBH_GPSMap::ShowDailyDealWindow()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDailyDealManager* manager = app->InitDailyDealManager();
    if (manager->GetCurrentDailyDeal() == NULL) {
        return;
    }
    CItem* item = manager->GetCurrentDealItem();
    if (item != NULL && item->m_unlockLevel <= CBH_Player::GetInstance()->GetLevel()) {
        AddModal(new CDailyDealWindow());
        Save();
    }
}

void CBH_GPSMap::ReleasePrefetchedManagerResource()
{
    for (int index = m_prefetchedManagers.size() - 1; index >= 0; --index) {
        m_prefetchedManagers[index]->freeResources();
    }
}

void CBH_GPSMap::Paint(ICGraphics2d& graphics)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    FillRect(graphics, 0, 0, app->m_screenWidth, app->m_screenHeight, 0xff636563);
    if (m_columns < 1 || m_rows < 1) {
        return;
    }

    int y = m_mapY;
    for (int row = 0; row < m_rows; ++row) {
        int x = m_mapX;
        int rowHeight = 0;
        for (int column = 0; column < m_columns; ++column) {
            int index = row * m_columns + column;
            if (index >= m_tiles.size()) {
                break;
            }
            ICRenderSurface* tile = m_tiles[index];
            if (tile == NULL) {
                x += m_mapWidth / m_columns;
                continue;
            }
            app->m_helper->DrawImage(tile, x, y, 1.0f, 0, 0, 0.0f, 0);
            x += ImageWidth(tile);
            rowHeight = ImageHeight(tile);
        }
        if (rowHeight == 0) {
            rowHeight = m_mapHeight / m_rows;
        }
        y += rowHeight;
    }
    for (int index = 0; index < m_haunts.size(); ++index) {
        m_haunts[index]->Paint(graphics, m_mapX, m_mapY);
    }
    for (int index = 0; index < m_instantInformants.size(); ++index) {
        m_instantInformants[index]->Paint(graphics, m_mapX, m_mapY);
    }
    for (int group = 0; group < 5; ++group) {
        for (int index = 0; index < m_missionInformants[group].size(); ++index) {
            m_missionInformants[group][index]->Paint(graphics, m_mapX, m_mapY);
        }
    }
    if (m_shop != NULL) {
        m_shop->Paint(graphics, m_mapX, m_mapY);
    }
    if (m_network != NULL && m_network->m_visible) {
        m_network->Paint(graphics, m_mapX, m_mapY);
    }
    if (m_zombiesPromo != NULL) {
        m_zombiesPromo->Paint(graphics, m_mapX, m_mapY);
    }
}

CGPSItem* CBH_GPSMap::FindItem(int x, int y)
{
    CGPSItem* item = NULL;
    if (m_zombiesPromo != NULL && m_zombiesPromo->CheckHit(x - m_mapX, y - m_mapY, &item)) {
        return item;
    }
    if (m_network != NULL && m_network->CheckHit(x - m_mapX, y - m_mapY, &item)) {
        return item;
    }
    if (m_shop != NULL && m_shop->CheckHit(x - m_mapX, y - m_mapY, &item)) {
        return item;
    }
    for (int group = 4; group >= 0; --group) {
        for (int index = m_missionInformants[group].size() - 1; index >= 0; --index) {
            if (m_missionInformants[group][index]->CheckHit(x - m_mapX, y - m_mapY, &item)) {
                return item;
            }
        }
    }
    for (int index = m_instantInformants.size() - 1; index >= 0; --index) {
        if (m_instantInformants[index]->CheckHit(x - m_mapX, y - m_mapY, &item)) {
            return item;
        }
    }
    for (int index = m_haunts.size() - 1; index >= 0; --index) {
        if (m_haunts[index]->CheckHit(x - m_mapX, y - m_mapY, &item)) {
            return item;
        }
    }
    return NULL;
}

void CBH_GPSMap::OnPointerEvent(PointerEventController::Event event)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (app->m_timeScale == 0.0f || HasModalChild() || (m_parent != NULL && m_parent->HasModalChild())) {
        return;
    }
    if (event.m_type == PointerEventController::EVENT_DOWN) {
        m_draggingMap = true;
        m_recordedMapDrag = false;
        m_scrollToItem = false;
        m_scrollVelocityX = 0.0f;
        m_scrollVelocityY = 0.0f;
        m_dragOriginX = m_mapX;
        m_dragOriginY = m_mapY;
        m_previousScrollX = m_scrollX;
        m_previousScrollY = m_scrollY;
        if (m_pressedItem != NULL) {
            m_pressedItem->SetPressed(false);
        }
        m_pressedItem = FindItem(event.m_position.x, event.m_position.y);
        if (m_pressedItem != NULL) {
            m_pressedItem->SetPressed(true);
        }
        return;
    }
    if (!m_draggingMap) {
        return;
    }
    if (event.m_type == PointerEventController::EVENT_MOVE) {
        m_scrollX += event.m_deltaX;
        m_scrollY += event.m_deltaY;
        CheckMapPosBounds();
        if (!m_recordedMapDrag) {
            CBH_Player::GetInstance()->m_gameStats.AddMapDrags(1);
            m_recordedMapDrag = true;
        }
    }
    int tolerance = App::Scale() * 15;
    bool insideTapArea = MathLib::Fabs(m_scrollX - m_dragOriginX) < tolerance
                      && MathLib::Fabs(m_scrollY - m_dragOriginY) < tolerance;
    if (m_pressedItem != NULL && (!insideTapArea || event.m_type == PointerEventController::EVENT_UP)) {
        m_pressedItem->SetPressed(false);
        m_pressedItem = NULL;
    }
    if (event.m_type != PointerEventController::EVENT_UP) {
        return;
    }
    m_draggingMap = false;
    if (insideTapArea) {
        CGPSItem* item = FindItem(event.m_position.x, event.m_position.y);
        if (item != NULL) {
            item->OnClick();
        }
    }
}

void CBH_GPSMap::ScrollToItem(const CGPSItem& item, float speed)
{
    int screenWidth = WindowApp::m_instance->m_screenWidth;
    int screenHeight = WindowApp::m_instance->m_screenHeight;
    float x = screenWidth / 2 - item.GetX() - item.GetWidth() / 2;
    float y = screenHeight / 2 - item.GetY() - item.GetHeight() / 2;
    m_scrollTargetX = RestingScrollPosition(x, screenWidth, m_mapWidth, MapWidthInset());
    m_scrollTargetY = RestingScrollPosition(y, screenHeight, m_mapHeight, MapHeightInset());
    m_scrollSpeedX = MathLib::Fabs(m_scrollTargetX - m_scrollX) * speed;
    m_scrollSpeedY = MathLib::Fabs(m_scrollTargetY - m_scrollY) * speed;
    m_scrollVelocityX = 0.0f;
    m_scrollVelocityY = 0.0f;
    m_scrollToItem = true;
}

void CBH_GPSMap::UpdateScroll()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (m_draggingMap && (app->m_timeScale == 0.0f || HasModalChild()
        || (m_parent != NULL && m_parent->HasModalChild()))) {
        m_draggingMap = false;
        m_scrollVelocityX = 0.0f;
        m_scrollVelocityY = 0.0f;
        if (m_pressedItem != NULL) {
            m_pressedItem->SetPressed(false);
            m_pressedItem = NULL;
        }
    }
    const int elapsed = app->m_updateFrameTime;
    if (m_draggingMap) {
        m_scrollVelocityX = LimitScrollStep((m_scrollX - m_previousScrollX) / (elapsed + 1), 1.0f);
        m_scrollVelocityY = LimitScrollStep((m_scrollY - m_previousScrollY) / (elapsed + 1), 1.0f);
    } else if (m_scrollToItem) {
        float remainingX = m_scrollTargetX - m_scrollX;
        float remainingY = m_scrollTargetY - m_scrollY;
        m_scrollX += LimitScrollStep(remainingX, m_scrollSpeedX * app->m_frameTimeSeconds);
        m_scrollY += LimitScrollStep(remainingY, m_scrollSpeedY * app->m_frameTimeSeconds);
        if (MathLib::Fabs(m_scrollTargetX - m_scrollX) < 1.0f
            && MathLib::Fabs(m_scrollTargetY - m_scrollY) < 1.0f) {
            m_scrollX = m_scrollTargetX;
            m_scrollY = m_scrollTargetY;
            m_scrollToItem = false;
        }
    } else {
        float edgeX = RestingScrollPosition(m_scrollX, app->m_screenWidth, m_mapWidth, MapWidthInset()) - m_scrollX;
        float edgeY = RestingScrollPosition(m_scrollY, app->m_screenHeight, m_mapHeight, MapHeightInset()) - m_scrollY;
        m_scrollX += m_scrollVelocityX * (elapsed + 1);
        m_scrollY += m_scrollVelocityY * (elapsed + 1);
        m_scrollVelocityX = SlowScroll(m_scrollVelocityX, edgeX, elapsed + 1, App::Scale());
        m_scrollVelocityY = SlowScroll(m_scrollVelocityY, edgeY, elapsed + 1, App::Scale());
        m_scrollX += LimitScrollStep(edgeX, elapsed * 0.5f);
        m_scrollY += LimitScrollStep(edgeY, elapsed * 0.5f);
    }
    CheckMapPosBounds();
    m_previousScrollX = m_scrollX;
    m_previousScrollY = m_scrollY;
}

void CBH_GPSMap::CheckMapPosBounds()
{
    int screenWidth = WindowApp::m_instance->m_screenWidth;
    int screenHeight = WindowApp::m_instance->m_screenHeight;
    if (m_scrollX > screenWidth * 0.6) {
        m_scrollX = static_cast<float>(screenWidth * 0.6);
    }
    if (m_scrollX + m_mapWidth < screenWidth * 0.4) {
        m_scrollX = static_cast<float>(screenWidth * 0.4 - m_mapWidth);
    }
    if (m_scrollY > screenHeight * 0.6) {
        m_scrollY = static_cast<float>(screenHeight * 0.6);
    }
    if (m_scrollY + m_mapHeight < screenHeight * 0.4) {
        m_scrollY = static_cast<float>(screenHeight * 0.4 - m_mapHeight);
    }
    m_mapX = static_cast<int>(m_scrollX) & ~1;
    m_mapY = static_cast<int>(m_scrollY) & ~1;
}
