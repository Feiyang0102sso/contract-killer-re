// App.h - the game application: the window application plus every game manager, the
// gServe connection, timing, device detection and the accelerometer orientation logic.
// Layout from App::App(CNode*), App::InitGameData() and App::Shutdown().
#ifndef APP_H
#define APP_H

#include "WindowApp.h"
#include "TimeValue.h"
#include "CMatrix2d.h"
#include "CRectangle.h"
#include "CVector2d.h"
#include "MathLib.h"
#include "tws.h"
#include <stdlib.h>
#include "CApplet.h"
#include "ICCore.h"
#include "ICGraphics2d.h"
#include "SplashWindow.h"
#include "YesNoDialog.h"
#include "CDialogWindow.h"
#include "CDemoMgr.h"
#include "CGServeHandler.h"

class CNode;
class AppSettings;
class AppMedia;
class CDH_SoundManager;
class CHighscoreMgr;
class DGHelper;
class CDH_ResourceManager;
class ResMap;
class CVisualCharacterTypeManager;
class CNationalityTypeManager;
class CEnemyWeaponTypeManager;
class CBH_PersonTypeManager;
class CHuntingInfo;
class CLocationManager;
class CMissionManager;
class CPersonsGroupManager;
class CWaveManager;
class CZombieMissionManager;
class CSceneSetupsManager;
class CBalanceManager;
class CSceneObjectManager;
class CIAPCurrencyManager;
class CEnergyManager;
class CDH_WeaponsManager;
class CSaveManager;
class CTipManager;
class CScriptManager;
class CQuestManager;
class CFiveDaysBonus;
class COffersManager;
class CAchievementManager;
class CCommonGameConfig;
class CSocialGiftManager;
class CNewsFeedManager;
class CProgressSystem;
class CTutorialManager;
class CBH_SurfaceManager;
class SG_Instance;
class CPopupController;
class CUpdateManager;
class CGrenadeManager;
class CGrenadePackManager;
class CPowerUpManager;
class CPowerUpPackManager;
class COverrideManager;
class CGCLeaderboardsManager;
class CNGSLocalUser;
class CProfileManager;  // added by fx2
class CPrivateData1;  // added by fx2
class CNotificationHandler;
class CGServeHandler;
class CNetworkAvailability;
struct lua_State;
class CShopItem;
class CItemManager;

/** Saved 3D viewport (shorts as the rasterizer state reports them). */
// app: derived from CRectangle (passed to ICGraphics2d::SetClip); the inline constructor
// gives the original two zero words at 0x2e4 in App::App.
struct SViewport : public CRectangle {
    SViewport()
    {
        m_x = 0;
        m_y = 0;
        m_width = 0;
        m_height = 0;
    }
};

class App : public WindowApp {
public:
    /** Kinds of device (GetDeviceType); DEVICE_UNKNOWN until detected. */
    enum EDeviceType {
        DEVICE_OTHER = 0,
        DEVICE_IPHONE = 2,
        DEVICE_IPHONE_3G = 3,
        DEVICE_IPHONE_3GS = 4,
        DEVICE_IPHONE_4 = 5,
        DEVICE_IPHONE_4_CDMA = 6,
        DEVICE_IPOD_1 = 7,
        DEVICE_IPOD_2 = 8,
        DEVICE_IPOD_3 = 9,
        DEVICE_IPOD_4 = 10,
        DEVICE_IPAD = 11,
        DEVICE_IPAD_2 = 12,
        DEVICE_UNKNOWN = 13
    };

    App(CNode* node);
    /** Create (once) and return the resource map; original 0x172138. */  // added by game1
    ResMap* InitResMap();  // added by game1
    // Lazily created managers (App.cpp 0x171af0..0x173c74); used by the Manager units.  // added by game1
    class CAvatarManager* InitAvatarManager();  // added by game1
    class CFactionTypeManager* InitFactionTypeManager();  // added by game1
    class CMiscItemManager* InitMiscItemManager();  // added by game1
    class CAmmoManager* InitAmmoManager();  // added by game1
    CIAPCurrencyManager* InitIAPCurrencyManager();  // added by game1
    CDH_WeaponsManager* InitWeaponManager();  // added by game1
    class CSquadTypeManager* InitSquadTypeManager();  // added by game1
    CBH_PersonTypeManager* InitPersonTypeManager();  // added by game1
    CVisualCharacterTypeManager* InitVisualCharacterTypeManager();  // added by game1
    CNationalityTypeManager* InitNationalityTypeManager();  // added by game1
    CEnemyWeaponTypeManager* InitEnemyWeaponTypeManager();  // added by game1
    CAchievementManager* InitAchievementManager();  // added by game1
    class CDailyDealManager* InitDailyDealManager();  // added by game1
    class CSubContractManager* InitSubContractManager();

