#ifndef GAMEPLAYHUD_H
#define GAMEPLAYHUD_H

#include "Collision.h"
#include "WindowBase.h"
#include "tws.h"

class CAmmo;
class CDH_Weapon;
class CBH_GPSHud;
class CBH_ProgressBar;
class CBH_TriggerButton;
class CDH_GameSofkeyWidget;
class CImageButton;
class CZoomSliderControl;

// Layout from the constructor 0x14c6b8 (object size 0x330).
class GamePlayHUD : public Window {
public:
    GamePlayHUD();
    virtual ~GamePlayHUD();
    virtual void Init() {}
    virtual void OnCommand(Event& event);
    virtual void Update();
    virtual void Paint(ICGraphics2d& graphics);

    void InitFireButton();
    void InitWeaponButton();
    void InitIngameShopButton();
    void InitSteadyButton();
    void InitScopeButton();
    void InitHealthReplehishButton();
    void InitGoPillButton();
    void InitUltrasoundEmitterButton();
    void InitCameraButton();
    void InitReloadButton();
    void InitZoomAndDock();
    void InitBars();
    void setCamerasCount(int count);
    void ShowCountdownTimer(bool show);
    void SetCountdownTimerValue(int elapsedMs);
    void SetCountdownTimerLimit(int limitMs);
    void SetCountdownTimerMultiplier(int multiplier);
    void SetMissionTimerValue(int elapsedMs);
    void CleanWeaponImage();
    void UpdateHUDsVisibility();
    void UpdateButtonPosition();
    void processModelAnimation();
    void switchWeapons();
    void switchScope();
    void DrawWeaponButton(CDH_Weapon* weapon);
    void DrawSteadyButton();
    void DrawScopeButton();
    void DrawReloadButton(CDH_Weapon* weapon);
    void DrawFireButton();
    void DrawHealthReplenishButton();
    void DrawGoPillButton();
    void DrawUnltrasoundEmitterButton();
    void Shot();
    void PlayEnjureEffect(float angle, bool armor);
    void PlayReviveEffect(float delay);
    int PointerPressed(int x, int y, int pointer);
    void CheckContollStateOnPointerReleased(int x, int y, int pointer);
    void CheckContollStateOnPointerMove(int x, int y, int oldX, int oldY, int pointer);
    void Show();
    void Hide(bool immediate);
    void SetZoomSliderEnable(bool enable);
    float GetZoomSliderProgress() const;
    void ReInitWeaponButton();
    void CleanBulletIcon();
    void ReInitBulletIcon(CDH_Weapon* weapon);
    bool isWeaponModelVisible() const;
    void OnScopeSwitched();
    void EnableDockCoverButton(bool enable);
    void SetDockCoverButtonVisibility(bool hide);
    void ChangeDockCoverButtonState();
    int GetActiveWeaponIndex();
    void SetEnabled(bool enabled);
    void BlinkIngameShop(int duration);

    // Accessors used by CSwerveGame (no out-of-line copies in the original object).
    void SetFireHeld(bool held) { m_fireHeld = held; }
    bool IsFireHeld() const { return m_fireHeld; }
    bool IsFirePointer(int pointer) const { return m_fireHeld && m_firePointer == pointer; }
    void ResetFireButton()
    {
        m_fireHeld = false;
        m_firePointer = -1;
    }
    bool IsModelAnimationActive() const { return m_modelAnimationState != 0; }
    bool IsSteadyAimActive() const { return m_steadyActive; }
    bool IsEnabled() const { return m_enabled; }
    void EnableSceneCommands() { m_commandsReady = true; }

    /** Tutorial hint image and its display timer. The destructor releases the named resource. */
    struct TutorialHint {
        inline ~TutorialHint();
        ICRenderSurface* m_image;  // 0x00
        const char* m_imageName;    // 0x04
        unsigned int m_duration;   // 0x08
        unsigned int m_elapsed;    // 0x0c
    };

    class ReviveWindow;
    class CZoomedScopeWindow;
    class WoundingWindow;

