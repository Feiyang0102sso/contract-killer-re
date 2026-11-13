#include "GamePlayHUD.h"
#include "CApplet.h"
#include "ICResourceManager.h"

#include <math.h>
#include <string.h>
#include "App.h"
#include "AppSettings.h"
#include "CAmmo.h"
#include "CBH_GPSHud.h"
#include "CBH_Player.h"
#include "CBH_ProgressBar.h"
#include "CBH_SurfaceManager.h"
#include "CBH_TriggerButton.h"
#include "CDH_SoundManager.h"
#include "CDH_BasicGameData.h"
#include "CDH_GameSoftkeyWidget.h"
#include "CDH_PlayerData.h"
#include "CDH_Weapon.h"
#include "CFpsCursor.h"
#include "CZoomSliderControl.h"
#include "CSwerveGame.h"
#include "CHuntingInfo.h"
#include "CGameAnalytics.h"
#include "CIngameShop.h"
#include "CFont.h"
#include "CMath.h"
#include "CFontMgr.h"
#include "CImageButton.h"
#include "CSwerve.h"
#include "DGHelper.h"
#include "dgCamera.h"
#include "ICRenderSurface.h"
#include "SScreenAdaptingConstant.h"
#include "ServiceWindows.h"
#include "SwerveHelper.h"
#include "ICStdUtil.h"

// Scope view of the zoomed weapon (0xe4 bytes), drawn by GamePlayHUD::Paint.
class GamePlayHUD::CZoomedScopeWindow : public ViewWindow {
public:
    CZoomedScopeWindow();
    virtual ~CZoomedScopeWindow();
    void InitResources(CDH_Weapon* weapon);
    virtual void Update();
    virtual void Paint3D(Graphics3D& graphics);

    SwerveAnimations m_animations;           // 0xa8
    SwerveAnimations::Animation* m_shoot;   // 0xbc
    SwerveAnimations::Animation* m_zoom;    // 0xc0
    float m_zoomPosition;                   // 0xc4
    float m_shotPosition;                   // 0xc8
    float m_zoomRecoil;                     // 0xcc
    int m_weaponId;                         // 0xd0
    int m_reservedD4;                       // 0xd4
    int m_worldTime;                        // 0xd8
    int m_shotTime;                         // 0xdc
    bool m_shooting;                        // 0xe0
    bool m_zoomEnabled;                     // 0xe1
};

// Hit direction effect (0xb4 bytes); a child of the HUD.
class GamePlayHUD::WoundingWindow : public ViewWindow {
public:
    explicit WoundingWindow(bool armor);
    void StartEffect(float angle);
    void StopEffect();
    virtual void Update();

    int m_remainingMs;                // 0xa8
    SwerveHelper::Group m_screen;     // 0xac
    SwerveHelper::Group m_arrow;      // 0xb0
};

// Phoenix serum effect (0xb4 bytes); a child of the HUD.
class GamePlayHUD::ReviveWindow : public ViewWindow {
public:
    ReviveWindow();
    void StopEffect();
    virtual void Update();

    int m_remainingMs;                // 0xa8
    SwerveHelper::Group m_controller; // 0xac
    bool m_soundPlayed;               // 0xb0
};

GamePlayHUD::ReviveWindow::ReviveWindow()
    : ViewWindow(World()), m_remainingMs(0)
{
    m_world = World(App::LoadObject3D("BIN_PHOENIX_SERUM"));
    m_world.getBackground().setColorClearEnable(false);
    m_controller = SwerveHelper::Group(m_world.find(DGHelper::getSwerveID("weaponControllerTransform")));
}

void GamePlayHUD::ReviveWindow::StopEffect()
{
    m_remainingMs = 0;
    if (m_controller != NULL) {
        m_controller.animate(0);
    }
}

void GamePlayHUD::ReviveWindow::Update()
{
    if (m_remainingMs > 0) {
        m_remainingMs -= WindowApp::m_instance->m_updateFrameTime;
        if (m_controller != NULL) {
            m_controller.animate(5000 - m_remainingMs);
        }
        if (!m_soundPlayed && m_remainingMs < 3200) {
            static_cast<App*>(WindowApp::m_instance)->m_soundManager->PlaySound("IDM_SPECIAL_REVIVE", false, false);
            m_soundPlayed = true;
        }
        if (m_remainingMs < 1) {
            StopEffect();
        }
    }
}

GamePlayHUD::GamePlayHUD()
    : Window(false), m_inputEnabled(true), m_scoped(false), m_showMedkit(false), m_showUltrasound(false),
      m_hasAlternateWeapon(true), m_scopeButtonAlways(true),
      m_steadyOn(NULL), m_steadyOff(NULL), m_scopeEnter(NULL), m_scopeExit(NULL), m_bulletIcon(NULL),
      m_silencerIcon(NULL), m_medkit(NULL), m_tablet(NULL), m_ultrasound(NULL),
      m_aimAssistIcon(NULL), m_cameraFrame(NULL), m_cameraFrameSelected(NULL),
      m_scopeButtonWidth(0),
      m_coverButton(NULL), m_healthBar(NULL), m_armorBar(NULL), m_multiplierBar(NULL), m_missionTimer(NULL),
      m_experienceHud(NULL), m_pauseButton(NULL), m_showCountdownTimer(false), m_field274(false),
      m_firePointer(-1), m_fireHeld(false), m_commandsReady(false),
      m_zoomedScope(NULL), m_woundingWindow(NULL), m_armorHitWindow(NULL), m_reviveWindow(NULL),
      m_steadyRemainingMs(-1), m_steadyLimitMs(-1), m_steadyActive(false), m_visibilityAlpha(0.0f),
      m_enabled(false), m_currentHint(NULL), m_field2dc(false), m_field2e0(0)
{
    m_pauseButton = new CDH_GameSofkeyWidget();
    m_pauseButton->SetAlign(ALIGN_LEFT | ALIGN_TOP);
    int pauseInset = App::Scale() * 5;
    m_pauseButton->SetOutsetSpacing(pauseInset, 0);
    AddToFront(m_pauseButton);
    m_weaponImages[0] = NULL;
    m_weaponImages[1] = NULL;
    m_zoomedScope = new CZoomedScopeWindow();
    m_woundingWindow = new WoundingWindow(false);
    m_armorHitWindow = new WoundingWindow(true);
    AddToFront(m_woundingWindow);
    AddToFront(m_armorHitWindow);
    m_reviveWindow = new ReviveWindow();
    AddToFront(m_reviveWindow);
    InitFireButton();
    InitWeaponButton();
    InitIngameShopButton();
    InitSteadyButton();
    InitScopeButton();
    InitHealthReplehishButton();
    InitGoPillButton();
    InitUltrasoundEmitterButton();
    InitCameraButton();
    InitReloadButton();
    m_aimAssistIcon = static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->CreateSurface(
        "IDB_HUD_AIM_ASSIST_ICON", true);
    static_cast<App*>(WindowApp::m_instance)->m_helper->LoadImageSurface(&m_tapToShoot, "SUR_TUTORIAL_TAP_TO_SHOT",
                                                                         false);
    InitZoomAndDock();
    InitBars();
    m_experienceHud = new CBH_GPSHud(true);
    m_experienceHud->SetOutsetSpacing(0, SScreenAdaptingConstant(150, 300)());
    AddToFront(m_experienceHud);
}

void GamePlayHUD::InitFireButton()
{
    static_cast<App*>(WindowApp::m_instance)->m_helper->LoadImageSurface(&m_fireOn, "SUR_HUD_FIRE_BUTTON_ON", false);
    static_cast<App*>(WindowApp::m_instance)->m_helper->LoadImageSurface(&m_fireOff, "SUR_HUD_FIRE_BUTTON_OFF", false);
    if (m_fireOn != NULL && m_fireOff != NULL) {
        unsigned int width = 0;
        unsigned int height = 0;
        m_fireOn->GetWidthAndHeight(width, height);
        DGHelper* helper = static_cast<App*>(WindowApp::m_instance)->m_helper;
        m_fireArea.append(CRectangle(helper->m_screenWidth - width, helper->m_screenHeight - height, width, height),
                          0, 0);
    }
}

