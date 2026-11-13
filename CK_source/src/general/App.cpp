// App.cpp - the game's window application.
// Function order follows the original debug map (app, 2026-11-11).
#include "App.h"
#define AD_BANNER_SIZES_NO_MARGINS
#include "AdBannerSizes.h"
#undef AD_BANNER_SIZES_NO_MARGINS
#include "ICInAppPurchase.h"

// The original initializes the IAP class ID between the sizes and margins.
static AdBannerMarginsX s_adBannerMarginsX;
static AdBannerMarginsY s_adBannerMarginsY;

#include "AppMedia.h"
#include "AppSettings.h"
#include "ButtonWindow.h"
#include "CAmmoManager.h"
#include "CAmmoPackManager.h"
#include "CArmorManager.h"
#include "CAvatarManager.h"
#include "CBH_Player.h"
#include "CBH_SurfaceManager.h"
#include "CBountyGameFactory.h"
#include "CBountyManager.h"
#include "CAppInfo.h"
#include "CCurrencyManager.h"
#include "CDailyDealManager.h"
#include "CDH_ResourceManager.h"
#include "CDH_SoundManager.h"
#include "CDH_WeaponsManager.h"
#include "CEnergyManager.h"
#include "CFiveDaysBonus.h"
#include "CFiveDaysIncentiveBonus.h"
#include "CFlurry.h"
#include "CFont.h"
#include "CFontMgr.h"
#include "CFactionTypeManager.h"
#include "CFriendGatedMissionManager.h"
#include "CSocialGiftsManager.h"
#include "CFileData1.h"
#include "CGameAnalytics.h"
#include "CGameObjectManager.h"
#include "CGoPillManager.h"
#include "CGameApp.h"
#include "CGServeHandler.h"
#include "CGServeMessageManager.h"
#include "CHighscoreMgr.h"
#include "CHuntingInfo.h"
#include "DGHelper.h"
#include "CFileUtil.h"
#include "CIAPCurrencyManager.h"
#include "CAchievementManager.h"
#include "CChallengeManager.h"
#include "CPopupController.h"
#include "CSubContractManager.h"
#include "CTrickShotManager.h"
#include "CLocationManager.h"
#include "CLoginSteps.h"
#include "CMissionManager.h"
#include "CMedicantManager.h"
#include "CMiscItemManager.h"
#include "CBH_PersonTypeManager.h"
#include "CEnemyWeaponTypeManager.h"
#include "CNationalityTypeManager.h"
#include "CVehicleManager.h"
#include "CVirtualFriendManager.h"
#include "CVisualCharacterTypeManager.h"
#include "CVisualObjectManager.h"
#include "CScriptManager.h"
#include "CSquadTypeManager.h"
#include "CNGS.h"
#include "CNGSFactory.h"
#include "CNGSLocalUser.h"
#include "CNGSLoginFlow.h"
#include "CNGSDataIncentive.h"
#include "ICSystemFont.h"
#include "SScreenAdaptingConstant.h"
#include "CNGSOfferManager.h"
#include "CNGSSession.h"
#include "CNGSURLMgr.h"
#include "CNGS_Platform.h"
#include "CNetworkAvailability.h"
#include "CNewsFeedManager.h"
#include "CNotificationHandler.h"
#include "COverrideManager.h"
#include "CPrivateData1.h"
#include "CProfileManager.h"
#include "CProgressSystem.h"
#include "CPublicData1.h"
#include "CSaveManager.h"
#include "CStdUtil_Cocoa.h"
#include "CTjManager.h"
#include "CTipManager.h"
#include "CTutorialManager.h"
#include "CUtility.h"
#include "ICFileMgr.h"
#include "ICStdUtil.h"
#include "ICStdUtil_inl.h"
#include "CNode.h"
#include "LanguageDialog.h"
#include "LocalPushManager.h"
#include "ResMap.h"
#include "SG_Instance.h"
#include "SwerveHelper.h"
#include "TapjoyInterface.h"
#include <math.h>
#include <string.h>

// Named constants of the original App.o (__TEXT,__const).
static const char GSERV_OFFER_URL[] = "http://goffer.glu.com/gOffer/";
static const char NGSERV_URL[] = "http://ng-bounty.glu.com/bounty/";
static const char FLURRY_API_KEY[] = "1GQE9DWB9DP459DYZHE1";
static const char TAPJOY_APP_ID[] = "9b0a75e0-3a11-432a-8c24-414e30f69b35";
static const wchar_t gameRunningFilename[] = L"gamerun.dat";

int App::m_platformType = 1;
bool App::m_bIsLoginOK = true;
bool App::m_haveGamecenter;

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AppSizeCheck[sizeof(App) == 0x3a4 ? 1 : -1];
#endif

App::App(CNode* node)
    : m_initialized(false)
{
    m_keepFastTick = 0;
    m_displayProgram = NULL;
    m_node = node;
    m_timeScale = 1.0f;
    // One run of zero stores: the original merges 0x1e8..0x27c and 0x294..0x2c0 into two memsets.
    m_settings = NULL;
    m_media = NULL;
    m_field1f0 = NULL;
    m_field1f4 = NULL;
    m_field1f8 = NULL;
    m_field1fc = NULL;
    m_soundManager = NULL;
    m_highscoreManager = NULL;
    m_helper = NULL;
    m_resourceManager = NULL;
    m_resMap = NULL;
    m_vehicleManager = NULL;
    m_bountyManager = NULL;
    m_visualCharacterTypeManager = NULL;
    m_visualObjectManager = NULL;
    m_nationalityTypeManager = NULL;
    m_enemyWeaponTypeManager = NULL;
    m_personTypeManager = NULL;
    m_factionTypeManager = NULL;
    m_squadTypeManager = NULL;
    m_huntingInfo = NULL;
    m_locationManager = NULL;
    m_missionManager = NULL;
    m_ammoManager = NULL;
    m_ammoPackManager = NULL;
    m_medicantManager = NULL;
    m_goPillManager = NULL;
    m_armorManager = NULL;
    m_iapCurrencyManager = NULL;
    m_energyManager = NULL;
    m_weaponsManager = NULL;
    m_saveManager = NULL;
    m_tipManager = NULL;
    m_miscItemManager = NULL;
    m_currencyManager = NULL;
    m_avatarManager = NULL;
    m_scriptManager = NULL;
    m_achievementManager = NULL;
    m_trickShotManager = NULL;
    m_virtualFriendManager = NULL;
    m_friendMissionManager = NULL;
    m_challengeManager = NULL;
    m_dailyDealManager = NULL;
    m_subContractManager = NULL;
    m_progressSystem = NULL;
    m_tutorialManager = NULL;
    m_surfaceManager = NULL;
    m_sgInstance = NULL;
    m_popupController = NULL;
    m_bIsLoginOK = true;
    m_haveGamecenter = false;
    m_privateData = NULL;
    m_serverPrivateData = NULL;
    m_localPrivateData = NULL;
    m_notificationHandler = NULL;
    m_gserveHandler = NULL;
    m_publicData = NULL;
    m_serverPublicData = NULL;
    m_localPublicData = NULL;
    m_newsFeedManager = NULL;
    m_fileData = NULL;
    m_loginProgressDialog = NULL;
    m_field398 = -1;
    m_socialConnectDialog = NULL;
    m_field3a0 = false;
    m_field3a1 = false;
    m_field3a2 = false;
    m_field3a3 = false;
    m_field2e0 = 0;
    m_pushStatisticsSent = false;
    m_pushStatisticsDelay = 5.0f;
    m_orientationChangeTime = 0.0f;
    m_pendingOrientation = -1;
    m_acceleration.m_x = 0.0f;
    m_acceleration.m_y = 0.0f;
    m_acceleration.m_z = 0.0f;
    m_softkeyWidth = 0;
    m_softkeyHeight = 0;
    m_paintLayer = 0;
    m_gameWasRunning = false;
    m_tickRateChangeTime = 0;
    m_tickRate = 30;
    m_wantedTickRate = 30;
    CApplet::m_pCore->SetTickRate(30);
    // Game Center needs iOS 4.2
    CStrWChar version = CApplet::m_pCore->GetPlatformVersion();
    if (wcscmp(version.GetWCharPtr(), L"4.2") < 0) {
        m_haveGamecenter = false;
    }
}

