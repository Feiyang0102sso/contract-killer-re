#ifndef CBH_GPSMAP_H
#define CBH_GPSMAP_H

#include "PointerEventController.h"
#include "WindowBase.h"
#include "CBaseScreen.h"
#include "CDH_Weapon.h"
#include "CShopWindow.h"
#include "CShopItemInfoWindow.h"
#include "gps/CGPSHaunt.h"
#include "gps/CGPSHauntCamera.h"
#include "gps/CGPSCooperation.h"
#include "gps/CGPSInformantMisson.h"
#include "gps/CGPSInstantInformant.h"
#include "gps/CGPSItemBar.h"
#include "gps/CGPSNetwork.h"
#include "gps/CGPSShop.h"
#include "gps/CGPSZombiesPromo.h"

class TiXmlElement;
class CLocation;
class CWindowQueue;
class CBH_GPSHud;
class CBH_InformantsHud;
class CBeforeTargettingDialog;
class CBeforeHuntDialog;
class CBH_PersonType;
class CFriendGatedMission;
class CGPSBounty;
class CGPSHuntingItem;
class CDH_Weapon;
class CChallenge;
class CBH_GPSScan;
class CDialogWindow;
struct SMissionInviteMessage;

/** Tile grid and pointer handling for the CK GPS map. */
class CBH_GPSMap : public CBaseScreen, public CGPSHauntListener, public CGPSCameraListener,
                   public IGPSMissionInformantListener, public CGPSShopListener, public CGPSNetworkListener,
                   public CGPSInstantInformantListener, public CGPSBarListener,
                   public CGPSZombiesPromoListener, public CGPSCGPSCooperationListener {
public:
    /** One scheduled local push: text and delay in seconds. */
    struct SLocalPush {
        SLocalPush(XString text, int time)
        {
            m_text = text;
            m_time = time;
        }

        /** QuickSort comparator: later pushes first. */
        static int cmp(const void* left, const void* right)
        {
            return static_cast<const SLocalPush*>(left)->m_time > static_cast<const SLocalPush*>(right)->m_time;
        }

        XString m_text;   // 0x00
        int m_time;       // 0x04
    };

    CBH_GPSMap();
    virtual ~CBH_GPSMap();
    virtual void Paint(ICGraphics2d& graphics);
    virtual void Update();
    virtual void OnCommand(Event& event);
    virtual void Init();
    virtual void Shutdown();
    virtual void OnGetFocus();
    virtual void OnPause();
    virtual void OnEnterBackground();
    // Listener overrides in CK primary-vtable order (slots 48..62).
    virtual void NeedSave();
    virtual void OnHauntSelected(CGPSHaunt* haunt);
    virtual void OnCameraSelected(CGPSHauntCamera* camera);
    virtual void OnGpsMissionInformantSelected(CGPSMissionInformant* informant);
    virtual void OnShopItemSelected(CGPSShop* shop);
    virtual void OnGPSNetworkSelected(CGPSNetwork* network);
    virtual void OnGPSInstantInformantSelected(CGPSInstantInformant* informant);
    virtual void OnBarItemSelected(CGPSBar* bar);
    virtual void OnBarVisible();
    virtual void OnCoopVisible();
    virtual void OnTargetMissionsVisible();
    virtual void OnVehicleMissionsVisible();
    virtual void PlaceSubContract(CGPSSubContract* contract, unsigned int radius);
    virtual void OnZombiesItemSelected(CGPSZombiesPromo* promo);
    virtual void OnCooperationItemSelected(CGPSCooperation* cooperation);

    void LoadTiles();
    void SetTileLayout(const TiXmlElement& map);
    void AddMapPatch(ICRenderSurface* surface);
    void OnPointerEvent(PointerEventController::Event event);
    void CheckMapPosBounds();
    void CreateHaunts();
    void CreateShop();
    void CreateNetwork();
    void CreateInstantInformants();
    void CreateInformantsImpl(Vector<CGPSMissionInformant*>* informants, const wchar_t* path);
    void CreateAlternativeStoryLineInformants();
    void UpdateMissionInformants();
    void HideAllInformantsImpl(Vector<CGPSMissionInformant*>* informants);
    void UnhideInformantImpl(Vector<CGPSMissionInformant*>* informants);
    void ShowMissionStartDialogs(int missionId);
    void MakeDailyDealCheck();
    void CheckGiftReceivedNews();
    void CheckChallengeNews();
    void CheckForNewChallenges();
    void ShowDailyDealWindow();
    bool StartTargettingMission(int friendId, bool revenge);
    void Load();
    void Save();
    CGPSHauntCamera* GetCameraById(int id);
    CGPSInstantInformant* FindInstantInformant(CGPSInstantInformant::Type type, bool weaponGated) const;
    bool IsMovingCameraMissionsFinished() const;
    void StartTutorialMission();
    void StartHunt(CGPSHauntCamera* camera, const CLocation* location, bool useScript);
    void ShowShop(CShopWindow::ETab tab, bool openTapJoy);
    bool WillInformantUnlockOnLevelUp();
    void CancelInstantInformantWithCamera(int cameraId);
    void SetupLocalPushNotifications();
    void InitPrefetchedManager();
    bool LoadPrefetchedManagerResource();
    void ReleasePrefetchedManagerResource();
    void UpdateMapItems();
    void ShowLowMoneyDialog();
    void ShowMissionPriorDialogs(int missionId);
    void ShowBeforeHuntDialog();
    void FillBeforeHuntInfo(const CGPSHuntingItem* item);
    void FillBeforeTargetHuntInfo(const CGPSBounty* bounty);
    void FillBeforeHuntInfo(const CGPSHuntingItem* item, const char* image, ICRenderSurface* surface);
    static void ShowShopWithWeapon(const CDH_Weapon& weapon);
    static void ShowShopWithUnlockedWeapon(int weaponId, CShopItemInfoWindow::ESource source);
    static void CheckGameCenterAccounts();
    static void ShowShopWithItem(CItemManager* manager, CShopWindow::ETab tab, CItem::EItemTypes type, int id);
    bool ShowNextQueueDialog(bool animate);
    void FlushDialogsQueue();
    void ShowCoopWindow();
    void OnGPSSocialNetworkSelected();
    void CollectSubContractsNotifications(Vector<SLocalPush>& pushes);

    static bool m_isFirstLoading;
    static CBH_GPSMap* m_instance;

private:
    CGPSItem* FindItem(int x, int y);
    static bool IsItemIntersects(const CGPSItem& item, int left, int top, int right, int bottom);
    static bool IsRegionFree(int left, int top, int right, int bottom, const Vector<const CGPSItem*>& items);
    void FilterRegionItems(int left, int top, int right, int bottom, Vector<const CGPSItem*>& items) const;
    void ShowMissionDialog();
    CBeforeHuntDialog* CreateBeforeHuntDialog(bool allowLater);
    void PrepareHuntBrief(const CGPSHauntCamera& camera, const CGPSHuntingItem& item);
    void SetWeaponInBothHands(CDH_Weapon* weapon);
    void SetWeaponWithIDInBothHands(int id);
    void SetWeaponWithCategoryInBothHands(const XString& category);
    void ShowBeforeFriendsGatedMission();
    void StartNextFriendGatedMission();
    CGPSHaunt* GetHauntByName(const XString& name);
    void ShowBountyPriorDialogs(const CGPSBounty* bounty, const XString& hauntName, CFont* font);
    void ShowBountyPriorDialogs(const XString& message, const XString& title, CFont* font);
    void ShowMessage(XString title, XString message, const char* image, CFont* font);
    CBeforeTargettingDialog* CreateBeforeTargettingDialog(bool allowLater);
    bool MakePlayerAmmoCheck();
    bool MakePlayerWeaponCheck(int missionId);
    bool MakeLowAmmoCheck(const CDH_Weapon* weapon);
    bool MakePlayerEnergyCheck(int cost);
    bool MakePlayerTranqMissionCheck();
    bool MakePlayerWeaponRangeCheck(float range);
    bool MakeSilentWeaponCheck();
    bool MakePlayerWeaponGatedCategoryCheck(CDH_Weapon::EWeaponCategory category);
    bool MakePlayerWeaponGatedIDCheck(int id);
    CDH_Weapon* GetMostExpensiveWeaponInCategory(int category) const;
    int GetMostExpensiveWeaponId(const Vector<int>& ids);
    bool CheckMissionEquipment(CGPSHauntCamera* camera, CGPSHuntingItem* item);
    void ShowBuyInformantDialog(CGPSInstantInformant* informant);
    void ShowInformantIsSearchingDialog(CGPSInstantInformant* informant);
    void ResolveInformantPurchase(CGPSInstantInformant* informant);
    CGPSHauntCamera* GetCameraForBlueInformant();
    CGPSHauntCamera* GetRandomCamera(bool unlocked, bool movingOnly);
    CGPSHauntCamera* GetRandomCamera();
    CGPSHauntCamera* GetMovingRandomCamera() const;
    void CreateBar();
    void CreateCooperationIcon();
    void CreateZombiesPromoIcon();
    void ScrollToItem(const CGPSItem& item, float speed);
    void UpdateScroll();
    void ShowWhatsNewDialog();
    void ShowRocketLauncherInroduce();
    void HandleBonuses(int elapsed);
    void ShowFiveDaysBonusMessage();
    void ShowFiveDaysIncentiveBonusExpirationMessage();
    void ShowFiveDaysBonusTeaser();
    void ShowFiveDaysIncentivisedBonusMessage();
    void ShowFiveDaysInventiveBonusRecomendedItem();
    void RequestTargetPicker();
    void ShowSelectFriendDialog();
    bool QueueChallengeNewsDialog(CChallenge* challenge, int sender, int count, bool completed);

public:
    // CK layout, offsets from CBH_GPSMap::CBH_GPSMap() (0xe77f0) and member accesses.
    // Base CBaseScreen is 0xd8 bytes; the nine listener bases sit at 0xd8..0xf8.
    CWindowQueue* m_startDialogs;                       // 0xfc
    bool m_showFeaturedInstall;                         // 0x100 (ctor: true) Load
    int m_backgroundCount;                              // 0x104 OnEnterBackground
    Vector<ICRenderSurface*> m_tiles;                   // 0x108
    CBH_GPSHud* m_hud;                                  // 0x118 Init
    int m_currentChallengeDay;                          // 0x11c (ctor: -1)
    int m_mapWidth;                                     // 0x120 LoadTiles
    int m_mapHeight;                                    // 0x124 LoadTiles
    int m_columns;                                      // 0x128
    int m_rows;                                         // 0x12c
    int m_mapX;                                         // 0x130 LoadTiles
    int m_mapY;                                         // 0x134 LoadTiles
    int m_dragOriginX;                                  // 0x138 OnPointerEvent (map X at press)
    int m_dragOriginY;                                  // 0x13c
    int m_field140;                                     // 0x140
    bool m_field144;                                    // 0x144
    bool m_showRocketLauncherIntro;                     // 0x145 Load
    bool m_draggingMap;                                 // 0x146 OnPointerEvent
    unsigned char m_unknown147[0x150 - 0x147];          // 0x147 not yet mapped
    bool m_field150;                                    // 0x150
    Vector<int> m_vector154;                            // 0x154 element type unknown
    Vector<int> m_vector164;                            // 0x164 element type unknown
    int m_field174;                                     // 0x174
    int m_pendingCommand;                               // 0x178 ShowLowMoneyDialog
    unsigned int m_lastDailyDealDisplay;                // 0x17c GetTimeSeconds
    bool m_field180;                                    // 0x180
    int m_field184;                                     // 0x184
    int m_field188;                                     // 0x188
    bool m_field18c;                                    // 0x18c
    bool m_field18d;                                    // 0x18d
    CGPSHauntCamera* m_selectedCamera;                  // 0x190 FillBeforeHuntInfo
    int m_field194;                                     // 0x194
    int m_field198;                                     // 0x198
    int m_field19c;                                     // 0x19c
    CBH_GPSScan* m_gpsScan;                             // 0x1a0
    bool m_field1a4;                                    // 0x1a4
    Vector<CDialogWindow*> m_dialogsQueue;              // 0x1a8 ShowNextQueueDialog
    int m_scrollState;                                  // 0x1b8 UpdateScroll / OnPointerEvent
    float m_scrollVelocityX;                            // 0x1bc UpdateScroll (fling, state 3)
    float m_scrollVelocityY;                            // 0x1c0
    float m_scrollSpeedX;                               // 0x1c4 UpdateScroll (move to target, state 2)
    float m_scrollSpeedY;                               // 0x1c8
    float m_scrollTargetX;                              // 0x1cc
    float m_scrollTargetY;                              // 0x1d0
    float m_scrollX;                                    // 0x1d4 CheckMapPosBounds
    float m_scrollY;                                    // 0x1d8 CheckMapPosBounds
    float m_previousScrollX;                            // 0x1dc
    float m_previousScrollY;                            // 0x1e0
    bool m_recordedMapDrag;                             // 0x1e4 (ctor: true) drag counted in stats
    Vector<CGPSItem*> m_items;                          // 0x1e8 every map item
    Vector<CGPSMissionInformant*> m_missionInformants[5]; // 0x1f8
    Vector<CGPSInstantInformant*> m_instantInformants;  // 0x248
    Vector<CGPSHaunt*> m_haunts;                        // 0x258
    Vector<CGPSItem*> m_clickableItems;                 // 0x268 hit-test list (haunts, cameras, informants)
    CBH_InformantsHud* m_shortcuts;                     // 0x278
    CGPSInstantInformant* m_informantType4;             // 0x27c CreateInstantInformants (type 4)
    CGPSInstantInformant* m_weaponGatedInformant;       // 0x280 CreateInstantInformants
    CGPSInstantInformant* m_informantType5;             // 0x284 CreateInstantInformants (type 5)
    int m_field288;                                     // 0x288
    CGPSCooperation* m_cooperation;                     // 0x28c
    CGPSBar* m_bar;                                     // 0x290
    CGPSShop* m_shop;                                   // 0x294
    CGPSNetwork* m_network;                             // 0x298
    CGPSItem* m_pressedItem;                            // 0x29c OnPointerEvent
    int m_field2a0;                                     // 0x2a0
    int m_field2a4;                                     // 0x2a4
    XString m_friendGatedBossName;                      // 0x2a8
    bool m_needSave;                                    // 0x2ac NeedSave
    int m_field2b0;                                     // 0x2b0
    bool m_openFriendsWindow;                           // 0x2b4 ShowCoopWindow
    bool m_field2b5;                                    // 0x2b5 cleared by Init
    Vector<CItemManager*> m_prefetchedManagers;         // 0x2b8
    int m_prefetchIndex;                                // 0x2c8
    int m_socialNetworkMode;                            // 0x2cc (ctor: 1, ShowCoopWindow: 2)

    // Members whose CK offsets are not mapped yet (kept after the known layout).
    CGPSInstantInformant* m_selectedInformant;
    int m_priorDialogIndex;
    bool m_advanceMissionDialog;
    bool m_showingHuntConfirmation;
    bool m_acceptInformant;
    bool m_showTutorialFinish;
    bool m_dailyDealShown;
    unsigned int m_nextGiftSaveAttempt;
    CGPSZombiesPromo* m_zombiesPromo;
    unsigned int m_nextChallengeSaveAttempt;
    int m_mapIdleElapsed;
    int m_bonusTeaserElapsed;
    bool m_scrollToItem;
    bool m_openTargetPickerAfterLogin;
    const CFriendGatedMission* m_friendGatedMission;
    CBH_PersonType* m_friendGatedBoss;

public:
    static bool safehouseIsPainting; // added by ui
    static bool weaponIsPainting; // added by ui
    static bool shopIsPainting; // added by ui
};

#endif