void GamePlayHUD::InitWeaponButton()
{
    CDH_Weapon* weapon01 = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon01(true);
    CDH_Weapon* weapon02 = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon02(true);
    if (weapon01 == weapon02) {
        m_hasAlternateWeapon = false;
    }
    m_weaponImages[0] = weapon01->getGameImage();
    m_weaponImages[1] = weapon02->getGameImage();
    static_cast<App*>(WindowApp::m_instance)->m_helper->LoadImageSurface(&m_weaponSwitch,
                                                                         "SUR_HUD_WEAPON_SWITCH_BUTTON", false);
    int x = static_cast<App*>(WindowApp::m_instance)->m_helper->m_screenWidth - ImageWidth(m_weaponSwitch);
    int y = static_cast<App*>(WindowApp::m_instance)->m_helper->m_screenHeight - ImageHeight(m_fireOn)
        - ImageHeight(m_weaponSwitch) - ImageHeight(m_weaponSwitch) / 2;
    m_weaponSwitchArea.append(CRectangle(x, y, ImageWidth(m_weaponSwitch), ImageHeight(m_weaponSwitch)), 0, 0);
    int scale = App::Scale();
    for (int index = 0; index < 2; index++) {
        unsigned int width = 0;
        unsigned int height = 0;
        m_weaponImages[index]->GetWidthAndHeight(width, height);
        int left = static_cast<App*>(WindowApp::m_instance)->m_helper->m_screenWidth - scale * 5 - width;
        m_weaponAreas[index].append(CRectangle(left, 2, width, height), 0, 0);
    }
}

void GamePlayHUD::InitIngameShopButton()
{
    m_ingameShop = new CImageButton();
    m_ingameShop->AddImage("IDB_HUD_INGAME_SHOP_BUTTON_ON", 0, 0);
    m_ingameShop->SetCommandParameters(0x2ebf5358, 0, 0, 0);
    m_ingameShop->SetLayoutType(LAYOUT_SIMPLE);
    m_ingameShop->SetAlign(ALIGN_LEFT | ALIGN_TOP);
    m_ingameShop->SetOutsetSpacing(SScreenAdaptingConstant(50, 100)(), 0, 0, 0);
    AddToFront(m_ingameShop);
}

void GamePlayHUD::InitSteadyButton()
{
    static_cast<App*>(WindowApp::m_instance)->m_helper->LoadImageSurface(&m_steadyOn, "SUR_HUD_STEADY_BUTTON_ON",
                                                                         false);
    static_cast<App*>(WindowApp::m_instance)->m_helper->LoadImageSurface(&m_steadyOff, "SUR_HUD_STEADY_BUTTON_OFF",
                                                                         false);
    unsigned int width = 0;
    unsigned int height = 0;
    m_steadyOn->GetWidthAndHeight(width, height);
    m_steadyLimitMs = 5000;
    float skill = static_cast<App*>(WindowApp::m_instance)->m_helper->GetPlayerData()->GetSkillLevelCoef(
        static_cast<CDH_PlayerData::skill_type_t>(2));
    m_steadyRemainingMs = m_steadyLimitMs = static_cast<int>((skill * 1.5f + 1.0f) * m_steadyLimitMs);
    m_steadyActive = false;
    m_steadyArea.append(CRectangle(0, 0, width, height), -(static_cast<int>(width) / 2),
                        -(static_cast<int>(height) / 2));
    m_modelAnimationState = 0;
    m_steadyArea.SetPosition(static_cast<App*>(WindowApp::m_instance)->m_helper->m_screenWidth >> 1,
                             static_cast<App*>(WindowApp::m_instance)->m_helper->m_screenHeight - 2 - (height >> 1));
}

void GamePlayHUD::InitScopeButton()
{
    static_cast<App*>(WindowApp::m_instance)->m_helper->LoadImageSurface(&m_scopeEnter, "SUR_HUD_SCOPE_BUTTON_ENTER",
                                                                         false);
    static_cast<App*>(WindowApp::m_instance)->m_helper->LoadImageSurface(&m_scopeExit, "SUR_HUD_SCOPE_BUTTON_EXIT",
                                                                         false);
    const CAmmo* ammo = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetActiveWeapon()
        ->GetActiveAmmo();
    m_ammo = ammo;
    CBH_SurfaceManager* surfaces = static_cast<App*>(WindowApp::m_instance)->m_surfaceManager;
    if (ammo != NULL) {
        XString icon = ammo->m_bulletIcon;
        m_bulletIcon = surfaces->CreateSurface(XString::AnsiString(icon.c_str()), true);
    } else {
        m_bulletIcon = surfaces->CreateSurface("IDB_HUD_HANDGUN_BULLET_ICON", true);
    }
    m_silencerIcon = static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->CreateSurface("IDB_HUD_SILENCER", true);
    if (m_scopeEnter != NULL && m_scopeExit != NULL) {
        unsigned int width = 0;
        unsigned int height = 0;
        m_scopeEnter->GetWidthAndHeight(width, height);
        m_scopeButtonWidth = width;
        m_scopeArea.append(CRectangle(0, static_cast<App*>(WindowApp::m_instance)->m_helper->m_screenHeight - height,
                                      width, height), 0, 0);
    }
}

void GamePlayHUD::InitHealthReplehishButton()
{
    static_cast<App*>(WindowApp::m_instance)->m_helper->LoadImageSurface(&m_medkit, "SUR_HUD_HEALTH_REPLENISH_BUTTON",
                                                                         false);
    if (m_medkit != NULL) {
        unsigned int width = 0;
        unsigned int height = 0;
        m_medkit->GetWidthAndHeight(width, height);
        int x = m_scopeButtonWidth + 2;
        int y = static_cast<App*>(WindowApp::m_instance)->m_helper->m_screenHeight - 2 - height;
        unsigned int scopeHeight = 0;
        m_scopeEnter->GetWidthAndHeight(width, scopeHeight);
        m_medkitArea.append(CRectangle(x, y, width, height), 0, 0);
    }
}

void GamePlayHUD::InitGoPillButton()
{
    static_cast<App*>(WindowApp::m_instance)->m_helper->LoadImageSurface(&m_tablet, "SUR_HUD_TABLET_BUTTON", false);
    if (m_tablet != NULL) {
        unsigned int width = 0;
        unsigned int height = 0;
        m_tablet->GetWidthAndHeight(width, height);
        CRectangle& medkit = m_medkitArea.m_bounds;
        m_tabletArea.append(CRectangle(medkit.m_x + medkit.m_width + 2,
                                       static_cast<App*>(WindowApp::m_instance)->m_helper->m_screenHeight - 2 - height,
                                       width, height), 0, 0);
    }
}

void GamePlayHUD::InitUltrasoundEmitterButton()
{
    static_cast<App*>(WindowApp::m_instance)->m_helper->LoadImageSurface(&m_ultrasound, "SUR_HUD_ULTRASOUND_EMITTER",
                                                                         false);
    if (m_ultrasound != NULL) {
        unsigned int width = 0;
        unsigned int height = 0;
        m_ultrasound->GetWidthAndHeight(width, height);
        CRectangle& tablet = m_tabletArea.m_bounds;
        int x = tablet.m_x + tablet.m_width + 2 + SScreenAdaptingConstant(2, 4)();
        int y = static_cast<App*>(WindowApp::m_instance)->m_helper->m_screenHeight - 2 - height
            + SScreenAdaptingConstant(3, 0)();
        m_ultrasoundArea.append(CRectangle(x, y, width, height), 0, 0);
    }
}

void GamePlayHUD::InitCameraButton()
{
    static_cast<App*>(WindowApp::m_instance)->m_helper->LoadImageSurface(&m_cameraFrame, "SUR_HUD_CAMERA_FRAME", false);
    static_cast<App*>(WindowApp::m_instance)->m_helper->LoadImageSurface(&m_cameraFrameSelected,
                                                                         "SUR_HUD_CAMERA_FRAME_SELECT", false);
}

void GamePlayHUD::InitReloadButton()
{
    m_reload = static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->CreateSurface("IDB_HUD_RELOAD_BUTTON_ON",
                                                                                         true);
    int width = 0;
    int height = 0;
    m_reload->GetWidthAndHeight(reinterpret_cast<unsigned int&>(width), reinterpret_cast<unsigned int&>(height));
    m_reloadButtonArea.append(CRectangle(0, 0, width, height), 0, 0);
    m_reloadButtonArea.SetPosition(static_cast<App*>(WindowApp::m_instance)->m_screenWidth - width,
                                   SScreenAdaptingConstant(60, 120)());
    CRectangle& button = m_reloadButtonArea.m_bounds;
    m_reloadTouchArea.append(CRectangle(0, 0, width * 2, button.m_y + button.m_height + height * 2), 0, 0);
    m_reloadTouchArea.SetPosition(static_cast<App*>(WindowApp::m_instance)->m_screenWidth - width * 2, 0);
}

