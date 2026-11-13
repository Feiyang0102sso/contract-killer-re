#include "CBH_GPSMap.h"
#include "CBH_Math.h"
#include <limits.h>

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
#include "CDailyDealManager.h"
#include "CDailyDealWindow.h"
#include "CDH_SoundManager.h"
#include "CDH_Weapon.h"
#include "CDH_WeaponsManager.h"
#include "CDialogWindow.h"
#include "CDrawUtil.h"
#include "CFactionType.h"
#include "CFactionTypeManager.h"
#include "CFiveDaysBonus.h"
#include "CFiveDaysBonusDialog.h"
#include "CFiveDaysIncentiveBonus.h"
#include "CFriendGatedMission.h"
#include "CFriendGatedMissionManager.h"
#include "CFriendsWindow.h"
#include "CGameAnalytics.h"
#include "CGameScene.h"
#include "CGServeHandler.h"
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
#include "CMainItemScreen.h"
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
const int PendingMissionPrior = 0x0b;
const int PendingStoryIntro = 0x0c;
const int PendingBountyIntro = 0x0d;
const int PendingGameRating = 0x0e;
const int PendingHuntConfirmation = 0x0f;
const int PendingAmmoWarning = 0x10;
const int PendingEnergyWarning = 0x11;
const int PendingRangeWarning = 0x12;
const int PendingEquipRangeWarning = 0x13;
const int PendingSilentWeaponWarning = 0x14;
const int PendingTranqAmmoWarning = 0x15;
const int PendingMissionWeaponWarning = 0x16;
const int PendingWeaponCategoryWarning = 0x17;
const int PendingWeaponIdWarning = 0x18;
const int PendingEquipTranqWarning = 0x19;
const int PendingBuyInformant = 0x1a;
const int PendingRushInformant = 0x1b;
const int PendingFreeInformant = 0x1c;
const int PendingLowMoneyWarning = 0x1d;
const int PendingFriendGatedMission = 0x1e;
const int ViewNewChallenges = 0x79399e3c;
const int GServeLoginFinished = 3;
const int LowMoneyGetCommand = -0x17b4f1c;  // ShowLowMoneyDialog second button
const int ReviewUnreadNews = 0x5ce88d33;
const int ResumeTutorialLogin = 0x3da61159;
const int DismissFeaturedOffer = 0x005320af;
const int FirstLocationCommand = 0x070cb9b7;
const int LastLocationCommand = 0x070cb9be;
const int AcknowledgeWhatsNew = 0x137fed7f;
const int RefreshMapAfterGiftXp = static_cast<int>(0xbd680582u);
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

/** Horizontal center of a rectangle. */
inline int RectCenterX(const CRectangle& rect)
{
    return rect.m_x + (rect.m_width >> 1);
}

/** Vertical center of a rectangle. */
inline int RectCenterY(const CRectangle& rect)
{
    return rect.m_y + (rect.m_height >> 1);
}

}

class CChallengesDialog : public CTextDialog {
public:
    CChallengesDialog();
    void SetChallengeText(const XString& text);

    TextWindow* m_headerText;    // 0x120 "new challenges" text
    TextWindow* m_challengeText; // 0x124 list of the challenges
};

// Original 0xf05b0: two buttons lifted from the bottom edge, a header text and the challenge text
// in the content (no title).
CChallengesDialog::CChallengesDialog()
{
    SScreenAdaptingConstant buttonBottom(20, 40);
    SetButtons(BUTTON_OK, BUTTON_IGNORE);
    SetButtonsCommands(ViewNewChallenges, -1, -1);
    m_buttons[0]->SetCorner(m_buttons[0]->Left(), m_height - m_buttons[0]->m_height - buttonBottom());
    m_buttons[0]->SetCaption("IDS_VIEW");
    m_buttons[1]->SetCorner(m_buttons[1]->Left(), m_height - m_buttons[1]->m_height - buttonBottom());
    m_buttons[1]->setImage("IDB_BUTTON_SET_BOUNTY");
    m_buttons[1]->setPressedImage("IDB_BUTTON_SET_BOUNTY_ACTIVE");
    m_buttons[1]->SetCaption("IDS_CONTINUE");
    m_content->SetPercentWidth(70, 0, 0);

    Window* column = new Window(false);
    column->SetLayoutType(LAYOUT_SIMPLE);
    column->SetAlign(0xc);
    column->SetPercentWidth(100, 0, 0);
    column->SetHeightByContent(0, 0);
    m_content->AddToFront(column);

    Window* headerRow = new Window(false);
    headerRow->SetLayoutType(LAYOUT_SIMPLE);
    headerRow->SetAlign(0x14);
    headerRow->SetHeightByContent(0, 0);
    headerRow->SetPercentWidth(100, 0, 0);
    CFontMgr::GetInstance()->GetFont(15);  // result unused (original)
    ICSystemFont* font = ICSystemFont::CreateInstance();
    font->Initialize(SScreenAdaptingConstant(12, 24)(), NULL, 1);
    m_headerText = new TextWindow(font);
    m_headerText->SetHeightByContent(0, 0);
    m_headerText->m_debugFrame = true;
    m_headerText->SetPercentWidth(100, 0, 0);
    m_headerText->SetAlign(0x24);
    m_headerText->SetText(ResString("IDS_CHALLENGE_NEW_CHALLENGES_TEXT"));
    headerRow->AddToFront(m_headerText);
    column->AddToFront(headerRow, 0, 0);

    Window* challengeRow = new Window(false);
    challengeRow->SetLayoutType(LAYOUT_SIMPLE);
    challengeRow->SetAlign(0xc);
    challengeRow->SetHeightByContent(0, 0);
    m_challengeText = new TextWindow(CFontMgr::GetInstance()->GetFont(15));
    m_challengeText->SetAlign(1);
    m_challengeText->SetHeightByContent(0, 0);
    m_challengeText->SetPercentWidth(100, 0, 0);
    challengeRow->AddToFront(m_challengeText);
    column->AddToFront(challengeRow, 0, 1);
}

// Original 0xf04f0
void CChallengesDialog::SetChallengeText(const XString& text)
{
    CFontMgr::GetInstance()->GetFont(15);  // result unused (original)
    ICSystemFont* font = ICSystemFont::CreateInstance();
    font->Initialize(SScreenAdaptingConstant(12, 24)(), NULL, 1);
    m_challengeText->SetText(text, font);
}

bool CBH_GPSMap::m_isFirstLoading = true;
CBH_GPSMap* CBH_GPSMap::m_instance;
bool CBH_GPSMap::shopIsPainting;
bool CBH_GPSMap::weaponIsPainting;
bool CBH_GPSMap::safehouseIsPainting;

CBH_GPSMap::CBH_GPSMap()
    : CBaseScreen(false, false, false, true, false, true), m_startDialogs(NULL), m_showFeaturedInstall(true),
      m_backgroundCount(0), m_currentChallengeDay(-1), m_mapX(0), m_mapY(0),
      m_dailyDealShown(false), m_showRocketLauncherIntro(true), m_draggingMap(false), m_field150(false),
      m_pendingCommand(0), m_lastDailyDealDisplay(CStdUtil_Cocoa::GetTimeSeconds()),
      m_field180(false), m_mapIdleElapsed(0), m_bonusTeaserElapsed(0), m_skipAmmoCheck(false),
      m_selectedCamera(NULL), m_selectedInformant(NULL), m_targetingInformant(NULL),
      m_field19c(0), m_gpsScan(NULL), m_field1a4(false), m_scrollState(0),
      m_scrollVelocityX(0.0f), m_scrollVelocityY(0.0f), m_scrollSpeedX(0.0f), m_scrollSpeedY(0.0f),
      m_scrollTargetX(0.0f), m_scrollTargetY(0.0f), m_scrollX(0.0f), m_scrollY(0.0f),
      m_previousScrollX(0.0f), m_previousScrollY(0.0f), m_recordedMapDrag(true),
      m_shortcuts(NULL), m_informantType4(NULL), m_weaponGatedInformant(NULL), m_informantType5(NULL),
      m_movingMissionInformant(NULL), m_cooperation(NULL), m_bar(NULL), m_shop(NULL),
      m_network(NULL), m_pressedItem(NULL), m_friendGatedMission(NULL), m_friendGatedBoss(NULL),
      m_needSave(false), m_prefetchIndex(0), m_socialNetworkMode(1)
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
    m_mapWidth = 0;
    m_mapHeight = 0;
    XString filename = static_cast<App*>(WindowApp::m_instance)->InitResMap()->getFilename("GPS_MAP_DESCRIPTION");
    ResMap* resources = static_cast<App*>(WindowApp::m_instance)->InitResMap();
    char* xml = resources->loadText(filename);
    TiXmlDocument document;
    document.Parse(xml);
    delete[] xml;

    // Original 0xe7a74: no NULL checks on the nodes.
    TiXmlElement* map = static_cast<TiXmlElement*>(document.FirstChild("maps")->FirstChild("map"));
    map->Attribute("width", &m_mapWidth);
    m_mapWidth -= MapWidthInset();
    map->Attribute("height", &m_mapHeight);
    m_mapHeight -= MapHeightInset();
    map->Attribute("tiles_count_x", &m_columns);
    map->Attribute("tiles_count_y", &m_rows);
    m_mapX = WindowApp::m_instance->m_screenWidth / 2 - m_mapWidth / 2;
    m_mapY = WindowApp::m_instance->m_screenHeight / 2 - m_mapHeight / 2;
}

void CBH_GPSMap::InitPrefetchedManager()
{
    if (m_prefetchedManagers.size() != 0) {
        return;
    }
    m_prefetchedManagers.ensureCapacity(10);
    m_prefetchedManagers.addElement(static_cast<App*>(WindowApp::m_instance)->InitWeaponManager());
    m_prefetchedManagers.addElement(static_cast<App*>(WindowApp::m_instance)->InitAmmoManager());
    m_prefetchedManagers.addElement(static_cast<App*>(WindowApp::m_instance)->m_ammoPackManager);
    m_prefetchedManagers.addElement(static_cast<App*>(WindowApp::m_instance)->InitIAPCurrencyManager());
    m_prefetchedManagers.addElement(static_cast<App*>(WindowApp::m_instance)->m_medicantManager);
    m_prefetchedManagers.addElement(static_cast<App*>(WindowApp::m_instance)->m_goPillManager);
    m_prefetchedManagers.addElement(static_cast<App*>(WindowApp::m_instance)->m_armorManager);
    m_prefetchedManagers.addElement(static_cast<App*>(WindowApp::m_instance)->m_energyManager);
    m_prefetchedManagers.addElement(static_cast<App*>(WindowApp::m_instance)->InitMiscItemManager());
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

    if (!static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial()) {
        CheckForNewChallenges();
    }
    if (CBH_Player::GetInstance()->m_fieldb4 > 0 && CBH_Player::GetInstance()->m_energy < 3) {
        CBH_Player::GetInstance()->m_gameStats.AddOutOfEnergyTimes(1);
    }

    // A hunt result is pending when the last hunt item is still set.
    bool huntPending = CBH_Player::GetInstance()->m_lastHuntResult != 0 && CBH_Player::GetInstance()->m_lastHuntItem >= 0;
    bool weaponGated = static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_fieldc0;

    // Decide which shop page (if any) opens on top of the map.
    if (static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsShowShop() == true) {
        ShowShop(static_cast<CShopWindow::ETab>(static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->GetCurrentShopTab()), false);
        CGameAnalytics::logStoreEnter(static_cast<CGameAnalytics::EShopEnterFrom>(0));
    } else if (static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsShowHome() == true) {
        CBH_NetworkWindow::GetInstance()->BringToFrontModal(static_cast<App*>(WindowApp::m_instance)->m_mainLayer);
    } else if (!huntPending && static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_weaponGated
               && static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_requiredWeaponId == -1) {
        CDH_Weapon* weapon = GetMostExpensiveWeaponInCategory(static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_weaponCategory);
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
        static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->GetUnlockedWeaponsIdOnLevel(CBH_Player::GetInstance()->m_level, unlocked);
        if (unlocked.size() > 0) {
            ShowShopWithUnlockedWeapon(GetMostExpensiveWeaponId(unlocked), static_cast<CShopItemInfoWindow::ESource>(1));
            CGameAnalytics::logStoreEnter(static_cast<CGameAnalytics::EShopEnterFrom>(0));
        }
    } else if (CBH_Player::GetInstance()->m_lastHuntItem >= 0 && !static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial()
               && static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon01(true)->GetActiveAmmo() != NULL
               && static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon02(true)->GetActiveAmmo() != NULL) {
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
                        *static_cast<App*>(WindowApp::m_instance)->m_bountyManager->GetInfo(static_cast<CGPSBounty*>(item)->m_bountyType);
                    missionName = info.m_name;
                } else {
                    missionName = XString::Format(L"mission_%d", item->GetMissionId());
                }
                CGameAnalytics::logMissionFailed(static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_missionId, missionName);
            }
        }
        if (static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial() == true && !CBH_Player::GetInstance()->HasAnyAmmo()) {
            CBH_Player* player = CBH_Player::GetInstance();
            player->SetAmmoForWeaponId(static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon01(true)->m_id, 1);
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
        if (!static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial()) {
            CBH_Player::GetInstance()->MissionCompleted();
        }
    }

    CBH_Player* player = CBH_Player::GetInstance();
    player->m_lastHuntItem = -1;
    player->Save();
    if (static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsAutoStartMission() == true) {
        StartTutorialMission();
    } else {
        m_hud = new CBH_GPSHud(false);
        int titleHeight = GetTitleBarHeight();
        m_hud->SetOrigin(0, titleHeight);
        AddToFront(m_hud);
        CheckGameCenterAccounts();
    }
    UpdateMissionInformants();
    static_cast<App*>(WindowApp::m_instance)->m_challengeManager->Save();
    Save();
    if (static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial() == true) {
        static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->ShowStateInitMessage();
    }
    m_openFriendsWindow = false;
    m_openTargetPickerAfterLogin = false;
}

void CBH_GPSMap::AddMapPatch(ICRenderSurface* surface)
{
    // the original skips missing tiles
    if (surface != NULL) {
        m_tiles.addElement(surface);
    }
}

// Original 0xee184: ShowMessage is called with a NULL this (original; it does not use this).
void CBH_GPSMap::OnHauntSelected(CGPSHaunt* haunt)
{
    static_cast<CBH_GPSMap*>(NULL)->ShowMessage(haunt->m_screenName, haunt->m_description, NULL, NULL);
    haunt->SetAttractSpeed(0.2f);
}