    // app: inline (weak D1/D0 in the original App.o); the members free the media cache,
    // release the world and destroy the images.
    inline virtual ~App()
    {
    }

    virtual void Init();
    virtual void Shutdown();
    virtual Window* NewDisplayWindow();
    virtual void Start();
    virtual void OnPause();
    virtual void OnResume();
    virtual void OnEnterBackground();
    virtual void OnEnterForeground();
    virtual void OnBeginWindowPaint(ICGraphics2d& graphics, Window* window);
    virtual void OnEndWindowPaint(ICGraphics2d& graphics, Window* window);
    virtual void OnBeginWindowPaint3D(ICGraphics2d& graphics, Window* window);
    virtual void OnEndWindowPaint3D(ICGraphics2d& graphics, Window* window);
    virtual void OnCommand(Event& event);
    virtual void Update();
    virtual void Paint(ICGraphics2d& graphics);
    virtual void UpdateTime();
    /** Seconds since the application became active. */
    int GetActiveTime() const;

    static bool IsInitialized();
    static unsigned char IsNetTimeAvailable();  // changed by app (was bool: the original returns the session flag unchanged)
    static unsigned int GetTime();
    static int GetCurrentDay();
    bool TimePause();
    bool TimeResume();
    void UpdateTickRate();
    static Object3D LoadObject3D(const char* name);
    static MediaRes MediaCacheRes(const char* name, bool create);
    static void InitNGServer(App* app);
    static void StartNGServer(App* app);
    void InitPlayerIndependentGameData();
    void InitGameData();
    enum EGServeLoginType {
        LoginAtStartup = 0,
        LoginBothNetworks = 1,
        LoginLoadProfile = 2,
        LoginFacebook = 3,
        LoginGameCenter = 4,
        LoginSwitchAccount = 5,
        LoginForceFacebook = 6
    };
    void StartLogin(EGServeLoginType type, bool showProgress);
    CProgressSystem* InitProgressSystem();
    static void ClearGameData();
    static int GetDeviceType();
    static void OnUpdatesInstalled();
    static void DeleteGameRunningFile();
    static void CreateGameRunningFile();
    void HandleRender();
    static void SetDisplayProgram(ICGraphics2d::IDisplayProgram* program);
    static void RenderWorld(const Graphics3D& graphics,
                            const World& world);
    static void Inc2DLayer();
    static bool IsEnoughFreeSpace();
    static CShopItem* GetItemByStringUId(const XString& id);
    static CItemManager* GetItemManagerByName(const XString& name);
    static CItemManager* GetItemManagerByType(int type);
    static bool IsHD();
    static bool IsVGA();
    static bool IsXGA();
    static bool IsWVGA();
    static bool IsAtrix();
    static bool IsKindle();
    /** Alignment of the menu back softkey (original 0x171108). */  // added by app
    static int SoftkeyMenuBackAlign();  // added by app
    /** Softkey button: an image for the pause/back commands, else the name on the button images. */  // added by app
    static class ButtonWindow* NewSoftkeyButton(const XString& name, int command, int secondCommand, int align);  // added by app
    static class ButtonWindow* NewSoftkeyButton(const XString& name, int command, int align);  // added by app
    // b145: call-free inline accessor; CAchievementManager::Update reads the tutorial manager through it
    // (it shifts the inliner worklist so the two GameCenter_Facade::GetInstance copies swap stack slots).
    CTutorialManager* GetTutorialManager() const { return m_tutorialManager; }
    // b145: call-free accessor, read by the CGameScene loading step before CNGSLoginFlow::GetInstance.
    CNetworkAvailability* GetNetworkAvailability() const { return m_networkAvailability; }
    // b074i: original 117850 weak body; WVGA keeps 1 first, then selects 2 by HD.
    static int Scale()
    {
        int scale = 1;
        if (!IsWVGA()) {
            // select (not if) so IsHD is tested with cmp #0 like the original
            scale = IsHD() ? 2 : 1;
        }
        return scale;
    }
    int ConsumeActiveTime();
    static void LoadSGArchetype(int archetype, int character);
    void StartGServerLight();  // changed by b055x (was static; callers pass this in r0)

    static int m_platformType;      // 1
    static int m_deviceType;        // EDeviceType, detected on first use
    static bool m_haveGamecenter;
    static bool m_bIsLoginOK;