void GamePlayHUD::InitZoomAndDock()
{
    CZoomSliderControl* slider = new CZoomSliderControl("SUR_ZOOM_SLIDER", "SUR_ZOOM_TRACK");
    m_zoomSliderControl = slider;
    int centerOffset = 370;
    if (!App::IsXGA()) {
        centerOffset = App::IsVGA() ? 330 : 168;
    }
    int radius = 463;
    if (!App::IsXGA()) {
        radius = App::IsVGA() ? 424 : 216;
    }
    slider->SetParams(21, centerOffset, radius);
    m_zoomSliderControl->SetProgress(1.0f);
    m_zoomSliderControl->Enable(false);
    AddToFront(m_zoomSliderControl);
    CDH_GameSofkeyWidget::GetPauseButtonHeight();
    ImageRes hide("SUR_DOCKCOVER_HIDE");
    ImageRes unhide("SUR_DOCKCOVER_UNHIDE");
    m_coverButton = new CBH_TriggerButton("SUR_DOCKCOVER_UNHIDE", "SUR_DOCKCOVER_HIDE");
    m_coverButton->SetAlign(ALIGN_LEFT | ALIGN_BOTTOM);
    m_coverButton->SetOutsetSpacing(m_scopeArea.m_bounds.m_height + m_coverButton->m_height / 2, 0);
    m_coverButton->SetCommand(static_cast<int>(0x811575c5u));
    AddToFront(m_coverButton);
}

void GamePlayHUD::InitBars()
{
    ImageRes pauseImage("SUR_HUD_PAUSE_BUTTON");
    unsigned int pauseWidth;
    unsigned int pauseHeight;
    ICRenderSurface* pauseSurface = pauseImage.GetSurface();
    pauseHeight = 0;
    if (pauseSurface != NULL) {
        pauseSurface->GetWidthAndHeight(pauseWidth, pauseHeight);
    }
    int x = pauseWidth + SScreenAdaptingConstant(11, 22)() + 2;
    int healthY = SScreenAdaptingConstant(6, 12)();
    int width = SScreenAdaptingConstant(110, 220)();
    m_healthBar = new CBH_ProgressBar(2, width, -1);
    m_healthBar->SetCorner(x, healthY + 2);
    m_healthBar->SetMaxValue(CBH_Player::GetInstance()->GetMaxHp() / 1000);
    m_healthBar->SetCurrValue(static_cast<int>(MathLib::Ceil(CBH_Player::GetInstance()->m_hp / 1000.0)));
    m_healthBar->SetDescImage("IDB_HUD_ICON_HEALTH");
    AddToFront(m_healthBar);
    int y = healthY + 2 + m_healthBar->m_height;
    m_armorBar = new CBH_ProgressBar(1, width, -1);
    m_armorBar->SetCorner(x, y);
    m_armorBar->SetStyle(CBH_ProgressBar::STYLE_VALUE);
    m_armorBar->SetMaxValue(CBH_Player::GetInstance()->m_armorDamageProofMax / 1000);
    m_armorBar->SetCurrValue(CBH_Player::GetInstance()->m_armorDamageProof / 1000);
    m_armorBar->SetDescImage("IDB_HUD_ICON_ARMOR");
    AddToFront(m_armorBar);
    if (!CBH_Player::GetInstance()->IsArmorAvailable()) {
        m_armorBar->ClearFlags(FLAG_VISIBLE | FLAG_ENABLED);
    }
    const SScreenAdaptingConstant multiplierX(158, 316);
    int multiplierSeconds = static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData
        .m_tripleMultiplierSeconds;
    m_multiplierBar = new CBH_ProgressBar(4, width, -1);
    m_multiplierBar->SetCorner(multiplierX(), y);
    m_multiplierBar->SetStyle(CBH_ProgressBar::STYLE_CLOCK);
    m_multiplierBar->SetCurrValue(multiplierSeconds);
    m_multiplierBar->SetMaxValue(multiplierSeconds);
    m_multiplierBar->SetDescImage("IDB_HUD_ICON_XP_MULTY_3");
    AddToFront(m_multiplierBar);
    if (static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_cameraDurationMs > 0) {
        const SScreenAdaptingConstant timerWidth(75, 150);
        const SScreenAdaptingConstant timerX(270, 540);
        const SScreenAdaptingConstant timerY(32, 64);
        m_missionTimer = new CBH_ProgressBar(6, timerWidth(), -1);
        m_missionTimer->SetCorner(timerX(), timerY());
        m_missionTimer->SetStyle(CBH_ProgressBar::STYLE_CLOCK);
        m_missionTimer->SetCurrValue(0);
        m_missionTimer->SetMaxValue(static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData
                                        .m_cameraDurationMs / 1000);
        m_missionTimer->SetDescImage("IDB_HUD_ICON_TIMER");
        AddToFront(m_missionTimer);
    }
}

void GamePlayHUD::setCamerasCount(int count)
{
    unsigned int width = 0;
    unsigned int height = 0;
    m_cameraFrame->GetWidthAndHeight(width, height);
    if (count > 0) {
        int top = (static_cast<App*>(WindowApp::m_instance)->m_helper->m_screenHeight - static_cast<int>(height) * count)
            / 2;
        for (int index = 0; index < count; index++) {
            m_cameraButtonAreas.addElement(Collision(CRectangle(2, height * index + top, width, height), 0, 0));
        }
    }
}

inline GamePlayHUD::TutorialHint::~TutorialHint()
{
    CApplet::m_pApp->m_resourceManager->ReleaseResource(m_imageName);
}

GamePlayHUD::~GamePlayHUD()
{
    for (int index = 0; index < m_cameraImages.size(); index++) {
        if (m_cameraImages[index] != NULL) {
            static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(m_cameraImages[index]);
        }
    }
    if (m_aimAssistIcon != NULL) {
        static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->ReleaseSurface("IDB_HUD_AIM_ASSIST_ICON");
        m_aimAssistIcon = NULL;
    }
    m_cameraImages.removeAllElements();
    static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(m_bulletIcon);
    if (m_zoomedScope != NULL) {
        delete m_zoomedScope;
        m_zoomedScope = NULL;
    }
    ICResourceManager* resources = CApplet::m_pApp->m_resourceManager;
    resources->ReleaseResource("SUR_HUD_WEAPON_SWITCH_BUTTON");
    resources->ReleaseResource("SUR_HUD_STEADY_BUTTON_ON");
    resources->ReleaseResource("SUR_HUD_STEADY_BUTTON_OFF");
    resources->ReleaseResource("SUR_HUD_SCOPE_BUTTON_ENTER");
    resources->ReleaseResource("SUR_HUD_SCOPE_BUTTON_EXIT");
    resources->ReleaseResource("SUR_HUD_FIRE_BUTTON_ON");
    resources->ReleaseResource("SUR_HUD_FIRE_BUTTON_OFF");
    resources->ReleaseResource("SUR_TUTORIAL_TAP_TO_SHOT");
    resources->ReleaseResource("SUR_HUD_CAMERA_FRAME");
    resources->ReleaseResource("SUR_HUD_CAMERA_FRAME_SELECT");
    resources->ReleaseResource("SUR_HUD_DOWNTOWN_01_CAMERA_01");
    resources->ReleaseResource("SUR_HUD_DOWNTOWN_01_CAMERA_02");
    resources->ReleaseResource("SUR_HUD_DOWNTOWN_01_CAMERA_03");
    resources->ReleaseResource("SUR_HUD_DOWNTOWN_01_CAMERA_04");
    resources->ReleaseResource("SUR_HUD_HEALTH_REPLENISH_BUTTON");
    resources->ReleaseResource("SUR_HUD_TABLET_BUTTON");
    resources->ReleaseResource("SUR_HUD_ULTRASOUND_EMITTER");
    CleanWeaponImage();
    resources->ReleaseResource("IDB_HUD_HANDGUN_BULLET_ICON");
    static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->ReleaseSurface("IDB_HUD_SILENCER");
    SAFE_DELETE_VECTOR(m_tutorialHints);
    static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->ReleaseSurface("IDB_HUD_RELOAD_BUTTON_ON");
}

void GamePlayHUD::CleanWeaponImage()
{
    CDH_Weapon* weapon01 = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon01(true);
    CDH_Weapon* weapon02 = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon02(true);
    ICResourceManager* resources = CApplet::m_pApp->m_resourceManager;
    if (resources != NULL) {
        if (m_weaponImages[0] != NULL) {
            resources->ReleaseResource(weapon01->m_gameImageId);
            resources->ReleaseResource(weapon01->m_bulletOnImageId);
            weapon01->releaseResources();
            m_weaponImages[0] = NULL;
        }
        if (m_weaponImages[1] != NULL) {
            resources->ReleaseResource(weapon02->m_gameImageId);
            resources->ReleaseResource(weapon02->m_bulletOnImageId);
            weapon02->releaseResources();
            m_weaponImages[1] = NULL;
        }
    }
}