unsigned char App::IsNetTimeAvailable()
{
    CNGSSession* session = CNGS::GetInstance()->GetLocalUser()->m_session;
    if (session == NULL) {
        return 0;
    }
    unsigned char valid = session->isValidNetworkTime();
    if (valid) {
        return 1;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (app == NULL) {
        return 0;
    }
    if (app->m_networkAvailability != NULL && app->m_networkAvailability->IsAvailable()) {
        session->resetNetworkTime();
    }
    return valid;
}

int App::GetCurrentDay()
{
    return DGHelper::GetDaysFromTimestamp(GetTime());
}

unsigned int App::GetTime()
{
    unsigned int time = CStdUtil_Cocoa::GetTimeSeconds();
    CNGSSession* session = CNGS::GetInstance()->GetLocalUser()->m_session;
    if (session->isValidNetworkTime() == 1) {
        time = session->getNetworkCurrentTimeSeconds();
    }
    return time;
}

void App::UpdateTickRate()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    bool fastTick = true;
    if (app->m_keepFastTick == 0) {
        fastTick = app->m_updateFrameTime > 110;
    }
    int wantedRate = 5;
    if (fastTick) {
        wantedRate = 30;
    }
    if (wantedRate != m_wantedTickRate) {
        m_tickRateChangeTime = app->m_updateTime;
        m_wantedTickRate = wantedRate;
        if (fastTick == true && m_tickRate != wantedRate) {
            m_tickRate = wantedRate;
            CApplet::m_pCore->SetTickRate(wantedRate);
        }
    } else if (m_tickRate != m_wantedTickRate && app->m_updateTime - m_tickRateChangeTime > 1000) {
        m_tickRate = m_wantedTickRate;
        CApplet::m_pCore->SetTickRate(m_wantedTickRate);
    }
}

Object3D App::LoadObject3D(const char* name)
{
    Object3D object;
    App* app = static_cast<App*>(WindowApp::m_instance);
    const XString fileName = app->InitResMap()->getFilename(name);
    if (fileName.Length() != 0) {
        object = SwerveHelper::LoadObject3D(fileName.c_str(), 0);
    }
    if (object.m_interface == NULL) {
        object = SwerveHelper::LoadObject3D(name);
    }
    return object;
}

void App::SetDisplayProgram(ICGraphics2d::IDisplayProgram* program)
{
    static_cast<App*>(WindowApp::m_instance)->m_displayProgram = program;
}

int App::SoftkeyMenuBackAlign()
{
    return 0x11;
}

ButtonWindow* App::NewSoftkeyButton(const XString& name, int command, int secondCommand, int align)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    ButtonWindow* button = new ButtonWindow();
    button->m_commandParam = 0;
    ImageRes image;
    ImageRes imageSelected;
    if (command == (int)0x9b991bc8 || command == (int)0x97204784) {
        image.Create("SUR_SK_BACK");
        imageSelected.Create("SUR_SK_BACK_SELECTED");
    } else if (command == (int)0xb4e0ade2) {
        image.Create("SUR_SK_PAUSE");
        imageSelected.Create("SUR_SK_PAUSE_SELECTED");
    }
    if (image.m_resource != NULL) {
        // Pause / back: a plain image button.
        button->SetLayoutType(Window::LAYOUT_SIMPLE);
        button->SetAlign(align);
        button->SetCommands(command, secondCommand);
        button->SetSleeped(secondCommand != 0);
        button->SetSounds(MediaCacheRes("IDM_MENU_CLICK", true), MediaRes());
        button->SetImages(image, imageSelected);
        button->SetBackgroundOpacity(1.0f);
        int width;
        int height;
        Window::GetImageSize(image.GetSurface(), width, height);
        button->SetDesiredWidth(width);
        button->SetDesiredHeight(height);
    } else {
        // Text on the three softkey images.
        CFont* font = CFontMgr::GetInstance()->GetFont(5);
        int minWidth = app->m_softkeyWidth;
        int textWidth = font->MeasureTextWidth(name.c_str(), -1, -1, NULL);
        int leftWidth = Window::ImageWidth(app->m_images[0].GetSurface());
        int rightWidth = Window::ImageWidth(app->m_images[2].GetSurface());
        button->SetLayoutType(Window::LAYOUT_SIMPLE);
        button->SetAlign(align);
        int width = leftWidth + textWidth + rightWidth;
        if (minWidth > width) {
            width = minWidth;
        }
        button->SetSize(width, app->m_softkeyHeight);
        button->SetName(name, font);
        button->SetCommands(command, secondCommand);
        button->SetSleeped(secondCommand != 0);
        button->SetSounds(MediaCacheRes("IDM_MENU_CLICK", true), MediaRes());
        button->SetImages(app->m_images[0], app->m_images[1], app->m_images[2],
                          app->m_images2[0], app->m_images2[1], app->m_images2[2]);
    }
    return button;
}

MediaRes App::MediaCacheRes(const char* name, bool create)
{
    Vector<MediaRes>& cache = static_cast<App*>(WindowApp::m_instance)->m_mediaCache;
    for (int index = 0; index < cache.size(); ++index) {
        MediaRes& cached = cache[index];
        if (strcmp(cached.GetResourceId(), name) == 0) {
            return cached;
        }
    }
    if (!create) {
        return MediaRes();
    }
    MediaRes resource(name);
    if (resource.m_resource != NULL) {
        cache.addElement(resource);
    }
    return resource;
}

ButtonWindow* App::NewSoftkeyButton(const XString& name, int command, int align)
{
    return NewSoftkeyButton(name, command, 0, align);
}

