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
const int PendingStoryIntro = 0x0c;
const int PendingHuntConfirmation = 0x0f;
const int PendingAmmoWarning = 0x10;
const int PendingEnergyWarning = 0x11;
const int PendingLowMoneyWarning = 0x1d;
const int ViewNewChallenges = 0x79399e3c;
const int GServeLoginFinished = 3;
const int LowMoneyGetCommand = -0x17b4f1c;  // ShowLowMoneyDialog second button
const int ConfirmInformant = 0x43a30531;
const int CancelInformant = 0x43a30532;
const int OpenMissionAmmoShop = 0x43a30533;
const int OpenMissionWeaponShop = 0x43a30534;
const int OpenMissionLoadout = 0x43a30535;
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
      m_backgroundCount(0), m_currentChallengeDay(-1), m_mapWidth(0), m_mapHeight(0), m_columns(0), m_rows(0),
      m_mapX(0), m_mapY(0), m_pendingCommand(0), m_lastDailyDealDisplay(CStdUtil_Cocoa::GetTimeSeconds()),
      m_dailyDealShown(false), m_showRocketLauncherIntro(true), m_draggingMap(false), m_field150(false),
      m_field180(false), m_field184(0), m_bonusTeaserElapsed(0), m_skipAmmoCheck(false), m_field194(0), m_field198(0),
      m_field19c(0), m_gpsScan(NULL), m_field1a4(false), m_scrollState(0), m_scrollVelocityX(0.0f), m_scrollVelocityY(0.0f),
      m_scrollSpeedX(0.0f), m_scrollSpeedY(0.0f), m_scrollTargetX(0.0f), m_scrollTargetY(0.0f), m_scrollX(0.0f),
      m_scrollY(0.0f), m_previousScrollX(0.0f), m_previousScrollY(0.0f),
      m_recordedMapDrag(true), m_shortcuts(NULL), m_movingMissionInformant(NULL),
      m_cooperation(NULL), m_bar(NULL), m_shop(NULL),
      m_network(NULL), m_pressedItem(NULL), m_needSave(false),
      m_prefetchIndex(0), m_socialNetworkMode(1),
      m_hud(NULL), m_selectedCamera(NULL),
      m_selectedInformant(NULL), m_priorDialogIndex(0), m_advanceMissionDialog(false),
      m_showingHuntConfirmation(false), m_acceptInformant(false), m_showTutorialFinish(false),
      m_nextGiftSaveAttempt(0), m_zombiesPromo(NULL),
      m_nextChallengeSaveAttempt(0), m_mapIdleElapsed(0),
      m_openTargetPickerAfterLogin(false),
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
    m_field2b5 = false;
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
    bool skipAmmoCheck = m_skipAmmoCheck;
    m_skipAmmoCheck = false;
    if (!MakePlayerEnergyCheck(item->m_energyCost) || !CheckMissionEquipment(camera, item, skipAmmoCheck)) {
        if (m_pendingCommand != PendingAmmoWarning) {
            m_selectedCamera = NULL;
        }
        return;
    }
    if (item->GetType() != 0) {
        App* app = static_cast<App*>(WindowApp::m_instance);
        app->m_soundManager->PlaySound("IDM_MENU_WINDOW_OPEN_CLOSE", false, false);
    }
    ShowMissionDialog();
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
    if (weapon == NULL || CBH_Player::GetInstance()->GetAvailableAmmo(weapon->GetDefaultAmmo(), true) >= weapon->m_clipSize) {
        return true;
    }
    XString message = ResString("IDS_MESSAGE_LOW_AMMO_IN_MAIN_WEAPON");
    m_pendingCommand = PendingAmmoWarning;
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
        m_pendingCommand = PendingAmmoWarning;
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

