#ifndef GAMEPLAYHUD_H
#define GAMEPLAYHUD_H

#include "Collision.h"
#include "WindowBase.h"
#include "tws.h"

class CAmmo;
class CDH_Weapon;
class CBH_ProgressBar;
class CBH_TriggerButton;
class CDH_GameSofkeyWidget;
class CImageButton;
class CZoomSliderControl;

class GamePlayHUD : public Window {
public:
    GamePlayHUD();
    virtual ~GamePlayHUD();
    virtual void Init() {}
    virtual void OnCommand(Event& event);
    virtual void Update();
    virtual void Paint(ICGraphics2d& graphics);

    void InitFireButton();
    void InitScopeButton();
    void InitReloadButton();
    void InitCameraButton();
    void setCamerasCount(int count);
    void InitWeaponButton();
    void InitZoomAndDock();
    void InitBars();
    void ShowCountdownTimer(bool show);
    void SetCountdownTimerValue(int elapsedMs);
    void SetCountdownTimerLimit(int limitMs);
    void SetCountdownTimerMultiplier(int multiplier);
    void SetMissionTimerValue(int elapsedMs);
    void DrawFireButton();
    void DrawScopeButton();
    void DrawReloadButton(CDH_Weapon* weapon);
    void DrawWeaponButton(CDH_Weapon* weapon);
    bool PaintReviveEffect(Graphics3D& graphics);
    void PlayReviveEffect(float delay);
    void processModelAnimation();
    bool IsModelAnimationActive() const;
    void ReInitBulletIcon(CDH_Weapon* weapon);
    bool isWeaponModelVisible() const;
    int GetActiveWeaponIndex();
    void CheckForAutomaticWeaponSwitch(CDH_Weapon* weapon);
    void switchScope();
    void switchWeapons();
    float GetZoomSliderProgress() const;
    int PointerPressed(int x, int y, int pointer);
    void CheckContollStateOnPointerReleased(int x, int y, int pointer);
    void CheckContollStateOnPointerMove(int x, int y, int oldX, int oldY, int pointer);
    void ResetFireButton();
    bool IsFirePointer(int pointer) const;
    bool IsFireHeld() const;
    bool IsSteadyAimActive() const;
    bool HasUsedSteadyAim() const;
    void SetEnabled(bool enabled);
    void Show();
    /** Whether the HUD is shown (added by swerve). */
    bool IsEnabled() const { return m_enabled; }
    void Hide(bool immediate);
    void SetZoomSliderEnable(bool enable);
    void OnScopeSwitched();
    void Shot();
    void PlayEnjureEffect(float angle, bool armor);
    void EnableDockCoverButton(bool enable);
    void BlinkIngameShop(int duration);
    void SetDockCoverButtonVisibility(bool hide);
    void ChangeDockCoverButtonState();

    /** Tutorial hint image and its display timer. The destructor releases the named resource. */
    struct TutorialHint {
        inline ~TutorialHint();
        ICRenderSurface* m_image;  // 0x00
        const char* m_imageName;    // 0x04
        unsigned int m_duration;   // 0x08
        unsigned int m_elapsed;    // 0x0c
    };

private:
    class ReviveWindow;
    class CZoomedScopeWindow;
    class WoundingWindow;
    bool ContainsFireButton(int x, int y) const;
    void InitSteadyButton();
    void InitIngameShopButton();
    void InitHealthReplehishButton();
    void InitGoPillButton();
    void InitUltrasoundEmitterButton();
    void DrawSteadyButton();
    void DrawHealthReplenishButton();
    void DrawGoPillButton();
    void DrawUnltrasoundEmitterButton();
    void UpdateButtonPosition();
    void UpdateAmmoIcon(const CAmmo* ammo);
    void CleanBulletIcon();
    void ReInitWeaponButton();
    void CleanWeaponImage();
    void UpdateHUDsVisibility();
    void PaintBars(ICGraphics2d& graphics);

