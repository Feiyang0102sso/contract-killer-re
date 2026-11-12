// CSwerveGame.h - CK ARMv7 mission window. The old CKZ candidate is preserved in recovery/legacy-game.
#ifndef CSWERVEGAME_H
#define CSWERVEGAME_H

#include "CHuntingInfo.h"
#include "CGameAIMap.h"
#include "CPathCalculator.h"
#include "CResBank.h"
#include "CSkyBox.h"
#include "CUnit.h"
#include "HashTable.h"
#include "ParticleSystem.h"
#include "WindowBase.h"
#include "tws.h"

class CTextDialog;
class CStrWChar;
class AnimationImpl;
class CVehicleType;
class CBH_PersonType;
class CBH_Player;
class CBountyResultScreen;
class CChallenge;
class CChallengeCompletedDialog;
class CCollision;
class CLevelUpWindow;
class CDH_Animation;
class CExplosiveObject;
class CFPSCursor;
class CSquad;
class CWindowQueue;
class DGCamera;
class DGCursor;
class DGScopeCursor;
class DGWorld;
class GamePlayHUD;
class GrenadeLauncher;
class IGameObject;
struct STagImage {
    /** The original constructor leaves m_width uninitialized. */
    STagImage() : m_height(0), m_pixels(NULL) {}
    ~STagImage() { delete[] m_pixels; }
    int m_width;
    int m_height;
    unsigned char* m_pixels;
};

class CSwerveGame : public Window {
public:
    typedef ::World World;
    typedef ::Graphics3D Graphics3D;

    CSwerveGame(ICRenderSurface* surface, signed char cameraId);
    virtual ~CSwerveGame();
    virtual void Init();
    virtual void Shutdown();
    virtual void OnCommand(Event& event);
    virtual void OnPointerEvent(Event& event);
    virtual void Update();
    virtual void Paint(ICGraphics2d& graphics);
    virtual void Paint3D(Graphics3D& graphics);
    virtual void OnPause();
    virtual void OnCreateState(Window** result, int state);
    virtual void OnStateInterrupt(int state, Window* window);
    virtual void Deactivate();
    virtual void Activate();
    virtual void Build();
    virtual void onStartClosing();