void GamePlayHUD::UpdateHUDsVisibility()
{
    if (m_pauseButton != NULL) {
        const bool visible = static_cast<App*>(WindowApp::m_instance)->m_settings->m_field1f;
        const int visibleFlag = FLAG_VISIBLE;
        if (visible) {
            m_pauseButton->SetFlags(visibleFlag);
        } else {
            m_pauseButton->ClearFlags(visibleFlag);
        }
    }
    if (m_healthBar != NULL) {
        const bool visible = static_cast<App*>(WindowApp::m_instance)->m_settings->m_field1f;
        const int visibleFlag = FLAG_VISIBLE;
        if (visible) {
            m_healthBar->SetFlags(visibleFlag);
        } else {
            m_healthBar->ClearFlags(visibleFlag);
        }
    }
    if (m_armorBar != NULL) {
        const bool visible = static_cast<App*>(WindowApp::m_instance)->m_settings->m_field1f;
        const int visibleFlag = FLAG_VISIBLE;
        if (visible) {
            m_armorBar->SetFlags(visibleFlag);
        } else {
            m_armorBar->ClearFlags(visibleFlag);
        }
    }
    if (m_missionTimer != NULL) {
        const bool visible = static_cast<App*>(WindowApp::m_instance)->m_settings->m_field1f;
        const int visibleFlag = FLAG_VISIBLE;
        if (visible) {
            m_missionTimer->SetFlags(visibleFlag);
        } else {
            m_missionTimer->ClearFlags(visibleFlag);
        }
    }
    if (m_multiplierBar != NULL) {
        if (static_cast<App*>(WindowApp::m_instance)->m_settings->m_field1f && m_showCountdownTimer) {
            m_multiplierBar->SetFlags(FLAG_VISIBLE);
        } else {
            m_multiplierBar->ClearFlags(FLAG_VISIBLE);
        }
    }
    if (m_ingameShop != NULL) {
        const bool visible = static_cast<App*>(WindowApp::m_instance)->m_settings->m_field1f;
        const int visibleFlag = FLAG_VISIBLE;
        if (visible) {
            m_ingameShop->SetFlags(visibleFlag);
        } else {
            m_ingameShop->ClearFlags(visibleFlag);
        }
    }
    if (m_experienceHud != NULL) {
        const bool visible = static_cast<App*>(WindowApp::m_instance)->m_settings->m_field1f;
        const int visibleFlag = FLAG_VISIBLE;
        if (visible) {
            m_experienceHud->SetFlags(visibleFlag);
        } else {
            m_experienceHud->ClearFlags(visibleFlag);
        }
    }
}

void GamePlayHUD::Update()
{
    UpdateHUDsVisibility();
    const CAmmo* ammo = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetActiveWeapon()
        ->GetActiveAmmo();
    if (ammo != m_ammo) {
        {
            CBH_SurfaceManager* surfaces = static_cast<App*>(WindowApp::m_instance)->m_surfaceManager;
            XString icon = m_ammo->m_bulletIcon;
            surfaces->ReleaseSurface(XString::AnsiString(icon.c_str()));
        }
        {
            CBH_SurfaceManager* surfaces = static_cast<App*>(WindowApp::m_instance)->m_surfaceManager;
            XString icon = ammo->m_bulletIcon;
            m_bulletIcon = surfaces->CreateSurface(XString::AnsiString(icon.c_str()), true);
        }
        m_ammo = ammo;
    }
    if (m_zoomSliderControl != NULL) {
        CZoomedScopeWindow* scope = m_zoomedScope;
        if (scope != NULL && scope->m_zoomEnabled) {
            if (!scope->m_shooting) {
                float position = 1.0f - m_zoomSliderControl->GetValue();
                // written as a select so it compiles to vmax like the original
                position = 0.0f > position ? 0.0f : position;
                scope->m_zoomPosition = scope->m_zoom->m_durationSec * position * 0.5f;
            }
            m_zoomedScope->Update();
        }
    }
    if (CBH_Player::GetInstance()->IsArmorAvailable() && (m_armorBar->m_flags & FLAG_VISIBLE) == 0
        && static_cast<App*>(WindowApp::m_instance)->m_settings->m_field1f) {
        m_armorBar->SetMaxValue(CBH_Player::GetInstance()->m_armorDamageProofMax / 1000);
        m_armorBar->SetCurrValue(CBH_Player::GetInstance()->m_armorDamageProof / 1000);
        m_armorBar->SetFlags(FLAG_VISIBLE | FLAG_ENABLED);
    } else if (!CBH_Player::GetInstance()->IsArmorAvailable() && (m_armorBar->m_flags & FLAG_VISIBLE) != 0) {
        m_armorBar->ClearFlags(FLAG_VISIBLE | FLAG_ENABLED);
    }
    if (m_healthBar != NULL) {
        m_healthBar->SetMaxValue(CBH_Player::GetInstance()->GetMaxHp() / 1000);
        m_healthBar->SetCurrValue(static_cast<int>(MathLib::Ceil(CBH_Player::GetInstance()->m_hp / 1000.0)));
    }
    if (m_armorBar != NULL) {
        int proof = CBH_Player::GetInstance()->m_armorDamageProof;
        int proofMax = CBH_Player::GetInstance()->m_armorDamageProofMax;
        if (proof >= 1000) {
            m_armorBar->SetCurrValue(proof / 1000);
            m_armorBar->SetMaxValue(proofMax / 1000);
        } else {
            m_armorBar->ClearFlags(FLAG_VISIBLE);
        }
    }
    UpdateButtonPosition();
    if (m_modelAnimationState != 0) {
        processModelAnimation();
    }
    int frameTime = WindowApp::m_instance->m_updateFrameTime;
    float alpha = m_visibilityAlpha + (m_enabled ? 1.0f : -1.0f) * frameTime / 1000.0f;
    // selects compile to vmax / vmin like the original
    alpha = 0.0f > alpha ? 0.0f : alpha;
    m_visibilityAlpha = 1.0f < alpha ? 1.0f : alpha;
    if (m_steadyActive) {
        m_steadyRemainingMs -= frameTime;
        if (m_steadyRemainingMs < 0) {
            m_steadyActive = false;
        }
    }
    if (m_currentHint != NULL) {
        m_currentHint->m_elapsed += frameTime;
        if (m_currentHint->m_elapsed >= m_currentHint->m_duration) {
            int index = m_tutorialHints.indexOf(m_currentHint);
            m_currentHint = NULL;
            if (index >= 0) {
                if (index + 1 < m_tutorialHints.size()) {
                    m_currentHint = m_tutorialHints[index + 1];
                } else {
                    SAFE_DELETE_VECTOR(m_tutorialHints);
                }
            }
        }
    }
    if (!static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->IsGamePaused()) {
        const CAmmo* ammo01 = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon01(true)
            ->GetActiveAmmo();
        const CAmmo* ammo02 = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon02(true)
            ->GetActiveAmmo();
        int available01 = CBH_Player::GetInstance()->GetAvailableAmmo(ammo01, true);
        int available02 = CBH_Player::GetInstance()->GetAvailableAmmo(ammo02, true);
        CDH_Weapon* weapon = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetActiveWeapon();
        if (weapon != NULL && weapon->IsReadyForUse() && weapon->m_ammo == 0 && m_hasAlternateWeapon
            && m_modelAnimationState == 0 && available02 + available01 > 0 && !weapon->CheckForReload(false)) {
            if (!m_scoped) {
                switchWeapons();
            } else {
                switchScope();
            }
        }
    }
}

void GamePlayHUD::UpdateButtonPosition()
{
    m_showMedkit = CBH_Player::GetInstance()->CanRestoreHealth()
        && CBH_Player::GetInstance()->m_hp < CBH_Player::GetInstance()->GetMaxHp();
    m_showUltrasound = CBH_Player::GetInstance()->m_ultrasoundAttemptCount > 0;
}

void GamePlayHUD::processModelAnimation()
{
    switch (m_modelAnimationState) {
    case 1:
        if (static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetActiveWeapon()
                ->isModelAnimationComplete() != 1) {
            return;
        }
        static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->OnWeaponHided();
        static_cast<App*>(WindowApp::m_instance)->m_helper->m_gamePlayManager.SwitchWeapons();
        static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetActiveWeapon()->fetchWeaponModel();
        m_modelAnimationState = 2;
        break;
    case 2:
        if (static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetActiveWeapon()
                ->isModelAnimationComplete() != 1) {
            return;
        }
        static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->OnWeaponShown();
        m_modelAnimationState = 0;
        break;
    case 4:
        if (static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetActiveWeapon()
                ->isModelAnimationComplete() != 1) {
            return;
        }
        static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->OnWeaponHided();
        {
            ReviveWindow* revive = m_reviveWindow;
            if (revive != NULL && static_cast<App*>(WindowApp::m_instance)->m_settings->m_field1f) {
                revive->m_remainingMs = 5000;
                revive->m_soundPlayed = false;
            }
        }
        m_modelAnimationState = 5;
        break;
    case 5:
        if (m_reviveWindow->m_remainingMs > 0) {
            return;
        }
        static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetActiveWeapon()->fetchWeaponModel();
        m_modelAnimationState = 2;
        break;
    }
}

void GamePlayHUD::switchWeapons()
{
    static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetActiveWeapon()->unfetchWeaponModel();
    m_modelAnimationState = 1;
    CBH_Player::GetInstance()->m_gameStats.AddInMIssionWeaponChanges(1);
}