void CBH_GPSMap::OnCameraSelected(CGPSHauntCamera* camera)
{
    if (!camera->HasHuntingItem()) {
        return;
    }
    CGPSHuntingItem* item = camera->GetGPSHuntingItem();
    if (item == NULL) {
        return;
    }
    m_selectedCamera = camera;
    if (!MakePlayerEnergyCheck(item->m_energyCost)) {
        return;
    }
    if (camera->m_durationMs < 0 && item->m_tranquillize && !MakePlayerTranqMissionCheck()) {
        return;
    }
    if (item->m_randomWeapon < 0 && !MakePlayerWeaponRangeCheck(camera->m_minWeaponRange)) {
        return;
    }
    if (camera->m_durationMs <= 0 && item->m_silentKill && !MakeSilentWeaponCheck()) {
        return;
    }

    if (item->m_weaponGated) {
        int weaponId = item->m_gatedWeapon;
        CDH_Weapon::EWeaponCategory category = CDH_Weapon::GetWeaponCategoryType(camera->m_gatedWeaponCategory);
        if (weaponId >= 0) {
            if (!MakePlayerWeaponGatedIDCheck(weaponId)) {
                return;
            }
        } else if (!MakePlayerWeaponGatedCategoryCheck(category)) {
            return;
        }
        if (!m_skipAmmoCheck) {
            CDH_Weapon* primary = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon01(true);
            if (primary->m_category == category) {
                if (CBH_Player::GetInstance()->GetAvailableAmmo(primary->GetActiveAmmo(), true) == 0) {
                    m_pendingCommand = PendingAmmoWarning;
                    CDialogWindow* warning = new CDialogWindow(true);
                    warning->SetTitleText("IDS_DIALOG_TITLE_WARNING");
                    warning->SetMessageText(ResString("IDS_MESSAGE_NO_AMMO"));
                    warning->SetButtons(CDialogWindow::BUTTON_OK, CDialogWindow::BUTTON_AMMO);
                    warning->SetButtonsCommands(AcceptMissionDialog, LowMoneyGetCommand, -1);
                    static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddModal(warning);
                    return;
                }
            } else {
                CDH_Weapon* secondary = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon02(true);
                if (CBH_Player::GetInstance()->GetAvailableAmmo(secondary->GetActiveAmmo(), true) == 0) {
                    m_pendingCommand = PendingAmmoWarning;
                    CDialogWindow* warning = new CDialogWindow(true);
                    warning->SetTitleText("IDS_DIALOG_TITLE_WARNING");
                    warning->SetMessageText(ResString("IDS_MESSAGE_NO_AMMO"));
                    warning->SetButtons(CDialogWindow::BUTTON_OK, CDialogWindow::BUTTON_AMMO);
                    warning->SetButtonsCommands(AcceptMissionDialog, LowMoneyGetCommand, -1);
                    static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddModal(warning);
                    return;
                }
            }
        }
    } else {
        if (!m_skipAmmoCheck && !MakePlayerAmmoCheck()) {
            return;
        }
        m_skipAmmoCheck = false;
    }

    if (camera->HasTargettingMission()) {
        const CGPSBounty* bounty = static_cast<const CGPSBounty*>(camera->GetGPSHuntingItem());
        XString empty("");
        FillBeforeTargetHuntInfo(bounty);
        m_parent->AddModal(CreateBeforeTargettingDialog(true));
        m_pendingCommand = PendingHuntConfirmation;
    } else if (item->GetType() == 0) {
        ShowMissionPriorDialogs(item->GetMissionId());
    } else if (item->GetType() == 1) {
        ShowBeforeHuntDialog();
    }
}

bool CBH_GPSMap::MakePlayerEnergyCheck(int cost)
{
    if (!CBH_Player::GetInstance()->IsEnegyEnabled()) {
        return true;
    }
    if (CBH_Player::GetInstance()->m_energy >= cost) {
        return true;
    }
    CDialogWindow* warning = new CDialogWindow(true);
    warning->SetTitleText("IDS_DIALOG_TITLE_WARNING");
    warning->SetMessageText(XString::Format(ResString("IDS_MESSAGE_NO_ENERGY").c_str(), cost));
    warning->SetButtons(CDialogWindow::BUTTON_OK, CDialogWindow::BUTTON_IGNORE);
    warning->SetButtonsCommands(AcceptMissionDialog, CancelMissionDialog, -1);
    m_parent->AddModal(warning);
    m_pendingCommand = PendingEnergyWarning;
    CGameAnalytics::logNotEnoughEnergy(CBH_Player::GetInstance()->m_energy);
    return false;
}

bool CBH_GPSMap::MakeLowAmmoCheck(const CDH_Weapon* weapon)
{
    const CAmmo* ammo = weapon->GetDefaultAmmo();
    int available = CBH_Player::GetInstance()->GetAvailableAmmo(ammo, true);
    bool sufficient = true;
    if (available < weapon->m_clipSize) {
        sufficient = false;
        m_pendingCommand = PendingAmmoWarning;
        CDialogWindow* warning = new CDialogWindow(true);
        warning->SetTitleText("IDS_DIALOG_TITLE_WARNING");
        warning->SetMessageText(XString::Format(ResString("IDS_MESSAGE_LOW_AMMO_IN_MAIN_WEAPON").c_str(),
                                                weapon->m_name.c_str()));
        warning->SetButtons(CDialogWindow::BUTTON_OK, CDialogWindow::BUTTON_IGNORE, CDialogWindow::BUTTON_AMMO);
        warning->SetButtonsCommands(CancelMissionDialog, AcceptMissionDialog, LowMoneyGetCommand);
        m_parent->AddModal(warning);
    }
    return sufficient;
}

bool CBH_GPSMap::MakePlayerAmmoCheck()
{
    m_skipAmmoCheck = false;
    bool hasAmmo = false;
    if (CBH_Player::GetInstance()->HasAmmoInBothWeapons()) {
        hasAmmo = CBH_Player::GetInstance()->HasAnyAmmo();
    }

    if (!hasAmmo) {
        m_pendingCommand = PendingAmmoWarning;
        CDialogWindow* warning = new CDialogWindow(true);
        warning->SetTitleText("IDS_DIALOG_TITLE_WARNING");
        if (!CBH_Player::GetInstance()->HasAnyAmmo()) {
            warning->SetMessageText(ResString("IDS_MESSAGE_NO_AMMO"));
            warning->SetButtons(CDialogWindow::BUTTON_OK, CDialogWindow::BUTTON_AMMO);
            warning->SetButtonsCommands(AcceptMissionDialog, LowMoneyGetCommand, -1);
        } else {
            warning->SetMessageText(ResString("IDS_MESSAGE_NO_AMMO_IN_ONE_WEAPON"));
            warning->SetButtons(CDialogWindow::BUTTON_OK, CDialogWindow::BUTTON_IGNORE, CDialogWindow::BUTTON_AMMO);
            warning->SetButtonsCommands(CancelMissionDialog, AcceptMissionDialog, LowMoneyGetCommand);
        }
        m_parent->AddModal(warning);
        return false;
    }

    CDH_Weapon* primary = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon01(true);
    if (!MakeLowAmmoCheck(primary)) {
        return false;
    }
    CDH_Weapon* secondary = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon02(true);
    return MakeLowAmmoCheck(secondary);
}

bool CBH_GPSMap::MakePlayerTranqMissionCheck()
{
    XString text;
    bool offerWeapon;
    bool offerAmmo;
    if (!CBH_Player::GetInstance()->HasTranqWeapon()) {
        text.Assign(ResString("IDS_MESSAGE_NOT_READY_TO_TRANQ_NO_WEAPON"));
        m_pendingCommand = PendingMissionWeaponWarning;
        offerWeapon = true;
        offerAmmo = false;
    } else if (!CBH_Player::GetInstance()->HasTranqWeaponWithin()) {
        text.Assign(ResString("IDS_MESSAGE_NOT_READY_TO_TRANQ_NO_WEAPON_WITHIN"));
        m_pendingCommand = PendingEquipTranqWarning;
        offerWeapon = true;
        offerAmmo = false;
    } else if (!CBH_Player::GetInstance()->HasAmmoForTranqWeaponWithin()) {
        text.Assign(ResString("IDS_MESSAGE_NOT_READY_TO_TRANQ_NO_AMMO"));
        m_pendingCommand = PendingTranqAmmoWarning;
        offerWeapon = false;
        offerAmmo = true;
    } else {
        return true;
    }

    CDialogWindow* warning = new CDialogWindow(true);
    warning->SetTitleText("IDS_DIALOG_TITLE_WARNING");
    warning->SetMessageText(text);
    CDialogWindow::EDialogButton second = CDialogWindow::BUTTON_NONE;
    if (offerWeapon) {
        second = CDialogWindow::BUTTON_WEAPON;
    } else if (offerAmmo) {
        second = CDialogWindow::BUTTON_AMMO;
    }
    warning->SetButtons(CDialogWindow::BUTTON_OK, second);
    warning->SetButtonsCommands(AcceptMissionDialog, LowMoneyGetCommand, -1);
    m_parent->AddModal(warning);
    return false;
}

bool CBH_GPSMap::MakePlayerWeaponRangeCheck(float range)
{
    if (!(range < 0.0f)) {
        bool hasWeapon = CBH_Player::GetInstance()->HasWeaponWithRange(range);
        bool hasWeaponInHands = CBH_Player::GetInstance()->HasWeaponWithRangeInHands(range);
        if (!hasWeapon || !hasWeaponInHands) {
            if (hasWeapon) {
                m_pendingCommand = PendingEquipRangeWarning;
            } else {
                m_pendingCommand = PendingRangeWarning;
            }
            CDialogWindow* warning = new CDialogWindow(true);
            warning->SetTitleText("IDS_DIALOG_TITLE_WARNING");
            XString text = XString::Format(ResString("IDS_MESSAGE_NOT_READY_TO_SMALL_RANGE_WEAPON").c_str(),
                                           static_cast<int>(range));
            warning->SetMessageText(text);
            warning->SetButtons(CDialogWindow::BUTTON_OK, CDialogWindow::BUTTON_WEAPON);
            warning->SetButtonsCommands(AcceptMissionDialog, LowMoneyGetCommand, -1);
            m_parent->AddModal(warning);
            return false;
        }
    }
    return true;
}

bool CBH_GPSMap::MakeSilentWeaponCheck()
{
    DGHelper* helper = static_cast<App*>(WindowApp::m_instance)->m_helper;
    if (helper->GetGameData()->GetWeapon01(true)->isSilent()
        && !helper->GetGameData()->GetWeapon01(true)->m_tranquillizer) {
        return true;
    }
    if (helper->GetGameData()->GetWeapon02(true)->isSilent()
        && !helper->GetGameData()->GetWeapon02(true)->m_tranquillizer) {
        return true;
    }

    m_pendingCommand = PendingSilentWeaponWarning;
    CDialogWindow* warning = new CDialogWindow(true);
    warning->SetTitleText("IDS_DIALOG_TITLE_WARNING");
    warning->SetMessageText(ResString("IDS_MESSAGE_NOT_READY_SILENT_WEAPON"));
    warning->SetButtons(CDialogWindow::BUTTON_OK, CDialogWindow::BUTTON_BUY);
    warning->SetButtonsCommands(AcceptMissionDialog, LowMoneyGetCommand, -1);
    m_parent->AddModal(warning);
    return false;
}

bool CBH_GPSMap::MakePlayerWeaponGatedCategoryCheck(CDH_Weapon::EWeaponCategory category)
{
    CDH_Weapon* primary = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon01(true);
    CDH_Weapon* secondary = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon02(true);
    if (primary == NULL || secondary == NULL) {
        return false;
    }
    if (primary->m_category == category || secondary->m_category == category) {
        return true;
    }

    CDialogWindow* warning = new CDialogWindow(true);
    warning->SetTitleText("IDS_DIALOG_TITLE_WARNING");
    XString name = CDH_Weapon::GetWeaponCategoryName(category);
    XString text = XString::Format(ResString("IDS_MESSAGE_YOU_NEED_WEAPON").c_str(), name.c_str());
    warning->SetMessageText(text);
    warning->SetButtons(CDialogWindow::BUTTON_OK, CDialogWindow::BUTTON_WEAPON);
    m_pendingCommand = PendingWeaponCategoryWarning;
    warning->SetButtonsCommands(AcceptMissionDialog, LowMoneyGetCommand, -1);
    warning->SetButtonsCommandFirstParam(0, category, 0);
    m_parent->AddModal(warning);
    return false;
}