    void CleanUp();
    void LoadLocation();
    void SwitchOffEyeCandyGeometry();
    void FixWorldPosition();
    void DefineCollisionGeometry();
    void LoadCollision();
    void LoadObjects();
    void CreateObjectImpl(const char* pattern, const char* type);
    void LoadAIMap();
    void LoadWeapon();
    void LoadSounds();
    void InitScope();
    void CreateCursors();
    void CreateCameraAnimation();
    void CreateCameraHud3D();
    void ChangeCover();
    bool HasCoverAnimation() const;
    void DeleteCursors();
    void setCamera(int cameraId);
    int GetCurrCamId();
    int getCamerasCount();
    void setCursor(DGCursor* cursor);
    void UseScope(bool active);
    void onWeaponSwitch();
    void onSwitchToScope();  // added by weapon
    void onSwitchFromScope();  // added by weapon
    void onScopeInStart();  // added by weapon
    void SetFinalFOV(float fov, int durationMs);
    void UpdateFOV(int elapsed);
    void onScopeOutStart();  // added by weapon
    void ResetInScopeRoutine();  // added by weapon
    bool IsIntroTime() const;
    bool IsGamePaused() const;
    void SetSlowMode(bool enabled);
    void FinishGameWithResult(EBountyResult result, bool force);
    void GatherChallengeStats(EBountyResult result);
    void UpdateChallenges();
    void AddCompletedChallengesDialogs();
    void CleanupInternalEffects();
    void DeleteVehicles();
    void CreateVehicles();
    void PreliminaryUnitsSumulation();
    void CreateScene(bool reload);
    void initHunting();
    void FixPlotUnitPositon();
    /** Turn picking off for every Sprite3D below `group`. */
    void disableSpritePicking(::Group group);
    void SortUnitPickDistance(Vector<CUnit*>& units);
    CBH_PersonType* GetPersonaForBoss() const;
    CChallengeCompletedDialog* CheckForFriendCompleteChallenge(CChallenge* challenge);
    int AddLevelUpDialogs();
    CLevelUpWindow* AddLevelUpDialogInQueue(int level);
    void ShowMissionFinalDialogs();
    void ShowFailResultDialog();
    void ShowResultScreen(bool fadeIn);
    void SendMessageToTarget(bool firstTarget);
    void CheckDialogConditions();
    void ShowAmbushDialog();
    void ShowReturnToGpsConfirmation();
    bool ShowMissionVersionDialog();
    void ShowObjective();
    void ShowDialogs(int condition);
    void UpdateGameFinishing(int elapsedMs);
    void HuntingUpdate(int elapsedMs);
    void UpdateScene(int elapsedMs);
    void UpdatePopups(int elapsedMs);
    bool HandleTouchPressed(int x, int y, int pointer);
    bool HandleTouchReleased(int x, int y, int pointer);
    bool HandleTouchDragged(int x, int y, int pointer);
    void AmbientSoundsUpdate(int elapsed);
    void UpdatePeaceTime(int elapsedMs);
    void UpdateWarTime(int elapsedMs);
    void UpdateMultiplierTimer(int elapsedMs);
    void RenderEnvironment(Graphics3D& graphics);
    void LoadSkyBox();
    void InitUnits();
    void CreateUnitByName(const XString& name, EUnitMindType mindType, CSquad* squad);
    bool CanSpawnCivilians();
    void CreateCivilians();
    void PutAllSquadsInPlace();
    void CreateScriptedBoss();
    void CreateOriginalBoss();
    void CreateUnitByType(const CBH_PersonType* type, EUnitMindType mindType,
                          const XString& name, bool scripted, CSquad* squad);
    int GetAlivaUnitByType(EUnitMindType type);
    void UpdateUnits(int elapsed);
    void UpdateObjects(int elapsed);
    void UnitGarbageCollector();
    void UpdateEnvironment(int elapsed, bool updateOverlay);
    void UpdateMissionCameraTimer();
    void AvoidCollision(CUnit* unit);
    void CheckVisibility(CUnit* unit);
    void UpdateAutoAim(int elapsed);
    void UpdateHitechVisor();
    CUnit* GetUnitByGUID(int guid);
    void ShowUnitInfo(CUnit* unit, int startingDamage);
    void ShowLeftUnitPopUp(int defeated);
    void ShowUnitKillScreen(CUnit* unit);
    void OnUnitInfoScreenClosed();
    void DrawVisorDistances(ICGraphics2d& graphics);
    void AddStunTimerMS(int elapsed);
    void UnregisterAllDeadBodies();
    void DefineWinnerSide();
    void ShowReviveDialog();
    bool HasMissionAmmo();
    void ShowSpecialWeaponOffer();
    void ShowMinigunNotEnoughMoneyDialog();
    void TryToSwitchWeapon();
    void OnWeaponHided();
    void OnWeaponShown();
    void TakeSpecialWeapon();
    void ReleaseOldWeapon();
    void UpdateSpawingNewMinionForMovingCamera();
    bool IsRequiredSpawnNewMinion() const;
    void SpawnNewMinion();
    bool IsOnBoat() const;
    void PlayInjureEffect(CUnit* unit, bool armor);
    void CreateExplosionEffects();
    void PlayInternalExplosion(const vec3& position, const vec3& normal, float scale);
    void PlayInternalBodyBlast(const vec3& position);
    void CreateBodyBlastEffects();
    void SetRandomCamera();
    void DeleteScene();
    /** Post the level up story on the Facebook wall; false when not logged in. */
    bool PostFacebookMessage(int level);
    /** Post the killed story on the wall of friend `friendId`. */
    void PostFacebookMessageRandomFriend(CStrWChar friendId);
    /** Post the killed (target) or avenged story on the wall of the friend of this mission. */
    void PostFacebookMessageTargettedMission(bool killed);
    void ShowAmmoDialog();
    void OpenIngameShop();
    void RegisterUnitNeutralization(EUnitMindType type);
    void QueueResult(EBountyResult result);
    void Shot(int x, int y);
    void LaunchGrenade(const CDH_Weapon& weapon);
    void BlastGrenade(const vec3& position, const vec3& normal, const CDH_Weapon& weapon);
    void OnExplosionOccured(CExplosiveObject* object);
    void Explode(const vec3& position, int damage, int radius, const CDH_Weapon* weapon);
    void HandleGameObjectShot(CDH_Weapon* weapon, float x, float y);
    bool DefineShotProbability(EHitType hit, const CDH_Weapon* weapon, CUnit* unit);
    float GetDistanceToCollision(float x, float y);
    vec3 GetCurrentCameraPos();
    float GetDistanceToUnit(CUnit* unit);
    void SetTagImage(const XString& glyph, unsigned char* pixels, int width, int height);
    const STagImage* GetTagImage(const XString& glyph);