void GamePlayHUD::switchScope()
{
    CDH_Weapon* weapon = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetActiveWeapon();
    if (m_scoped) {
        if (!weapon->m_scopeAttached) {
            static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->m_camera->restoreView(true);
        } else {
            m_zoomSliderControl->Enable(false);
            m_zoomedScope->m_zoomEnabled = false;
        }
        weapon->ScopeOut();
        CSwerveGame* game = static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame;
        game->setCursor(game->m_fpsCursor);
    } else {
        weapon->ScopeIn();
    }
    m_modelAnimationState = 3;
}

void GamePlayHUD::Paint(ICGraphics2d& graphics)
{
    if (!m_enabled && !(m_visibilityAlpha > 0.0f)) {
        return;
    }
    m_zoomedScope->RunPaint3D(graphics);
    DGHelper* helper = static_cast<App*>(WindowApp::m_instance)->m_helper;
    int screenWidth = helper->m_screenWidth;
    int screenHeight = helper->m_screenHeight;
    helper->PushAlphaBlend(m_visibilityAlpha);
    CDH_Weapon* weapon = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetActiveWeapon();
    CFontMgr::GetInstance()->GetFont(1);
    DrawWeaponButton(weapon);
    if (m_steadyRemainingMs >= 0 && m_modelAnimationState == 0) {
        DrawSteadyButton();
    }
    DrawScopeButton();
    DrawReloadButton(weapon);
    if (static_cast<App*>(WindowApp::m_instance)->m_settings->IsFireButtonEnabled()) {
        DrawFireButton();
    }
    if (m_showMedkit) {
        DrawHealthReplenishButton();
    }
    if (CBH_Player::GetInstance()->IsSpeedModeAvailable() || CBH_Player::GetInstance()->IsSpeedMode()) {
        DrawGoPillButton();
    }
    if (m_showUltrasound) {
        DrawUnltrasoundEmitterButton();
    }
    if (m_currentHint != NULL) {
        const char* name = m_currentHint->m_imageName;
        if (strcmp(name, "SUR_TUTORIAL_SLIDE_TO_LOOK_AROUND") == 0) {
            static_cast<App*>(WindowApp::m_instance)->m_helper->DrawImage(m_currentHint->m_image, screenWidth / 2, 10,
                                                                          0.6f, 0, 0, 0.0f, DGHelper::DRAW_HCENTER);
        } else if (strcmp(name, "SUR_TUTORIAL_SLIDE_TO_ZOOM_OUT") == 0) {
            static_cast<App*>(WindowApp::m_instance)->m_helper->DrawImage(m_currentHint->m_image, 11, 144, 0.6f, 0, 0,
                                                                          0.0f, DGHelper::DRAW_VCENTER);
        } else if (strcmp(name, "SUR_TUTORIAL_SLIDE_TO_ZOOM_IN") == 0) {
            static_cast<App*>(WindowApp::m_instance)->m_helper->DrawImage(
                m_currentHint->m_image, screenWidth - 11, 144, 0.6f, 0, 0, 0.0f,
                DGHelper::DRAW_RIGHT | DGHelper::DRAW_VCENTER);
        } else if (strcmp(name, "SUR_TUTORIAL_SLIDE_TO_AIM") == 0) {
            static_cast<App*>(WindowApp::m_instance)->m_helper->DrawImage(m_currentHint->m_image, screenWidth / 2, 80,
                                                                          0.6f, 0, 0, 0.0f, DGHelper::DRAW_HCENTER);
        } else if (strcmp(name, "SUR_TUTORIAL_BUTTON_TO_SHOT") == 0) {
            if (!static_cast<App*>(WindowApp::m_instance)->m_settings->IsFireButtonEnabled()) {
                static_cast<App*>(WindowApp::m_instance)->m_helper->DrawImage(
                    m_tapToShoot, screenWidth / 2, screenHeight / 2, 0.6f, 0, 0, 0.0f,
                    DGHelper::DRAW_HCENTER | DGHelper::DRAW_VCENTER);
            } else {
                static_cast<App*>(WindowApp::m_instance)->m_helper->DrawImage(
                    m_currentHint->m_image, screenWidth - 50, screenHeight - 5, 0.6f, 0, 0, 0.0f,
                    DGHelper::DRAW_RIGHT | DGHelper::DRAW_BOTTOM);
            }
        }
    }
    static_cast<App*>(WindowApp::m_instance)->m_helper->PopAlphaBlend();
}

void GamePlayHUD::DrawWeaponButton(CDH_Weapon* weapon)
{
    if (!static_cast<App*>(WindowApp::m_instance)->m_settings->m_field1f) {
        return;
    }
    if (static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->IsIntroTime()) {
        return;
    }
    int index = GetActiveWeaponIndex();
    if (m_weaponImages[index] != NULL && !m_scoped) {
        CRectangle& image = m_weaponAreas[index].m_bounds;
        static_cast<App*>(WindowApp::m_instance)->m_helper->DrawImage(
            m_weaponImages[index], image.m_x + (image.m_width >> 1), image.m_y + (image.m_height >> 1), 1.0f, 0, 0,
            0.0f, DGHelper::DRAW_HCENTER | DGHelper::DRAW_VCENTER);
        if (m_hasAlternateWeapon) {
            CRectangle& button = m_weaponSwitchArea.m_bounds;
            static_cast<App*>(WindowApp::m_instance)->m_helper->DrawImage(
                m_weaponSwitch, button.m_x + (button.m_width >> 1), button.m_y + (button.m_height >> 1), 1.0f, 0, 0,
                0.0f, DGHelper::DRAW_HCENTER | DGHelper::DRAW_VCENTER);
        }
    }
    CFont* font = CFontMgr::GetInstance()->GetFont(8);
    if (weapon == NULL || font == NULL) {
        return;
    }
    wchar_t text[20];
    const CAmmo* ammo = weapon->GetActiveAmmo();
    if (ammo != NULL && CBH_Player::GetInstance()->GetAvailableAmmo(ammo, true) != 0xfffffff) {
        int clip = weapon->m_ammo;
        int reserve = CBH_Player::GetInstance()->GetAvailableAmmo(ammo, true) - clip;
        if (reserve < 0) {
            reserve = 0;
        }
        ICStdUtil::SWPrintF(text, L"%d / %d ", clip, reserve);
    } else {
        ICStdUtil::SWPrintF(text, L"\x221e");
    }
    int textWidth = font->MeasureTextWidth(text, -1, -1, NULL);
    CRectangle& image = m_weaponAreas[index].m_bounds;
    int x = image.m_x - textWidth + image.m_width - App::Scale() * 6;
    int y = image.m_y + image.m_height + 2;
    font->PaintText(text, wcslen(text), x, y, -1, -1);
    CDH_Weapon* active = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetActiveWeapon();
    int bulletWidth = ImageWidth(active->getBulletOnImage());
    int fontHeight = font->GetFontHeight();
    static_cast<App*>(WindowApp::m_instance)->m_helper->DrawImage(
        m_bulletIcon, x - bulletWidth, y + fontHeight / 2, 1.0f, 0, 0, 0.0f,
        DGHelper::DRAW_HCENTER | DGHelper::DRAW_VCENTER);
    int textHeight = ImageHeight(m_bulletIcon);
    if (textHeight < fontHeight) {
        textHeight = fontHeight;
    }
    int silencerTextX = 0;
    int bottom = -1;
    if (active->m_silencerAvailable && active->isSilent()) {
        XString count = XString::Format(L"%d", CBH_Player::GetInstance()->m_silentShotsCount);
        int silencerY = fontHeight + y;
        static_cast<App*>(WindowApp::m_instance)->m_helper->DrawImage(
            m_silencerIcon, x - bulletWidth * 4, silencerY, 1.0f, 0, 0, 0.0f,
            DGHelper::DRAW_HCENTER | DGHelper::DRAW_VCENTER);
        unsigned int silencerWidth;
        unsigned int silencerHeight;
        m_silencerIcon->GetWidthAndHeight(silencerWidth, silencerHeight);
        int iconHeight = ImageHeight(m_silencerIcon);
        int countWidth = font->MeasureTextWidth(count.c_str(), -1, -1, NULL);
        silencerTextX = x - bulletWidth * 5 - countWidth + ImageWidth(m_silencerIcon) / 2;
        font->PaintText(count.c_str(), count.Length(), silencerTextX, silencerY - ImageHeight(m_silencerIcon), -1, -1);
        bottom = silencerY + iconHeight / 2;
    }
    if (static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetActiveWeapon()->m_cyberScopeEquipped) {
        int aimAssistX;
        if (silencerTextX == 0) {
            aimAssistX = x - bulletWidth * 4;
        } else {
            aimAssistX = silencerTextX - SScreenAdaptingConstant(20, 40)();
        }
        ICRenderSurface* icon = m_aimAssistIcon;
        static_cast<App*>(WindowApp::m_instance)->m_helper->DrawImage(
            icon, aimAssistX, SScreenAdaptingConstant(50, 100)(), 1.0f, 0, 0, 0.0f,
            DGHelper::DRAW_HCENTER | DGHelper::DRAW_VCENTER);
    }
    int reloadWidth;
    int reloadHeight;
    m_reload->GetWidthAndHeight(reinterpret_cast<unsigned int&>(reloadWidth),
                                reinterpret_cast<unsigned int&>(reloadHeight));
    int reloadX = static_cast<App*>(WindowApp::m_instance)->m_screenWidth - reloadWidth;
    if (bottom < textHeight + y) {
        bottom = textHeight + y;
    }
    m_reloadButtonArea.SetPosition(reloadX, bottom + App::Scale() * 2);
    m_reloadTouchArea.SetPosition(static_cast<App*>(WindowApp::m_instance)->m_screenWidth - reloadWidth * 2, 0);
}