void App::InitNGServer(App* app)
{
    CNGS* service = CNGS::GetInstance();
    CNGSURLMgr::GetInstance()->setServerEnvironment(SERVER_ENV_4);
    CNGSURLMgr::GetInstance()->setBaseURL(SERVER_BASE_URL_GOFFER, SERVER_ENV_4, GSERV_OFFER_URL);
    service->SetServerURL(NGSERV_URL);
    service->m_gameId = 570;
    service->SetFactory(new CBountyGameFactory());

    {
        // The version strings live until the save interfaces are registered.
        wchar_t number[8];
        ICStdUtil::SWPrintF(number, L"%d", CAppInfo::GetInstance()->GetVersionMajor());
        CStrWChar major(number);
        ICStdUtil::SWPrintF(number, L"%d", CAppInfo::GetInstance()->GetVersionMinor());
        CStrWChar minor(number);
        ICStdUtil::SWPrintF(number, L"%d", CAppInfo::GetInstance()->GetVersionMicro());
        CStrWChar micro(number);
        service->setVersion(major, minor, micro);

        app->m_messageManager = new CGServeMessageManager();
        if (app->m_gserveHandler == NULL) {
            app->m_gserveHandler = new CGServeHandler();
            app->m_gserveHandler->Init();
        }
        app->m_profileManager = CNGS::GetInstance()->GetProfileManager();
        app->m_profileManager->setCollectionRange(1000, 1002);
        app->m_notificationHandler = new CNotificationHandler();
        CNGS::GetInstance()->GetLocalUser()->registerNotificationHandler(app->m_notificationHandler);
        COfferManager::GetInstance()->registerNotificationHandler(app->m_notificationHandler);

        app->m_privateData = new CPrivateData1();
        app->m_serverPrivateData = new CPrivateData1();
        app->m_localPrivateData = new CPrivateData1();
        app->m_publicData = new CPublicData1();
        app->m_serverPublicData = new CPublicData1();
        app->m_localPublicData = new CPublicData1();
        app->m_profileManager->registerSaveRestoreInterface(app->m_publicData, app->m_serverPublicData, app->m_localPublicData);
        app->m_profileManager->registerSaveRestoreInterface(app->m_privateData, app->m_serverPrivateData, app->m_localPrivateData);

        app->m_fileData = new CFileData1();
        app->m_localFileData = new CFileData1();
        app->m_serverFileData = new CFileData1();
        app->m_profileManager->registerSaveRestoreInterface(app->m_fileData, app->m_serverFileData, app->m_localFileData);
    }
    StartNGServer(app);
}

void App::StartNGServer(App* app)
{
    static_cast<App*>(WindowApp::m_instance)->m_networkAvailability->IsAvailable();
}

void App::InitPlayerIndependentGameData()
{
    m_newsFeedManager = new CNewsFeedManager();
}

void App::InitGameData()
{
    m_dailyDealManager = new CDailyDealManager();
    m_vehicleManager = new CVehicleManager();
    m_bountyManager = new CBountyManager();
    m_visualObjectManager = new CVisualObjectManager();
    InitAvatarManager();
    m_scriptManager = new CScriptManager();
    m_fiveDaysBonus = new CFiveDaysBonus();
    m_fiveDaysIncentiveBonus = new CFiveDaysIncentiveBonus();
    InitFactionTypeManager();
    InitMiscItemManager();
    m_huntingInfo = new CHuntingInfo();
    m_locationManager = new CLocationManager();
    m_missionManager = new CMissionManager();
    InitAmmoManager();
    m_ammoPackManager = new CAmmoPackManager();
    InitIAPCurrencyManager();
    m_medicantManager = new CMedicantManager();
    m_goPillManager = new CGoPillManager();
    m_armorManager = new CArmorManager();
    InitProgressSystem();
    m_gameObjectManager = new CGameObjectManager();
    InitWeaponManager();
    m_currencyManager = new CCurrencyManager();
    m_currencyManager->loadResources();
    m_energyManager = new CEnergyManager();
    m_tipManager = new CTipManager();
    m_tutorialManager = new CTutorialManager();
    static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddToFront(m_tutorialManager);
    m_tutorialManager->ClearFlags(Window::FLAG_FOCUSABLE);
    m_popupController = new CPopupController();
    static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddToFront(m_popupController);
    m_socialGiftManager = new CSocialGiftManager();
    m_trickShotManager = new CTrickShotManager();
    m_virtualFriendManager = new CVirtualFriendManager();
    m_friendMissionManager = new CFriendGatedMissionManager();
    m_subContractManager = new CSubContractManager();
    m_challengeManager = new CChallengeManager();
    m_initialized = true;
}

CAvatarManager* App::InitAvatarManager()
{
    if (m_avatarManager == NULL) {
        m_avatarManager = new CAvatarManager;
    }
    return m_avatarManager;
}

CFactionTypeManager* App::InitFactionTypeManager()
{
    if (m_factionTypeManager == NULL) {
        m_factionTypeManager = new CFactionTypeManager();
    }
    return m_factionTypeManager;
}

CMiscItemManager* App::InitMiscItemManager()
{
    if (m_miscItemManager == NULL) {
        m_miscItemManager = new CMiscItemManager();
    }
    return m_miscItemManager;
}

CAmmoManager* App::InitAmmoManager()
{
    if (m_ammoManager == NULL) {
        m_ammoManager = new CAmmoManager;
    }
    return m_ammoManager;
}

CIAPCurrencyManager* App::InitIAPCurrencyManager()
{
    if (m_iapCurrencyManager == NULL) {
        m_iapCurrencyManager = new CIAPCurrencyManager;
    }
    return m_iapCurrencyManager;
}

CProgressSystem* App::InitProgressSystem()
{
    if (m_progressSystem == NULL) {
        m_progressSystem = new CProgressSystem;
    }
    return m_progressSystem;
}

CDH_WeaponsManager* App::InitWeaponManager()
{
    if (m_weaponsManager == NULL) {
        m_weaponsManager = new CDH_WeaponsManager;
    }
    return m_weaponsManager;
}

void App::ClearGameData()
{
    static_cast<App*>(WindowApp::m_instance)->m_initialized = false;
    delete CBH_Player::GetInstance();
    CTjManager* tjManager = CTjManager::GetInstance();
    delete tjManager;
    CGameApp::GetInstance()->ReInit();
    COfferManager::GetInstance()->resetOffers();
    CNGSLoginFlow::GetInstance()->Reset();
    static_cast<App*>(WindowApp::m_instance)->m_loginProgressDialog = NULL;
    static_cast<App*>(WindowApp::m_instance)->m_field398 = -1;
    static_cast<App*>(WindowApp::m_instance)->m_socialConnectDialog = NULL;
    static_cast<App*>(WindowApp::m_instance)->m_field3a0 = false;
    static_cast<App*>(WindowApp::m_instance)->m_field3a1 = false;
    CNGS::GetInstance()->Reset(1);
    if (static_cast<App*>(WindowApp::m_instance)->m_gserveHandler != NULL) {
        delete static_cast<App*>(WindowApp::m_instance)->m_gserveHandler;
        static_cast<App*>(WindowApp::m_instance)->m_gserveHandler = NULL;
    }
}