    const CAmmo* m_ammo;                   // 0xa4 ammo whose bullet icon is loaded
    bool m_inputEnabled;                   // 0xa8
    bool m_scoped;                         // 0xa9
    bool m_showMedkit;                     // 0xaa
    bool m_showUltrasound;                 // 0xab
    bool m_hasAlternateWeapon;             // 0xac
    bool m_scopeButtonAlways;              // 0xad cleared by CSwerveGame on platform 0
    bool m_scopeButtonShown;               // 0xae written by DrawScopeButton
    CImageButton* m_ingameShop;            // 0xb0 owned by the window tree
    ICRenderSurface* m_weaponSwitch;       // 0xb4
    ICRenderSurface* m_weaponImages[2];    // 0xb8 owned by the weapons
    ICRenderSurface* m_steadyOn;           // 0xc0
    ICRenderSurface* m_steadyOff;          // 0xc4
    ICRenderSurface* m_scopeEnter;         // 0xc8
    ICRenderSurface* m_scopeExit;          // 0xcc
    ICRenderSurface* m_bulletIcon;         // 0xd0
    ICRenderSurface* m_silencerIcon;       // 0xd4
    ICRenderSurface* m_reload;             // 0xd8
    ICRenderSurface* m_medkit;             // 0xdc
    ICRenderSurface* m_tablet;             // 0xe0
    ICRenderSurface* m_ultrasound;         // 0xe4
    ICRenderSurface* m_fireOn;             // 0xe8
    ICRenderSurface* m_fireOff;            // 0xec
    ICRenderSurface* m_tapToShoot;         // 0xf0
    ICRenderSurface* m_aimAssistIcon;      // 0xf4
    ICRenderSurface* m_cameraFrame;        // 0xf8
    ICRenderSurface* m_cameraFrameSelected; // 0xfc
    Vector<ICRenderSurface*> m_cameraImages; // 0x100 released by the destructor
    int m_scopeButtonWidth;                // 0x110
    int m_field114;                        // 0x114 not initialized
    Collision m_weaponSwitchArea;          // 0x118
    Collision m_weaponAreas[2];            // 0x13c
    Collision m_scopeArea;                 // 0x184
    Collision m_fireArea;                  // 0x1a8
    Vector<Collision> m_cameraButtonAreas; // 0x1cc
    Collision m_medkitArea;                // 0x1dc
    Collision m_tabletArea;                // 0x200
    Collision m_ultrasoundArea;            // 0x224
    int m_field248;                        // 0x248 not initialized
    CBH_TriggerButton* m_coverButton;      // 0x24c
    CZoomSliderControl* m_zoomSliderControl; // 0x250
    CBH_ProgressBar* m_healthBar;          // 0x254
    CBH_ProgressBar* m_armorBar;           // 0x258
    CBH_ProgressBar* m_multiplierBar;      // 0x25c
    CBH_ProgressBar* m_missionTimer;       // 0x260
    CBH_GPSHud* m_experienceHud;           // 0x264
    CDH_GameSofkeyWidget* m_pauseButton;   // 0x268
    bool m_showCountdownTimer;             // 0x26c
    int m_field270;                        // 0x270 not initialized
    bool m_field274;                       // 0x274
    int m_firePointer;                     // 0x278
    bool m_fireHeld;                       // 0x27c
    bool m_commandsReady;                  // 0x27d
    CZoomedScopeWindow* m_zoomedScope;     // 0x280 owned separately from the window tree
    WoundingWindow* m_woundingWindow;      // 0x284
    WoundingWindow* m_armorHitWindow;      // 0x288
    ReviveWindow* m_reviveWindow;          // 0x28c child window
    Collision m_steadyArea;                // 0x290
    int m_steadyRemainingMs;               // 0x2b4
    int m_steadyLimitMs;                   // 0x2b8
    bool m_steadyActive;                   // 0x2bc
    float m_visibilityAlpha;               // 0x2c0
    bool m_enabled;                        // 0x2c4
    Vector<TutorialHint*> m_tutorialHints; // 0x2c8
    TutorialHint* m_currentHint;           // 0x2d8
    bool m_field2dc;                       // 0x2dc
    int m_field2e0;                        // 0x2e0
    Collision m_reloadButtonArea;          // 0x2e4
    Collision m_reloadTouchArea;           // 0x308
    int m_modelAnimationState;             // 0x32c set by InitSteadyButton
};

#endif