void GamePlayHUD::DrawSteadyButton()
{
    if (!static_cast<App*>(WindowApp::m_instance)->m_settings->m_field1f
        || static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->IsIntroTime()) {
        return;
    }
    unsigned int width;
    unsigned int height;
    m_steadyOn->GetWidthAndHeight(width, height);
    unsigned int imageWidth = width;
    int left = m_steadyArea.m_bounds.m_x;
    int top = m_steadyArea.m_bounds.m_y;
    int boundsHeight = m_steadyArea.m_bounds.m_height;
    int boundsWidth = m_steadyArea.m_bounds.m_width;
    unsigned int fill = imageWidth * m_steadyRemainingMs / m_steadyLimitMs;
    unsigned int imageHeight = height;
    ICGraphics2d* graphics = ICGraphics2d::GetInstance();
    int x = left - (imageWidth >> 1) + (boundsWidth >> 1);
    int y = top - (imageHeight >> 1) + (boundsHeight >> 1);
    CRectangle clip = graphics->GetClip();
    graphics->SetClip(width + x - fill, y, fill, height);
    static_cast<App*>(WindowApp::m_instance)->m_helper->DrawImage(m_steadyOn, x, y, 1.0f, 0, 0, 0.0f, 0);
    graphics->SetClip(x, y, width - fill, height);
    static_cast<App*>(WindowApp::m_instance)->m_helper->DrawImage(m_steadyOff, x, y, 1.0f, 0, 0, 0.0f, 0);
    graphics->SetClip(clip);
}

void GamePlayHUD::DrawScopeButton()
{
    if (!static_cast<App*>(WindowApp::m_instance)->m_settings->m_field1f
        || static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->IsIntroTime()
        || CSwerveGame::m_pInstance->m_specialWeapon != NULL) {
        return;
    }
    m_scopeButtonShown = false;
    if (!m_scopeButtonAlways
        && !static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetActiveWeapon()->m_scopeAttached) {
        return;
    }
    m_scopeButtonShown = true;
    ICRenderSurface* image = m_scopeEnter;
    if (m_scoped) {
        image = m_scopeExit;
    }
    CRectangle& bounds = m_scopeArea.m_bounds;
    static_cast<App*>(WindowApp::m_instance)->m_helper->DrawImage(
        image, bounds.m_x + (bounds.m_width >> 1), bounds.m_y + (bounds.m_height >> 1), 1.0f, 0, 0, 0.0f,
        DGHelper::DRAW_HCENTER | DGHelper::DRAW_VCENTER);
}

void GamePlayHUD::DrawReloadButton(CDH_Weapon* weapon)
{
    if (weapon == NULL) {
        return;
    }
    const CAmmo* ammo = weapon->GetDefaultAmmo();
    int available = CBH_Player::GetInstance()->GetAvailableAmmo(ammo, true);
    if (!m_scoped && static_cast<App*>(WindowApp::m_instance)->m_settings->m_field1f
        && !static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->IsIntroTime()
        && available != 0xfffffff) {
        CRectangle& bounds = m_reloadButtonArea.m_bounds;
        static_cast<App*>(WindowApp::m_instance)->m_helper->DrawImage(
            m_reload, bounds.m_x + (bounds.m_width >> 1), bounds.m_y + (bounds.m_height >> 1), 1.0f, 0, 0, 0.0f,
            DGHelper::DRAW_HCENTER | DGHelper::DRAW_VCENTER);
    }
}

void GamePlayHUD::DrawFireButton()
{
    if (!static_cast<App*>(WindowApp::m_instance)->m_settings->m_field1f
        || static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->IsIntroTime()) {
        return;
    }
    bool ready = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetActiveWeapon()->IsReadyForShot();
    // select between the two images like the original (load after the call)
    ICRenderSurface* image = ready ? m_fireOn : m_fireOff;
    CRectangle& bounds = m_fireArea.m_bounds;
    static_cast<App*>(WindowApp::m_instance)->m_helper->DrawImage(
        image, bounds.m_x + (bounds.m_width >> 1), bounds.m_y + (bounds.m_height >> 1), 1.0f, 0, 0, 0.0f,
        DGHelper::DRAW_HCENTER | DGHelper::DRAW_VCENTER);
}

/** Number centered in the area (font 8). */
static void DrawNumberWithinThe(const Collision& area, int number)
{
    CFont* font = CFontMgr::GetInstance()->GetFont(8);
    if (font != NULL) {
        wchar_t text[20];
        ICStdUtil::SWPrintF(text, L"%d", number);
        int width = font->MeasureTextWidth(text, -1, -1, NULL);
        int height = font->GetFontHeight();
        int x = area.m_bounds.m_x - width / 2 + (area.m_bounds.m_width >> 1) + App::Scale() * 2;
        int y = area.m_bounds.m_y - height / 2 + (area.m_bounds.m_height >> 1) - App::Scale() * 3;
        font->PaintText(text, wcslen(text), x, y, -1, -1);
    }
}

/** Text centered horizontally on the top edge of the area (font 8). */
static void DrawStringAboveThe(const Collision& area, const wchar_t* text)
{
    CFont* font = CFontMgr::GetInstance()->GetFont(8);
    if (font != NULL) {
        int width = font->MeasureTextWidth(text, -1, -1, NULL);
        int height = font->GetFontHeight();
        int x = area.m_bounds.m_x - width / 2 + (area.m_bounds.m_width >> 1) + App::Scale() * 2;
        int y = area.m_bounds.m_y - height / 2 - App::Scale() * 3;
        font->PaintText(text, wcslen(text), x, y, -1, -1);
    }
}

void GamePlayHUD::DrawHealthReplenishButton()
{
    if (!static_cast<App*>(WindowApp::m_instance)->m_settings->m_field1f
        || static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->IsIntroTime()) {
        return;
    }
    CRectangle& bounds = m_medkitArea.m_bounds;
    static_cast<App*>(WindowApp::m_instance)->m_helper->DrawImage(
        m_medkit, bounds.m_x + (bounds.m_width >> 1), bounds.m_y + (bounds.m_height >> 1), 1.0f, 0, 0, 0.0f,
        DGHelper::DRAW_HCENTER | DGHelper::DRAW_VCENTER);
    DrawNumberWithinThe(m_medkitArea, CBH_Player::GetInstance()->m_medkits);
}

void GamePlayHUD::DrawGoPillButton()
{
    if (!static_cast<App*>(WindowApp::m_instance)->m_settings->m_field1f
        || static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->IsIntroTime()) {
        return;
    }
    CRectangle& bounds = m_tabletArea.m_bounds;
    static_cast<App*>(WindowApp::m_instance)->m_helper->DrawImage(
        m_tablet, bounds.m_x + (bounds.m_width >> 1), bounds.m_y + (bounds.m_height >> 1), 1.0f, 0, 0, 0.0f,
        DGHelper::DRAW_HCENTER | DGHelper::DRAW_VCENTER);
    int pills = CBH_Player::GetInstance()->GetRemainingPillCount();
    int speedMs = CBH_Player::GetInstance()->GetSpeedTimeMS();
    int seconds;
    if (CBH_Player::GetInstance()->GetReactixControlTimeSec() >= 1) {
        seconds = CBH_Player::GetInstance()->GetReactixControlTimeSec();
    } else {
        DrawNumberWithinThe(m_tabletArea, pills);
        seconds = speedMs / 1000;
    }
    if (seconds > 0) {
        wchar_t text[20];
        const wchar_t* format = L"%d : %d";
        if (seconds % 60 < 10) {
            format = L"%d : 0%d";
        }
        ICStdUtil::SWPrintF(text, format, seconds / 60, seconds % 60);
        DrawStringAboveThe(m_tabletArea, text);
    }
}