void App::Init()
{
    m_sgInstance = new SG_Instance();
    const char* sight = "SIGHT_HVGA";
    if (IsXGA()) {
        sight = "SIGHT_XGA";
    } else if (IsVGA()) {
        sight = "SIGHT_VGA";
    }
    m_sgInstance->Init((char*)sight);

    m_settings = new AppSettings();
    m_media = new AppMedia();
    m_media->SetActivated(true);
    m_media->Init();
    m_surfaceManager = new CBH_SurfaceManager();
    m_networkAvailability = new CNetworkAvailability();
    InitNGServer(this);

    m_saveManager = new CSaveManager();
    m_saveManager->Init();
    InitResMap();
    m_resourceManager = new CDH_ResourceManager();
    m_soundManager = new CDH_SoundManager();
    m_highscoreManager = new CHighscoreMgr();
    m_helper = new DGHelper();

    MediaCacheRes("IDM_WINDOW_FLYOUT", true);
    MediaCacheRes("IDM_ITEM_SELECTED", true);
    MediaCacheRes("IDM_MENU_CLICK", true);
    m_images[0].Create("SUR_BUTTON_LEFT");
    m_images[1].Create("SUR_BUTTON_TILE");
    m_images[2].Create("SUR_BUTTON_RIGHT");
    m_images2[0].Create("SUR_BUTTON_LEFT");
    m_images2[1].Create("SUR_BUTTON_TILE");
    m_images2[2].Create("SUR_BUTTON_RIGHT");
    App* app = static_cast<App*>(WindowApp::m_instance);
    // Longer screen side (select as in the original).
    int screenLength = app->m_screenWidth > app->m_screenHeight ? app->m_screenWidth : app->m_screenHeight;
    m_softkeyWidth = screenLength / 7;
    m_softkeyHeight = Window::ImageHeight(m_images[1].GetSurface());

    {
        CStrWChar runningPath;
        CFileUtil::GetApplicationDataPathForFile(runningPath, gameRunningFilename);
        ICFileMgr* files = ICFileMgr::GetInstance();
        m_gameWasRunning = files->Exists(runningPath.GetWCharPtr()) != 0;
        files->Delete(runningPath.GetWCharPtr());
    }

    CFlurry::StartSession(FLURRY_API_KEY);
    CFlurry::SetSessionReportsOnPauseEnabled(true);
    CFlurry::SetSessionReportsOnCloseEnabled(false);
    CStrWChar platform = CApplet::m_pCore->GetPlatformIdentifier();
    // AdMarvel site id: the first iPad has its own (select as in the original).
    CGameAnalytics::SendAdMarvelID(wcscmp(platform.GetWCharPtr(), L"iPad1,1") == 0 ? 9658 : 9657);
    CGameAnalytics::SendTapJoyID(XString(TAPJOY_APP_ID));
    CGameAnalytics::SendDeferredPushToken();
    CUtility::RegisterUIKeys();
    LocalPushManager::CancelLocalPushNotifications(false);
    m_overrideManager = new COverrideManager();
}

bool App::IsXGA()
{
    unsigned short width = CApplet::m_UIScreenMainScreenWidth;
    unsigned short height = CApplet::m_UIScreenMainScreenHeight;
    // Longer side 1024 and shorter side 768 (selects as in the original).
    unsigned short longSide = width > height ? width : height;
    unsigned short shortSide = width < height ? width : height;
    return longSide == 1024 && shortSide == 768;
}

bool App::IsVGA()
{
    unsigned short width = CApplet::m_UIScreenMainScreenWidth;
    unsigned short height = CApplet::m_UIScreenMainScreenHeight;
    // Longer side 960 and shorter side 640 (selects as in the original).
    unsigned short longSide = width > height ? width : height;
    unsigned short shortSide = width < height ? width : height;
    return longSide == 960 && shortSide == 640;
}

bool App::IsWVGA()
{
    return false;
}

ResMap* App::InitResMap()
{
    if (m_resMap == NULL) {
        m_resMap = new ResMap;
        m_resMap->init();
    }
    return m_resMap;
}