    const CAmmo* m_displayedAmmo;        // 0xa4, borrowed from the ammo manager
    unsigned char m_reservedA8;
    bool m_scoped;                     // 0xa9
    bool m_showMedkit;                 // 0xaa
    bool m_showUltrasound;             // 0xab
    bool m_hasAlternateWeapon;        // 0xac
    unsigned char m_reservedAD[3];
    CImageButton* m_ingameShop;        // 0xb0, owned by the window tree
    ICRenderSurface* m_weaponSwitch;   // 0xb4
    ICRenderSurface* m_weaponImages[2]; // 0xb8
    ICRenderSurface* m_steadyOn;        // 0xc0
    ICRenderSurface* m_steadyOff;
    ICRenderSurface* m_scopeEnter;     // 0xc8
    ICRenderSurface* m_scopeExit;
    ICRenderSurface* m_bulletIcon;     // 0xd0
    ICRenderSurface* m_silencerIcon;   // 0xd4
    ICRenderSurface* m_reload;         // 0xd8
    ICRenderSurface* m_medkit;          // 0xdc
    ICRenderSurface* m_reactix;         // 0xe0
    ICRenderSurface* m_ultrasound;      // 0xe4
    ICRenderSurface* m_fireOn;          // 0xe8
    ICRenderSurface* m_fireOff;         // 0xec
    ICRenderSurface* m_tapToShoot;      // 0xf0
    ICRenderSurface* m_aimAssistIcon;  // 0xf4
    ICRenderSurface* m_cameraFrame;     // 0xf8
    ICRenderSurface* m_cameraFrameSelected;
    CDH_Weapon* m_weaponImagesFor[2]; // 0x100
    unsigned char m_reserved108[0x10];
    CRectangle m_weaponBounds;
    unsigned char m_reserved120[0x78];
    CRectangle m_scopeBounds;          // 0x198
    unsigned char m_reserved1a0[8];
    Collision m_fireHitArea;           // 0x1a8, bounds at 0x1bc
    Vector<Collision> m_cameraButtonAreas; // 0x1cc
    // fps: consumable buttons are Collision areas (bounds at +0x14), as DrawNumberWithinThe expects.
    Collision m_medkitArea;             // 0x1dc, bounds at 0x1f0
    Collision m_reactixArea;            // 0x200, bounds at 0x214
    Collision m_ultrasoundArea;         // 0x224, bounds at 0x238
    unsigned char m_reserved248[0x04];
    CBH_TriggerButton* m_coverButton;   // 0x24c
    CZoomSliderControl* m_zoomSliderControl; // 0x250
    CBH_ProgressBar* m_healthBar;       // 0x254
    CBH_ProgressBar* m_armorBar;        // 0x258
    CBH_ProgressBar* m_multiplierBar;   // 0x25c
    CBH_ProgressBar* m_missionTimer;    // 0x260
    unsigned char m_reserved264[4];
    CDH_GameSofkeyWidget* m_pauseButton; // 0x268
    bool m_showCountdownTimer;         // 0x26c
    unsigned char m_reserved26d[0x0b];
    int m_firePointer;                 // 0x278
    bool m_fireHeld;                   // 0x27c
    unsigned char m_reserved27d[3];
    CZoomedScopeWindow* m_zoomedScope;  // 0x280, owned separately from the window tree
    WoundingWindow* m_woundingWindow;  // 0x284
    WoundingWindow* m_armorHitWindow;  // 0x288
    unsigned char m_reserved28c[0x14];
    int m_displayedMultiplier;
    ReviveWindow* m_reviveWindow;
    // 0x2a8 was previously occupied by the candidate's standalone scope model.
    CRectangle m_steadyBounds;
    unsigned char m_reserved2b0[4];
    int m_steadyRemainingMs;          // 0x2b4
    int m_steadyLimitMs;
    bool m_steadyActive;
    bool m_steadyUsed;
    unsigned char m_reserved2be[2];
    float m_visibilityAlpha;           // 0x2c0
    bool m_enabled;                    // 0x2c4
    unsigned char m_reserved2c5[3];
    Vector<TutorialHint*> m_tutorialHints; // 0x2c8 (fps)
    TutorialHint* m_currentHint;       // 0x2d8 (fps)
    unsigned char m_reserved2dc[8];
    Collision m_reloadButtonArea;      // 0x2e4, bounds at 0x2f8
    Collision m_reloadTouchArea;       // 0x308
    int m_modelAnimationState;         // 0x32c
};

#endif