void GamePlayHUD::DrawUnltrasoundEmitterButton()
{
    if (!static_cast<App*>(WindowApp::m_instance)->m_settings->m_field1f
        || static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->IsIntroTime()) {
        return;
    }
    CRectangle& bounds = m_ultrasoundArea.m_bounds;
    static_cast<App*>(WindowApp::m_instance)->m_helper->DrawImage(
        m_ultrasound, bounds.m_x + (bounds.m_width >> 1), bounds.m_y + (bounds.m_height >> 1), 1.0f, 0, 0, 0.0f,
        DGHelper::DRAW_HCENTER | DGHelper::DRAW_VCENTER);
    CBH_Player* player = CBH_Player::GetInstance();
    int stunMs = CSwerveGame::m_pInstance->m_stunMs;
    DrawNumberWithinThe(m_ultrasoundArea, player->m_ultrasoundAttemptCount);
    if (stunMs > 999) {
        int seconds = stunMs / 1000;
        wchar_t text[20];
        const wchar_t* format = L"%d : %d";
        if (seconds % 60 < 10) {
            format = L"%d : 0%d";
        }
        ICStdUtil::SWPrintF(text, format, stunMs / 60000, seconds % 60);
        DrawStringAboveThe(m_ultrasoundArea, text);
    }
}

void GamePlayHUD::Shot()
{
    CZoomedScopeWindow* scope = m_zoomedScope;
    if (scope != NULL) {
        scope->m_shooting = true;
        scope->m_shotTime = 0;
    }
}

void GamePlayHUD::PlayEnjureEffect(float angle, bool armor)
{
    if (armor) {
        if (m_armorHitWindow != NULL) {
            m_armorHitWindow->StartEffect(angle);
        }
    } else {
        if (m_woundingWindow != NULL) {
            m_woundingWindow->StartEffect(angle);
        }
    }
}

void GamePlayHUD::WoundingWindow::StartEffect(float angle)
{
    if (static_cast<App*>(WindowApp::m_instance)->m_settings->m_field1f) {
        m_remainingMs = 667;
        SwerveHelper::Transform rotation(CSwerve::GetInstance()->m_swerve);
        float angleAxis[4];
        angleAxis[0] = angle;
        angleAxis[1] = 0.0f;
        angleAxis[2] = 0.0f;
        angleAxis[3] = 1.0f;
        rotation.postRotate(angleAxis);
        m_arrow.setTransform(rotation);
    }
}

void GamePlayHUD::PlayReviveEffect(float)
{
    CDH_Weapon* weapon = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetActiveWeapon();
    if (weapon == NULL) {
        return;
    }
    weapon->ResetAutoShot();
    weapon->unfetchWeaponModel();
    m_modelAnimationState = 4;
}

void GamePlayHUD::ShowCountdownTimer(bool show)
{
    if (m_multiplierBar != NULL) {
        m_showCountdownTimer = show;
    }
}

void GamePlayHUD::SetCountdownTimerValue(int elapsedMs)
{
    if (m_multiplierBar != NULL) {
        m_multiplierBar->SetCurrValue(elapsedMs / 1000);
    }
}

void GamePlayHUD::SetCountdownTimerLimit(int limitMs)
{
    if (m_multiplierBar != NULL) {
        m_multiplierBar->SetMaxValue(limitMs / 1000);
    }
}

void GamePlayHUD::SetCountdownTimerMultiplier(int multiplier)
{
    if (m_multiplierBar != NULL) {
        switch (multiplier) {
        case 1:
            m_multiplierBar->SetDescImage("IDB_HUD_ICON_XP_MULTY_1");
            break;
        case 2:
            m_multiplierBar->SetDescImage("IDB_HUD_ICON_XP_MULTY_2");
            break;
        }
    }
}

void GamePlayHUD::SetMissionTimerValue(int elapsedMs)
{
    if (m_missionTimer != NULL) {
        m_missionTimer->SetCurrValue(elapsedMs / 1000);
    }
}

GamePlayHUD::CZoomedScopeWindow::CZoomedScopeWindow()
    : ViewWindow(World()),
      m_shoot(NULL), m_zoom(NULL), m_zoomPosition(0.0f), m_shotPosition(0.0f), m_zoomRecoil(0.0f),
      m_weaponId(0), m_reservedD4(0), m_worldTime(0), m_shotTime(0), m_shooting(false),
      m_zoomEnabled(false)
{
#ifndef CK_PC // PORT: layout check of the ARMv7 build only
    typedef char ScopeWindowSizeCheck[sizeof(CZoomedScopeWindow) == 0xe4 ? 1 : -1];
#endif
    CDH_Weapon* weapon = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetActiveWeapon();
    if (weapon != NULL) {
        InitResources(weapon);
    }
}

void GamePlayHUD::CZoomedScopeWindow::InitResources(CDH_Weapon* weapon)
{
    if (weapon != NULL) {
        m_weaponId = weapon->GetId();
        m_shoot = NULL;
        m_zoom = NULL;
        m_animations.Reset();
        m_world.Clear();
        XString::AnsiString model(weapon->m_scopeHudModel.c_str());
        m_world = World(App::LoadObject3D(model));
        if (m_world != NULL) {
            m_animations.LoadFrom(m_world);
            m_shoot = m_animations.Anim(SwerveHelper::Id("shoot"));
            m_zoom = m_animations.Anim(SwerveHelper::Id("zoom"));
        }
    }
}

GamePlayHUD::CZoomedScopeWindow::~CZoomedScopeWindow()
{
    m_shoot = NULL;
    m_zoom = NULL;
    m_animations.Reset();
}

void GamePlayHUD::CZoomedScopeWindow::Update()
{
    if (m_shoot == NULL || m_zoom == NULL) {
        return;
    }
    CDH_Weapon* weapon = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetActiveWeapon();
    if (weapon->GetId() != m_weaponId) {
        InitResources(weapon);
    }
    if (m_shooting && m_shoot->m_durationSec > 0.0f) {
        float duration = m_shoot->m_durationSec;
        float phase = m_shotPosition / duration;
        float angle = (phase + phase - 1.0f) * 3.141580104827881f;
        m_zoomRecoil = (duration - m_zoomPosition) * (cos(angle) + 1.0) * 0.5;
    } else {
        m_zoomRecoil = 0.0f;
    }
    m_shoot->Start(0.0f, 1.0f, 0.0f, 0.0f, 1.0f);
    m_zoom->Start(0.0f, 1.0f, 0.0f, 0.0f, 1.0f);
    m_zoom->SetPositionSec(m_zoomPosition + m_zoomRecoil);
    m_shoot->SetPositionSec(m_shotPosition);
    m_shoot->Pause();
    m_zoom->Pause();
    if (m_shooting) {
        m_shotPosition = m_shotTime * 0.001f;
        m_shotTime += WindowApp::m_instance->m_updateFrameTime;
        if (m_shotPosition > m_shoot->m_durationSec) {
            m_shotTime = 0;
            m_shooting = false;
            m_shotPosition = 0.0f;
        }
    }
    m_worldTime += WindowApp::m_instance->m_updateFrameTime;
    m_world.animate(m_worldTime);
}

void GamePlayHUD::CZoomedScopeWindow::Paint3D(Graphics3D& graphics)
{
    if (m_zoomEnabled) {
        ViewWindow::Paint3D(graphics);
    }
}

GamePlayHUD::WoundingWindow::WoundingWindow(bool armor)
    : ViewWindow(World()), m_remainingMs(0)
{
#ifndef CK_PC // PORT: layout check of the ARMv7 build only
    typedef char WoundingWindowSizeCheck[sizeof(WoundingWindow) == 0xb4 ? 1 : -1];
#endif
    if (armor) {
        m_world = World(App::LoadObject3D("BIN_ARMOR_HIT"));
    } else {
        m_world = World(App::LoadObject3D("BIN_WOUNDING"));
    }
    m_screen = SwerveHelper::Group(m_world.find(DGHelper::getSwerveID("injure_screenTransform")));
    m_arrow = SwerveHelper::Group(m_world.find(DGHelper::getSwerveID("arrowTransform")));
}

void GamePlayHUD::WoundingWindow::StopEffect()
{
    m_remainingMs = 0;
    if (m_screen != NULL) {
        m_screen.animate(0);
    }
    m_arrow.animate(0);
}

void GamePlayHUD::WoundingWindow::Update()
{
    if (m_remainingMs > 0) {
        m_remainingMs -= WindowApp::m_instance->m_updateFrameTime;
        if (m_screen != NULL) {
            m_screen.animate(667 - m_remainingMs);
        }
        m_arrow.animate(667 - m_remainingMs);
        if (m_remainingMs < 1) {
            StopEffect();
        }
    }
}