void App::Shutdown()
{
    if (m_saveManager != NULL && m_field1b8) {
        m_saveManager->Save(true);
    }
    CGameAnalytics::logCurrentCash(CBH_Player::GetInstance()->FieldAt(0x1c));
    CGameAnalytics::logCurrentContractCredits(CBH_Player::GetInstance()->FieldAt(0x20));
    CGameAnalytics::logEnergyValue(CBH_Player::GetInstance()->FieldAt(0x18));
    DeleteGameRunningFile();
    if (m_sgInstance != NULL) {
        m_sgInstance->Destroy();
        if (m_sgInstance != NULL) {
            delete m_sgInstance;
            m_sgInstance = NULL;
        }
    }
    m_media->Shutdown();
    if (m_tipManager != NULL) {
        delete m_tipManager;
        m_tipManager = NULL;
    }
    if (m_progressSystem != NULL) {
        delete m_progressSystem;
        m_progressSystem = NULL;
    }
    if (m_weaponsManager != NULL) {
        delete m_weaponsManager;
        m_weaponsManager = NULL;
    }
    if (m_socialGiftManager != NULL) {
        delete m_socialGiftManager;
        m_socialGiftManager = NULL;
    }
    if (m_newsFeedManager != NULL) {
        delete m_newsFeedManager;
        m_newsFeedManager = NULL;
    }
    if (m_gameObjectManager != NULL) {
        delete m_gameObjectManager;
        m_gameObjectManager = NULL;
    }
    if (m_currencyManager != NULL) {
        delete m_currencyManager;
        m_currencyManager = NULL;
    }
    if (m_avatarManager != NULL) {
        delete m_avatarManager;
        m_avatarManager = NULL;
    }
    if (m_scriptManager != NULL) {
        delete m_scriptManager;
        m_scriptManager = NULL;
    }
    if (m_fiveDaysIncentiveBonus != NULL) {
        delete m_fiveDaysIncentiveBonus;
        m_fiveDaysIncentiveBonus = NULL;
    }
    if (m_fiveDaysBonus != NULL) {
        delete m_fiveDaysBonus;
        m_fiveDaysBonus = NULL;
    }
    if (m_achievementManager != NULL) {
        delete m_achievementManager;
        m_achievementManager = NULL;
    }
    if (m_helper != NULL) {
        delete m_helper;
        m_helper = NULL;
    }
    if (m_ammoManager != NULL) {
        delete m_ammoManager;
        m_ammoManager = NULL;
    }
    if (m_ammoPackManager != NULL) {
        delete m_ammoPackManager;
        m_ammoPackManager = NULL;
    }
    if (m_iapCurrencyManager != NULL) {
        delete m_iapCurrencyManager;
        m_iapCurrencyManager = NULL;
    }
    if (m_medicantManager != NULL) {
        delete m_medicantManager;
        m_medicantManager = NULL;
    }
    if (m_goPillManager != NULL) {
        delete m_goPillManager;
        m_goPillManager = NULL;
    }
    if (m_armorManager != NULL) {
        delete m_armorManager;
        m_armorManager = NULL;
    }
    if (m_energyManager != NULL) {
        delete m_energyManager;
        m_energyManager = NULL;
    }
    if (m_vehicleManager != NULL) {
        delete m_vehicleManager;
        m_vehicleManager = NULL;
    }
    if (m_miscItemManager != NULL) {
        delete m_miscItemManager;
        m_miscItemManager = NULL;
    }
    if (m_bountyManager != NULL) {
        delete m_bountyManager;
        m_bountyManager = NULL;
    }
    if (m_nationalityTypeManager != NULL) {
        delete m_nationalityTypeManager;
        m_nationalityTypeManager = NULL;
    }
    if (m_enemyWeaponTypeManager != NULL) {
        delete m_enemyWeaponTypeManager;
        m_enemyWeaponTypeManager = NULL;
    }
    if (m_personTypeManager != NULL) {
        delete m_personTypeManager;
        m_personTypeManager = NULL;
    }
    if (m_factionTypeManager != NULL) {
        delete m_factionTypeManager;
        m_factionTypeManager = NULL;
    }
    if (m_squadTypeManager != NULL) {
        delete m_squadTypeManager;
        m_squadTypeManager = NULL;
    }
    if (m_huntingInfo != NULL) {
        delete m_huntingInfo;
        m_huntingInfo = NULL;
    }
    if (m_locationManager != NULL) {
        delete m_locationManager;
        m_locationManager = NULL;
    }
    if (m_missionManager != NULL) {
        delete m_missionManager;
        m_missionManager = NULL;
    }
    if (m_highscoreManager != NULL) {
        delete m_highscoreManager;
        m_highscoreManager = NULL;
    }
    if (m_soundManager != NULL) {
        delete m_soundManager;
        m_soundManager = NULL;
    }
    if (m_resourceManager != NULL) {
        delete m_resourceManager;
        m_resourceManager = NULL;
    }
    if (m_trickShotManager != NULL) {
        delete m_trickShotManager;
        m_trickShotManager = NULL;
    }
    if (m_virtualFriendManager != NULL) {
        delete m_virtualFriendManager;
        m_virtualFriendManager = NULL;
    }
    if (m_friendMissionManager != NULL) {
        delete m_friendMissionManager;
        m_friendMissionManager = NULL;
    }
    if (m_challengeManager != NULL) {
        delete m_challengeManager;
        m_challengeManager = NULL;
    }
    if (m_dailyDealManager != NULL) {
        delete m_dailyDealManager;
        m_dailyDealManager = NULL;
    }
    if (m_subContractManager != NULL) {
        delete m_subContractManager;
        m_subContractManager = NULL;
    }
    m_mediaCache.setSize(0);
    if (m_media != NULL) {
        delete m_media;
        m_media = NULL;
    }
    if (m_resMap != NULL) {
        delete m_resMap;
        m_resMap = NULL;
    }
    if (m_settings != NULL) {
        delete m_settings;
        m_settings = NULL;
    }
    CBH_Player::GetInstance()->ReleaseResources();
    if (m_visualObjectManager != NULL) {
        delete m_visualObjectManager;
        m_visualObjectManager = NULL;
    }
    if (m_visualCharacterTypeManager != NULL) {
        delete m_visualCharacterTypeManager;
        m_visualCharacterTypeManager = NULL;
    }
    if (m_surfaceManager != NULL) {
        m_surfaceManager->Clear();
        if (m_surfaceManager != NULL) {
            delete m_surfaceManager;
            m_surfaceManager = NULL;
        }
    }
    if (m_overrideManager != NULL) {
        delete m_overrideManager;
        m_overrideManager = NULL;
    }
    CNGS::GetInstance()->Reset(1);
    if (m_privateData != NULL) {
        delete m_privateData;
        m_privateData = NULL;
    }
    if (m_serverPrivateData != NULL) {
        delete m_serverPrivateData;
        m_serverPrivateData = NULL;
    }
    if (m_localPrivateData != NULL) {
        delete m_localPrivateData;
        m_localPrivateData = NULL;
    }
    if (m_publicData != NULL) {
        delete m_publicData;
        m_publicData = NULL;
    }
    if (m_serverPublicData != NULL) {
        delete m_serverPublicData;
        m_serverPublicData = NULL;
    }
    if (m_localPublicData != NULL) {
        delete m_localPublicData;
        m_localPublicData = NULL;
    }
    if (m_fileData != NULL) {
        delete m_fileData;
        m_fileData = NULL;
    }
    if (m_serverFileData != NULL) {
        delete m_serverFileData;
        m_serverFileData = NULL;
    }
    if (m_localFileData != NULL) {
        delete m_localFileData;
        m_localFileData = NULL;
    }
    if (m_notificationHandler != NULL) {
        delete m_notificationHandler;
        m_notificationHandler = NULL;
    }
    m_world.Clear();
    if (m_saveManager != NULL) {
        delete m_saveManager;
        m_saveManager = NULL;
    }
}

void App::DeleteGameRunningFile()
{
    CStrWChar path;
    CFileUtil::GetApplicationDataPathForFile(path, gameRunningFilename);
    ICFileMgr::GetInstance()->Delete(path.GetWCharPtr());
}

void App::OnPause()
{
    if (m_media != NULL) {
        m_media->Pause();
    }
}

void App::OnResume()
{
    if (m_media != NULL) {
        m_media->Resume();
    }
}

void App::OnEnterBackground()
{
    if (!m_initialized) {
        return;
    }
    CGameAnalytics::logCurrentCash(CBH_Player::GetInstance()->FieldAt(0x1c));
    CGameAnalytics::logCurrentContractCredits(CBH_Player::GetInstance()->FieldAt(0x20));
    CGameAnalytics::logEnergyValue(CBH_Player::GetInstance()->FieldAt(0x18));
    CTjManager::GetInstance()->OnAppEnterBackground();
    m_saveManager->Save(false);
    static_cast<App*>(WindowApp::m_instance)->m_soundManager->StopAll();
    static_cast<App*>(WindowApp::m_instance)->m_soundManager->Pause("IDM_MENU_MUSIC");
    if (!CNGSLoginFlow::GetInstance()->IsRunning()) {
        TCVector<int> collections;
        m_profileManager->getAllRegisteredCollections(collections);
        m_profileManager->save(collections, false);
    }
    CNGS::GetInstance()->Pause();
}

void App::CreateGameRunningFile()
{
    CStrWChar path;
    CFileUtil::GetApplicationDataPathForFile(path, gameRunningFilename);
    ICFileMgr* files = ICFileMgr::GetInstance();
    ICFile* file = files->Open(path.GetWCharPtr(), ICFileMgr::AccessWrite);
    if (file != NULL) {
        files->Close(file);
    }
}

Window* App::NewDisplayWindow()
{
    return new DisplayWindow;
}

void App::Start()
{
    m_mainLayer = new WindowTransparent(false);
    m_mainLayer->SetAlign(Window::ALIGN_NONE);
    m_desktop->AddToFront(m_mainLayer);
    m_topLayer = new WindowTransparent(false);
    m_topLayer->ClearFlags(0x40);
    m_topLayer->SetAlign(Window::ALIGN_NONE);
    m_desktop->AddToFront(m_topLayer);
    m_mainLayer->AddModal(new StartDelayWindow);
    m_field1b8 = 1;
}

void App::UpdateTime()
{
    int elapsed = m_frameTimes[7];
    if (elapsed < 1) {
        elapsed = 1;
    }
    m_updateFrameTime = elapsed;
    if (elapsed > 200) {
        elapsed = 200;
    }
    m_updateFrameTime = MathLib::Round(elapsed * m_timeScale);
    m_updateTime += m_updateFrameTime;
}