bool CBH_GPSMap::MakePlayerWeaponGatedIDCheck(int id)
{
    CDH_Weapon* primary = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon01(true);
    CDH_Weapon* secondary = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon02(true);
    if (primary == NULL || secondary == NULL) {
        return false;
    }
    if (primary->m_id == id || secondary->m_id == id) {
        return true;
    }
    CDH_Weapon* required = static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->GetWeaponById(id);
    if (required == NULL) {
        return true;
    }

    CDialogWindow* warning = new CDialogWindow(true);
    warning->SetTitleText("IDS_DIALOG_TITLE_WARNING");
    XString text = XString::Format(ResString("IDS_MESSAGE_YOU_NEED_WEAPON").c_str(), required->m_name.c_str());
    warning->SetMessageText(text);
    warning->SetButtons(CDialogWindow::BUTTON_OK, CDialogWindow::BUTTON_WEAPON);
    m_pendingCommand = PendingWeaponIdWarning;
    warning->SetButtonsCommands(AcceptMissionDialog, LowMoneyGetCommand, -1);
    warning->SetButtonsCommandFirstParam(0, id, 0);
    m_parent->AddModal(warning);
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
    // Original 0xe8958: the weapon manager is fetched again for every step.
    CDH_Weapon* best = NULL;
    int bestPrice = -1;
    for (CDH_Weapon* weapon = static_cast<CDH_Weapon*>(static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->firstItem());
         weapon != NULL;
         weapon = static_cast<CDH_Weapon*>(static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->nextItem())) {
        if (!weapon->IsPriceless() && weapon->m_category == category && bestPrice < weapon->getPriceCredits()) {
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
    static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddModal(shop);
    CShopItemInfoWindow* details = new CShopItemInfoWindow(static_cast<App*>(WindowApp::m_instance)->InitWeaponManager(),
                                                           0, weapon.m_id,
                                                           weapon.GetStatsString(), false,
                                                           CShopItemInfoWindow::SOURCE_SHOP);
    shop->AddModal(details);
}

void CBH_GPSMap::ShowShopWithUnlockedWeapon(int weaponId, CShopItemInfoWindow::ESource source)
{
    CRegularShop* shop = CRegularShop::GetInstance(true);
    shop->SetTab(CShopWindow::TAB_WEAPONS);
    static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddModal(shop);
    CItem* weapon = static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->getItemById(weaponId);
    CShopItemInfoWindow* details = new CShopItemInfoWindow(static_cast<App*>(WindowApp::m_instance)->InitWeaponManager(), 0, weaponId,
                                                           weapon->GetStatsString(), false, source);
    shop->AddModal(details);
}

// Original 0xe8b20: prefers the weapon with the highest credit price, else the highest cash
// price; an empty list (or only free weapons) dereferences NULL (original).
int CBH_GPSMap::GetMostExpensiveWeaponId(const Vector<int>& ids)
{
    CDH_Weapon* creditWeapon = NULL;
    CDH_Weapon* cashWeapon = NULL;
    int bestCashPrice = 0;
    int bestCreditPrice = 0;
    for (int index = 0; index < ids.size(); ++index) {
        CDH_Weapon* weapon = static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->GetWeaponById(ids[index]);
        if (weapon->getPriceCredits() > bestCreditPrice) {
            creditWeapon = weapon;
            bestCreditPrice = weapon->getPriceCredits();
        }
        if (weapon->m_pricePoints > bestCashPrice) {
            cashWeapon = weapon;
            bestCashPrice = weapon->m_pricePoints;
        }
    }
    if (creditWeapon == NULL) {
        creditWeapon = cashWeapon;
    }
    return creditWeapon->m_id;
}

bool CBH_GPSMap::MakePlayerWeaponCheck(int missionId)
{
    const CMission* mission = static_cast<App*>(WindowApp::m_instance)->m_missionManager->GetMissionById(missionId);
    if (mission == NULL) {
        return false;
    }
    CDH_Weapon* primary = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon01(true);
    CDH_Weapon* secondary = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon02(true);
    if (primary == NULL || secondary == NULL) {
        return false;
    }
    const Vector<int>& permitted = mission->m_permittedWeapons;
    int count = permitted.size();
    if (count == 0) {
        return true;
    }
    int primaryId = primary->m_id;
    int secondaryId = secondary->m_id;
    for (int index = 0; index < count; ++index) {
        int id = permitted[index];
        if (primaryId == id || secondaryId == id) {
            return true;
        }
    }

    CDialogWindow* warning = new CDialogWindow(true);
    warning->SetTitleText("IDS_DIALOG_TITLE_WARNING");
    XString message;
    if (count < 2) {
        const CDH_Weapon* weapon = static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->GetWeaponById(permitted[0]);
        if (weapon == NULL) {
            return false;
        }
        message.Assign(XString::Format(ResString("IDS_MESSAGE_YOU_NEED_WEAPON").c_str(), weapon->m_name.c_str()));
    } else {
        message.Assign(ResString("IDS_MESSAGE_YOU_NEED_ONE_OF_WEAPON"));
        for (int index = 0; index < count; ++index) {
            const CDH_Weapon* weapon = static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->GetWeaponById(permitted[index]);
            if (weapon == NULL) {
                return false;
            }
            XString name;
            if (index == count - 1) {
                name.Assign(XString::Format(L" %s.", weapon->m_name.c_str()));
            } else {
                name.Assign(XString::Format(L" %s,", weapon->m_name.c_str()));
            }
            message.Append(name);
        }
    }
    warning->SetMessageText(message);
    warning->SetButtons(CDialogWindow::BUTTON_OK, CDialogWindow::BUTTON_WEAPON);
    warning->SetButtonsCommands(AcceptMissionDialog, LowMoneyGetCommand, -1);
    m_pendingCommand = PendingMissionWeaponWarning;
    m_parent->AddModal(warning);
    return false;
}

void CBH_GPSMap::OnCommand(Event& event)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    int command = event.m_id;
    if (app->m_timeScale == 0.0f && command != RefreshMapAfterGiftXp
        && (command < FirstLocationCommand || command > LastLocationCommand)) {
        return;
    }

    switch (command) {
    case AcceptMissionDialog:
        if (m_dialogsQueue.size() != 0) {
            m_dialogsQueue.removeElementAt(m_dialogsQueue.size() - 1);
        }
        if (ShowNextQueueDialog(false)) {
            return;
        }
        switch (m_pendingCommand) {
        case 0:
            return;
        case 4: {
            CAmmo* ammo = static_cast<App*>(WindowApp::m_instance)->InitAmmoManager()->GetAmmoByType(XString("pistol"));
            if (ammo != NULL) {
                CBH_Player::GetInstance()->IncAmmo(ammo, 2);
                m_pendingCommand = 0;
            }
            return;
        }
        case 0x0a:
        case PendingMissionPrior:
            ShowBeforeHuntDialog();
            return;
        case PendingStoryIntro:
        case PendingBountyIntro:
            m_selectedCamera->SetAttractSpeed(1.1f);
            m_pendingCommand = 0;
            return;
        case PendingGameRating: {
            m_pendingCommand = 0;
            CBH_Player* player = CBH_Player::GetInstance();
            player->m_appRated = true;
            player->Save();
            CGameAnalytics::logGameRated(true);
            static_cast<App*>(WindowApp::m_instance)->m_helper->LaunchRateMe();
            return;
        }
        case PendingHuntConfirmation:
            StartHunt(m_selectedCamera, NULL, true);
            return;
        case PendingEnergyWarning:
            m_pendingCommand = 0;
            ShowShop(CShopWindow::TAB_ENERGY, false);
            CGameAnalytics::logStoreEnter(CGameAnalytics::SHOP_ENTER_MISSION_PROMPT);
            return;
        case PendingBuyInformant: {
            bool bought = m_selectedInformant->Buy();
            m_pendingCommand = 0;
            if (!bought) {
                if (!static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsTutorialInstantInformant(m_selectedInformant->m_id)) {
                    ShowLowMoneyDialog();
                    return;
                }
                m_pendingCommand = PendingFreeInformant;
                m_selectedInformant->BuyFree();
                ShowMessage(ResString("IDS_TUTORIAL_TITLE"), ResString("IDS_TUTORIAL_INFORMANT_NO_MONEY"),
                            "IDB_DIALOG_TELEPHONE", NULL);
            } else {
                OnGPSInstantInformantSelected(m_selectedInformant);
            }
            WindowApp::HandleTunnelCommand(-0x72ce08c6, 0, 0, 0);
            Save();
            return;
        }
        case PendingFreeInformant:
            m_pendingCommand = 0;
            OnGPSInstantInformantSelected(m_selectedInformant);
            return;
        case PendingLowMoneyWarning:
            m_pendingCommand = 0;
            ShowShop(CShopWindow::TAB_BANK, false);
            CGameAnalytics::logStoreEnter(CGameAnalytics::SHOP_ENTER_MISSION_PROMPT);
            return;
        case PendingFriendGatedMission:
            m_pendingCommand = 0;
            StartNextFriendGatedMission();
            return;
        default:
            m_pendingCommand = 0;
            return;
        }

    case CancelMissionDialog:
        switch (m_pendingCommand) {
        case PendingMissionPrior:
            FlushDialogsQueue();
            break;
        case PendingGameRating:
            CGameAnalytics::logGameRated(false);
            break;
        case PendingAmmoWarning:
            m_pendingCommand = 0;
            m_skipAmmoCheck = true;
            OnCameraSelected(m_selectedCamera);
            return;
        }
        m_pendingCommand = 0;
        if (m_dialogsQueue.size() != 0) {
            m_dialogsQueue.removeElementAt(m_dialogsQueue.size() - 1);
        }
        ShowNextQueueDialog(false);
        return;

    case LowMoneyGetCommand: {
        const CDH_Weapon* weapon;
        switch (m_pendingCommand) {
        case PendingAmmoWarning:
        case PendingTranqAmmoWarning:
            m_pendingCommand = 0;
            ShowShop(CShopWindow::TAB_AMMO, false);
            CGameAnalytics::logStoreEnter(CGameAnalytics::SHOP_ENTER_MISSION_PROMPT);
            return;
        case PendingRangeWarning:
        case PendingEquipRangeWarning:
            m_pendingCommand = 0;
            weapon = static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->GetWeaponById(34);
            break;
        case PendingSilentWeaponWarning: {
            m_pendingCommand = 0;
            const CItem* item = static_cast<App*>(WindowApp::m_instance)->InitMiscItemManager()->getItemById(0);
            if (item == NULL) {
                return;
            }
            CMainItemScreen* inventory = new CMainItemScreen(0, 0);
            static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddModal(inventory);
            CShopItemInfoWindow* details = new CShopItemInfoWindow(
                static_cast<App*>(WindowApp::m_instance)->InitMiscItemManager(), 12, item->m_id,
                item->GetStatsString(), true, CShopItemInfoWindow::SOURCE_SHOP);
            inventory->AddModal(details);
            CGameAnalytics::logStoreEnter(CGameAnalytics::SHOP_ENTER_MISSION_PROMPT);
            return;
        }
        case PendingMissionWeaponWarning:
        case PendingEquipTranqWarning:
            m_pendingCommand = 0;
            weapon = static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->GetWeaponById(27);
            if (weapon == NULL || !weapon->m_tranquillizer) {
                return;
            }
            break;
        case PendingWeaponCategoryWarning:
            m_pendingCommand = 0;
            weapon = GetMostExpensiveWeaponInCategory(event.m_param1);
            break;
        case PendingWeaponIdWarning:
            m_pendingCommand = 0;
            weapon = static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->GetWeaponById(event.m_param1);
            break;
        case PendingRushInformant:
            m_pendingCommand = 0;
            if (m_selectedInformant->BuyInfoFaster()) {
                Save();
                OnGPSInstantInformantSelected(m_selectedInformant);
            } else {
                ShowLowMoneyDialog();
            }
            return;
        case PendingLowMoneyWarning:
            m_pendingCommand = 0;
            ShowShop(CShopWindow::TAB_BANK, true);
            CGameAnalytics::logStoreEnter(CGameAnalytics::SHOP_ENTER_MISSION_PROMPT);
            return;
        default:
            m_pendingCommand = 0;
            return;
        }
        if (weapon != NULL) {
            ShowShopWithWeapon(*weapon);
            CGameAnalytics::logStoreEnter(CGameAnalytics::SHOP_ENTER_MISSION_PROMPT);
        }
        return;
    }

    case GServeLoginFinished: {
        if (!App::m_bIsLoginOK) {
            App::m_bIsLoginOK = true;
            return;
        }
        CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
        if (!user->isAuthenticated(SOCIAL_NETWORK_FACEBOOK)
            && (!user->isAuthenticated(SOCIAL_NETWORK_GAME_CENTER)
                || !GameCenter_Facade::GetInstance()->isGameCenterSupported())) {
            return;
        }
        if (m_openFriendsWindow) {
            CFriendsWindow* friends = new CFriendsWindow();
            friends->SetTab(m_socialNetworkMode);
            AddModal(friends);
            m_openFriendsWindow = false;
        } else if (m_openTargetPickerAfterLogin) {
            ShowSelectFriendDialog();
            m_openTargetPickerAfterLogin = false;
        }
        return;
    }
    case OfferFriendGatedMission:
        ShowBeforeFriendsGatedMission();
        return;
    case ChooseTargetFriend: {
        CIncentivizedWindow* picker = reinterpret_cast<CIncentivizedWindow*>(event.m_param1);
        Vector<int> targets = picker->GetSelectedIds();
        if (targets.size() > 0) {
            m_selectedCamera->SetTargettingMissionOnCamera(m_targetingInformant, targets[0], false, false);
            m_targetingInformant->OnBountyGiven(m_selectedCamera->m_id);
            OnCameraSelected(m_selectedCamera);
            m_selectedCamera->SetAttractSpeed(1.1f);
        }
        picker->Close();
        return;
    }
    case ResumeTutorialLogin: {
        if (m_dialogsQueue.size() != 0) {
            m_dialogsQueue.removeElementAt(m_dialogsQueue.size() - 1);
        }
        if (static_cast<App*>(WindowApp::m_instance)->m_networkAvailability->IsAvailable()) {
            CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
            if (!user->isAuthenticated(SOCIAL_NETWORK_FACEBOOK)
                && !user->isAuthenticated(SOCIAL_NETWORK_GAME_CENTER)
                && GameCenter_Facade::GetInstance()->isGameCenterSupported()) {
                static_cast<App*>(WindowApp::m_instance)->StartLogin(App::LoginBothNetworks, true);
            }
        }
        return;
    }
    case DismissFeaturedOffer:
        m_showFeaturedInstall = false;
        Save();
        return;
    case ReviewUnreadNews:
        if (event.m_param1 == 0 && app->m_newsFeedManager != NULL) {
            app->m_newsFeedManager->MarkAllMessagesAsRead();
        }
        return;
    case AcknowledgeWhatsNew:
        CBH_Player::GetInstance()->ChangeGameVersion();
        return;
    case RefreshMapAfterGiftXp:
        UpdateMapItems();
        return;
    case CFiveDaysBonusDialog::INCENTIVE_BONUS_ACCEPTED:
        ShowFiveDaysInventiveBonusRecomendedItem();
        return;
    case CBH_InformantsHud::Shop:
        OnShopItemSelected(m_shop);
        return;
    case CBH_InformantsHud::Bank: {
        CGameAnalytics::logStoreEnter(CGameAnalytics::SHOP_ENTER_GPS_ITEM);
        CRegularShop* shop = CRegularShop::GetInstance(true);
        shop->SetTab(CShopWindow::TAB_BANK);
        m_parent->AddModal(shop);
        return;
    }
    case CBH_InformantsHud::SafeHouse:
        OnGPSNetworkSelected(m_network);
        return;
    case CBH_InformantsHud::Friends:
        OnBarItemSelected(m_bar);
        return;
    case CBH_InformantsHud::Target:
        OnGPSInstantInformantSelected(m_informantType4);
        return;
    case ViewNewChallenges:
        ShowCoopWindow();
        return;
    case CBH_InformantsHud::Coop:
        OnCooperationItemSelected(m_cooperation);
        return;
    case CBH_InformantsHud::Weapon:
        OnGPSInstantInformantSelected(m_weaponGatedInformant);
        return;
    case CBH_InformantsHud::Vehicle:
        if (m_movingMissionInformant != NULL) {
            OnGpsMissionInformantSelected(m_movingMissionInformant);
        } else {
            OnGPSInstantInformantSelected(m_informantType5);
        }
        return;
    case 0x070cb9b7: {
        XString location("GasStation");
        return;
    }
    case 0x070cb9b8: {
        XString location("Skyscraper");
        return;
    }
    case 0x070cb9b9: {
        XString location("Bridge");
        return;
    }
    case 0x070cb9ba: {
        XString location("Docks2");
        return;
    }
    case 0x070cb9bb: {
        XString location("Docks1");
        return;
    }
    case 0x070cb9bc: {
        XString location("OldTown");
        return;
    }
    case 0x070cb9bd: {
        XString location("DownTown2");
        return;
    }
    case 0x070cb9be: {
        XString location("DownTown1");
        return;
    }
    }
}

CGPSHaunt* CBH_GPSMap::GetHauntByName(const XString& name)
{
    for (int index = 0; index < m_haunts.size(); ++index) {
        CGPSHaunt* haunt = m_haunts[index];
        if (haunt->m_name == name) {
            return haunt;
        }
    }
    return NULL;
}

void CBH_GPSMap::ShowBountyPriorDialogs(const CGPSBounty* bounty, const XString& hauntName, CFont* font)
{
    XString message = bounty->GetBountyMessage().Replace(L"[LocationName]", GetHauntByName(hauntName)->m_screenName.c_str(), true);
    XString title = bounty->GetObjectiveTitle().Upper();
    ShowBountyPriorDialogs(message, title, font);
}

void CBH_GPSMap::ShowBountyPriorDialogs(const XString& message, const XString& title, CFont* font)
{
    m_pendingCommand = PendingBountyIntro;
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
    static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddModal(dialog);
}

CBeforeHuntDialog* CBH_GPSMap::CreateBeforeHuntDialog(bool allowLater)
{
    CBeforeHuntDialog* dialog = new CBeforeHuntDialog();
    CBH_Player* player = CBH_Player::GetInstance();
    if (player->m_missionBriefImage != NULL) {
        dialog->SetExtraImageFromImageID(player->m_missionBriefImage);
    } else if (player->m_field208 != 0) {
        dialog->SetExtraImage(reinterpret_cast<ICRenderSurface*>(player->m_field208));
    }
    dialog->SetTitleText(player->m_missionBriefTitle);
    dialog->SetMessageText(player->m_missionBriefText);
    CDialogWindow::EDialogButton later = CDialogWindow::BUTTON_NONE;
    if (allowLater) {
        later = CDialogWindow::BUTTON_LATER;
    }
    dialog->SetButtons(CDialogWindow::BUTTON_OK, later);
    dialog->SetButtonsCommands(AcceptMissionDialog, CancelMissionDialog, -1);
    int experience = player->m_field21c;
    int energy = player->m_missionBriefEnergy;
    int money = player->m_field220;
    unsigned char hasBonus = *reinterpret_cast<const unsigned char*>(&player->m_field238);
    eGameFont rewardFont = GAME_FONT_DEFAULT;
    if ((hasBonus & 1) != 0) {
        rewardFont = static_cast<eGameFont>(17);
    }
    dialog->AddDownTextWithIcon(XString("+") + XString(money), "IDB_HUD_ICON_MONEY", false, true, rewardFont);
    dialog->AddDownTextWithIcon(XString("+") + XString(experience), "IDB_HUD_ICON_EXP", false, true, rewardFont);
    if (CBH_Player::GetInstance()->IsEnegyEnabled()) {
        dialog->AddDownTextWithIcon(L"   " + ResString("IDS_DIALOG_COST") + L":", NULL, false, false, GAME_FONT_DEFAULT);
        dialog->AddDownTextWithIcon(XString(energy), "IDB_HUD_ICON_ENERGY", false, false, GAME_FONT_DEFAULT);
    }
    return dialog;
}



void CBH_GPSMap::ShowBeforeFriendsGatedMission()
{
    int missionId = CBH_Player::GetInstance()->GetCurrentFriendGatedMissionID();
    m_friendGatedMission = static_cast<App*>(WindowApp::m_instance)->m_friendMissionManager->GetMissionById(missionId);
    CBH_PersonTypeManager* people = static_cast<App*>(WindowApp::m_instance)->InitPersonTypeManager();
    const CFactionType* faction = static_cast<App*>(WindowApp::m_instance)->InitFactionTypeManager()->GetRandomFaction();
    m_friendGatedBoss = people->GetType(faction->GetBoss());
    const CVisualCharacterType* visual = m_friendGatedBoss->m_visualCharacter;
    m_friendGatedBossName.Assign(m_friendGatedBoss->m_nationality->GetRandomFullName(visual->m_isMan));
    WindowApp::HandleTunnelCommand(0x62b2a2ce, 0, 0, 0);

    const CBountyManager::SBountyGeneralInfo* bounty =
        static_cast<App*>(WindowApp::m_instance)->m_bountyManager->GetInfo(m_friendGatedMission->m_bountyType);
    const char* image = visual->m_screenImage;
    CBH_Player::GetInstance()->m_missionBriefImage = image;
    ICRenderSurface* surface = static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->CreateSurface(image, true);
    CBH_Player::GetInstance()->m_field208 = reinterpret_cast<int>(surface);
    CBH_Player::GetInstance()->m_missionBriefEnergy = bounty->m_energy;
    CBH_Player::GetInstance()->m_field21c = m_friendGatedMission->m_rewardXp;
    CBH_Player::GetInstance()->m_field220 = m_friendGatedMission->m_rewardMoney;

    const CBountyManager::SMessage* message = static_cast<App*>(WindowApp::m_instance)->m_bountyManager->GetRandomTargettingMessage();
    XString brief = ResString("IDS_DIALOG_NAME") + XString(": ") + m_friendGatedBossName;
    brief.Append(XString("\n"));
    brief.Append(ResString("IDS_DIALOG_GOAL") + XString(": ") + message->m_text);
    CBH_Player::GetInstance()->m_missionBriefText.Assign(brief);
    CBH_Player::GetInstance()->m_missionBriefTitle.Assign(message->m_title);
    m_parent->AddModal(CreateBeforeHuntDialog(true));
    m_pendingCommand = PendingFriendGatedMission;
}

void CBH_GPSMap::StartNextFriendGatedMission()
{
    if (m_friendGatedMission == NULL || m_friendGatedBoss == NULL) {
        return;
    }
    const CBountyManager::SBountyGeneralInfo* bounty =
        static_cast<App*>(WindowApp::m_instance)->m_bountyManager->GetInfo(m_friendGatedMission->m_bountyType);
    CHuntingInfo::SAssignedData data;
    CGPSHauntCamera* camera = GetRandomCamera(false, false);
    if (camera == NULL) {
        return;
    }
    const CLocation* location = static_cast<App*>(WindowApp::m_instance)->m_locationManager->GetLocation(camera->m_location);
    if (location == NULL) {
        return;
    }

    unsigned int seed = CRandGen::GetInstance()->GetRand(0xfffffff);
    int cameraId = camera->m_id;
    int bountyType = m_friendGatedMission->m_bountyType;
    int level = CBH_Player::GetInstance()->GetLevel();
    XString script = CBountyManager::GetRandomScriptWithSeed(*bounty, bountyType, location->m_name,
                                                           cameraId, level, seed);
    data.m_bossType = m_friendGatedBoss;
    data.m_bossName.Assign(m_friendGatedBossName);
    data.m_squadType = static_cast<App*>(WindowApp::m_instance)->InitSquadTypeManager()->GetType(m_friendGatedBoss->m_name);
    if (script.Length() != 0) {
        data.m_scriptType = static_cast<App*>(WindowApp::m_instance)->m_scriptManager->GetType(script);
    }
    CProgressSystem* progress = static_cast<App*>(WindowApp::m_instance)->InitProgressSystem();
    data.m_civilianCount = progress->GetCivilianCount(CBH_Player::GetInstance()->GetLevel());
    // The original assigns the two rewards to the opposite gameplay fields.
    data.m_experienceReward = m_friendGatedMission->m_rewardMoney;
    data.m_moneyReward = m_friendGatedMission->m_rewardXp;
    data.m_locationName.Assign(location->m_name);
    data.m_ambientSound = camera->m_ambientSound;
    data.m_missionId = -1;
    data.m_cameraHorizontalLimit = camera->m_horizontalLimit;
    data.m_cameraVerticalLimit = camera->m_verticalLimit;
    data.m_hasTarget = false;
    data.m_friendGated = true;
    data.m_alertSeconds = bounty->m_alertSeconds;
    data.m_escapeSeconds = bounty->m_escapeSeconds;
    data.m_tripleMultiplierSeconds = static_cast<App*>(WindowApp::m_instance)->m_helper->m_config.GetIntegerValueByToken(
        "TIME_XP_MULTI_X3_SEC", true);
    data.m_doubleMultiplierSeconds = static_cast<App*>(WindowApp::m_instance)->m_helper->m_config.GetIntegerValueByToken(
        "TIME_XP_MULTI_X2_SEC", true);
    data.m_ambush = false;
    data.m_tranquillize = bounty->m_tranquillize;
    data.m_multiplierCountdown = true;
    data.m_bossFirst = bounty->m_bossFirst;
    data.m_bossWins = bounty->m_bossWins;
    data.m_bossSpawnsLast = bounty->m_bossSpawnsLast;
    data.m_silentKill = bounty->m_silentKill;

    // Borrow the camera's location and limits. This contract does not occupy the camera.
    static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->SetHuntingData(data);
    CBH_Player::GetInstance()->ReduceEnergy(bounty->m_energy);
    ++CBH_Player::GetInstance()->m_fieldb4;
    CGameAnalytics::logMissionStarted(data.m_missionName);
    m_friendGatedMission = NULL;
    m_friendGatedBoss = NULL;
    m_parent->SetInterrupt(1);
}

CBeforeTargettingDialog* CBH_GPSMap::CreateBeforeTargettingDialog(bool allowLater)
{
    CBH_Player* player = CBH_Player::GetInstance();
    int friendId = player->m_targetFriendId;
    CStrWChar facebookId(player->m_missionBriefTargetFB);
    CBeforeTargettingDialog* dialog;
    if (friendId >= 0) {
        dialog = new CBeforeTargettingDialog(friendId);
    } else {
        dialog = new CBeforeTargettingDialog(facebookId);
    }
    if (player->m_missionBriefImage != NULL) {
        dialog->SetExtraImageFromImageID(player->m_missionBriefImage);
    }
    ICSystemFont* font = ICSystemFont::CreateInstance();
    font->Initialize(SScreenAdaptingConstant(12, 24)(), NULL, 1);
    dialog->SetTitleText(player->m_missionBriefTitle);
    dialog->SetMessageText(player->m_missionBriefText, font);
    CDialogWindow::EDialogButton later = CDialogWindow::BUTTON_NONE;
    if (allowLater) {
        later = CDialogWindow::BUTTON_LATER;
    }
    dialog->SetButtons(CDialogWindow::BUTTON_OK, later);
    dialog->SetButtonsCommands(AcceptMissionDialog, CancelMissionDialog, -1);

    int money = player->m_field220;
    int energy = player->m_missionBriefEnergy;
    int experience = player->m_field21c;
    unsigned char hasBonus = *reinterpret_cast<const unsigned char*>(&player->m_field238);
    eGameFont rewardFont = GAME_FONT_DEFAULT;
    if ((hasBonus & 1) != 0) {
        rewardFont = static_cast<eGameFont>(17);
    }
    dialog->AddDownTextWithIcon(XString("+") + XString(money), "IDB_HUD_ICON_MONEY", false, true, rewardFont);
    dialog->AddDownTextWithIcon(XString("+") + XString(experience), "IDB_HUD_ICON_EXP", false, true, rewardFont);
    if (CBH_Player::GetInstance()->IsEnegyEnabled()) {
        dialog->AddDownTextWithIcon(L"   " + ResString("IDS_DIALOG_COST") + L":", NULL, false, false, GAME_FONT_DEFAULT);
        dialog->AddDownTextWithIcon(XString(energy), "IDB_HUD_ICON_ENERGY", false, false, GAME_FONT_DEFAULT);
    }
    return dialog;
}

bool CBH_GPSMap::StartTargettingMission(int friendId, bool revenge)
{
    CGPSHauntCamera* camera = GetRandomCamera(true, false);
    if (camera != NULL) {
        m_selectedCamera = camera;
        camera->SetTargettingMissionOnCamera(m_informantType4, friendId, revenge, false);
        m_informantType4->OnBountyGiven(m_selectedCamera->m_id);
        OnCameraSelected(m_selectedCamera);
        m_selectedCamera->SetAttractSpeed(1.1f);
        return true;
    }
    ShowMessage(ResString("IDS_DIALOG_TITLE_WARNING"), ResString("IDS_MESSAGE_NO_FREE_CAMERAS"), NULL, NULL);
    return false;
}

void CBH_GPSMap::ShowSelectFriendDialog()
{
    CNGSRemoteUserList* friends = CNGS::GetInstance()->GetLocalUser()->m_remoteUserLists[0];
    if (friends->m_users.Size() == 0) {
        CTextDialog* dialog = new CTextDialog();
        dialog->SetMessageText(ResString("IDS_FRIENDS_NO_FRIENDS_TO_TARGET_TEXT"));
        dialog->SetTitleText(ResString("IDS_FRIENDS_NO_FRIENDS_TO_TARGET_TITLE"));
        dialog->SetButtons(CDialogWindow::BUTTON_OK);
        static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddModal(dialog);
        return;
    }

    CIncentivizedWindow* picker = new CIncentivizedWindow(ResString("IDS_FRIENDS_TARGET"));
    picker->SetSingleSelection();
    picker->SetTitleText(ResString("IDS_FRIENDS_TARGET_TITLE"));
    picker->SetCommand(ChooseTargetFriend);
    for (int index = 0; index < friends->m_users.Size(); ++index) {
        CNGSRemoteUser* user = friends->m_users[index];
        CStrWChar name = user->GetNickName();
        XString nickname(name.GetWCharPtr(), wcslen(name.GetWCharPtr()) * sizeof(wchar_t));
        picker->AddFriend(nickname, user->GetClientID());
    }
    AddModal(picker);
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
    // the original keeps the icon in a local only
    CGPSZombiesPromo* promo = new CGPSZombiesPromo();
    promo->SetPos(500 + MapWidthInset(), 350 + MapHeightInset());
    promo->m_listener = this;
    m_items.addElement(promo);
}

void CBH_GPSMap::CreateCooperationIcon()
{
    m_cooperation = new CGPSCooperation();
    // the original uses the height inset for both coordinates
    m_cooperation->SetPos(290 + MapHeightInset(), 270 + MapHeightInset());
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

// Original 0xe8394: in the tutorial the home icon is only created when it is visible.
void CBH_GPSMap::CreateNetwork()
{
    if (static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial() != true
        || static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsHomeVisible() == true) {
        m_network = new CGPSNetwork();
        m_network->SetPos(465, 285);
        m_network->SetListener(this);
        m_items.addElement(m_network);
    }
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



CGPSHauntCamera* CBH_GPSMap::GetCameraForBlueInformant()
{
    CGPSHauntCamera* camera = GetRandomCamera(false, false);
    if (camera == NULL) {
        camera = GetRandomCamera();
    }
    return camera;
}

CGPSHauntCamera* CBH_GPSMap::GetRandomCamera(bool unlocked, bool allowMoving)
{
    Vector<CGPSHauntCamera*> available;
    for (int index = 0; index < m_haunts.size(); ++index) {
        CGPSHaunt* haunt = m_haunts[index];
        for (int number = 0; number < haunt->m_cameras.size(); ++number) {
            if (!allowMoving && haunt->m_cameras[number]->m_durationMs > 0) {
                continue;
            }
            if (haunt->m_cameras[number]->IsAvailable() && unlocked) {
                available.addElement(haunt->m_cameras[number]);
            } else if (haunt->m_cameras[number]->IsLockedAvailable() && !unlocked) {
                available.addElement(haunt->m_cameras[number]);
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
    m_selectedInformant = informant;
    CDialogWindow* dialog = new CDialogWindow(true);
    dialog->SetTitleText(informant->m_name.Upper());
    XString text = informant->m_startText;
    int cashCost = informant->m_cashCost;
    int creditsCost = informant->m_creditsCost;
    if (cashCost > 0) {
        text.Append(XString("\n") + ResString("IDS_DIALOG_IT_WILL_COST")
                    + L" $" + XString(cashCost));
    }
    if (creditsCost > 0) {
        text.Append(XString("\n") + ResString("IDS_DIALOG_IT_WILL_COST")
                    + L" " + XString(creditsCost) + L" " + ResString("IDS_DIALOG_CREDITS") + L".");
    }
    dialog->SetMessageText(text);
    dialog->SetButtons(CDialogWindow::BUTTON_OK, CDialogWindow::BUTTON_IGNORE);
    dialog->SetButtonsCommands(AcceptMissionDialog, CancelMissionDialog, -1);
    m_parent->AddModal(dialog);
    m_pendingCommand = PendingBuyInformant;
}

void CBH_GPSMap::ShowInformantIsSearchingDialog(CGPSInstantInformant* informant)
{
    m_selectedInformant = informant;
    CDialogWindow* dialog = new CDialogWindow(true);
    dialog->SetTitleText(informant->m_name.Upper());
    XString text = informant->m_comeLaterText;
    int resetCost = informant->m_resetCost;
    text.Append(XString("\n") + ResString("IDS_DIALOG_CREDITS") + XString(":") + XString(resetCost));
    dialog->SetMessageText(text);
    dialog->SetButtons(CDialogWindow::BUTTON_BUY, CDialogWindow::BUTTON_IGNORE);
    dialog->SetButtonsCommands(LowMoneyGetCommand, AcceptMissionDialog, -1);
    m_parent->AddModal(dialog);
    m_pendingCommand = PendingRushInformant;
}

void CBH_GPSMap::OnGPSInstantInformantSelected(CGPSInstantInformant* informant)
{
    CGPSHauntCamera* camera;
    switch (informant->m_type) {
    case CGPSInstantInformant::Moving:
        camera = GetMovingRandomCamera();
        break;
    case CGPSInstantInformant::Blue:
        camera = GetCameraForBlueInformant();
        break;
    default:
        camera = GetRandomCamera(true, false);
        break;
    }

    if (!informant->m_bought) {
        if (CBH_Player::GetInstance()->GetTutorialState() == 11) {
            CGameAnalytics::logTutorialEvents(CGameAnalytics::TUTORIAL_EVENT_INFORMANT_PURCHASED);
        }
        ShowBuyInformantDialog(informant);
    } else if (informant->IsBountyAvailable()) {
        if (camera != NULL) {
            m_selectedCamera = camera;
            if (informant->m_type == CGPSInstantInformant::Targeting) {
                m_targetingInformant = informant;
                m_openTargetPickerAfterLogin = true;
                if (!static_cast<App*>(WindowApp::m_instance)->m_networkAvailability->IsAvailable()) {
                    static_cast<App*>(WindowApp::m_instance)->m_gserveHandler->ShowOfflineModeDialog();
                    return;
                }
                CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
                if (!user->isAuthenticated(SOCIAL_NETWORK_FACEBOOK)
                    || (!user->isAuthenticated(SOCIAL_NETWORK_GAME_CENTER)
                        && GameCenter_Facade::GetInstance()->isGameCenterSupported())) {
                    static_cast<App*>(WindowApp::m_instance)->StartLogin(App::LoginBothNetworks, true);
                    return;
                }
                ShowSelectFriendDialog();
                return;
            }
            if (informant->m_type == CGPSInstantInformant::Gated) {
                camera->SetGatedMissionOnCamera(informant, 0, false);
            } else {
                camera->SetBountyOnCamera(informant, false, false);
            }
            informant->OnBountyGiven(camera->m_id);
            const CGPSBounty* bounty = static_cast<const CGPSBounty*>(camera->GetGPSHuntingItem());
            ICSystemFont* font = NULL;
            if (CStrWChar(bounty->m_targetFBUser.c_str()).GetLength() > 0) {
                font = ICSystemFont::CreateInstance();
                font->Initialize(SScreenAdaptingConstant(12, 24)(), NULL, 1);
            }
            ShowBountyPriorDialogs(bounty, camera->m_haunt, font);
            static_cast<App*>(WindowApp::m_instance)->m_soundManager->PlaySound("IDM_MAP_INFO_POPUP", false, false);
        } else {
            ShowMessage(ResString("IDS_DIALOG_TITLE_WARNING"), ResString("IDS_MESSAGE_NO_FREE_CAMERAS"), NULL, NULL);
        }
    } else if (informant->m_cameraId < 0) {
        if (informant->IsSearching()) {
            ShowInformantIsSearchingDialog(informant);
        }
    } else {
        camera = GetCameraById(informant->m_cameraId);
        if (camera == NULL) {
            m_selectedCamera = NULL;
        } else {
            const CGPSHuntingItem* item = camera->GetGPSHuntingItem();
            if (item != NULL && item->GetType() != 1) {
                informant->OnBountyCanceled();
                return;
            }
            m_selectedCamera = camera;
            const CGPSBounty* bounty = static_cast<const CGPSBounty*>(camera->GetGPSHuntingItem());
            if (bounty == NULL) {
                informant->OnBountyCanceled();
                return;
            }
            ICSystemFont* font = NULL;
            bool hasTarget;
            {
                CStrWChar targetName(bounty->m_targetFBUser.c_str());
                hasTarget = targetName.GetLength() > 0 || bounty->m_targetFriendId >= 0;
            }
            if (hasTarget) {
                font = ICSystemFont::CreateInstance();
                font->Initialize(SScreenAdaptingConstant(12, 24)(), NULL, 1);
            }
            ShowBountyPriorDialogs(bounty, camera->m_haunt, font);
        }
    }

    Save();
    informant->SetAttractSpeed(0.2f);
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
    if (!static_cast<App*>(WindowApp::m_instance)->GetNetworkAvailability()->IsAvailable()) {
        static_cast<App*>(WindowApp::m_instance)->m_gserveHandler->ShowOfflineModeDialog();
        return;
    }
    CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
    if (!user->isAuthenticated(SOCIAL_NETWORK_FACEBOOK)
        || (!user->isAuthenticated(SOCIAL_NETWORK_GAME_CENTER)
            && GameCenter_Facade::GetInstance()->isGameCenterSupported())) {
        static_cast<App*>(WindowApp::m_instance)->StartLogin(App::LoginBothNetworks, true);
    } else {
        if (m_openFriendsWindow) {
            CFriendsWindow* window = new CFriendsWindow();
            window->SetTab(m_socialNetworkMode);
            AddModal(window);
        }
        m_openFriendsWindow = false;
    }
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
    if (CBH_Player::GetInstance()->GetTutorialState() == 7) {
        CGameAnalytics::logTutorialEvents(CGameAnalytics::TUTORIAL_EVENT_ENTERED_SAFEHOUSE);
    }
    CBH_NetworkWindow* network = CBH_NetworkWindow::GetInstance();
    network->BringToFrontModal(static_cast<App*>(WindowApp::m_instance)->m_mainLayer);
    WindowApp::HandleTunnelCommand(-0x4e79c923, 0, 0, 0);
}

void CBH_GPSMap::NeedSave()
{
    m_needSave = true;
}

void CBH_GPSMap::OnShopItemSelected(CGPSShop*)
{
    CGameAnalytics::logStoreEnter(CGameAnalytics::SHOP_ENTER_GPS_ITEM);
    CRegularShop* shop = CRegularShop::GetInstance(true);
    if (static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial() == 1) {
        shop->SetTab(static_cast<CShopWindow::ETab>(static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->GetCurrentShopTab()));
    }
    // the original adds the shop without checking its parent
    m_parent->AddModal(shop);
}

void CBH_GPSMap::OnBarItemSelected(CGPSBar*)
{
    if (HasModalChild()) {
        return;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (!app->GetNetworkAvailability()->IsAvailable()) {
        static_cast<App*>(WindowApp::m_instance)->m_gserveHandler->ShowOfflineModeDialog();
        return;
    }
    m_openFriendsWindow = true;
    m_socialNetworkMode = 1;
    OnGPSSocialNetworkSelected();
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
    if (!HasModalChild()) {
        ShowCoopWindow();
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
    // the original keeps m_movingMissionInformant here
    HideAllInformantsImpl(&m_missionInformants[0]);
    UnhideInformantImpl(&m_missionInformants[0]);
    for (int group = 1; group < 5; ++group) {
        HideAllInformantsImpl(&m_missionInformants[group]);
        if (!static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial()) {
            UnhideInformantImpl(&m_missionInformants[group]);
        }
    }
}

__attribute__((noinline)) void CBH_GPSMap::HideAllInformantsImpl(Vector<CGPSMissionInformant*>* informants)
{
    for (int index = 0; index < informants->size(); ++index) {
        (*informants)[index]->SetVisible(false);
    }
}

__attribute__((noinline)) void CBH_GPSMap::UnhideInformantImpl(Vector<CGPSMissionInformant*>* informants)
{
    CGPSMissionInformant* first = NULL;
    int firstMissionId = INT_MAX;
    for (int index = 0; index < informants->size(); ++index) {
        CGPSMissionInformant* informant = (*informants)[index];
        if (informant->IsAvailable() && informant->m_missionId < firstMissionId) {
            first = informant;
            firstMissionId = informant->m_missionId;
        }
    }
    if (first != NULL) {
        first->SetVisible(true);
        const Vector<CGPSMissionInformant*>& movingInformants = m_missionInformants[3];
        for (int index = 0; index < movingInformants.size(); ++index) {
            if (first == movingInformants[index]) {
                m_movingMissionInformant = first;
            }
        }
    }
}

void CBH_GPSMap::OnGpsMissionInformantSelected(CGPSMissionInformant* informant)
{
    if (informant == NULL) {
        return;
    }
    if (CBH_Player::GetInstance()->GetTutorialState() == 3) {
        CGameAnalytics::logTutorialEvents(CGameAnalytics::TUTORIAL_EVENT_STORY_INFORMANT_SELECTED);
    }
    if (!MakePlayerWeaponCheck(informant->m_missionId)) {
        return;
    }
    CGPSHauntCamera* camera = GetCameraById(informant->m_cameraId);
    if (camera->HasBlueBounty()) {
        ShowMessage(ResString("IDS_DIALOG_TITLE_WARNING"),
                    ResString("IDS_MESSAGE_CAN_NOT_TAKE_RED_MISSION_BECAUSE_OF_BLUE"), NULL, NULL);
        camera->SetAttractSpeed(1.1f);
        return;
    }
    if (!camera->HasMission()) {
        if (camera->GetGPSHuntingItem() != NULL) {
            ShowMessage(ResString("IDS_DIALOG_TITLE_WARNING"),
                        ResString("IDS_MESSAGE_CAN_NOT_TAKE_RED_MISSION_BECAUSE_OF_BLUE"), NULL, NULL);
            camera->SetAttractSpeed(1.1f);
            return;
        }
        ShowMissionStartDialogs(informant->m_missionId);
        camera->SetMissionOnCamera(informant->m_missionId, informant);
        WindowApp::HandleTunnelCommand(-0x2051ee21, 0, 0, 0);
        Save();
    } else {
        ShowMissionStartDialogs(informant->m_missionId);
    }
    m_selectedCamera = camera;
    informant->SetAttractSpeed(0.2f);
}

// Original 0xefefc: up to 5 rounds (doubling the radius) of 1000 random spots on a circle around
// the camera; at the 999th spot the ring is scanned every 2 degrees (shrinking the radius by 2)
// and then the whole region pixel by pixel. Without a free spot the last tried one is used.
void CBH_GPSMap::PlaceSubContract(CGPSSubContract* contract, unsigned int radius)
{
    CGPSHauntCamera* camera = contract->GetHauntCamera();
    int cameraWidth = camera->GetWidth();
    int cameraX = camera->GetX();
    int cameraY = camera->GetY();
    int cameraHeight = camera->GetHeight();
    int width = contract->GetWidth();
    int height = contract->GetHeight();
    CRandGen* random = CRandGen::GetInstance();
    unsigned int searchRadius = radius;
    if (searchRadius < static_cast<unsigned int>(width * 3)) {
        searchRadius = width * 3;
    }
    int centerY = cameraY + cameraHeight / 2;
    int halfWidth = width / 2;
    int halfCameraWidth = cameraWidth / 2;
    int centerX = cameraX + halfCameraWidth;
    int halfHeight = height / 2;
    int x = cameraX;
    int y = centerY;
    for (int round = 5; round != 0; --round) {
        int mapWidth = m_mapWidth;
        int top = SScreenAdaptingConstant(40, 80)() + MapHeightInset();
        int mapHeight = m_mapHeight;
        int bottom = SScreenAdaptingConstant(mapHeight - SubContractBottomInset())();
        Vector<const CGPSItem*> items;
        int attempt = 0;
        items.addElement(camera);
        int maxY = halfHeight + centerY + searchRadius;
        if (mapHeight - height < maxY) {
            maxY = mapHeight - height;
        }
        if (bottom - height < maxY) {
            maxY = bottom - height;
        }
        int maxX = halfWidth + centerX + searchRadius;
        if (mapWidth - width < maxX) {
            maxX = mapWidth - width;
        }
        int minY = centerY - searchRadius - halfHeight;
        if (minY < top) {
            minY = top;
        }
        int minX = centerX - searchRadius - halfWidth;
        if (minX < 0) {
            minX = 0;
        }
        FilterRegionItems(minX, minY, maxX, maxY, items);
        int scanTop = cameraY + cameraHeight / 2 - halfHeight - searchRadius;
        if (scanTop < top) {
            scanTop = top;
        }
        int scanLeft = halfCameraWidth + cameraX - halfWidth - searchRadius;
        if (scanLeft < 1) {
            scanLeft = 0;
        }
        bool found = false;
        for (; attempt < 1000; ++attempt) {
            float distance = random->GetRandRange(halfWidth + halfCameraWidth, searchRadius);
            float angle = random->GetRand(360);
            x = ClampMapCoordinate(static_cast<int>(MathLib::Sin(angle) * distance) + (centerX - halfWidth), minX, maxX);
            y = ClampMapCoordinate(static_cast<int>(MathLib::Cos(angle) * distance) + (centerY - halfHeight), minY, maxY);
            if (IsRegionFree(x, y, x + width, y + height, items)) {
                found = true;
                break;
            }
            if (attempt == 998) {
                for (unsigned int ring = searchRadius; halfWidth + halfCameraWidth < static_cast<int>(ring); ring -= 2) {
                    for (int degrees = 0; degrees < 360; degrees += 2) {
                        x = ClampMapCoordinate(static_cast<int>(MathLib::Sin(degrees) * ring) + (centerX - halfWidth), minX, maxX);
                        y = ClampMapCoordinate(static_cast<int>(MathLib::Cos(degrees) * ring) + (centerY - halfHeight), minY, maxY);
                        if (IsRegionFree(x, y, x + width, y + height, items)) {
                            found = true;
                            break;
                        }
                    }
                    if (found) {
                        break;
                    }
                }
                if (found) {
                    break;
                }
                for (x = scanLeft; x < maxX; ++x) {
                    for (y = scanTop; y < maxY; ++y) {
                        if (IsRegionFree(x, y, width + x, height + y, items)) {
                            found = true;
                            break;
                        }
                    }
                    if (found) {
                        break;
                    }
                }
                if (found) {
                    break;
                }
            }
        }
        if (found) {
            break;
        }
        searchRadius = searchRadius << 1;
    }
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
    for (unsigned int index = 0; index < static_cast<unsigned int>(item.m_subItems.size()); ++index) {
        const CRectangle& rect = item.m_subItems[index];
        if (rect.m_x <= right && rect.m_y <= bottom
            && rect.m_x + rect.m_width >= left && rect.m_y + rect.m_height >= top) {
            return true;
        }
    }
    return false;
}

__attribute__((noinline)) bool CBH_GPSMap::IsRegionFree(int left, int top, int right, int bottom,
                             const Vector<const CGPSItem*>& items)
{
    for (unsigned int index = 0; index < static_cast<unsigned int>(items.size()); ++index) {
        if (IsItemIntersects(*items[index], left, top, right, bottom)) {
            return false;
        }
    }
    return true;
}

// Original 0xf02e0: every map item, then every haunt camera and its sub contract.
void CBH_GPSMap::FilterRegionItems(int left, int top, int right, int bottom,
                                  Vector<const CGPSItem*>& items) const
{
    for (unsigned int index = 0; index < static_cast<unsigned int>(m_items.size()); ++index) {
        const CGPSItem* item = m_items[index];
        if (IsItemIntersects(*item, left, top, right, bottom) == true) {
            items.addElement(item);
        }
    }
    for (unsigned int haunt = 0; haunt < static_cast<unsigned int>(m_haunts.size()); ++haunt) {
        const Vector<CGPSHauntCamera*>& cameras = m_haunts[haunt]->m_cameras;
        for (unsigned int index = 0; index < static_cast<unsigned int>(cameras.size()); ++index) {
            const CGPSHauntCamera* camera = cameras[index];
            if (IsItemIntersects(*camera, left, top, right, bottom) == true) {
                items.addElement(camera);
            }
            const CGPSSubContract* contract = camera->m_subContract;
            if (contract != NULL && IsItemIntersects(*contract, left, top, right, bottom) == true) {
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

__attribute__((noinline)) void CBH_GPSMap::ShowLowMoneyDialog()
{
    m_pendingCommand = PendingLowMoneyWarning;
    CDialogWindow* dialog = new CDialogWindow(true);
    dialog->SetButtons(CDialogWindow::BUTTON_BUY, CDialogWindow::BUTTON_GET, CDialogWindow::BUTTON_IGNORE);
    dialog->SetMessageText(ResString("IDS_MENU_LOW_MONEY_DIALOG"), NULL);
    dialog->SetTitleText("IDS_DIALOG_TITLE_WARNING");
    dialog->SetButtonsCommands(AcceptMissionDialog, LowMoneyGetCommand, CancelMissionDialog);
    m_parent->AddModal(dialog);
}

__attribute__((noinline)) void CBH_GPSMap::ShowCoopWindow()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (!app->GetNetworkAvailability()->IsAvailable()) {
        static_cast<App*>(WindowApp::m_instance)->m_gserveHandler->ShowOfflineModeDialog();
        return;
    }
    m_openFriendsWindow = true;
    m_socialNetworkMode = 2;
    OnGPSSocialNetworkSelected();
}

void CBH_GPSMap::FillBeforeTargetHuntInfo(const CGPSBounty* bounty)
{
    // The target dialog owns its avatar; the briefing holds text and reward values.
    int friendId = bounty->m_targetFriendId;
    CBH_Player::GetInstance()->m_missionBriefImage = "IDB_DIALOG_NOTEBOOK";
    ICRenderSurface* avatar =
        static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->CreateSurface("IDB_AVATAR_01", true);
    CBH_Player::GetInstance()->m_field208 = reinterpret_cast<int>(avatar);
    int energy = bounty->m_energyCost;
    CBH_Player::GetInstance()->m_missionBriefEnergy = energy;
    int experience = bounty->m_experienceReward;
    CBH_Player::GetInstance()->m_field21c = experience;
    int money = bounty->m_moneyReward;
    CBH_Player::GetInstance()->m_field220 = money;
    CBH_Player::GetInstance()->m_targetFriendId = friendId;
    CBH_Player::GetInstance()->m_missionBriefTargetFB = "";

    XString name;
    name.Assign(bounty->m_bossName);
    XString goal = bounty->GetObjectiveGoal().Replace(
        L"[LocationName]", GetHauntByName(m_selectedCamera->m_haunt)->m_screenName.c_str(), true);
    const CBountyManager::SMessage* message = static_cast<App*>(WindowApp::m_instance)->m_bountyManager->GetRandomTargettingMessage();
    XString brief = ResString("IDS_DIALOG_NAME") + XString(": ") + name;
    brief.Append(XString("\n"));
    brief.Append(message->m_text + XString(""));
    CBH_Player::GetInstance()->m_missionBriefText.Assign(brief);
    XString title = message->m_title;
    CBH_Player::GetInstance()->m_missionBriefTitle.Assign(title);
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
    const char* image;
    if (CStrWChar(item->m_targetFBUser.c_str()).GetLength() < 1 && item->m_targetFriendId < 0) {
        // Boss portrait when the target is not a friend (the original checks nothing on the way).
        image = item->GetObjectiveBossType()->m_visualCharacter->m_screenImage;
    } else {
        image = "IDB_DIALOG_NOTEBOOK";
    }
    FillBeforeHuntInfo(item, image, NULL);
}

void CBH_GPSMap::FillBeforeHuntInfo(const CGPSHuntingItem* item, const char* image, ICRenderSurface* surface)
{
    CBH_Player::GetInstance()->m_missionBriefImage = image;
    CBH_Player::GetInstance()->m_field208 = reinterpret_cast<int>(surface);
    CBH_Player::GetInstance()->m_missionBriefTitle.Assign(item->GetObjectiveTitle());
    CBH_Player::GetInstance()->m_missionBriefEnergy = item->m_energyCost;
    CBH_Player::GetInstance()->m_field21c = item->m_experienceReward;
    CBH_Player::GetInstance()->m_field220 = item->m_moneyReward;
    CBH_Player::GetInstance()->m_targetFriendId = -1;
    CBH_Player::GetInstance()->m_missionBriefTargetFB = CStrWChar(item->m_targetFBUser.c_str());
    *reinterpret_cast<unsigned char*>(&CBH_Player::GetInstance()->m_field238) = item->m_hasBonus;

    XString goal;
    if (item->GetType() == 0) {
        goal.Assign(item->GetObjectiveGoal());
    } else {
        XString objective = item->GetObjectiveGoal();
        CGPSHaunt* haunt = GetHauntByName(m_selectedCamera->m_haunt);
        goal.Assign(objective.Replace(L"[LocationName]", haunt->m_screenName.c_str(), true));
    }
    XString brief = ResString("IDS_DIALOG_NAME") + XString(": ") + item->GetObjectiveBossScreenName();
    brief.Append(XString("\n"));
    brief.Append(ResString("IDS_DIALOG_GOAL") + XString(": ") + goal + XString(""));
    CBH_Player::GetInstance()->m_missionBriefText.Assign(brief);
}

void CBH_GPSMap::ShowMissionPriorDialogs(int missionId)
{
    CDialogWindow* queued;
    m_pendingCommand = PendingMissionPrior;
    const CMission* mission = static_cast<App*>(WindowApp::m_instance)->m_missionManager->GetMissionById(missionId);
    static_cast<App*>(WindowApp::m_instance)->m_soundManager->PlaySound("IDM_MENU_WINDOW_OPEN_CLOSE", false, false);
    for (int index = 0; index < mission->m_priorDialogs.size(); ++index) {
        CMission::Dialog entry = mission->m_priorDialogs[index];
        CDialogWindow* dialog = new CDialogWindow(true);
        dialog->SetExtraImageFromImageID(entry.m_image);
        dialog->SetTitleText(mission->m_screenName.Upper());
        CBH_PersonType* boss = static_cast<App*>(WindowApp::m_instance)->InitPersonTypeManager()->GetType(mission->m_boss);
        bool isMan = boss->m_visualCharacter->m_isMan;
        XString text = CGPSHuntingItem::ParametrizeTextWithParams(entry.m_text, mission->m_bossName, isMan);
        dialog->SetMessageText(text, NULL);
        dialog->SetButtons(CDialogWindow::BUTTON_OK, CDialogWindow::BUTTON_NONE, CDialogWindow::BUTTON_NONE);
        dialog->SetButtonsCommands(AcceptMissionDialog, -1, -1);
        queued = dialog;
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
    Window* dialog;
    if (CBH_Player::GetInstance()->m_missionBriefTargetFB.GetLength() <= 0) {
        dialog = CreateBeforeHuntDialog(true);
    } else {
        dialog = CreateBeforeTargettingDialog(true);
    }
    CGPSSubContract* contract = NULL;
    if (m_selectedCamera != NULL) {
        contract = m_selectedCamera->m_subContract;
    }
    if (contract != NULL) {
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
    m_pendingCommand = PendingHuntConfirmation;
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
    CDialogWindow* queued;
    if (m_dialogsQueue.size() != 0) {
        return;
    }
    static_cast<App*>(WindowApp::m_instance)->m_soundManager->PlaySound("IDM_MENU_WINDOW_OPEN_CLOSE", false, false);
    const CMission* mission = static_cast<App*>(WindowApp::m_instance)->m_missionManager->GetMissionById(missionId);
    for (int index = 0; index < mission->m_startDialogs.size(); ++index) {
        CMission::Dialog entry = mission->m_startDialogs[index];
        CDialogWindow* dialog = new CDialogWindow(true);
        dialog->SetExtraImageFromImageID(entry.m_image);
        dialog->SetTitleText(mission->m_screenName.Upper());
        CBH_PersonType* boss = static_cast<App*>(WindowApp::m_instance)->InitPersonTypeManager()->GetType(mission->m_boss);
        bool isMan = boss->m_visualCharacter->m_isMan;
        XString text = CGPSHuntingItem::ParametrizeTextWithParams(entry.m_text, mission->m_bossName, isMan);
        dialog->SetMessageText(text, NULL);
        dialog->SetButtons(CDialogWindow::BUTTON_OK, CDialogWindow::BUTTON_NONE, CDialogWindow::BUTTON_NONE);
        dialog->SetButtonsCommands(AcceptMissionDialog, -1, -1);
        queued = dialog;
        m_dialogsQueue.insertElementAt(queued, 0);
    }
    m_pendingCommand = PendingStoryIntro;
    ShowNextQueueDialog(true);
}

// Original 0xe8e34: the map element is cleared and every item writes a fresh element.
void CBH_GPSMap::Save()
{
    m_needSave = false;
    TiXmlElement* map = CXmlHelper::ObtainElement(
        static_cast<App*>(WindowApp::m_instance)->m_saveManager->GetRootElement(), XString("gps_map"));
    map->Clear();
    map->SetAttribute("show_featured_install", m_showFeaturedInstall);
    map->SetAttribute("FI_minimizes_count", m_backgroundCount);
    map->SetAttribute("currentChallengeDay", m_currentChallengeDay);
    map->SetAttribute("needToShowRocketLauncherIntro", m_showRocketLauncherIntro);
    for (int index = 0; index < m_items.size(); ++index) {
        m_items[index]->Save(map);
    }
    static_cast<App*>(WindowApp::m_instance)->m_saveManager->SaveOnNextUpdate(false);
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
        if (informant == NULL) {
            return false;
        }
        const int missionId = informant->m_missionId;
        if (!CBH_Player::GetInstance()->IsMissionCompleted(missionId)) {
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
    static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddModal(dialog);
    return true;
}

void CBH_GPSMap::StartTutorialMission()
{
    // the original resets the player first, does not check the camera and saves afterwards
    CBH_Player::GetInstance()->SetPlayerStartParams();
    CGPSHauntCamera* camera = GetCameraById(4);
    if (!camera->HasHuntingItem()) {
        camera->SetMissionOnCamera(1);
    }
    StartHunt(camera, NULL, true);
    Save();
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
    CGPSHuntingItem* item = camera->GetGPSHuntingItem();
    CHuntingInfo::SAssignedData data;
    if (item == NULL) {
        return;
    }
    CGPSBounty* ambush = NULL;
    if (camera->HasTempMission()) {
        item = camera->m_temporaryItem;
    } else if (!item->m_weaponGated) {
        int firstLevel = static_cast<App*>(WindowApp::m_instance)->m_helper->m_config.GetIntegerValueByToken(
            "LEVEL_FROM_AMBUSHES_CAN_APPEAR", true);
        if (CBH_Player::GetInstance()->GetLevel() >= firstLevel && camera->GetGPSHuntingItem()->GetType() == 1) {
            float probability = static_cast<App*>(WindowApp::m_instance)->m_helper->m_config.GetFloatValueByToken(
                "AMBUSH_APPEAR_PROB", true);
            if (CBH_Math::GetProbabiliyOutcome(static_cast<int>(probability * 100.0f))) {
                ambush = new CGPSBounty(
                    camera, static_cast<App*>(WindowApp::m_instance)->m_bountyManager->GetRandomAmbushTypeId(), NULL);
                XString message = ambush->GetBountyMessage();
                CGPSHaunt* haunt = GetHauntByName(camera->m_haunt);
                data.m_field6c.Assign(message.Replace(L"[LocationName]", haunt->m_screenName.c_str(), true));
                ambush->SetTargetFriendId(item->m_targetFriendId);
                ambush->SetTargetFBUser(CStrWChar(item->m_targetFBUser.c_str()));
                ambush->m_revenge = item->m_revenge;
                item = ambush;
            }
        }
    }

    if (item != NULL) {
        int estimatedWeaponId = item->GetEstimatedWeaponID();
        int estimatedAmmo = item->GetEstimatedAmmonCount();
        CDH_Weapon* estimatedWeapon = static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->GetWeaponById(estimatedWeaponId);
        CBH_Player::GetInstance()->SetEstimatedWeapon(estimatedWeapon, estimatedAmmo);
        if (camera != NULL && item->m_weaponGated) {
            data.m_requiredWeaponId = item->m_gatedWeapon;
            data.m_weaponGated = data.m_requiredWeaponId < 0;
            if (data.m_weaponGated) {
                SetWeaponWithCategoryInBothHands(camera->m_gatedWeaponCategory);
                data.m_weaponCategory = CDH_Weapon::GetWeaponCategoryType(camera->m_gatedWeaponCategory);
            } else {
                SetWeaponWithIDInBothHands(data.m_requiredWeaponId);
            }
        }
    }
    if (item->m_randomWeapon != -1) {
        data.m_randomWeaponId = item->m_randomWeapon;
        CDH_Weapon* weapon = static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->GetWeaponById(data.m_randomWeaponId);
        weapon->GetDefaultAmmo();
        CBH_Player::GetInstance()->SetEstimatedWeapon(weapon, weapon->m_clipSize * 20);
    }
    CBH_Player* player = CBH_Player::GetInstance();
    player->m_lastHuntItem = camera->m_id;
    player->Save();
    player = CBH_Player::GetInstance();
    player->m_lastHuntResult = 0;
    player->Save();
    int level = CBH_Player::GetInstance()->GetLevel();
    int missionAnalyticsId = item->GetAAId();
    data.m_missionName.Assign(XString::Format(L"%02d-%03d-%02d", level, missionAnalyticsId, camera->m_id));

    if (location == NULL) {
        location = static_cast<App*>(WindowApp::m_instance)->m_locationManager->GetLocation(camera->m_location);
    }
    data.m_locationName.Assign(location->m_name);
    data.m_ambientSound = camera->m_ambientSound;
    data.m_civilianCount = item->m_civilianCount;
    data.m_tripleMultiplierSeconds = static_cast<App*>(WindowApp::m_instance)->m_helper->m_config.GetIntegerValueByToken(
        "TIME_XP_MULTI_X3_SEC", true);
    data.m_doubleMultiplierSeconds = static_cast<App*>(WindowApp::m_instance)->m_helper->m_config.GetIntegerValueByToken(
        "TIME_XP_MULTI_X2_SEC", true);
    CBH_Player::GetInstance();
    data.m_field84 = 0;
    data.m_multiplierCountdown = ambush == NULL && item->m_warTimer;
    data.m_alertSeconds = item->m_alertSeconds;
    data.m_escapeSeconds = item->m_escapeSeconds;
    data.m_field9c = true;
    data.m_bossFirst = item->m_bossFirst;
    data.m_bossWins = item->m_bossWins;
    data.m_bossSpawnsLast = item->m_bossSpawnsLast;
    data.m_silentKill = item->m_silentKill;
    data.m_hasTarget = item->m_targetFriendId != -1;
    if (!data.m_hasTarget) {
        data.m_hasTarget = CStrWChar(item->m_targetFBUser.c_str()).GetLength() > 0;
    }
    data.m_revenge = item->m_revenge;
    data.m_cameraHorizontalLimit = camera->m_horizontalLimit;
    data.m_cameraVerticalLimit = camera->m_verticalLimit;
    data.m_introTimeMs = camera->m_introTimeMs;
    data.m_cameraDurationMs = camera->m_durationMs;
    data.m_cameraHUDModel.Assign(camera->m_hudModel);
    data.m_additionalMinionCount = camera->m_additionalMinionCount;
    if (useScript) {
        XString script = item->GetScriptName(camera);
        if (script.Length() != 0) {
            data.m_scriptType = static_cast<App*>(WindowApp::m_instance)->m_scriptManager->GetType(script);
        }
    }

    CSquadType* squad = static_cast<App*>(WindowApp::m_instance)->InitSquadTypeManager()->GetType(item->m_squadName);
    if (squad == NULL) {
        return;
    }
    CProgressSystem* progress = static_cast<App*>(WindowApp::m_instance)->InitProgressSystem();
    progress->SetMissionDifficulty(item->GenerateDiffRandomizer());
    data.m_bossType = item->m_boss;
    data.m_squadType = squad;
    data.m_bossName.Assign(item->m_bossName.c_str(), wcslen(item->m_bossName.c_str()) * sizeof(wchar_t));
    data.m_experienceReward = item->m_experienceReward;
    data.m_moneyReward = item->m_moneyReward;
    data.m_experiencePenalty = item->m_experiencePenalty;
    data.m_moneyPenalty = item->m_moneyPenalty;
    data.m_ambush = ambush != NULL;
    data.m_tranquillize = ambush == NULL && item->m_tranquillize;
    data.m_missionId = item->GetMissionId();
    data.m_field4c = 4;
    data.m_cameraIndex = camera->m_cameraIndex;
    data.m_informantUnlockOnLevelUp = WillInformantUnlockOnLevelUp();
    if (data.m_missionId >= 0) {
        int giftId = static_cast<App*>(WindowApp::m_instance)->m_missionManager->GetMissionById(data.m_missionId)->m_weaponGiftId;
        if (giftId >= 0) {
            DGHelper* helper = static_cast<App*>(WindowApp::m_instance)->m_helper;
            CDH_Weapon* gift = static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->GetWeaponById(giftId);
            helper->m_gamePlayManager.SetWeapon01(gift->m_id);
            static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->GetWeaponById(giftId)->setAvailable(true);
            CBH_Player::GetInstance()->SetAmmoForWeaponId(giftId, 2);
        }
    }
    data.m_hasFakeBoss = false;
    if (data.m_missionId >= 0) {
        const CMission* mission = static_cast<App*>(WindowApp::m_instance)->m_missionManager->GetMissionById(data.m_missionId);
        data.m_hasFakeBoss = mission->m_fakeBoss.Length() > 0;
    }
    static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->SetHuntingData(data);
    CBH_Player::GetInstance()->ReduceEnergy(item->m_energyCost);
    ++CBH_Player::GetInstance()->m_fieldb4;
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
    CDH_Weapon::EWeaponCategory category = CDH_Weapon::GetWeaponCategoryType(categoryName);
    CDH_Weapon* first = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon01(true);
    CDH_Weapon* second = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon02(true);
    if (first != NULL && second != NULL && first->m_category != second->m_category) {
        CDH_Weapon* weapons[2] = {first, second};
        CDH_Weapon* selected = NULL;
        for (int index = 0; index < 2; ++index) {
            if (weapons[index]->m_category == category) {
                selected = weapons[index];
            }
        }
        SetWeaponInBothHands(selected);
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
    if (static_cast<App*>(WindowApp::m_instance)->m_newsFeedManager->GetNewsCount(NEWS_GIFT) == 0) {
        return;
    }
    Vector<const SMessage*> messages;
    static_cast<App*>(WindowApp::m_instance)->m_newsFeedManager->GetNewsByType(NEWS_GIFT, messages);
    HashTable<HashTableKey<int>, int> giftCounts(8);
    HashTable<HashTableKey<int>, int> giftSenders(8);
    for (int index = 0; index < messages.size(); ++index) {
        const SGiftReceivedMessage* received = static_cast<const SGiftReceivedMessage*>(messages[index]);
        if (!received->m_claimed) {
            int itemIndex = received->m_itemIndex;
            if (static_cast<App*>(WindowApp::m_instance)->m_socialGiftManager->GetGift(itemIndex) != NULL) {
                static_cast<App*>(WindowApp::m_instance)->m_socialGiftManager->GiveGift(itemIndex);
                int zero = 0;
                HashTableKey<int> key(received->m_itemIndex);
                const int* count = giftCounts.Get(key);
                if (count == NULL) {
                    count = &zero;
                }
                int nextCount = *count + 1;
                giftCounts.Set(key, nextCount);
                giftSenders.Add(HashTableKey<int>(received->m_itemIndex), received->m_senderId);
                received->m_claimed = true;
                // The previous reconstruction committed the reward and its receipt together before acknowledging
                // the server message. The original acknowledges immediately after giving the reward.
                CNGS::GetInstance()->GetLocalUser()->m_notificationHandler->RemoveGameSpecificMessage(received->m_messageId);
            }
        }
    }
    if (giftCounts.m_count != 0) {
        XString title("Title");
        XString text("Text");
        XString nickname("Win32 user");
        XString name("trololol challenge");
        Vector<HashTableKey<int> > giftIds;
        giftCounts.ToKeys(giftIds);
        for (int index = 0; index < giftIds.size(); ++index) {
            HashTableKey<int> key = giftIds[index];
            SGift* gift = static_cast<App*>(WindowApp::m_instance)->m_socialGiftManager->GetGift(key.m_value);
            name.Assign(gift->GetItem()->m_name);
            int zero = 0;
            const int* sender = giftSenders.Get(key);
            if (sender == NULL) {
                sender = &zero;
            }
            CNGSRemoteUser* friendUser =
                CNGS::GetInstance()->GetLocalUser()->m_remoteUserLists[0]->findRemoteUserByID(*sender);
            if (friendUser == NULL) {
                return;
            }
            CStrWChar friendName = friendUser->GetNickName();
            nickname.Assign(friendName.GetWCharPtr(), wcslen(friendName.GetWCharPtr()) * sizeof(wchar_t));
            int count = *giftCounts.Get(key);
            if (count < 2) {
                title.Assign(ResString("IDS_GIFT_RECIEVED_TITLE"));
                text.Assign(XString::Format(ResString("IDS_GIFT_RECIEVED_TEXT").c_str(),
                                            name.c_str(), nickname.c_str()));
            } else {
                title.Assign(ResString("IDS_GIFT_RECIEVED_MULTIPLE_TITLE"));
                text.Assign(XString::Format(ResString("IDS_GIFT_RECIEVED_MULTIPLE_TEXT").c_str(),
                                            name.c_str(), nickname.c_str(), count - 1));
            }
            ICSystemFont* font = ICSystemFont::CreateInstance();
            font->Initialize(SScreenAdaptingConstant(12, 24)(), NULL, 1);
            CTextDialog* dialog = new CTextDialog();
            dialog->SetButtons(CDialogWindow::BUTTON_OK);
            dialog->SetTitleText(title);
            dialog->SetMessageText(text, font);
            m_startDialogs->Add(dialog);
        }
        m_startDialogs->Start();
    }
}

void CBH_GPSMap::CheckChallengeNews()
{
    Vector<ENewsType> challengeTypes;
    challengeTypes.addElement(NEWS_CHALLENGE_INVITE);
    challengeTypes.addElement(NEWS_CHALLENGE_COMPLETED);
    for (int typeIndex = 0; typeIndex < challengeTypes.size(); ++typeIndex) {
        ENewsType type = challengeTypes[typeIndex];
        if (static_cast<App*>(WindowApp::m_instance)->m_newsFeedManager->GetNewsCount(type) == 0) {
            continue;
        }
        Vector<const SMessage*> messages;
        static_cast<App*>(WindowApp::m_instance)->m_newsFeedManager->GetNewsByType(type, messages);
        HashTable<HashTableKey<int>, int> challengeCounts(8);
        HashTable<HashTableKey<int>, int> challengeSenders(8);
        for (int index = 0; index < messages.size(); ++index) {
            const SMissionInviteMessage* message = static_cast<const SMissionInviteMessage*>(messages[index]);
            if (message->m_accepted) {
                continue;
            }
            CChallenge* challenge = static_cast<App*>(WindowApp::m_instance)->m_challengeManager->GetChallengeById(message->m_missionId);
            if (challenge == NULL) {
                continue;
            }
            if (challenge->m_active) {
                HashTableKey<int> key(message->m_missionId);
                int zero = 0;
                const int* count = challengeCounts.Get(key);
                if (count == NULL) {
                    count = &zero;
                }
                int nextCount = *count + 1;
                challengeCounts.Set(key, nextCount);
                challengeSenders.Add(HashTableKey<int>(message->m_missionId), message->m_senderId);
                challenge->FriendInvited(message->m_senderId);
                if (message->m_type == NEWS_CHALLENGE_COMPLETED) {
                    challenge->FriendCompleteChallenge(message->m_senderId);
                }
            }
            message->m_accepted = true;
            // The previous reconstruction persisted the friend IDs and reward flags before acknowledging
            // the server messages. The original acknowledges each known challenge here.
            CNGS::GetInstance()->GetLocalUser()->m_notificationHandler->RemoveGameSpecificMessage(message->m_messageId);
        }
        if (challengeCounts.m_count != 0) {
            XString title("Title");
            XString text("Text");
            XString nickname("Win32 user");
            XString name("Unknown challenge");
            Vector<HashTableKey<int> > challengeIds;
            challengeCounts.ToKeys(challengeIds);
            for (int index = 0; index < challengeIds.size(); ++index) {
                HashTableKey<int> key = challengeIds[index];
                CChallenge* challenge = static_cast<App*>(WindowApp::m_instance)->m_challengeManager->GetChallengeById(key.m_value);
                name.Assign(challenge->m_name);
                int zero = 0;
                const int* firstSender = challengeSenders.Get(key);
                if (firstSender == NULL) {
                    firstSender = &zero;
                }
                int sender = *firstSender;
                CNGSRemoteUser* friendUser =
                    CNGS::GetInstance()->GetLocalUser()->m_remoteUserLists[0]->findRemoteUserByID(sender);
                if (friendUser == NULL) {
                    return;
                }
                CStrWChar friendName = friendUser->GetNickName();
                nickname.Assign(friendName.GetWCharPtr(), wcslen(friendName.GetWCharPtr()) * sizeof(wchar_t));

                const char* singleTitle = NULL;
                const char* singleText = NULL;
                const char* multipleTitle = NULL;
                const char* multipleText = NULL;
                if (type == NEWS_CHALLENGE_INVITE) {
                    singleTitle = "IDS_CHALLENGE_INVITE_DIALOG_TITLE";
                    singleText = "IDS_CHALLENGE_INVITE_DIALOG_TEXT";
                    multipleTitle = "IDS_CHALLENGE_MULTIPLE_INVITE_DIALOG_TITLE";
                    multipleText = "IDS_CHALLENGE_MULTIPLE_INVITE_DIALOG_TEXT";
                } else if (type == NEWS_CHALLENGE_COMPLETED) {
                    if (challenge->GetSoloReward()->m_awarded) {
                        singleTitle = "IDS_CHALLENGE_FRIEND_COMPLETED_POPUP_TITLE";
                        singleText = "IDS_CHALLENGE_FRIEND_COMPLETED_POPUP_MESSAGE";
                        multipleTitle = "IDS_CHALLENGE_COMPLETE_MULTIPLE_TITLE";
                        multipleText = "IDS_CHALLENGE_COMPLETE_MULTIPLE_TEXT";
                    } else {
                        singleTitle = "IDS_CHALLENGE_FRIEND_COMPLETED_NO_REWARD_POPUP_TITLE";
                        singleText = "IDS_CHALLENGE_FRIEND_COMPLETED_NO_REWARD_POPUP_MESSAGE";
                        multipleTitle = "IDS_CHALLENGE_COMPLETE_NO_REWARD_MULTIPLE_TITLE";
                        multipleText = "IDS_CHALLENGE_COMPLETE_NO_REWARD_MULTIPLE_TEXT";
                    }
                }

                int count = *challengeCounts.Get(key);
                if (count < 2) {
                    title.Assign(ResString(singleTitle));
                    XString reward = challenge->GetRewardString(challenge->GetRewardForFriend(sender));
                    text.Assign(XString::Format(ResString(singleText).c_str(), nickname.c_str(),
                                                name.c_str(), reward.c_str()));
                } else {
                    XString firstReward = challenge->GetRewardString(challenge->GetX1Reward());
                    XString secondReward = challenge->GetRewardString(challenge->GetX2Reward());
                    title.Assign(ResString(multipleTitle));
                    text.Assign(XString::Format(ResString(multipleText).c_str(), nickname.c_str(),
                                                count - 1, name.c_str(), firstReward.c_str(), secondReward.c_str()));
                }
                ICSystemFont* font = ICSystemFont::CreateInstance();
                font->Initialize(SScreenAdaptingConstant(12, 24)(), NULL, 1);
                CDialogWindow* dialog;
                if (type == NEWS_CHALLENGE_COMPLETED) {
                    dialog = new CChallengeCompletedDialog();
                } else {
                    dialog = new CTextDialog();
                }
                dialog->SetButtons(CDialogWindow::BUTTON_OK);
                dialog->SetTitleText(title);
                dialog->SetMessageText(text, font);
                dialog->SetSound("IDM_COOP_CHALLENGE");
                m_startDialogs->Add(dialog);
            }
        }
    }
    m_startDialogs->Start();
}

// Original 0xe8828: a new challenge day with at least three active challenges queues the
// challenge dialog (no modal / queue checks in the original).
void CBH_GPSMap::CheckForNewChallenges()
{
    if (static_cast<App*>(WindowApp::m_instance)->GetNetworkAvailability()->IsAvailable()) {
        int day = static_cast<App*>(WindowApp::m_instance)->m_challengeManager->GetCurrentChallengeDay();
        if (day != m_currentChallengeDay) {
            Vector<CChallenge*> challenges = static_cast<App*>(WindowApp::m_instance)->m_challengeManager->GetActiveChallenges();
            if (challenges.size() > 2) {
                CChallengesDialog* dialog = new CChallengesDialog();
                dialog->SetTitleText(ResString("IDS_CHALLENGE_NEW_CHALLENGES_TITLE"));
                XString text = XString::Format(ResString("IDS_CHALLENGE_LIST_TEXT").c_str(), challenges[0]->m_name.c_str(),
                                               challenges[1]->m_name.c_str(), challenges[2]->m_name.c_str());
                dialog->SetChallengeText(text);
                m_startDialogs->Add(dialog);
                m_startDialogs->Start();
                m_currentChallengeDay = day;
                Save();
            }
        }
    }
}

void CBH_GPSMap::Update()
{
    int elapsed = static_cast<App*>(WindowApp::m_instance)->m_updateFrameTime;
    MakeDailyDealCheck();
    CheckMapPosBounds();
    CheckGiftReceivedNews();
    CheckChallengeNews();
    for (int index = 0; index < m_items.size(); ++index) {
        m_items[index]->Update(elapsed);
    }

    // Original 0xe9f84: an item with an attract speed scrolls the map to it (scroll state 2).
    for (int index = 0; index < m_clickableItems.size(); ++index) {
        float speed = m_clickableItems[index]->AttractSpeed();
        if (speed > 0.0f) {
            CRectangle horizontal = m_clickableItems[index]->GetRect();
            m_scrollTargetX = static_cast<App*>(WindowApp::m_instance)->m_screenWidth / 2 - RectCenterX(horizontal);
            CRectangle vertical = m_clickableItems[index]->GetRect();
            m_scrollTargetY = static_cast<App*>(WindowApp::m_instance)->m_screenHeight / 2 - RectCenterY(vertical);
            float targetX = static_cast<App*>(WindowApp::m_instance)->m_screenWidth - m_mapWidth;
            if (!(m_scrollTargetX < targetX)) {
                targetX = m_scrollTargetX;
                if (targetX > 0.0f) {
                    targetX = 0.0f;
                }
            }
            m_scrollTargetX = targetX;
            float targetY = static_cast<App*>(WindowApp::m_instance)->m_screenHeight - m_mapHeight;
            if (!(m_scrollTargetY < targetY)) {
                targetY = m_scrollTargetY;
                if (targetY > 0.0f) {
                    targetY = 0.0f;
                }
            }
            m_scrollTargetY = targetY;
            float mapX = m_mapX;
            float mapY = m_mapY;
            m_scrollSpeedX = MathLib::Fabs((targetX - mapX) * speed);
            m_scrollSpeedY = MathLib::Fabs((targetY - mapY) * speed);
            m_scrollX = mapX;
            m_scrollY = mapY;
            m_clickableItems[index]->SetAttractSpeed(0.0f);
            SetScrollState(2);
        }
    }
    CTjManager::GetInstance()->CheckFeaturedAppCondition();
    CTjManager::GetInstance()->Update(elapsed);
    if (m_draggingMap) {
        // The drag speed becomes the fling velocity. The original only caps positive velocity at one pixel per millisecond.
        SetScrollState(1);
        float inverseTicks = 1.0f / (elapsed + 1);
        m_scrollVelocityX = (m_scrollX - m_previousScrollX) * inverseTicks;
        m_scrollVelocityY = (m_scrollY - m_previousScrollY) * inverseTicks;
        if (m_scrollVelocityX > 1.0f) {
            m_scrollVelocityX = 1.0f;
        }
        if (m_scrollVelocityY > 1.0f) {
            m_scrollVelocityY = 1.0f;
        }
    } else {
        UpdateScroll();
    }
    m_previousScrollX = m_scrollX;
    m_previousScrollY = m_scrollY;
    m_mapX = static_cast<int>(m_scrollX) & ~1;
    m_mapY = static_cast<int>(m_scrollY) & ~1;

    static_cast<App*>(WindowApp::m_instance)->InitAchievementManager()->Update();
    static_cast<App*>(WindowApp::m_instance)->m_challengeManager->Update();
    if ((m_flags & FLAG_FOCUSED) == 0 || m_pendingCommand != 0) {
        m_mapIdleElapsed = 0;
    } else {
        m_mapIdleElapsed += elapsed;
        bool isNewVersion = CBH_Player::GetInstance()->IsGameVersionNew();
        bool isNewPlayer = CBH_Player::GetInstance()->m_newPlayer;
        if (m_mapIdleElapsed > 200 && (isNewPlayer || isNewVersion)) {
            ShowWhatsNewDialog();
            return;
        }
        static_cast<App*>(WindowApp::m_instance)->InitAchievementManager()->Update();
        static_cast<App*>(WindowApp::m_instance)->m_challengeManager->Update();
        if (m_mapIdleElapsed >= 2000) {
            HandleBonuses(elapsed);
        }
        if (m_showRocketLauncherIntro
            && !static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial()
            && (CRegularShop::GetInstance(false) == NULL || CRegularShop::GetInstance(true)->m_parent == NULL)) {
            m_showRocketLauncherIntro = false;
            ShowRocketLauncherInroduce();
            m_needSave = true;
        }
    }
    if (m_backgroundCount > 0) {
        if (m_showFeaturedInstall) {
            CNGSLoginFlow::GetInstance()->IsRunning();
        }
        if (!CGameScene::IsGGNShowed()) {
            CPHInterface::startPublisherContentRequest("game_launch", false, false);
        }
        m_backgroundCount = 0;
    }
    if (m_needSave) {
        Save();
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

// Original 0xeabb4: tops the rocket ammo up to 3 (no player save here) and queues the intro.
void CBH_GPSMap::ShowRocketLauncherInroduce()
{
    CDH_Weapon* launcher = static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->GetWeaponById(RocketLauncherId);
    if (launcher != NULL) {
        launcher->setAvailable(true);
        const CAmmo* ammo = launcher->GetActiveAmmo();
        if (CBH_Player::GetInstance()->GetAvailableAmmo(ammo, true) < 3) {
            CBH_Player::GetInstance()->SetAmmoCount(ammo, 3);
        }
    }
    CIntroduceItemWindow* dialog = new CIntroduceItemWindow(
        static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->GetWeaponById(RocketLauncherId), XString(""), true, false);
    dialog->SetOnCloseCommand(AcceptMissionDialog);
    m_startDialogs->Add(dialog);
    m_startDialogs->Start();
}

void CBH_GPSMap::HandleBonuses(int elapsed)
{
    // the original asks for the player every time and does not restart the start dialogs here
    bool newDay = false;
    if (CBH_Player::GetInstance()->TryMakeFiveDaysBonus(&newDay)) {
        ShowFiveDaysBonusMessage();
        if (CBH_Player::GetInstance()->CanShowFiveDaysIncentiveBonusExpirationMessage()) {
            ShowFiveDaysIncentiveBonusExpirationMessage();
            CBH_Player::GetInstance()->RegisterFiveDaysIncentiveBonusExpirationMessageShown();
        }
    } else if (newDay) {
        m_bonusTeaserElapsed += elapsed;
        if (m_bonusTeaserElapsed > 60000) {
            m_bonusTeaserElapsed = 0;
            ShowFiveDaysBonusTeaser();
        }
    }
    if (CBH_Player::GetInstance()->TryMakeFiveDaysIncentiveBonus()) {
        ShowFiveDaysIncentivisedBonusMessage();
    }
}

void CBH_GPSMap::ShowFiveDaysBonusMessage()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    int day = app->m_fiveDaysBonus->GetDay();
    // the original shows the dialog at once on the parent (not through m_startDialogs)
    CFiveDaysBonusDialog* dialog = new CFiveDaysBonusDialog(day, CFiveDaysBonusDialog::DAILY_BONUS);
    m_parent->AddModal(dialog);
}

void CBH_GPSMap::ShowFiveDaysIncentiveBonusExpirationMessage()
{
    CTextDialog* dialog = new CTextDialog();
    dialog->SetTitleText("IDS_DIALOG_TITLE_WARNING");
    dialog->SetMessageText(ResString("IDS_DAILY_BONUS_EXPIRE"));
    dialog->SetButtons(CDialogWindow::BUTTON_OK);
    // the original shows it at once on the parent
    m_parent->AddModal(dialog);
}

void CBH_GPSMap::ShowFiveDaysBonusTeaser()
{
    // the original asks CBH_Player::GetInstance() for the test and for the update
    int day = App::GetCurrentDay();
    if (day <= CBH_Player::GetInstance()->m_lastFiveDaysBonusTeaserDay) {
        return;
    }
    CBH_Player::GetInstance()->m_lastFiveDaysBonusTeaserDay = day;
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
    CFiveDaysBonusDialog* dialog = new CFiveDaysBonusDialog(day, CFiveDaysBonusDialog::INCENTIVE_BONUS);
    m_parent->AddModal(dialog);
}

void CBH_GPSMap::ShowFiveDaysInventiveBonusRecomendedItem()
{
    int day = CBH_Player::GetInstance()->m_activeFiveDaysIncentiveBonusDay;
    switch (day) {
    case 1:
        ShowShopWithItem(static_cast<App*>(WindowApp::m_instance)->m_armorManager,
                         CShopWindow::TAB_OTHER, CItem::ITEM_TYPE_FIRST, 0);
        break;
    case 2:
        ShowShopWithItem(static_cast<App*>(WindowApp::m_instance)->m_ammoPackManager,
                         CShopWindow::TAB_AMMO, CItem::ITEM_TYPE_FIRST, 0);
        break;
    case 3:
        ShowShopWithItem(static_cast<App*>(WindowApp::m_instance)->m_goPillManager,
                         CShopWindow::TAB_OTHER, CItem::ITEM_TYPE_FIRST, 0);
        break;
    case 4:
        ShowShopWithItem(static_cast<App*>(WindowApp::m_instance)->m_ammoPackManager,
                         CShopWindow::TAB_AMMO, CItem::ITEM_TYPE_FIRST, 2);
        break;
    case 5:
        ShowShopWithItem(static_cast<App*>(WindowApp::m_instance)->m_medicantManager,
                         CShopWindow::TAB_OTHER, CItem::ITEM_TYPE_FIRST, 3);
        break;
    }
}

void CBH_GPSMap::MakeDailyDealCheck()
{
    // the original only skips the tutorial; ShowDailyDealWindow stores the display time
    if (static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial()) {
        return;
    }
    if (CStdUtil_Cocoa::GetTimeSeconds() - m_lastDailyDealDisplay > 1200
        || (CBH_Player::GetInstance()->m_completedMissionCount == 1 && !m_dailyDealShown)) {
        m_dailyDealShown = true;
        ShowDailyDealWindow();
    }
}

void CBH_GPSMap::ShowDailyDealWindow()
{
    // the original does not check the item level and remembers when the deal was shown
    if (static_cast<App*>(WindowApp::m_instance)->InitDailyDealManager()->GetCurrentDailyDeal() == NULL) {
        return;
    }
    unsigned int now = CStdUtil_Cocoa::GetTimeSeconds();
    AddModal(new CDailyDealWindow());
    m_lastDailyDealDisplay = now;
    Save();
}

void CBH_GPSMap::ReleasePrefetchedManagerResource()
{
    for (int index = m_prefetchedManagers.size() - 1; index >= 0; --index) {
        m_prefetchedManagers[index]->freeResources();
    }
}

void CBH_GPSMap::Paint(ICGraphics2d& graphics)
{
    if (!shopIsPainting && !safehouseIsPainting && !weaponIsPainting) {
        CDrawUtil::Fill(0, 0, static_cast<App*>(WindowApp::m_instance)->m_screenWidth,
                        static_cast<App*>(WindowApp::m_instance)->m_screenHeight, 0xff636563,
                        ICGraphics2d::Primitive_Rectangles);

        // Original 0xe9184: tiles are drawn back to back; each row advances by the height of its
        // last tile (no NULL tile handling in the original).
        if (m_tiles.size() > 0 && m_rows > 0) {
            int x = m_mapX;
            int y = m_mapY;
            for (int row = 0; row < m_rows; ++row) {
                ICRenderSurface* tile = NULL;
                for (int column = 0; column < m_columns; ++column) {
                    int index = row * m_columns + column;
                    if (index < m_tiles.size()) {
                        tile = m_tiles[index];
                        static_cast<App*>(WindowApp::m_instance)->m_helper->DrawImage(tile, x, y, 1.0f, 0, 0, 0.0f, 0);
                        x += ImageWidth(tile);
                    }
                }
                x = m_mapX;
                y += ImageHeight(tile);
            }
        }
        for (int index = 0; index < m_items.size(); ++index) {
            m_items[index]->Paint(graphics, m_mapX, m_mapY);
        }
        m_gpsScan->Paint(m_mapX, m_mapY);
        CBaseScreen::Paint(graphics);
    }
}

// Original 0xe92f4: press picks the touched item, release clicks it when the map barely moved
// (15 * scale pixels) and starts the fling (scroll state 3), move drags the map.
void CBH_GPSMap::OnPointerEvent(PointerEventController::Event event)
{
    if (!HasModalChild() && static_cast<App*>(WindowApp::m_instance)->m_timeScale != 0.0f) {
        if (event.m_type == PointerEventController::EVENT_MOVE) {
            m_scrollX += event.m_deltaX;
            m_scrollY += event.m_deltaY;
            if (!m_recordedMapDrag) {
                CBH_Player::GetInstance()->m_gameStats.AddMapDrags(1);
                m_recordedMapDrag = true;
            }
            if (MathLib::Fabs(m_mapX - m_dragOriginX) < App::Scale() * 15
                && MathLib::Fabs(m_mapY - m_dragOriginY) < App::Scale() * 15) {
                return;
            }
            if (m_pressedItem != NULL) {
                m_pressedItem->SetPressed(false);
                m_pressedItem = NULL;
            }
        } else if (event.m_type == PointerEventController::EVENT_UP) {
            m_draggingMap = false;
            m_recordedMapDrag = true;
            m_pressedItem = NULL;
            SetScrollState(3);
            if (MathLib::Fabs(m_mapX - m_dragOriginX) < App::Scale() * 15
                && MathLib::Fabs(m_mapY - m_dragOriginY) < App::Scale() * 15) {
                for (int index = 0; index < m_items.size(); ++index) {
                    CGPSItem* item;
                    if (m_items[index]->CheckHit(event.m_position.x - m_mapX, event.m_position.y - m_mapY, &item) == true) {
                        item->OnClick();
                        item->SetPressed(false);
                        return;
                    }
                }
            }
        } else if (event.m_type == PointerEventController::EVENT_DOWN) {
            m_draggingMap = true;
            m_dragOriginX = m_mapX;
            m_dragOriginY = m_mapY;
            m_recordedMapDrag = false;
            for (int index = 0; index < m_items.size(); ++index) {
                CGPSItem* item;
                if (m_items[index]->CheckHit(event.m_position.x - m_mapX, event.m_position.y - m_mapY, &item) == true) {
                    if (m_pressedItem != NULL) {
                        m_pressedItem->SetPressed(false);
                    }
                    m_pressedItem = item;
                    item->SetPressed(true);
                    return;
                }
            }
        }
    }
}

/** -1, 0 or 1 (float). */
static inline float ScrollSign(float value)
{
    if (value < 0.0f) {
        return -1.0f;
    }
    return value > 0.0f ? 1.0f : 0.0f;
}

/** -1, 0 or 1 (double). */
static inline double ScrollSignDouble(float value)
{
    if (value < 0.0f) {
        return -1.0;
    }
    return value > 0.0f ? 1.0 : 0.0;
}

// Original 0xea550: state 2 moves towards the scroll target, state 3 is the fling after a drag;
// both fall back to state 1 when they stop. A map dragged past its edge springs back.
void CBH_GPSMap::UpdateScroll()
{
    int elapsed = static_cast<App*>(WindowApp::m_instance)->m_updateFrameTime;
    // how far the map is outside its allowed area
    float overX = 0.0f;
    if (m_mapX > -MapWidthInset()) {
        overX = -(m_mapX + MapWidthInset());
    } else if (m_mapX + m_mapWidth < static_cast<App*>(WindowApp::m_instance)->m_screenWidth) {
        overX = static_cast<App*>(WindowApp::m_instance)->m_screenWidth - (m_mapX + m_mapWidth);
    }
    float overY = 0.0f;
    if (m_mapY > -MapHeightInset()) {
        overY = -(m_mapY + MapHeightInset());
    } else if (m_mapY + m_mapHeight < static_cast<App*>(WindowApp::m_instance)->m_screenHeight) {
        overY = static_cast<App*>(WindowApp::m_instance)->m_screenHeight - (m_mapY + m_mapHeight);
    }

    if (m_scrollState == 2) {
        float distanceX = m_scrollTargetX - m_scrollX;
        float distanceY = m_scrollTargetY - m_scrollY;
        float stepX = ScrollSign(distanceX) * m_scrollSpeedX * elapsed / 1000.0f;
        float stepY = ScrollSign(distanceY) * m_scrollSpeedY * elapsed / 1000.0f;
        if (MathLib::Fabs(stepX) > MathLib::Fabs(distanceX)) {
            stepX = distanceX;
        }
        if (MathLib::Fabs(stepY) > MathLib::Fabs(distanceY)) {
            stepY = distanceY;
        }
        m_scrollX += stepX;
        m_scrollY += stepY;
        if (MathLib::Fabs(m_scrollTargetX - m_mapX) < MapWidthInset() + 4) {
            m_scrollSpeedX = 0.0f;
        }
        if (MathLib::Fabs(m_scrollTargetY - m_mapY) < MapHeightInset() + 4) {
            m_scrollSpeedY = 0.0f;
        }
        if (MathLib::Sqrt(m_scrollSpeedX * m_scrollSpeedX + m_scrollSpeedY * m_scrollSpeedY) == 0.0f) {
            SetScrollState(1);
        }
    } else if (m_scrollState == 3) {
        float brakeFactor = App::Scale() * 0.0008f;
        float ticks = elapsed + 1;
        float resistance = App::Scale() * 2000.0f;
        m_scrollX += m_scrollVelocityX * ticks;
        m_scrollY += m_scrollVelocityY * ticks;
        float brakeX = ScrollSign(m_scrollVelocityX) * brakeFactor * ticks * ((resistance + overX * overX) / resistance);
        if (MathLib::Fabs(brakeX) > MathLib::Fabs(m_scrollVelocityX)) {
            m_scrollVelocityX = 0.0f;
        } else {
            m_scrollVelocityX -= brakeX;
        }
        float brakeY = ScrollSign(m_scrollVelocityY) * brakeFactor * ticks * ((resistance + overY * overY) / resistance);
        if (MathLib::Fabs(brakeY) > MathLib::Fabs(m_scrollVelocityY)) {
            m_scrollVelocityY = 0.0f;
        } else {
            m_scrollVelocityY -= brakeY;
        }
        if (MathLib::Sqrt(m_scrollVelocityX * m_scrollVelocityX + m_scrollVelocityY * m_scrollVelocityY) < 0.01) {
            m_scrollVelocityX = 0.0f;
            SetScrollState(1);
        }
    }

    if (MathLib::Sqrt(overX * overX + overY * overY) > 0.0f) {
        float stepX = elapsed * 0.5 * ScrollSignDouble(overX);
        if (MathLib::Fabs(stepX) > MathLib::Fabs(overX)) {
            stepX = overX;
        }
        float stepY = elapsed * 0.5 * ScrollSignDouble(overY);
        float scrollX = m_scrollX;
        float scrollY = m_scrollY;
        if (MathLib::Fabs(stepY) > MathLib::Fabs(overY)) {
            stepY = overY;
        }
        m_scrollX = scrollX + stepX;
        m_scrollY = scrollY + stepY;
    }
}

// Original 0xea450: keeps 40 % of the screen covered by the map (Update copies it to m_mapX).
void CBH_GPSMap::CheckMapPosBounds()
{
    if (m_scrollX > static_cast<App*>(WindowApp::m_instance)->m_screenWidth * 0.6) {
        m_scrollX = static_cast<App*>(WindowApp::m_instance)->m_screenWidth * 0.6;
    }
    if (m_scrollX + m_mapWidth < static_cast<App*>(WindowApp::m_instance)->m_screenWidth * 0.4) {
        m_scrollX = static_cast<App*>(WindowApp::m_instance)->m_screenWidth * 0.4 - m_mapWidth;
    }
    if (m_scrollY > static_cast<App*>(WindowApp::m_instance)->m_screenHeight * 0.6) {
        m_scrollY = static_cast<App*>(WindowApp::m_instance)->m_screenHeight * 0.6;
    }
    if (m_scrollY + m_mapHeight < static_cast<App*>(WindowApp::m_instance)->m_screenHeight * 0.4) {
        m_scrollY = static_cast<App*>(WindowApp::m_instance)->m_screenHeight * 0.4 - m_mapHeight;
    }
}