    // CK: WindowApp ends at 0x1bc (0x10 smaller than CKZ); App::App(CNode*) clears these four words.
    CPrivateData1* m_localPrivateData;          // 0x1bc
    class CPublicData1* m_localPublicData;       // 0x1c0
    CPrivateData1* m_serverPrivateData;         // 0x1c4
    class CPublicData1* m_serverPublicData;      // 0x1c8
    class CFileData1* m_localFileData;           // 0x1cc freed in Shutdown
    class CFileData1* m_serverFileData;          // 0x1d0
    bool m_initialized;                        // 0x1d4 game data created
    int m_keepFastTick;                          // 0x1d8
    ICGraphics2d::IDisplayProgram* m_displayProgram;  // 0x1dc
    CNode* m_node;                              // 0x1e0
    float m_timeScale;                          // 0x1e4 0 while paused
    AppSettings* m_settings;                    // 0x1e8
    AppMedia* m_media;                          // 0x1ec
    void* m_field1f0;                           // 0x1f0
    void* m_field1f4;                           // 0x1f4
    void* m_field1f8;                           // 0x1f8
    void* m_field1fc;                           // 0x1fc
    CDH_SoundManager* m_soundManager;           // 0x200
    CHighscoreMgr* m_highscoreManager;          // 0x204
    DGHelper* m_helper;                         // 0x208
    CDH_ResourceManager* m_resourceManager;     // 0x20c
    ResMap* m_resMap;                           // 0x210
    class CVehicleManager* m_vehicleManager;   // 0x214
    class CBountyManager* m_bountyManager;     // 0x218
    CVisualCharacterTypeManager* m_visualCharacterTypeManager;  // 0x21c
    class CVisualObjectManager* m_visualObjectManager;  // 0x220
    CNationalityTypeManager* m_nationalityTypeManager;          // 0x224
    CEnemyWeaponTypeManager* m_enemyWeaponTypeManager;          // 0x228
    CBH_PersonTypeManager* m_personTypeManager;                 // 0x22c
    class CFactionTypeManager* m_factionTypeManager;          // 0x230
    class CSquadTypeManager* m_squadTypeManager;              // 0x234
    CHuntingInfo* m_huntingInfo;                             // 0x238
    CLocationManager* m_locationManager;                     // 0x23c
    CMissionManager* m_missionManager;                       // 0x240
    class CAmmoManager* m_ammoManager;                       // 0x244
    class CAmmoPackManager* m_ammoPackManager;                // 0x248
    class CMedicantManager* m_medicantManager;                // 0x24c
    class CGoPillManager* m_goPillManager;                    // 0x250
    class CArmorManager* m_armorManager;                      // 0x254
    CIAPCurrencyManager* m_iapCurrencyManager;               // 0x258
    CEnergyManager* m_energyManager;                         // 0x25c
    CDH_WeaponsManager* m_weaponsManager;                    // 0x260
    CSaveManager* m_saveManager;                             // 0x264
    CTipManager* m_tipManager;                               // 0x268
    class CMiscItemManager* m_miscItemManager;                // 0x26c
    class CCurrencyManager* m_currencyManager;               // 0x270
    class CAvatarManager* m_avatarManager;                    // 0x274
    CScriptManager* m_scriptManager;                         // 0x278
    CFiveDaysBonus* m_fiveDaysBonus;                         // 0x27c
    class CFiveDaysIncentiveBonus* m_fiveDaysIncentiveBonus;  // 0x280
    CAchievementManager* m_achievementManager;               // 0x284
    class CGameObjectManager* m_gameObjectManager;            // 0x288
    CSocialGiftManager* m_socialGiftManager;                 // 0x28c
    CNewsFeedManager* m_newsFeedManager;                      // 0x290
    class CTrickShotManager* m_trickShotManager;              // 0x294
    class CVirtualFriendManager* m_virtualFriendManager;    // 0x298
    class CFriendGatedMissionManager* m_friendMissionManager; // 0x29c
    class CChallengeManager* m_challengeManager;             // 0x2a0
    class CDailyDealManager* m_dailyDealManager;              // 0x2a4
    class CSubContractManager* m_subContractManager;          // 0x2a8
    CProgressSystem* m_progressSystem;                       // 0x2ac
    CTutorialManager* m_tutorialManager;                     // 0x2b0
    CBH_SurfaceManager* m_surfaceManager;                    // 0x2b4
    SG_Instance* m_sgInstance;                               // 0x2b8
    CPopupController* m_popupController;                     // 0x2bc
    COverrideManager* m_overrideManager;                    // 0x2c0
    // Former CKZ field +0x2e8 was described as the local user. CK stores the profile manager here.
    CProfileManager* m_profileManager;                      // 0x2c4
    CNotificationHandler* m_notificationHandler;             // 0x2c8
    class CGServeMessageManager* m_messageManager;           // 0x2cc
    CGServeHandler* m_gserveHandler;                         // 0x2d0
    CPrivateData1* m_privateData;                            // 0x2d4 freed in Shutdown
    class CPublicData1* m_publicData;                        // 0x2d8
    class CFileData1* m_fileData;                             // 0x2dc
    int m_field2e0;                                         // 0x2e0
    SViewport m_viewport;                                   // 0x2e4 saved 3D viewport (changed by app)
    CMatrix2d m_savedTransform;                             // 0x2ec saved 2D transform
    CVector2d m_savedTranslation;                           // 0x304
    float m_orientationChangeTime;                         // 0x30c
    int m_pendingOrientation;                              // 0x310 -1: none
    vec3 m_acceleration;                                   // 0x314 in screen space
    ImageRes m_images[3];                                   // 0x320
    ImageRes m_images2[3];                                  // 0x338
    int m_softkeyWidth;                                     // 0x350
    int m_softkeyHeight;                                    // 0x354
    World m_world;     // 0x358 (changed by app: was CClass* m_field358)
    int m_paintLayer;                                       // 0x35c
    Vector<MediaRes> m_mediaCache;                          // 0x360
    Window* m_mainLayer;                                    // 0x370
    Window* m_topLayer;                                     // 0x374
    bool m_gameWasRunning;                                  // 0x378 gamerun.dat existed at start
    int m_wantedTickRate;                                   // 0x37c
    int m_tickRate;                                         // 0x380
    int m_tickRateChangeTime;                               // 0x384
    bool m_pushStatisticsSent;                             // 0x388
    float m_pushStatisticsDelay;                            // 0x38c seconds
    CNetworkAvailability* m_networkAvailability;           // 0x390
    Window* m_loginProgressDialog;                         // 0x394
    int m_field398;                                         // 0x398
    Window* m_socialConnectDialog;                          // 0x39c social network connect dialog
    bool m_field3a0;                                        // 0x3a0
    bool m_field3a1;                                        // 0x3a1
    // app: App::App clears 0x39c..0x3a4 with one memset (eight bytes, more than two stores).
    bool m_field3a2;                                        // 0x3a2 added by app
    bool m_field3a3;                                        // 0x3a3 added by app
};