void App::Update()
{
    UpdateTickRate();
    static_cast<App*>(WindowApp::m_instance)->m_saveManager->Update();
    m_media->Update();
    if (!m_pushStatisticsSent) {
        m_pushStatisticsDelay -= static_cast<App*>(WindowApp::m_instance)->m_frameTimeSeconds;
        if (m_pushStatisticsDelay <= 0.0f) {
            m_pushStatisticsDelay = 5.0f;
            if (CApplet::m_pThis->m_pushToken.GetLength() != 0) {
                m_pushStatisticsSent = true;
                CGameAnalytics::SendPushStatistics(CApplet::m_pThis->m_pushToken.GetCharPtr());
                CGameAnalytics::SendDeferredPushToken();
            }
        }
    }
    if (m_overrideManager != NULL) {
        m_overrideManager->HandleUpdate(static_cast<App*>(WindowApp::m_instance)->m_updateFrameTime);
    }
    CNGS::GetInstance()->HandleUpdate(static_cast<App*>(WindowApp::m_instance)->m_updateFrameTime);
    m_messageManager->HandleUpdate();
    m_gserveHandler->HandleUpdate();
}

void App::Paint(ICGraphics2d&)
{
}

void App::HandleRender()
{
    m_paintLayer = 0;
    HandlePaint();
}

void App::RenderWorld(const Graphics3D& graphics,
                      const World& world)
{
    // The original calls Graphics3D::render(World const&) (global Graphics3D / World classes in CK).
    graphics.render(world);
}

void App::OnCommand(Event& event)
{
    switch (event.m_id) {
    case 0x79ea2236: // select language
        static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddModal(new SelectLanguageDialog());
        break;
    case (int)0xd519d14c: // Tapjoy
        if (TapjoyInterface::canDisplayInterface()) {
            TapjoyInterface::openTapjoyInterface();
        }
        return;
    case (int)0xad174cef: { // accelerometer
        // The command parameters carry the raw float bits of the acceleration.
        float x = *(float*)&event.m_param1;
        float y = *(float*)&event.m_param2;
        float z = *(float*)&event.m_param3;
        float absoluteX = MathLib::Fabs(x);
        float absoluteY = MathLib::Fabs(y);
        if (z > -0.8f && MathLib::Fabs(absoluteX - absoluteY) > 0.2f) {
            int orientation;
            if (x > y) {
                orientation = ICCore::DeviceOrientation_Portrait;
                if (absoluteX > absoluteY) {
                    orientation = ICCore::DeviceOrientation_LandscapeRight;
                }
            } else {
                orientation = ICCore::DeviceOrientation_PortraitUpsideDown;
                if (absoluteX > absoluteY) {
                    orientation = ICCore::DeviceOrientation_LandscapeLeft;
                }
            }
            if (m_pendingOrientation != orientation) {
                if (orientation == ICCore::DeviceOrientation_LandscapeRight ||
                    orientation == ICCore::DeviceOrientation_LandscapeLeft) {
                    m_pendingOrientation = orientation;
                    m_orientationChangeTime = WindowApp::m_instance->m_timeSeconds;
                }
            } else if (orientation != CApplet::m_pCore->GetDeviceOrientation() &&
                       WindowApp::m_instance->m_timeSeconds - m_orientationChangeTime > 0.8f) {
                CApplet::m_pCore->SetDeviceOrientation((ICCore::eDeviceOrientation)m_pendingOrientation);
            }
        } else {
            m_pendingOrientation = -1;
        }
        // Acceleration in screen coordinates.
        switch (CApplet::m_pCore->GetDeviceOrientation()) {
        case ICCore::DeviceOrientation_Portrait:
            m_acceleration.m_x = x;
            m_acceleration.m_y = y;
            break;
        case ICCore::DeviceOrientation_LandscapeRight:
            m_acceleration.m_x = y;
            m_acceleration.m_y = -x;
            break;
        case ICCore::DeviceOrientation_PortraitUpsideDown:
            m_acceleration.m_x = -x;
            m_acceleration.m_y = -y;
            break;
        case ICCore::DeviceOrientation_LandscapeLeft:
            m_acceleration.m_x = -y;
            m_acceleration.m_y = x;
            break;
        default:
            event.Clear();
            return;
        }
        m_acceleration.m_z = z;
        break;
    }
    default:
        return;
    }
    event.Clear();
}

void App::Inc2DLayer()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (app->m_displayProgram != NULL) {
        int layer = app->m_displayProgram->GetLayer();
        app->m_displayProgram->SetLayer(layer + 1);
    }
}

void App::OnBeginWindowPaint(ICGraphics2d&, Window*)
{
    if (m_displayProgram != NULL) {
        int layer = m_displayProgram->GetLayer();
        m_displayProgram->SetLayer(layer + 1);
    }
}

void App::OnEndWindowPaint(ICGraphics2d&, Window*)
{
}

void App::OnBeginWindowPaint3D(ICGraphics2d& graphics, Window*)
{
    const CMatrix2d* transform;
    const CVector2d* translation;
    graphics.GetTransform(&transform, &translation);
    m_savedTransform = *transform;
    m_savedTranslation = *translation;
    const CRectangle& clip = graphics.GetClip();
    m_viewport.m_x = clip.m_x;
    m_viewport.m_y = clip.m_y;
    m_viewport.m_width = clip.m_width;
    m_viewport.m_height = clip.m_height;
    m_node->RenderEnd();
}

void App::OnEndWindowPaint3D(ICGraphics2d& graphics, Window*)
{
    m_node->RenderBegin();
    graphics.SetClip(m_viewport);
    graphics.LoadTransform(&m_savedTransform, &m_savedTranslation);
    graphics.SetColor(0x10000, 0, 0, 0);
}

bool App::IsEnoughFreeSpace()
{
    return ICFileMgr::GetInstance()->GetFreeDiskSpace() > 1024;
}

bool App::IsHD()
{
    if (IsVGA() || IsXGA()) {
        return true;
    }
    return false;
}

void App::OnEnterForeground()
{
    LocalPushManager::CancelLocalPushNotifications(false);
    CNGS::GetInstance()->Resume();
    CNGS::GetInstance()->GetLocalUser()->m_notificationHandler->OnEnterForeground();
    if (m_initialized) {
        static_cast<App*>(WindowApp::m_instance)->m_soundManager->Resume("IDM_MENU_MUSIC");
        m_overrideManager->checkUpdates();
    }
}

// The base constructors (CNGSLoginFlow.h) set id, name, network and timeout.
CheckOverrideDataStep::CheckOverrideDataStep()
{
    m_classId = 0xd538bed3;
}

LoadPrivateGameDataStep::LoadPrivateGameDataStep()
{
}

CNGSLoginStep_CheckVersion::CNGSLoginStep_CheckVersion()
{
    m_name = "CheckVersion";
    m_classId = 0x81a8233f;
}