int GamePlayHUD::PointerPressed(int x, int y, int pointer)
{
    if (!m_inputEnabled) {
        return -1;
    }
    if (m_modelAnimationState != 0) {
        return 6;
    }
    const CAmmo* ammo01 = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon01(true)
        ->GetActiveAmmo();
    const CAmmo* ammo02 = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon02(true)
        ->GetActiveAmmo();
    int available01 = CBH_Player::GetInstance()->GetAvailableAmmo(ammo01, true);
    int available02 = CBH_Player::GetInstance()->GetAvailableAmmo(ammo02, true);
    if (!m_enabled) {
        return -1;
    }
    if (static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->IsIntroTime()) {
        return -1;
    }
    if (CSwerveGame::m_pInstance->m_specialWeapon == NULL && m_scopeButtonShown && m_scopeArea.checkCollision(x, y)) {
        CDH_Weapon* weapon = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetActiveWeapon();
        if (weapon != NULL && weapon->IsReadyForUse()) {
            static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->m_usedHudButton = true;
            switchScope();
            return 6;
        }
        return -1;
    }
    if (!m_scoped && m_hasAlternateWeapon && m_weaponSwitchArea.checkCollision(x, y) && available01 + available02 > 0) {
        CDH_Weapon* weapon = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetActiveWeapon();
        if (weapon != NULL && weapon->IsReadyForUse()) {
            switchWeapons();
            return 6;
        }
        return -1;
    }
    if (static_cast<App*>(WindowApp::m_instance)->m_settings->IsFireButtonEnabled() && m_fireArea.checkCollision(x, y)) {
        m_fireHeld = true;
        m_firePointer = pointer;
        return 2;
    }
    if (m_showMedkit && m_medkitArea.checkCollision(x, y)) {
        return 4;
    }
    if (m_steadyRemainingMs >= 0 && !m_steadyActive && m_steadyArea.checkCollision(x, y)) {
        // the original tests the flag again after the hit test
        if (m_steadyActive) {
            return 1;
        }
        static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->m_usedSteadyAim = true;
        m_steadyActive = true;
        CGameAnalytics::logSteadyActivated(
            static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_missionName,
            static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetActiveWeapon()->GetId());
        return 0;
    }
    if (CBH_Player::GetInstance()->IsSpeedModeAvailable() && m_tabletArea.checkCollision(x, y)) {
        if (CBH_Player::GetInstance()->GetReactixControlTimeSec() >= 1) {
            if (CBH_Player::GetInstance()->IsSpeedMode()) {
                CBH_Player::GetInstance()->StopConsumeReactixControl();
                return 1;
            }
            CBH_Player::GetInstance()->StartConsumeReactixControl();
            CGameAnalytics::logItemUsed(XString("reactix_control"));
        } else {
            CBH_Player::GetInstance()->ConsumeGoPill();
            CGameAnalytics::logItemUsed(XString("reactix"));
        }
        return 1;
    }
    if (CBH_Player::GetInstance()->m_ultrasoundAttemptCount > 0 && m_ultrasoundArea.checkCollision(x, y)) {
        CBH_Player::GetInstance()->ConsumeUltrasoundEmitter();
        CSwerveGame::m_pInstance->AddStunTimerMS(10000);
        return 5;
    }
    if (!m_reloadTouchArea.checkCollision(x, y)) {
        return -1;
    }
    if (CDH_Weapon* weapon = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetActiveWeapon()) {
        const CAmmo* ammo = weapon->GetActiveAmmo();
        if (ammo != NULL && CBH_Player::GetInstance()->GetAvailableAmmo(ammo, true) != 0xfffffff) {
            int clip = weapon->m_ammo;
            CBH_Player* player = CBH_Player::GetInstance();
            int available = player->GetAvailableAmmo(weapon->GetActiveAmmo(), true);
            int clipSize = weapon->m_clipSize;
            if (weapon->IsReadyForUse()) {
                if (weapon->IsReloading()) {
                    return 1;
                }
                if (clip < available && clip < clipSize) {
                    weapon->CheckForReload(true);
                }
            }
            return 1;
        }
    }
    return -1;
}

void GamePlayHUD::CheckContollStateOnPointerReleased(int x, int y, int pointer)
{
    if (static_cast<App*>(WindowApp::m_instance)->m_settings->IsFireButtonEnabled() && m_fireArea.checkCollision(x, y)
        && m_firePointer == pointer) {
        m_fireHeld = false;
        m_firePointer = -1;
    }
}

void GamePlayHUD::CheckContollStateOnPointerMove(int x, int y, int oldX, int oldY, int pointer)
{
    if (static_cast<App*>(WindowApp::m_instance)->m_settings->IsFireButtonEnabled()) {
        if (!m_fireArea.checkCollision(x, y) && m_fireArea.checkCollision(oldX, oldY) && m_firePointer == pointer) {
            m_fireHeld = false;
            m_firePointer = -1;
        }
    }
}

void GamePlayHUD::Show()
{
    m_enabled = true;
}

void GamePlayHUD::Hide(bool immediate)
{
    m_enabled = false;
    if (immediate) {
        m_visibilityAlpha = 0.0f;
    }
}

void GamePlayHUD::SetZoomSliderEnable(bool enable)
{
    m_zoomSliderControl->Enable(enable);
    m_zoomedScope->m_zoomEnabled = enable;
}

float GamePlayHUD::GetZoomSliderProgress() const
{
    if (m_zoomSliderControl == NULL) {
        return 0.0f;
    }
    return m_zoomSliderControl->GetValue();
}

void GamePlayHUD::ReInitWeaponButton()
{
    CDH_Weapon* weapon01 = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon01(true);
    CDH_Weapon* weapon02 = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon02(true);
    if (weapon01 == weapon02) {
        m_hasAlternateWeapon = false;
    }
    m_weaponImages[0] = weapon01->getGameImage();
    m_weaponImages[1] = weapon02->getGameImage();
}

void GamePlayHUD::CleanBulletIcon()
{
    CDH_Weapon* weapon = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon01(true);
    if (weapon != NULL) {
        const CAmmo* ammo = weapon->GetActiveAmmo();
        if (ammo != NULL) {
            CBH_SurfaceManager* surfaces = static_cast<App*>(WindowApp::m_instance)->m_surfaceManager;
            XString icon = ammo->m_bulletIcon;
            surfaces->ReleaseSurface(XString::AnsiString(icon.c_str()));
        }
    }
}

void GamePlayHUD::ReInitBulletIcon(CDH_Weapon* weapon)
{
    if (weapon != NULL) {
        const CAmmo* ammo = weapon->GetActiveAmmo();
        if (ammo != NULL) {
            CBH_SurfaceManager* surfaces = static_cast<App*>(WindowApp::m_instance)->m_surfaceManager;
            {
                XString icon = ammo->m_bulletIcon;
                m_bulletIcon = surfaces->CreateSurface(XString::AnsiString(icon.c_str()), true);
            }
            m_ammo = ammo;
        }
    }
}

bool GamePlayHUD::isWeaponModelVisible() const
{
    if (!m_scoped) {
        return true;
    }
    return m_modelAnimationState != 0;
}

void GamePlayHUD::OnScopeSwitched()
{
    m_scoped = !m_scoped;
    m_modelAnimationState = 0;
}

void GamePlayHUD::EnableDockCoverButton(bool enable)
{
    if (m_coverButton != NULL) {
        m_coverButton->Enable(enable);
    }
}

void GamePlayHUD::SetDockCoverButtonVisibility(bool hide)
{
    if (m_coverButton != NULL) {
        // the original only toggles visibility
        const unsigned int visibleFlag = FLAG_VISIBLE;
        if (!hide) {
            m_coverButton->SetFlags(visibleFlag);
        } else {
            m_coverButton->ClearFlags(visibleFlag);
        }
    }
}

void GamePlayHUD::ChangeDockCoverButtonState()
{
    if (m_coverButton != NULL) {
        m_coverButton->ChangeState();
    }
}

void GamePlayHUD::OnCommand(Event& event)
{
    if (m_commandsReady) {
        if (event.m_id == static_cast<int>(0x811575c5u)) {
            static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->ChangeCover();
            event.Clear();
        } else if (event.m_id == 0x2ebf5358) {
            CIngameShop* shop = new CIngameShop();
            static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddModal(shop);
        }
    }
}

int GamePlayHUD::GetActiveWeaponIndex()
{
    CDH_Weapon* weapon01 = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon01(true);
    // The second weapon is fetched but not compared (original behaviour).
    static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon02(true);
    CDH_Weapon* active = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetActiveWeapon();
    return active != weapon01;
}

void GamePlayHUD::SetEnabled(bool enabled)
{
    m_inputEnabled = enabled;
    const unsigned int enabledFlag = FLAG_ENABLED;
    if (enabled) {
        SetFlags(enabledFlag);
        SetFlags(FLAG_EVENTS_TO_CHILDREN);
    } else {
        ClearFlags(enabledFlag);
        ClearFlags(FLAG_EVENTS_TO_CHILDREN);
    }
}

void GamePlayHUD::BlinkIngameShop(int duration)
{
    m_ingameShop->Blink(duration);
}

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AssertGamePlayHUDSize[sizeof(GamePlayHUD) == 0x330 ? 1 : -1];
#endif