/** Delays the splash screen by a few frames after the start. */
class StartDelayWindow : public WindowTransparent {
public:
    StartDelayWindow()
        : WindowTransparent(false)
    {
        m_framesLeft = 3;
    }

    virtual ~StartDelayWindow()
    {
    }

    virtual void Update()
    {
        if (m_framesLeft-- > 0) {
            return;
        }
        Close();
        Window* layer = ((App*)WindowApp::m_instance)->m_mainLayer;
        // CK 0x1743e4: always starts with the Glu logo (CK has no restart delay).
        layer->AddModal(new SplashWindow(SplashWindow::TYPE_GLU_LOGO));
    }

    int m_framesLeft;   // 0xa4
};

/** Root window of the application: black, handles the global commands. */
class DisplayWindow : public WindowColor {
public:
    /** Global commands. */
    enum {
        COMMAND_SOUND_DIALOG = (int)0x81984c34,
        COMMAND_FORWARD_9B99 = (int)0x9b991bc8,
        COMMAND_UPDATE_GAME = 0x027637f3,
        COMMAND_EXIT = 0x127c5420,
        COMMAND_EXIT_CONSUMED = 0x32ee0670,
        COMMAND_ASK_RESTART = 0x367c4113,
        COMMAND_RESTART = 0x42650932,
        COMMAND_UPGRADE = 0x45ba7539,
        COMMAND_VERSION_CHANGED = 0x6458fa9c,
        COMMAND_SWITCH_ACCOUNT = (int)0x9b9e4c2d,      // added by app
        COMMAND_INCENTIVES_REWARDED = (int)0x9937390f, // added by app
        COMMAND_SERVER_GIFT = 0x6df3535f,              // added by app
        COMMAND_ACCOUNT_CHANGED = 0x5018cd15,          // added by app
        COMMAND_SESSION_CHANGED = 0x2c80a68d           // added by app
    };

    DisplayWindow()
        : WindowColor(0xff000000)
    {
    }

    virtual ~DisplayWindow()
    {
    }

    // app: the original body is a weak copy in App.o; defined (inline) at the end of App.cpp,
    // which is the only unit that creates the window.
    inline virtual void OnCommand(Event& event);
};

#endif