void App::StartLogin(EGServeLoginType type, bool showProgress)
{
    m_bIsLoginOK = true;
    CNGSLoginFlow* flow = CNGSLoginFlow::GetInstance();
    if (flow->IsRunning() && type == LoginSwitchAccount && m_socialConnectDialog != NULL) {
        m_socialConnectDialog->Close();
        m_socialConnectDialog = NULL;
    }
    flow->Reset();
    if (showProgress == true) {
        m_gserveHandler->ShowSocialNetworkConnectDialogs(-1);
    }
    unsigned char gameCenterAvailable = CNGS_Platform::GetInstance()->isGameCenterAvailable();
    switch (type) {
    case LoginAtStartup:
        flow->Queue(new DeviceOnlineCheckStep());
        flow->Queue(new VersionCheckStep());
        CNGS::GetInstance()->GetLocalUser();
        if (gameCenterAvailable) {
            flow->Queue(new GameCenterLoginStep());
            flow->Queue(new LoadFriendsListStep(SOCIAL_INTERFACE_GAME_CENTER));
        }
        flow->Queue(new FacebookLoginStep(false));
        flow->Queue(new LoadFriendsListStep(SOCIAL_INTERFACE_FACEBOOK));
        flow->Queue(new LoadPrivateGameDataStep());
        flow->Queue(new LoadGameDataStep());
        flow->Queue(new InitIncentiveOffersStep());
        flow->Queue(new CheckOverrideDataStep());
        break;
    case LoginBothNetworks:
        if (gameCenterAvailable) {
            flow->Queue(new GameCenterLoginStep());
            flow->Queue(new LoadFriendsListStep(SOCIAL_INTERFACE_GAME_CENTER));
        }
        flow->Queue(new FacebookLoginStep(false));
        flow->Queue(new LoadFriendsListStep(SOCIAL_INTERFACE_FACEBOOK));
        flow->Queue(new LoadPrivateGameDataStep());
        flow->Queue(new LoadGameDataStep());
        flow->Queue(new InitIncentiveOffersStep());
        flow->Queue(new CheckOverrideDataStep());
        break;
    case LoginLoadProfile:
        flow->Queue(new LoadGameDataStep());
        break;
    case LoginFacebook:
        flow->Queue(new FacebookLoginStep(false));
        flow->Queue(new LoadFriendsListStep(SOCIAL_INTERFACE_FACEBOOK));
        flow->Queue(new LoadPrivateGameDataStep());
        flow->Queue(new LoadGameDataStep());
        flow->Queue(new InitIncentiveOffersStep());
        flow->Queue(new CheckOverrideDataStep());
        break;
    case LoginGameCenter:
        flow->Queue(new GameCenterLoginStep());
        flow->Queue(new LoadFriendsListStep(SOCIAL_INTERFACE_GAME_CENTER));
        flow->Queue(new LoadPrivateGameDataStep());
        flow->Queue(new LoadGameDataStep());
        flow->Queue(new InitIncentiveOffersStep());
        flow->Queue(new CheckOverrideDataStep());
        break;
    case LoginSwitchAccount:
        flow->Queue(new CNGSLoginStep_SocialNetwork(CNGSLoginFlow::GetInstance()->m_activeNetwork));
        flow->Queue(new GameReloadStep());
        break;
    case LoginForceFacebook:
        flow->Queue(new FacebookLoginStep(true));
        flow->Queue(new LoadFriendsListStep(SOCIAL_INTERFACE_FACEBOOK));
        flow->Queue(new LoadPrivateGameDataStep());
        flow->Queue(new LoadGameDataStep());
        flow->Queue(new InitIncentiveOffersStep());
        flow->Queue(new CheckOverrideDataStep());
        break;
    }
    flow->Start();
}

CSquadTypeManager* App::InitSquadTypeManager()
{
    if (m_squadTypeManager == NULL) {
        m_squadTypeManager = new CSquadTypeManager();
    }
    return m_squadTypeManager;
}

CBH_PersonTypeManager* App::InitPersonTypeManager()
{
    if (m_personTypeManager == NULL) {
        m_personTypeManager = new CBH_PersonTypeManager();
    }
    return m_personTypeManager;
}

CVisualCharacterTypeManager* App::InitVisualCharacterTypeManager()
{
    if (m_visualCharacterTypeManager == NULL) {
        m_visualCharacterTypeManager = new CVisualCharacterTypeManager();
    }
    return m_visualCharacterTypeManager;
}

CNationalityTypeManager* App::InitNationalityTypeManager()
{
    if (m_nationalityTypeManager == NULL) {
        m_nationalityTypeManager = new CNationalityTypeManager();
    }
    return m_nationalityTypeManager;
}

CEnemyWeaponTypeManager* App::InitEnemyWeaponTypeManager()
{
    if (m_enemyWeaponTypeManager == NULL) {
        m_enemyWeaponTypeManager = new CEnemyWeaponTypeManager();
    }
    return m_enemyWeaponTypeManager;
}

CAchievementManager* App::InitAchievementManager()
{
    if (m_achievementManager == NULL) {
        m_achievementManager = new CAchievementManager();
    }
    return m_achievementManager;
}

CDailyDealManager* App::InitDailyDealManager()
{
    if (m_dailyDealManager == NULL) {
        m_dailyDealManager = new CDailyDealManager;
    }
    return m_dailyDealManager;
}

CItemManager* App::GetItemManagerByName(const XString& name)
{
    if (name == XString("armor")) {
        return static_cast<App*>(WindowApp::m_instance)->m_armorManager;
    }
    if (name == XString("ammo")) {
        return static_cast<App*>(WindowApp::m_instance)->InitAmmoManager();
    }
    if (name == XString("medkit")) {
        return static_cast<App*>(WindowApp::m_instance)->m_medicantManager;
    }
    if (name == XString("weapon")) {
        return static_cast<App*>(WindowApp::m_instance)->InitWeaponManager();
    }
    if (name == XString("goPill")) {
        return static_cast<App*>(WindowApp::m_instance)->m_goPillManager;
    }
    if (name == XString("energy")) {
        return static_cast<App*>(WindowApp::m_instance)->m_energyManager;
    }
    return NULL;
}

CItemManager* App::GetItemManagerByType(int type)
{
    // Case order follows the original block order.
    switch (type) {
    case 2:
        return static_cast<App*>(WindowApp::m_instance)->InitAmmoManager();
    case 9:
        return static_cast<App*>(WindowApp::m_instance)->m_ammoPackManager;
    case 11:
        return static_cast<App*>(WindowApp::m_instance)->m_armorManager;
    case 7:
        return static_cast<App*>(WindowApp::m_instance)->m_currencyManager;
    case 4:
        return static_cast<App*>(WindowApp::m_instance)->m_energyManager;
    case 10:
        return static_cast<App*>(WindowApp::m_instance)->m_goPillManager;
    case 3:
        return static_cast<App*>(WindowApp::m_instance)->m_medicantManager;
    case 5:
        return static_cast<App*>(WindowApp::m_instance)->InitIAPCurrencyManager();
    case 12:
        return static_cast<App*>(WindowApp::m_instance)->InitMiscItemManager();
    case 0:
        return static_cast<App*>(WindowApp::m_instance)->InitWeaponManager();
    default:
        return NULL;
    }
}