bool CBH_GPSMap::CheckMissionEquipment(CGPSHauntCamera* camera, CGPSHuntingItem* item, bool skipAmmoCheck)
{
    const bool weaponLoan = item->m_randomWeapon >= 0 || item->GetEstimatedWeaponID() >= 0;
    if (!weaponLoan && item->GetType() == 0 && !MakePlayerWeaponCheck(item->GetMissionId())) {
        return false;
    }
    if (camera->m_durationMs < 0 && item->m_tranquillize && !weaponLoan && !MakePlayerTranqMissionCheck()) {
        return false;
    }
    if (!weaponLoan && item->m_minWeaponRange < 0.0f && !MakePlayerWeaponRangeCheck(camera->m_minWeaponRange)) {
        return false;
    }
    if (camera->m_durationMs <= 0 && item->m_silentKill && !weaponLoan && !MakeSilentWeaponCheck()) {
        return false;
    }
    if (!item->m_weaponGated) {
        return weaponLoan || skipAmmoCheck || MakePlayerAmmoCheck();
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
    if (!skipAmmoCheck && CBH_Player::GetInstance()->GetAvailableAmmo(weapon->GetActiveAmmo(), true) <= 0) {
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
    case GServeLoginFinished: {
        if (!App::m_bIsLoginOK) {
            App::m_bIsLoginOK = true;
            m_openFriendsWindow = false;
            return;
        }
        if (m_openFriendsWindow) {
            CNGSLocalUser* local = CNGS::GetInstance()->GetLocalUser();
            if (local->isAuthenticated(SOCIAL_NETWORK_FACEBOOK) ||
                (local->isAuthenticated(SOCIAL_NETWORK_GAME_CENTER) &&
                 GameCenter_Facade::GetInstance()->isGameCenterSupported() == true)) {
                OpenPendingFriendsWindow();
            } else {
                m_openFriendsWindow = false;
            }
        }
        return;
    }
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
    case RefreshMapAfterGiftXp:
        UpdateMapItems();
        break;
    case CFiveDaysBonusDialog::INCENTIVE_BONUS_ACCEPTED:
        ShowFiveDaysInventiveBonusRecomendedItem();
        break;
    case CBH_InformantsHud::Shop:
        OnShopItemSelected(NULL);
        break;
    case CBH_InformantsHud::Bank: {
        CGameAnalytics::logStoreEnter(CGameAnalytics::SHOP_ENTER_GPS_ITEM);
        CRegularShop* shop = CRegularShop::GetInstance(true);
        shop->SetTab(CShopWindow::TAB_BANK);
        if (shop->m_parent == NULL) {
            m_parent->AddModal(shop);
        }
        break;
    }
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
        ShowCoopWindow();
        break;
    case CBH_InformantsHud::Coop:
        OnCooperationItemSelected(m_cooperation);
        break;
    case CBH_InformantsHud::Weapon: {
        CGPSInstantInformant* informant = FindInstantInformant(CGPSInstantInformant::Gated, true);
        if (informant != NULL) {
            OnGPSInstantInformantSelected(informant);
        }
        break;
    }
    case CBH_InformantsHud::Vehicle: {
        if (m_movingMissionInformant != NULL) {
            OnGpsMissionInformantSelected(m_movingMissionInformant);
            break;
        }
        CGPSInstantInformant* informant = FindInstantInformant(CGPSInstantInformant::Moving, false);
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
        if (m_pendingCommand == PendingAmmoWarning) {
            m_pendingCommand = 0;
        }
        m_selectedCamera = NULL;
        if (event.m_id == OpenMissionAmmoShop) {
            CRegularShop::Show(CShopWindow::TAB_AMMO);
        } else if (event.m_id == OpenMissionWeaponShop) {
            if (event.m_param1 >= 0) {
                App* app = static_cast<App*>(WindowApp::m_instance);
                CItem* item = app->InitWeaponManager()->getItemById(event.m_param1);
                if (item != NULL && item->m_unlockLevel <= CBH_Player::GetInstance()->GetLevel()) {
                    ShowShopWithUnlockedWeapon(event.m_param1, CShopItemInfoWindow::SOURCE_SHOP);
                } else {
                    CRegularShop::Show(CShopWindow::TAB_WEAPONS);
                }
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
    if (m_pendingCommand == PendingLowMoneyWarning &&
        (event.m_id == AcceptMissionDialog || event.m_id == LowMoneyGetCommand ||
         event.m_id == CancelMissionDialog)) {
        m_pendingCommand = 0;
        if (event.m_id != CancelMissionDialog) {
            ShowShop(CShopWindow::TAB_BANK, event.m_id == LowMoneyGetCommand);
            CGameAnalytics::logStoreEnter(CGameAnalytics::SHOP_ENTER_MISSION_PROMPT);
        }
        event.Clear();
        return;
    }
    if (m_pendingCommand == PendingEnergyWarning) {
        if (event.m_id == AcceptMissionDialog) {
            m_pendingCommand = 0;
            m_selectedCamera = NULL;
            ShowShop(CShopWindow::TAB_ENERGY, false);
            event.Clear();
            return;
        }
        if (event.m_id == CancelMissionDialog) {
            m_pendingCommand = 0;
            event.Clear();
            return;
        }
    }
    if (event.m_id == AcceptMissionDialog && m_dialogsQueue.size() != 0) {
        m_dialogsQueue.removeElementAt(m_dialogsQueue.size() - 1);
        if (ShowNextQueueDialog(false)) {
            event.Clear();
            return;
        }
        if (m_pendingCommand == PendingStoryIntro) {
            m_pendingCommand = 0;
            CGPSHauntCamera* camera = m_selectedCamera;
            m_selectedCamera = NULL;
            if (camera != NULL) {
                camera->SetAttractSpeed(1.1f);
            }
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
        if (m_pendingCommand == PendingAmmoWarning && m_selectedCamera != NULL) {
            m_pendingCommand = 0;
            m_skipAmmoCheck = true;
        }
        m_advanceMissionDialog = true;
        event.Clear();
    } else if (event.m_id == CancelMissionDialog) {
        if (m_pendingCommand == PendingAmmoWarning || m_pendingCommand == PendingHuntConfirmation) {
            m_pendingCommand = 0;
        }
        m_skipAmmoCheck = false;
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
    if (m_selectedCamera->HasTargettingMission() && item->GetType() == 1) {
        FillBeforeTargetHuntInfo(static_cast<const CGPSBounty*>(item));
        CBeforeTargettingDialog* targetDialog = CreateBeforeTargettingDialog(true);
        m_showingHuntConfirmation = true;
        m_parent->AddModal(targetDialog);
        return;
    }
    if (mission != NULL && item->GetType() == 0 && m_priorDialogIndex == 0) {
        m_priorDialogIndex = mission->m_priorDialogs.size();
        ShowMissionPriorDialogs(item->GetMissionId());
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
    ShowBeforeHuntDialog();
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
    static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddModal(dialog);
}

CBeforeHuntDialog* CBH_GPSMap::CreateBeforeHuntDialog(bool allowLater)
{
    CBH_Player* player = CBH_Player::GetInstance();
    CBeforeHuntDialog* dialog = new CBeforeHuntDialog();
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
    eGameFont rewardFont = GAME_FONT_DEFAULT;
    if (player->m_field238 != 0) {
        rewardFont = static_cast<eGameFont>(17);
    }
    XString money = XString("+") + XString(player->m_field220);
    dialog->AddDownTextWithIcon(money, "IDB_HUD_ICON_MONEY", false, true, rewardFont);
    XString experience = XString("+") + XString(player->m_field21c);
    dialog->AddDownTextWithIcon(experience, "IDB_HUD_ICON_EXP", false, true, rewardFont);
    if (player->IsEnegyEnabled()) {
        XString cost = XString(L"   ") + ResString("IDS_DIALOG_COST") + L":";
        dialog->AddDownTextWithIcon(cost, NULL, false, false, GAME_FONT_DEFAULT);
        dialog->AddDownTextWithIcon(XString(player->m_missionBriefEnergy), "IDB_HUD_ICON_ENERGY",
                                    false, false, GAME_FONT_DEFAULT);
    }
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
    if (!app->m_bountyManager->HasTargettingMessages()) {
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
    CBH_Player* player = CBH_Player::GetInstance();
    CBeforeTargettingDialog* dialog;
    if (player->m_targetFriendId >= 0) {
        dialog = new CBeforeTargettingDialog(player->m_targetFriendId);
    } else {
        dialog = new CBeforeTargettingDialog(player->m_missionBriefTargetFB);
    }
    if (player->m_missionBriefImage != NULL) {
        dialog->SetExtraImageFromImageID(player->m_missionBriefImage);
    }
    dialog->SetTitleText(player->m_missionBriefTitle);
    dialog->SetMessageText(player->m_missionBriefText);
    dialog->SetMissionRewards(player->m_field220, player->m_field21c, player->m_missionBriefEnergy, true);
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
    camera->SetAttractSpeed(1.1f);
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
    // the original keeps the icon in a local only (m_zombiesPromo stays NULL)
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
    dialog->SetTitleText(informant->m_name.Upper());
    XString text = informant->m_startText;
    if (informant->m_cashCost > 0) {
        text.Append(XString("\n") + ResString("IDS_DIALOG_IT_WILL_COST")
                    + L" $" + XString(informant->m_cashCost));
    }
    if (informant->m_creditsCost > 0) {
        text.Append(XString("\n") + ResString("IDS_DIALOG_IT_WILL_COST")
                    + L" " + XString(informant->m_creditsCost) + L" " + ResString("IDS_DIALOG_CREDITS") + L".");
    }
    dialog->SetMessageText(text);
    dialog->SetButtons(CDialogWindow::BUTTON_OK, CDialogWindow::BUTTON_IGNORE);
    dialog->SetButtonsCommands(ConfirmInformant, CancelInformant, -1);
    m_selectedInformant = informant;
    m_parent->AddModal(dialog);
}

void CBH_GPSMap::ShowInformantIsSearchingDialog(CGPSInstantInformant* informant)
{
    CDialogWindow* dialog = new CDialogWindow(true);
    dialog->SetTitleText(informant->m_name.Upper());
    XString text = informant->m_comeLaterText;
    text.Append(XString("\n") + ResString("IDS_DIALOG_CREDITS") + XString(":") + XString(informant->m_resetCost));
    dialog->SetMessageText(text);
    dialog->SetButtons(CDialogWindow::BUTTON_BUY, CDialogWindow::BUTTON_IGNORE);
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
        if (bought || !static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial() ||
            !app->m_tutorialManager->IsTutorialInstantInformant(informant->m_id)) {
            ShowLowMoneyDialog();
            return;
        }
        success = informant->BuyFree();
        CDialogWindow* dialog = new CDialogWindow(true);
        dialog->SetTitleText("IDS_TUTORIAL_TITLE");
        dialog->SetMessageText(ResString("IDS_TUTORIAL_INFORMANT_NO_MONEY"));
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
    informant->SetAttractSpeed(0.2f);
    if (!informant->m_bought) {
        if (CBH_Player::GetInstance()->GetTutorialState() == 11) {
            CGameAnalytics::logTutorialEvents(CGameAnalytics::TUTORIAL_EVENT_INFORMANT_PURCHASED);
        }
        ShowBuyInformantDialog(informant);
        Save();
        return;
    }
    if (informant->IsSearching()) {
        ShowInformantIsSearchingDialog(informant);
        Save();
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
    if (informant->m_type == CGPSInstantInformant::Blue) {
        camera = GetCameraForBlueInformant();
    } else if (informant->m_type == CGPSInstantInformant::Moving) {
        camera = GetMovingRandomCamera();
    } else {
        camera = GetRandomCamera(true, false);
    }
    if (camera != NULL) {
        if (informant->m_type == CGPSInstantInformant::Targeting) {
            RequestTargetPicker();
            return;
        } else if (informant->m_type == CGPSInstantInformant::Gated) {
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
        Save();
        return;
    }
    informant->OnBountyGiven(camera->m_id);
    camera->SetAttractSpeed(1.1f);
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

void CBH_GPSMap::OpenPendingFriendsWindow()
{
    if (m_openFriendsWindow) {
        m_openFriendsWindow = false;
        CFriendsWindow* window = new CFriendsWindow();
        window->SetTab(m_socialNetworkMode);
        AddModal(window);
    }
}

void CBH_GPSMap::OnGPSSocialNetworkSelected()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (!app->GetNetworkAvailability()->IsAvailable()) {
        app->m_gserveHandler->ShowOfflineModeDialog();
        return;
    }
    CNGSLocalUser* local = CNGS::GetInstance()->GetLocalUser();
    if (!local->isAuthenticated(SOCIAL_NETWORK_FACEBOOK) ||
        (!local->isAuthenticated(SOCIAL_NETWORK_GAME_CENTER) &&
         GameCenter_Facade::GetInstance()->isGameCenterSupported() == true)) {
        if (!CNGSLoginFlow::GetInstance()->IsRunning()) {
            app->StartLogin(App::LoginBothNetworks, true);
        }
        return;
    }
    OpenPendingFriendsWindow();
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
    if (informant == NULL || !informant->IsAvailable() || m_dialogsQueue.size() != 0) {
        return;
    }
    if (static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsMissionInformantBlink(informant->m_id)) {
        CGameAnalytics::logTutorialEvents(CGameAnalytics::TUTORIAL_EVENT_STORY_INFORMANT_SELECTED);
    }
    if (!MakePlayerWeaponCheck(informant->m_missionId)) {
        return;
    }
    CGPSHauntCamera* camera = GetCameraById(informant->m_cameraId);
    if (camera == NULL) {
        return;
    }
    CGPSHuntingItem* item = camera->GetGPSHuntingItem();
    if (camera->HasBlueBounty()
        || (item != NULL && (!camera->HasMission() || item->GetMissionId() != informant->m_missionId))) {
        CDialogWindow* warning = new CDialogWindow(true);
        warning->SetTitleText("IDS_DIALOG_TITLE_WARNING");
        warning->SetMessageText(ResString("IDS_MESSAGE_CAN_NOT_TAKE_RED_MISSION_BECAUSE_OF_BLUE"));
        warning->SetButtons(CDialogWindow::BUTTON_OK);
        m_parent->AddModal(warning);
        camera->SetAttractSpeed(1.1f);
        return;
    }
    if (item == NULL) {
        camera->SetMissionOnCamera(informant->m_missionId, informant);
        WindowApp::HandleTunnelCommand(-0x2051ee21, 0, 0, 0);
        Save();
    }
    m_selectedCamera = camera;
    ShowMissionStartDialogs(informant->m_missionId);
    if (m_dialogsQueue.size() == 0) {
        m_selectedCamera = NULL;
        camera->SetAttractSpeed(1.1f);
        return;
    }
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
    FillBeforeHuntInfo(bounty, "IDB_DIALOG_NOTEBOOK", NULL);
    CBH_Player* player = CBH_Player::GetInstance();
    player->m_targetFriendId = bounty->m_targetFriendId;

    App* app = static_cast<App*>(WindowApp::m_instance);
    if (app->m_bountyManager->HasTargettingMessages()) {
        const CBountyManager::SMessage* message = app->m_bountyManager->GetRandomTargettingMessage();
        if (message != NULL) {
            XString brief = ResString("IDS_DIALOG_NAME") + XString(": ") + bounty->m_bossName;
            brief.Append(XString("\n"));
            brief.Append(bounty->ParametrizeText(message->m_text));
            player->m_missionBriefText = brief;
            player->m_missionBriefTitle = message->m_title;
        }
    }
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
    CBH_Player::GetInstance()->m_missionBriefTitle = item->GetObjectiveTitle();
    CBH_Player::GetInstance()->m_missionBriefEnergy = item->m_energyCost;
    CBH_Player::GetInstance()->m_field21c = item->m_experienceReward;
    CBH_Player::GetInstance()->m_field220 = item->m_moneyReward;
    CBH_Player::GetInstance()->m_targetFriendId = -1;
    CBH_Player::GetInstance()->m_missionBriefTargetFB = CStrWChar(item->m_targetFBUser.c_str());
    CBH_Player::GetInstance()->m_field238 = item->m_hasBonus;

    XString goal = item->GetObjectiveGoal();
    if (item->GetType() != 0 && m_selectedCamera != NULL) {
        CGPSHaunt* haunt = GetHauntByName(m_selectedCamera->m_haunt);
        if (haunt != NULL) {
            goal = goal.Replace(L"[LocationName]", haunt->m_screenName.c_str(), true);
        }
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
    if (mission == NULL) {
        return;
    }
    app->m_soundManager->PlaySound("IDM_MENU_WINDOW_OPEN_CLOSE", false, false);
    for (int index = 0; index < mission->m_priorDialogs.size(); ++index) {
        CMission::Dialog entry = mission->m_priorDialogs[index];
        CDialogWindow* dialog = new CDialogWindow(true);
        if (entry.m_image != NULL) {
            dialog->SetExtraImageFromImageID(entry.m_image);
        }
        dialog->SetTitleText(entry.m_title.Upper());
        CBH_PersonType* boss = app->InitPersonTypeManager()->GetType(mission->m_boss);
        bool isMan = true;
        if (boss != NULL && boss->m_visualCharacter != NULL) {
            isMan = boss->m_visualCharacter->m_isMan;
        }
        XString text = CGPSHuntingItem::ParametrizeTextWithParams(entry.m_text, mission->m_bossName, isMan);
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
    if (m_selectedCamera == NULL) {
        return;
    }
    CGPSHuntingItem* item = m_selectedCamera->GetGPSHuntingItem();
    if (m_selectedCamera->HasTempMission()) {
        item = m_selectedCamera->m_temporaryItem;
    }
    if (item == NULL) {
        m_selectedCamera = NULL;
        return;
    }
    if (item->GetType() == 1 && item->m_targetFBUser.Length() != 0) {
        FillBeforeTargetHuntInfo(static_cast<const CGPSBounty*>(item));
    } else {
        FillBeforeHuntInfo(item);
    }
    Window* dialog = NULL;
    if (CBH_Player::GetInstance()->m_missionBriefTargetFB.GetLength() < 1) {
        dialog = CreateBeforeHuntDialog(true);
    } else {
        dialog = CreateBeforeTargettingDialog(true);
    }
    CGPSSubContract* contract = m_selectedCamera->m_subContract;
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
    m_showingHuntConfirmation = true;
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
    App* app = static_cast<App*>(WindowApp::m_instance);
    const CMission* mission = app->m_missionManager->GetMissionById(missionId);
    if (mission == NULL || m_selectedCamera == NULL || m_dialogsQueue.size() != 0) {
        return;
    }
    const CGPSHuntingItem* item = m_selectedCamera->GetGPSHuntingItem();
    if (item == NULL) {
        return;
    }
    app->m_soundManager->PlaySound("IDM_MENU_WINDOW_OPEN_CLOSE", false, false);
    for (int index = 0; index < mission->m_startDialogs.size(); ++index) {
        const CMission::Dialog& entry = mission->m_startDialogs[index];
        CDialogWindow* dialog = new CDialogWindow(true);
        if (entry.m_image != NULL) {
            dialog->SetExtraImageFromImageID(entry.m_image);
        }
        dialog->SetTitleText(entry.m_title.Upper());
        dialog->SetMessageText(item->ParametrizeText(entry.m_text));
        dialog->SetButtons(CDialogWindow::BUTTON_OK);
        dialog->SetButtonsCommands(AcceptMissionDialog, -1, -1);
        m_dialogsQueue.insertElementAt(dialog, 0);
    }
    if (m_dialogsQueue.size() != 0) {
        m_pendingCommand = PendingStoryIntro;
        ShowNextQueueDialog(true);
    }
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
    data.m_informantUnlockOnLevelUp = WillInformantUnlockOnLevelUp();
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
    App* app = static_cast<App*>(WindowApp::m_instance);
    CheckMapPosBounds();
    app->InitAchievementManager()->Update();
    CheckGiftReceivedNews();
    if (!static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial()) {
        app->m_challengeManager->Update();
        CheckForNewChallenges();
        CheckChallengeNews();
        MakeDailyDealCheck();
    }
    if (m_shop != NULL) {
        m_shop->Update(app->m_updateFrameTime);
    }
    if (m_network != NULL) {
        m_network->SetVisible(static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsHomeVisible());
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
    if (m_needSave) {
        Save();
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
    // Original 0xe9f84: an item with an attract speed scrolls the map to it (scroll state 2).
    for (int index = 0; index < m_clickableItems.size(); ++index) {
        float speed = m_clickableItems[index]->AttractSpeed();
        if (speed > 0.0f) {
            m_scrollTargetX = static_cast<App*>(WindowApp::m_instance)->m_screenWidth / 2
                - RectCenterX(m_clickableItems[index]->GetRect());
            m_scrollTargetY = static_cast<App*>(WindowApp::m_instance)->m_screenHeight / 2
                - RectCenterY(m_clickableItems[index]->GetRect());
            float minX = static_cast<App*>(WindowApp::m_instance)->m_screenWidth - m_mapWidth;
            m_scrollTargetX = m_scrollTargetX < minX ? minX : (0.0f < m_scrollTargetX ? 0.0f : m_scrollTargetX);
            float minY = static_cast<App*>(WindowApp::m_instance)->m_screenHeight - m_mapHeight;
            m_scrollTargetY = m_scrollTargetY < minY ? minY : (0.0f < m_scrollTargetY ? 0.0f : m_scrollTargetY);
            m_scrollSpeedX = MathLib::Fabs((m_scrollTargetX - m_mapX) * speed);
            m_scrollSpeedY = MathLib::Fabs((m_scrollTargetY - m_mapY) * speed);
            m_scrollX = m_mapX;
            m_scrollY = m_mapY;
            m_clickableItems[index]->SetAttractSpeed(0.0f);
            SetScrollState(2);
        }
    }
    if (m_draggingMap) {
        // the drag speed becomes the fling velocity, limited to one pixel per millisecond
        SetScrollState(1);
        float inverseTicks = 1.0f / (app->m_updateFrameTime + 1);
        m_scrollVelocityX = (m_scrollX - m_previousScrollX) * inverseTicks;
        m_scrollVelocityY = (m_scrollY - m_previousScrollY) * inverseTicks;
        m_scrollVelocityX = m_scrollVelocityX < -1.0f ? -1.0f : (1.0f < m_scrollVelocityX ? 1.0f : m_scrollVelocityX);
        m_scrollVelocityY = m_scrollVelocityY < -1.0f ? -1.0f : (1.0f < m_scrollVelocityY ? 1.0f : m_scrollVelocityY);
    } else {
        UpdateScroll();
    }
    m_previousScrollX = m_scrollX;
    m_previousScrollY = m_scrollY;
    m_mapX = static_cast<int>(m_scrollX) & ~1;
    m_mapY = static_cast<int>(m_scrollY) & ~1;
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
            if (m_skipAmmoCheck) {
                OnCameraSelected(m_selectedCamera);
            } else if (m_showingHuntConfirmation) {
                CGPSHauntCamera* camera = m_selectedCamera;
                m_selectedCamera = NULL;
                m_pendingCommand = 0;
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
        if (m_showRocketLauncherIntro && !static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial()
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
    App* app = static_cast<App*>(WindowApp::m_instance);
    CItemManager* manager;
    CShopWindow::ETab tab = CShopWindow::TAB_OTHER;
    int itemId = 0;
    switch (day) {
    case 1:
        manager = reinterpret_cast<CItemManager*>(app->m_armorManager);
        break;
    case 2:
        manager = reinterpret_cast<CItemManager*>(app->m_ammoPackManager);
        tab = CShopWindow::TAB_AMMO;
        break;
    case 3:
        manager = reinterpret_cast<CItemManager*>(app->m_goPillManager);
        break;
    case 4:
        manager = reinterpret_cast<CItemManager*>(app->m_ammoPackManager);
        tab = CShopWindow::TAB_AMMO;
        itemId = 2;
        break;
    case 5:
        manager = reinterpret_cast<CItemManager*>(app->m_medicantManager);
        itemId = 3;
        break;
    default:
        return;
    }
    ShowShopWithItem(manager, tab, CItem::ITEM_TYPE_FIRST, itemId);
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