    static CSwerveGame* m_pInstance;

    HashTable<XString, STagImage*> m_tagImages; // 0xa4
    bool m_finishing;                         // 0xb0
    unsigned char m_reservedB1[0x0f];
    bool m_playerInjured;                     // 0xc0
    bool m_shopDamageHintShown;
    unsigned char m_reservedC2[2];
    int m_medkitsBeforeHunt;                   // 0xc4
    unsigned int m_huntStart;                 // 0xc8
    int m_huntCounter;                        // 0xcc
    unsigned int m_challengeCheckTime;        // 0xd0
    float m_previousAutoAimHorizontal;        // 0xd4
    float m_previousAutoAimVertical;          // 0xd8
    Vector<CTextDialog*> m_challengeDialogs;   // 0xdc completed challenge dialogs
    CWindowQueue* m_dialogQueue;              // 0xec
    Vector<int> m_vectorF0;                   // 0xf0 element type unknown (freed without destructors)
    DGCamera* m_camera;                       // 0x100
    World m_world;                            // 0x104
    CSkyBox m_skyBox;                         // 0x108
    CUnitBody::RayIntersection m_collisionIntersection; // 0x114
    CUnitBody::Group m_collisionGeometry;     // 0x118
    CPathCalculator m_pathCalculator;         // 0x11c
    CGameAIMap m_aiMap;                       // 0x124
    CBH_Player* m_player;                     // 0x1c0
    CDH_Animation* m_coverAnimation;         // 0x1c4
    CBountyResultScreen* m_resultScreen;       // 0x1c8
    bool m_resultsQueued;                    // 0x1cc
    unsigned char m_reserved1cd[3];
    class CUnitInfoScreen* m_unitInfo;        // 0x1d0
    int m_multiplier;                        // 0x1d4
    int m_multiplierTimeMs;                  // 0x1d8
    bool m_multiplierEnabled;                // 0x1dc
    bool m_inScopeRoutine;                   // 0x1dd
    bool m_inScopeRoutineDone;               // 0x1de
    unsigned char m_reserved1df;
    int m_levelBeforeHunt;                    // 0x1e0
    bool m_ambushDialogShown;                // 0x1e4
    bool m_reviveOffered;                    // 0x1e5
    bool m_waitingForAmmo;
    bool m_inGameShopOpen;                    // 0x1e7
    bool m_randomWeaponIntroduced;           // 0x1e8
    bool m_hitechVisor;                       // 0x1e9
    bool m_bossCreated;                       // 0x1ea
    bool m_bossRunningAway;                   // 0x1eb
    unsigned char m_reserved1ec[4];
    unsigned int m_bossEscapePopupTime;       // 0x1f0
    EBountyResult m_delayedResult;            // 0x1f4
    int m_resultDelay;                        // 0x1f8
    unsigned char m_reserved1fc[4];
    int m_reviveShieldMs;
    short m_viewport[4];                     // 0x204 x, y, width, height set by CreateScene
    DGWorld* m_environment;                   // 0x20c
    Vector<CUnit*> m_units;                   // 0x210
    Vector<CUnit*> m_pickedUnits;             // 0x220
    Vector<CUnit*> m_blastUnits;              // 0x230
    unsigned char m_reserved240[4];
    CSquad* m_squad;                         // 0x244
    bool m_slowMode;                          // 0x248
    bool m_peaceTime;                         // 0x249
    unsigned char m_reserved24a;             // 0x24a bool: set while units are pre-simulated (CUnitBody.cpp reads it)
    bool m_bossMustEscape;                    // 0x24b
    bool m_missionAlert;                      // 0x24c
    unsigned char m_reserved24d[3];
    int m_spawnedMinions;                    // 0x250
    const CSquadType* m_squadType;            // 0x254
    int m_peaceElapsed;                       // 0x258
    int m_warElapsed;                         // 0x25c
    CUnit* m_aimedUnit;                       // 0x260
    int m_aimedTimeMs;                        // 0x264
    int m_missedShots;                       // 0x268
    int m_stunMs;                            // 0x26c
    CCollision* m_collision;                 // 0x270
    GrenadeLauncher* m_grenadeLauncher;       // 0x274
    Vector<IGameObject*> m_objects;           // 0x278
    unsigned char m_reserved288[0x14];
    Vector<int> m_seenDialogConditions;        // 0x29c
    Vector<const CVehicleType*> m_vehicleTypes; // 0x2ac
    Vector<CUnitBody::Group> m_vehicles;      // 0x2bc model copies of m_vehicleTypes
    CUnitBody::Group m_cameraHud;             // 0x2cc
    unsigned char m_reserved2d0[4];
    DGCursor* m_cursor;                       // 0x2d4
    CFPSCursor* m_fpsCursor;                   // 0x2d8
    DGScopeCursor* m_scopeCursor;             // 0x2dc
    unsigned char m_reserved2e0[0x76];
    bool m_sceneReady;                       // 0x356
    GamePlayHUD* m_hud;                       // 0x358
    unsigned char m_reserved35c[4];
    int m_cameraCount;                        // 0x360
    Vector<AnimationImpl*> m_sceneAnimations; // 0x364 deleted by DeleteScene
    ParticleSystem m_particles;               // 0x374
    ICRenderSurface* m_loadingSurface;        // 0x388
    int m_loadingFadeMs;                      // 0x38c
    signed char m_cameraId;                   // 0x390
    int m_field394;
    int m_stateDelay;                        // 0x398
    int m_field39c;
    Vector<bool> m_hudVisibleStack;          // 0x3a0 HUD state saved by Deactivate
    unsigned char m_reserved3b0[1];
    bool m_exitAfterDialogs;                 // 0x3b1
    CResBank m_resources;                     // 0x3b4
    float m_sceneMinX;                       // 0x3d8 scene bounds from dummy_size_1/2
    float m_sceneMinY;                       // 0x3dc
    unsigned char m_reserved3e0[4];
    float m_sceneMaxX;                       // 0x3e4
    float m_sceneMaxY;                       // 0x3e8
    unsigned char m_reserved3ec[5];
    bool m_hideCrossInScope;                 // 0x3f1
    unsigned char m_reserved3f2[2];
    int m_dialogRepeatMs;                     // 0x3f4
    int m_slowSpeedMultiplier;                // 0x3f8
    int m_missionElapsedMs;                   // 0x3fc
    int m_coverDuration;                     // 0x400
    bool m_usedHudButton;                    // 0x404 set by GamePlayHUD::PointerPressed, counted in DeleteScene
    bool m_usedSteadyAim;                    // 0x405 same, steady aim
    unsigned char m_reserved406[6];
    bool m_dragging;                         // 0x40c
    unsigned char m_reserved40d[3];
    // candidate-only state, parked in unknown space (0x4c4 is the body blast index, 0x404 the HUD flags)
    CDH_Weapon* m_previousMissionWeapon;     // 0x410
    int m_pointerX;                          // 0x414
    int m_pointerY;                          // 0x418
    unsigned char m_reserved41c[0x4c];
    CUnitBody::Node m_explosionEffect[1];     // 0x468
    CUnitBody::Node m_bodyBlastEffects[10];   // 0x46c body blast effect pool
    CDH_Animation* m_explosionAnimation[1];   // 0x494
    CDH_Animation* m_bodyBlastAnimations[10]; // 0x498 their animations
    int m_explosionEffectIndex;              // 0x4c0
    int m_bodyBlastIndex;                    // 0x4c4 next body blast slot (0..9)
    // candidate-only state of the minigun swap; the original keeps it elsewhere (unknown yet)
    int m_previousMissionAmmo;               // 0x4c8
    CDH_Weapon* m_specialWeapon;             // 0x4cc
    int m_specialWeaponState;                // 0x4d0
    CDH_Weapon* m_loadedMissionWeapons[2];    // 0x4d4
    int m_pendingMovingMinions;              // 0x4dc
    // FOV transition set by SetFinalFOV
    float m_finalFOV;                        // 0x4e0
    float m_fovSpeed;                        // 0x4e4
    int m_fovDurationMs;                     // 0x4e8
    int m_fovElapsedMs;                      // 0x4ec
};

#endif