inline void DisplayWindow::OnCommand(Event& event)
{
    if (static_cast<App*>(WindowApp::m_instance)->m_gserveHandler->OnCommand(event)) {
        return;
    }
    switch (event.m_id) {
    case COMMAND_SOUND_DIALOG:
        static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddModal(new GameStartSoundDialog());
        event.Clear();
        break;
    case COMMAND_SWITCH_ACCOUNT:
        static_cast<App*>(WindowApp::m_instance)->StartLogin(App::LoginSwitchAccount, true);
        event.Clear();
        break;
    case COMMAND_INCENTIVES_REWARDED: {
        // param1: number of unconsumed incentives to reward.
        COfferManager* offers = COfferManager::GetInstance();
        for (int index = 0; index < event.m_param1; index++) {
            CStrWChar friendName(offers->m_unconsumedIncentives[index]->m_invitedFriendName);
            CDataIncentive* incentive = offers->m_unconsumedIncentives[index];
            int quantity = incentive->m_quantity;
            CStrWChar unit(incentive->m_unitOfMeasurement);
            switch (offers->m_unconsumedIncentives[index]->m_offerCategoryId) {
            case 2: {
                // A friend joined the game.
                CDialogWindow* dialog = new CDialogWindow(true);
                dialog->SetButtons(CDialogWindow::BUTTON_OK);
                XString message = XString::Format(Window::ResString("IDS_FRIEND_JOINED_GAME_POPUP").c_str(),
                                                  friendName.GetWCharPtr());
                XString reward(L"");
                if (quantity > 0) {
                    reward = XString::Format(Window::ResString("IDS_FRIEND_JOINED_GAME_POPUP_RECEIVE").c_str(), quantity,
                                             unit.GetWCharPtr());
                }
                ICSystemFont* font = ICSystemFont::CreateInstance();
                font->Initialize(SScreenAdaptingConstant(15, 30)(), NULL, 1);
                dialog->SetMessageText(message + reward, font);
                static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddModal(dialog);
                CBH_Player::GetInstance()->AddCredits(quantity);
                CGameAnalytics::logFriendAccepted(XString(friendName.GetWCharPtr()), quantity);
                static_cast<App*>(WindowApp::m_instance)->m_newsFeedManager->AddFriendJoinedGame(
                    CStrWChar(friendName.GetWCharPtr()));
                break;
            }
            case 1:
                // A friend was invited.
                if (quantity > 0) {
                    CBH_Player::GetInstance()->AddMoney(quantity, (EGainSource)3);
                }
                static_cast<App*>(WindowApp::m_instance)->m_newsFeedManager->AddFriendInvitedToGame(
                    CStrWChar(friendName.GetWCharPtr()));
                break;
            }
            offers->queueUnconsumedIncentiveRewarded(index);
        }
        if (offers->m_unconsumedIncentives.Size() > 0) {
            offers->allUnconsumedIncentivesRewarded();
        }
        break;
    }
    case COMMAND_SERVER_GIFT: {
        CServerGiftNotifierDialog* dialog = new CServerGiftNotifierDialog();
        dialog->SetButtons(CDialogWindow::BUTTON_OK);
        dialog->SetTitleText(Window::ResString("IDS_AWARD_FROM_SERVER_TITLE"));
        dialog->SetMessageText(Window::ResString("IDS_AWARD_FROM_SERVER_NOTIFIER"));
        AddToFront(dialog);
        break;
    }
    case COMMAND_RESTART:
        CNGSLoginFlow::GetInstance()->Reset();
        WindowApp::m_instance->InitializeRestartApplication();
        event.Clear();
        break;
    case COMMAND_ASK_RESTART: {
        CDialogWindow* dialog = new CDialogWindow(true);
        dialog->SetButtons(CDialogWindow::BUTTON_OK);
        dialog->SetMessageText(XString("Application will restart"));
        dialog->SetTitleText("Restart");
        dialog->SetButtonsCommands(COMMAND_RESTART, -1, -1);
        AddToFront(dialog);
        static_cast<App*>(WindowApp::m_instance)->m_timeScale = 0.0f;
        break;
    }
    case COMMAND_EXIT:
        CApplet::m_pCore->ExitApp((ICCore::eExitStatus)0);
        break;
    case COMMAND_VERSION_CHANGED: {
        CDialogWindow* dialog = new CDialogWindow(true);
        dialog->SetButtons(CDialogWindow::BUTTON_OK);
        dialog->SetTitleText(Window::ResString("IDS_VERSION_CHANGE_TITLE"));
        dialog->SetMessageText(Window::ResString("IDS_VERSION_CHANGE_DESC"));
        dialog->SetButtonsCommands(COMMAND_UPDATE_GAME, -1, -1);
        AddToFront(dialog);
        static_cast<App*>(WindowApp::m_instance)->m_timeScale = 0.0f;
        break;
    }
    case COMMAND_UPDATE_GAME:
        CApplet::m_pCore->LaunchURL(L"http://gcs.glu.com/gcs/iphone/iphone.jsp?t=bounty&src=bounty&p=iphone", 0);
        exit(0);
    case COMMAND_ACCOUNT_CHANGED: {
        CDialogWindow* dialog = new CDialogWindow(true);
        dialog->SetTitleText(Window::ResString("IDS_ACCOUNT_CHANGE_TITLE"));
        dialog->SetMessageText(Window::ResString("IDS_ACCOUNT_CHANGE_DESC"));
        dialog->SetButtons(CDialogWindow::BUTTON_OK);
        dialog->SetButtonsCommands(COMMAND_EXIT, -1, -1);
        WindowApp::m_instance->m_desktop->AddModal(dialog);
        static_cast<App*>(WindowApp::m_instance)->m_timeScale = 0.0f;
        break;
    }
    case COMMAND_SESSION_CHANGED: {
        CDialogWindow* dialog = new CDialogWindow(true);
        dialog->SetTitleText(Window::ResString("IDS_SESSION_CHANGE_TITLE"));
        dialog->SetMessageText(Window::ResString("IDS_SESSION_CHANGE_DESC"));
        dialog->SetButtons(CDialogWindow::BUTTON_OK);
        dialog->SetButtonsCommands(COMMAND_EXIT, -1, -1);
        WindowApp::m_instance->m_desktop->AddModal(dialog);
        static_cast<App*>(WindowApp::m_instance)->m_timeScale = 0.0f;
        break;
    }
    case COMMAND_FORWARD_9B99:
        WindowApp::PutEvent(Event::COMMAND, (int)0x97204784, 0, 0, 0);
        event.Clear();
        break;
    case COMMAND_EXIT_CONSUMED:
        CApplet::m_pCore->ExitApp((ICCore::eExitStatus)0);
        event.Clear();
        break;
    case COMMAND_UPGRADE:
        CDemoMgr::GetInstance()->LaunchUpgrade();
        event.Clear();
        break;
    }
}

// ---- Not in the original App.o function list (kept: other units may use them) ----

bool App::IsInitialized()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    return app != NULL && app->m_initialized;
}

CSubContractManager* App::InitSubContractManager()
{
    if (m_subContractManager == NULL) {
        m_subContractManager = new CSubContractManager();
    }
    return m_subContractManager;
}
