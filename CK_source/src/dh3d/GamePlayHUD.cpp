#include "GamePlayHUD.h"
#include "CApplet.h"
#include "ICResourceManager.h"

#include <math.h>
#include "App.h"
#include "AppSettings.h"
#include "CAmmo.h"
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
#include "CFont.h"
#include "CFontMgr.h"
#include "CImageButton.h"
#include "DGHelper.h"
#include "dgCamera.h"
#include "ICRenderSurface.h"
#include "SScreenAdaptingConstant.h"
#include "ServiceWindows.h"
#include "SwerveHelper.h"
#include "ICStdUtil.h"

namespace {
const int ReviveDurationMs = 5000;
const int ReviveSoundDelayMs = 1800;
const int WoundingDurationMs = 667;
enum ModelAnimationState {
    ModelReady = 0, WeaponLeaving = 1, WeaponEntering = 2, ScopeTransition = 3,
    ReviveWeaponLeaving = 4, RevivePlaying = 5
};

bool Contains(const CRectangle& bounds, int x, int y)
{
    return x >= bounds.m_x && x < bounds.m_x + bounds.m_width
        && y >= bounds.m_y && y < bounds.m_y + bounds.m_height;
}
}

// The original window is allocated separately and drawn by GamePlayHUD::Paint.
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

class GamePlayHUD::WoundingWindow : public ViewWindow {
public:
    explicit WoundingWindow(bool armor);
    void StartEffect(float angle);
    void StopEffect();
    virtual void Update();

private:
    int m_remainingMs;                // 0xa8
    SwerveHelper::Group m_screen;     // 0xac
    SwerveHelper::Group m_arrow;      // 0xb0
};

class GamePlayHUD::ReviveWindow : public ViewWindow {
public:
    ReviveWindow();
    void StopEffect();
    virtual void Update();

    int m_remainingMs;
    SwerveHelper::Group m_controller;
    bool m_soundPlayed;
};

GamePlayHUD::ReviveWindow::ReviveWindow()
    : ViewWindow(World()), m_remainingMs(0), m_soundPlayed(false)
{
    m_world = World(App::LoadObject3D("BIN_PHOENIX_SERUM"));
    if (m_world != NULL) {
        Background background = m_world.getBackground();
        if (background != NULL) {
            background.setColorClearEnable(false);
        }
        m_controller = SwerveHelper::Group(m_world.find(DGHelper::getSwerveID("weaponControllerTransform")));
    }
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
    if (m_remainingMs <= 0) {
        return;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    m_remainingMs -= app->m_updateFrameTime;
    int elapsed = ReviveDurationMs - m_remainingMs;
    if (m_controller != NULL) {
        m_controller.animate(elapsed);
    }
    if (!m_soundPlayed && elapsed > ReviveSoundDelayMs) {
        app->m_soundManager->PlaySound("IDM_SPECIAL_REVIVE", false, false);
        m_soundPlayed = true;
    }
    if (m_remainingMs <= 0) {
        StopEffect();
    }
}

namespace {
void SetBounds(CRectangle& bounds, ICRenderSurface* surface, int x, int y, unsigned int anchor)
{
    unsigned int width = 0;
    unsigned int height = 0;
    if (surface != NULL) {
        surface->GetWidthAndHeight(width, height);
    }
    if (anchor & DGHelper::DRAW_RIGHT) {
        x -= width;
    }
    if (anchor & DGHelper::DRAW_BOTTOM) {
        y -= height;
    }
    bounds.m_x = x;
    bounds.m_y = y;
    bounds.m_width = width;
    bounds.m_height = height;
}

void DrawControl(DGHelper* helper, ICRenderSurface* image, const CRectangle& bounds)
{
    if (image != NULL) {
        helper->DrawImage(image, bounds.m_x, bounds.m_y, 1.0f, 0, 0, 0.0f, 0);
    }
}

void PaintBar(ICGraphics2d& graphics, CBH_ProgressBar* bar)
{
    int x = bar->m_x * 65536;
    int y = bar->m_y * 65536;
    graphics.Translate(x, y);
    bar->Paint(graphics);
    graphics.Translate(-x, -y);
}
}

GamePlayHUD::GamePlayHUD()
    : Window(false), m_displayedAmmo(NULL), m_scoped(false), m_hasAlternateWeapon(true),
      m_ingameShop(NULL), m_weaponSwitch(NULL),
      m_steadyOn(NULL), m_steadyOff(NULL),
      m_scopeEnter(NULL), m_scopeExit(NULL), m_bulletIcon(NULL),
      m_silencerIcon(NULL), m_reload(NULL), m_medkit(NULL), m_reactix(NULL), m_ultrasound(NULL),
      m_fireOn(NULL), m_fireOff(NULL), m_tapToShoot(NULL), m_aimAssistIcon(NULL),
      m_cameraFrame(NULL), m_cameraFrameSelected(NULL),
      m_healthBar(NULL), m_armorBar(NULL),
      m_multiplierBar(NULL), m_missionTimer(NULL), m_showCountdownTimer(false), m_firePointer(-1), m_fireHeld(false),
      m_zoomedScope(new CZoomedScopeWindow()), m_displayedMultiplier(3),
      m_reviveWindow(NULL), m_steadyRemainingMs(0), m_steadyLimitMs(0), m_steadyActive(false),
      m_steadyUsed(false), m_visibilityAlpha(0.0f), m_enabled(true),
      m_currentHint(NULL), m_modelAnimationState(ModelReady)
{
    m_weaponImages[0] = NULL;
    m_weaponImages[1] = NULL;
    m_weaponImagesFor[0] = NULL;
    m_weaponImagesFor[1] = NULL;
    m_pauseButton = new CDH_GameSofkeyWidget();
    m_pauseButton->SetAlign(ALIGN_LEFT | ALIGN_TOP);
    int pauseInset = App::Scale() * 5;
    m_pauseButton->SetOutsetSpacing(pauseInset, 0);
    m_pauseButton->ClearFlags(FLAG_VISIBLE | FLAG_ENABLED);
    AddToFront(m_pauseButton);
    m_woundingWindow = new WoundingWindow(false);
    m_armorHitWindow = new WoundingWindow(true);
    AddToFront(m_woundingWindow);
    AddToFront(m_armorHitWindow);

    InitFireButton();
    static_cast<App*>(WindowApp::m_instance)->m_helper->LoadImageSurface(
        &m_tapToShoot, "SUR_TUTORIAL_TAP_TO_SHOT", false);
    InitWeaponButton();
    InitSteadyButton();
    InitScopeButton();
    InitReloadButton();
    InitCameraButton();
    InitZoomAndDock();
    InitHealthReplehishButton();
    InitGoPillButton();
    InitUltrasoundEmitterButton();
    UpdateButtonPosition();
    InitIngameShopButton();
    InitBars();
}

inline GamePlayHUD::TutorialHint::~TutorialHint()
{
    CApplet::m_pApp->m_resourceManager->ReleaseResource(m_imageName);
}

GamePlayHUD::~GamePlayHUD()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CleanBulletIcon();
    CleanWeaponImage();
    ICResourceManager* resources = CApplet::m_pApp->m_resourceManager;
    if (m_cameraFrame != NULL) {
        resources->ReleaseResource("SUR_HUD_CAMERA_FRAME");
    }
    if (m_cameraFrameSelected != NULL) {
        resources->ReleaseResource("SUR_HUD_CAMERA_FRAME_SELECT");
    }
    if (m_tapToShoot != NULL) {
        resources->ReleaseResource("SUR_TUTORIAL_TAP_TO_SHOT");
    }
    ICRenderSurface* surfaces[] = {
        m_fireOn, m_fireOff, m_weaponSwitch, m_scopeEnter, m_scopeExit, m_reload,
        m_medkit, m_reactix, m_ultrasound, m_steadyOn, m_steadyOff,
        m_silencerIcon, m_aimAssistIcon
    };
    for (unsigned int index = 0; index < sizeof(surfaces) / sizeof(surfaces[0]); ++index) {
        if (surfaces[index] != NULL) {
            app->m_surfaceManager->ReleaseSurface(surfaces[index]);
        }
    }
    SAFE_DELETE_VECTOR(m_tutorialHints);
    delete m_missionTimer;
    delete m_multiplierBar;
    delete m_armorBar;
    delete m_healthBar;
    delete m_zoomedScope;
    delete m_reviveWindow;
}

void GamePlayHUD::InitFireButton()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    m_fireOn = app->m_surfaceManager->CreateSurface("SUR_HUD_FIRE_BUTTON_ON", true);
    m_fireOff = app->m_surfaceManager->CreateSurface("SUR_HUD_FIRE_BUTTON_OFF", true);
    if (m_fireOn != NULL && m_fireOff != NULL) {
        CRectangle bounds;
        SetBounds(bounds, m_fireOn, app->m_helper->m_screenWidth, app->m_helper->m_screenHeight,
                  DGHelper::DRAW_RIGHT | DGHelper::DRAW_BOTTOM);
        m_fireHitArea.append(bounds, 0, 0);
    }
}

void GamePlayHUD::InitWeaponButton()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    m_weaponSwitch = app->m_surfaceManager->CreateSurface("SUR_HUD_WEAPON_SWITCH_BUTTON", true);
    m_silencerIcon = app->m_surfaceManager->CreateSurface("IDB_HUD_SILENCER", true);
    m_aimAssistIcon = app->m_surfaceManager->CreateSurface("IDB_HUD_AIM_ASSIST_ICON", true);
    ReInitWeaponButton();
    CDH_Weapon* weapon = app->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
    if (weapon != NULL) {
        UpdateAmmoIcon(weapon->GetActiveAmmo());
    } else {
        UpdateAmmoIcon(NULL);
    }
    SetBounds(m_weaponBounds, m_weaponSwitch, app->m_helper->m_screenWidth, m_fireHitArea.m_bounds.m_y,
              DGHelper::DRAW_RIGHT | DGHelper::DRAW_BOTTOM);
    m_weaponBounds.m_y -= m_weaponBounds.m_height / 2;
}

void GamePlayHUD::InitCameraButton()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_helper->LoadImageSurface(&m_cameraFrame, "SUR_HUD_CAMERA_FRAME", false);
    app->m_helper->LoadImageSurface(&m_cameraFrameSelected, "SUR_HUD_CAMERA_FRAME_SELECT", false);
}

void GamePlayHUD::setCamerasCount(int count)
{
    m_cameraButtonAreas.setSize(0);
    if (m_cameraFrame == NULL || count <= 0) {
        return;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    unsigned int width = 0;
    unsigned int height = 0;
    m_cameraFrame->GetWidthAndHeight(width, height);
    int step = static_cast<int>(height);
    int top = (app->m_helper->m_screenHeight - count * step) / 2;
    for (int index = 0; index < count; ++index) {
        CRectangle bounds(2, top + index * step, width, height);
        m_cameraButtonAreas.addElement(Collision(bounds, 0, 0));
    }
}

void GamePlayHUD::InitIngameShopButton()
{
    m_ingameShop = new CImageButton();
    m_ingameShop->AddImage("IDB_HUD_INGAME_SHOP_BUTTON_ON", 0, 0);
    m_ingameShop->m_command = 0x2ebf5358;
    m_ingameShop->SetLayoutType(LAYOUT_SIMPLE);
    m_ingameShop->SetAlign(ALIGN_LEFT | ALIGN_TOP);
    const SScreenAdaptingConstant top(50, 100);
    m_ingameShop->SetOutsetSpacing(top(), 0, 0, 0);
    m_ingameShop->ClearFlags(FLAG_VISIBLE | FLAG_ENABLED);
    AddToFront(m_ingameShop);
}

void GamePlayHUD::BlinkIngameShop(int duration)
{
    m_ingameShop->Blink(duration);
}

void GamePlayHUD::InitScopeButton()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    m_scopeEnter = app->m_surfaceManager->CreateSurface("SUR_HUD_SCOPE_BUTTON_ENTER", true);
    m_scopeExit = app->m_surfaceManager->CreateSurface("SUR_HUD_SCOPE_BUTTON_EXIT", true);
    SetBounds(m_scopeBounds, m_scopeEnter, 0, app->m_helper->m_screenHeight, DGHelper::DRAW_BOTTOM);
}

void GamePlayHUD::InitReloadButton()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    m_reload = app->m_surfaceManager->CreateSurface("IDB_HUD_RELOAD_BUTTON_ON", true);
    CRectangle rectangle;
    SetBounds(rectangle, m_reload, 0, 0, 0);
    m_reloadButtonArea.append(rectangle, 0, 0);
    const SScreenAdaptingConstant top(60, 120);
    m_reloadButtonArea.SetPosition(app->m_screenWidth - rectangle.m_width, top());
    rectangle.m_width *= 2;
    rectangle.m_height = rectangle.m_height * 2 + m_reloadButtonArea.m_bounds.m_y
        + m_reloadButtonArea.m_bounds.m_height;
    m_reloadTouchArea.append(rectangle, 0, 0);
    m_reloadTouchArea.SetPosition(app->m_screenWidth - rectangle.m_width, 0);
}

void GamePlayHUD::InitZoomAndDock()
{
    m_zoomSliderControl = new CZoomSliderControl("SUR_ZOOM_SLIDER", "SUR_ZOOM_TRACK");
    int centerOffset = 168;
    int radius = 216;
    if (App::IsXGA()) {
        centerOffset = 370;
        radius = 463;
    } else if (App::IsVGA()) {
        centerOffset = 330;
        radius = 424;
    }
    m_zoomSliderControl->SetParams(21, centerOffset, radius);
    m_zoomSliderControl->SetProgress(1.0f);
    m_zoomSliderControl->Enable(false);
    AddToFront(m_zoomSliderControl);

    CDH_GameSofkeyWidget::GetPauseButtonHeight();
    m_coverButton = new CBH_TriggerButton("SUR_DOCKCOVER_UNHIDE", "SUR_DOCKCOVER_HIDE");
    m_coverButton->SetAlign(ALIGN_LEFT | ALIGN_BOTTOM);
    m_coverButton->SetOutsetSpacing(m_scopeBounds.m_height + m_coverButton->m_height / 2, 0);
    m_coverButton->SetCommand(static_cast<int>(0x811575c5u));
    m_coverButton->m_commandParam = 0;
    m_coverButton->ClearFlags(FLAG_VISIBLE | FLAG_ENABLED);
    AddToFront(m_coverButton);

    // The neighboring candidate consumable button still uses manual bounds.
}

void GamePlayHUD::InitBars()
{
    const SScreenAdaptingConstant width(110, 220);
    const SScreenAdaptingConstant healthX(11, 22);
    const SScreenAdaptingConstant healthY(6, 12);
    const SScreenAdaptingConstant multiplierX(158, 316);
    const SScreenAdaptingConstant timerX(270, 540);
    const SScreenAdaptingConstant timerY(32, 64);
    const SScreenAdaptingConstant timerWidth(75, 150);
    App* app = static_cast<App*>(WindowApp::m_instance);
    const CHuntingInfo::SAssignedData& assignment = app->m_huntingInfo->m_assignedData;
    int x = ImageWidth(m_pauseButton->m_images[0].GetSurface()) + healthX() + 2;
    int y = healthY() + 2;

    m_healthBar = new CBH_ProgressBar(2, width(), -1);
    m_healthBar->SetCorner(x, y);
    m_healthBar->SetDescImage("IDB_HUD_ICON_HEALTH");
    y += m_healthBar->m_height;

    m_armorBar = new CBH_ProgressBar(1, width(), -1);
    m_armorBar->SetCorner(x, y);
    m_armorBar->SetDescImage("IDB_HUD_ICON_ARMOR");

    m_multiplierBar = new CBH_ProgressBar(4, width(), -1);
    m_multiplierBar->SetCorner(multiplierX(), y);
    m_multiplierBar->SetStyle(CBH_ProgressBar::STYLE_CLOCK);
    m_multiplierBar->SetDescImage("IDB_HUD_ICON_XP_MULTY_3");

    if (assignment.m_cameraDurationMs > 0) {
        m_missionTimer = new CBH_ProgressBar(6, timerWidth(), -1);
        m_missionTimer->SetCorner(timerX(), timerY());
        m_missionTimer->SetStyle(CBH_ProgressBar::STYLE_CLOCK);
        m_missionTimer->SetCurrValue(0);
        m_missionTimer->SetMaxValue(assignment.m_cameraDurationMs / 1000);
        m_missionTimer->SetDescImage("IDB_HUD_ICON_TIMER");
    }
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
    if (m_multiplierBar == NULL) {
        return;
    }
    switch (multiplier) {
    case 1:
        m_multiplierBar->SetDescImage("IDB_HUD_ICON_XP_MULTY_1");
        break;
    case 2:
        m_multiplierBar->SetDescImage("IDB_HUD_ICON_XP_MULTY_2");
        break;
    }
}

void GamePlayHUD::SetMissionTimerValue(int elapsedMs)
{
    if (m_missionTimer != NULL) {
        m_missionTimer->SetCurrValue(elapsedMs / 1000);
    }
}

void GamePlayHUD::InitSteadyButton()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    m_steadyOn = app->m_surfaceManager->CreateSurface("SUR_HUD_STEADY_BUTTON_ON", true);
    m_steadyOff = app->m_surfaceManager->CreateSurface("SUR_HUD_STEADY_BUTTON_OFF", true);
    float skill = app->m_helper->GetPlayerData()->GetSkillLevelCoef(static_cast<CDH_PlayerData::skill_type_t>(2));
    m_steadyLimitMs = static_cast<int>(5000.0f * (1.0f + 1.5f * skill));
    m_steadyRemainingMs = m_steadyLimitMs;
    SetBounds(m_steadyBounds, m_steadyOn, app->m_helper->m_screenWidth / 2,
        app->m_helper->m_screenHeight - 2, DGHelper::DRAW_HCENTER | DGHelper::DRAW_BOTTOM);
}

void GamePlayHUD::DrawSteadyButton()
{
    if (m_steadyRemainingMs <= 0 || m_steadyLimitMs <= 0 || m_steadyOn == NULL || m_steadyOff == NULL) {
        return;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    ICGraphics2d* graphics = ICGraphics2d::GetInstance();
    CRectangle clip = graphics->GetClip();
    int fill = m_steadyBounds.m_width * m_steadyRemainingMs / m_steadyLimitMs;
    int empty = m_steadyBounds.m_width - fill;
    graphics->SetClip(m_steadyBounds.m_x + empty, m_steadyBounds.m_y, fill, m_steadyBounds.m_height);
    DrawControl(app->m_helper, m_steadyOn, m_steadyBounds);
    if (empty > 0) {
        graphics->SetClip(m_steadyBounds.m_x, m_steadyBounds.m_y, empty, m_steadyBounds.m_height);
        DrawControl(app->m_helper, m_steadyOff, m_steadyBounds);
    }
    graphics->SetClip(clip);
}

void GamePlayHUD::InitHealthReplehishButton()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    m_medkit = app->m_surfaceManager->CreateSurface("SUR_HUD_HEALTH_REPLENISH_BUTTON", true);
    SetBounds(m_medkitArea.m_bounds, m_medkit, m_scopeBounds.m_x + m_scopeBounds.m_width + 2,
              app->m_helper->m_screenHeight - 2, DGHelper::DRAW_BOTTOM);
}

void GamePlayHUD::InitGoPillButton()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    m_reactix = app->m_surfaceManager->CreateSurface("SUR_HUD_TABLET_BUTTON", true);
    SetBounds(m_reactixArea.m_bounds, m_reactix, m_medkitArea.m_bounds.m_x + m_medkitArea.m_bounds.m_width + 2,
              app->m_helper->m_screenHeight - 2, DGHelper::DRAW_BOTTOM);
}

void GamePlayHUD::InitUltrasoundEmitterButton()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    m_ultrasound = app->m_surfaceManager->CreateSurface("SUR_HUD_ULTRASOUND_EMITTER", true);
    const SScreenAdaptingConstant gap(2, 4);
    const SScreenAdaptingConstant verticalOffset(3, 0);
    int x = m_reactixArea.m_bounds.m_x + m_reactixArea.m_bounds.m_width + 2 + gap();
    int y = app->m_helper->m_screenHeight - 2 + verticalOffset();
    SetBounds(m_ultrasoundArea.m_bounds, m_ultrasound, x, y, DGHelper::DRAW_BOTTOM);
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
        int x = area.m_bounds.m_x - width / 2 + (area.m_bounds.m_width >> 1) + 2 * App::Scale();
        int y = area.m_bounds.m_y - height / 2 + (area.m_bounds.m_height >> 1) - 3 * App::Scale();
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
        int x = area.m_bounds.m_x - width / 2 + (area.m_bounds.m_width >> 1) + 2 * App::Scale();
        int y = area.m_bounds.m_y - height / 2 - 3 * App::Scale();
        font->PaintText(text, wcslen(text), x, y, -1, -1);
    }
}

void GamePlayHUD::UpdateButtonPosition()
{
    CBH_Player* player = CBH_Player::GetInstance();
    m_showMedkit = player->CanRestoreHealth() && player->m_hp < player->GetMaxHp();
    m_showUltrasound = player->m_ultrasoundAttemptCount > 0;
}

void GamePlayHUD::DrawHealthReplenishButton()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    DrawControl(app->m_helper, m_medkit, m_medkitArea.m_bounds);
    DrawNumberWithinThe(m_medkitArea, CBH_Player::GetInstance()->m_medkits);
}

void GamePlayHUD::DrawGoPillButton()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CBH_Player* player = CBH_Player::GetInstance();
    DrawControl(app->m_helper, m_reactix, m_reactixArea.m_bounds);
    int seconds = player->GetReactixControlTimeSec();
    if (seconds < 1) {
        DrawNumberWithinThe(m_reactixArea, player->GetRemainingPillCount());
        seconds = player->GetSpeedTimeMS() / 1000;
    }
    if (seconds > 0) {
        XString text = XString::Format(L"%d : %02d", seconds / 60, seconds % 60);
        DrawStringAboveThe(m_reactixArea, text.c_str());
    }
}

void GamePlayHUD::DrawUnltrasoundEmitterButton()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    DrawControl(app->m_helper, m_ultrasound, m_ultrasoundArea.m_bounds);
    DrawNumberWithinThe(m_ultrasoundArea, CBH_Player::GetInstance()->m_ultrasoundAttemptCount);
    int seconds = app->m_helper->m_swerveGame->m_stunMs / 1000;
    if (seconds > 0) {
        XString text = XString::Format(L"%d : %02d", seconds / 60, seconds % 60);
        DrawStringAboveThe(m_ultrasoundArea, text.c_str());
    }
}

void GamePlayHUD::PaintBars(ICGraphics2d& graphics)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    const CSwerveGame* game = app->m_helper->m_swerveGame;
    const CHuntingInfo::SAssignedData& assignment = app->m_huntingInfo->m_assignedData;
    CBH_Player* player = game->m_player;
    m_healthBar->SetMaxValue(player->GetMaxHp() / 1000);
    m_healthBar->SetCurrValue((player->m_hp + 999) / 1000);
    PaintBar(graphics, m_healthBar);
    if (player->IsArmorAvailable()) {
        m_armorBar->SetMaxValue(player->m_armorDamageProofMax / 1000);
        m_armorBar->SetCurrValue(player->m_armorDamageProof / 1000);
        PaintBar(graphics, m_armorBar);
    }
    ShowCountdownTimer(game->m_multiplierEnabled && game->m_multiplier > 1);
    if (m_showCountdownTimer) {
        int limit = assignment.m_tripleMultiplierSeconds;
        if (game->m_multiplier == 2) {
            limit = assignment.m_doubleMultiplierSeconds;
        }
        if (m_displayedMultiplier != game->m_multiplier) {
            if (game->m_multiplier == 3) {
                m_multiplierBar->SetDescImage("IDB_HUD_ICON_XP_MULTY_3");
            } else {
                SetCountdownTimerMultiplier(game->m_multiplier);
            }
            m_displayedMultiplier = game->m_multiplier;
        }
        SetCountdownTimerLimit(limit * 1000);
        SetCountdownTimerValue(game->m_multiplierTimeMs);
        PaintBar(graphics, m_multiplierBar);
    }
    if (m_missionTimer != NULL) {
        PaintBar(graphics, m_missionTimer);
    }
}

void GamePlayHUD::Paint(ICGraphics2d& graphics)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CSwerveGame* game = app->m_helper->m_swerveGame;
    if (game->IsIntroTime() || m_visibilityAlpha <= 0.0f) {
        return;
    }
    m_zoomedScope->RunPaint3D(graphics);
    app->m_helper->PushAlphaBlend(m_visibilityAlpha);
    CDH_Weapon* weapon = app->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
    DrawScopeButton();
    DrawWeaponButton(weapon);
    DrawReloadButton(weapon);
    DrawFireButton();
    if (app->m_settings->m_field1f) {
        PaintBars(graphics);
        if (m_modelAnimationState == ModelReady) {
            DrawSteadyButton();
        }
    }
    if (app->m_settings->m_field1f) {
        if (m_showMedkit) {
            DrawHealthReplenishButton();
        }
        if (game->m_player->IsSpeedModeAvailable() || game->m_player->IsSpeedMode()) {
            DrawGoPillButton();
        }
        if (m_showUltrasound) {
            DrawUnltrasoundEmitterButton();
        }
    }
    if (m_currentHint != NULL) {
        const char* imageName = m_currentHint->m_imageName;
        ICRenderSurface* image = m_currentHint->m_image;
        int screenWidth = app->m_helper->m_screenWidth;
        int screenHeight = app->m_helper->m_screenHeight;
        int x = screenWidth / 2;
        int y = 10;
        unsigned int align = DGHelper::DRAW_HCENTER;
        if (strcmp(imageName, "SUR_TUTORIAL_SLIDE_TO_ZOOM_OUT") == 0) {
            x = 11;
            y = 144;
            align = DGHelper::DRAW_VCENTER;
        } else if (strcmp(imageName, "SUR_TUTORIAL_SLIDE_TO_ZOOM_IN") == 0) {
            x = screenWidth - 11;
            y = 144;
            align = DGHelper::DRAW_RIGHT | DGHelper::DRAW_VCENTER;
        } else if (strcmp(imageName, "SUR_TUTORIAL_SLIDE_TO_AIM") == 0) {
            y = 80;
        } else if (strcmp(imageName, "SUR_TUTORIAL_BUTTON_TO_SHOT") == 0) {
            if (app->m_settings->IsFireButtonEnabled()) {
                x = screenWidth - 50;
                y = screenHeight - 5;
                align = DGHelper::DRAW_RIGHT | DGHelper::DRAW_BOTTOM;
            } else {
                image = m_tapToShoot;
                y = screenHeight / 2;
                align = DGHelper::DRAW_HCENTER | DGHelper::DRAW_VCENTER;
            }
        } else if (strcmp(imageName, "SUR_TUTORIAL_SLIDE_TO_LOOK_AROUND") != 0) {
            image = NULL;
        }
        if (image != NULL) {
            app->m_helper->DrawImage(image, x, y, 0.6f, 0, 0, 0.0f, align);
        }
    }
    app->m_helper->PopAlphaBlend();
}

void GamePlayHUD::DrawFireButton()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (!app->m_settings->IsFireButtonEnabled() || app->m_helper->m_swerveGame->IsIntroTime()) {
        return;
    }
    CDH_Weapon* weapon = app->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
    ICRenderSurface* image = m_fireOff;
    if (weapon != NULL && weapon->IsReadyForShot()) {
        image = m_fireOn;
    }
    DrawControl(app->m_helper, image, m_fireHitArea.m_bounds);
}

void GamePlayHUD::DrawScopeButton()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_Weapon* weapon = app->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
    if (weapon != NULL && (weapon->m_scopeAttached || weapon->m_zoomWithoutScope)) {
        ICRenderSurface* image = m_scopeEnter;
        if (m_scoped) {
            image = m_scopeExit;
        }
        DrawControl(app->m_helper, image, m_scopeBounds);
    }
}

void GamePlayHUD::DrawReloadButton(CDH_Weapon* weapon)
{
    if (m_scoped || weapon == NULL) {
        return;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (CBH_Player::GetInstance()->GetAvailableAmmo(weapon->GetDefaultAmmo(), true) != 0xfffffff) {
        DrawControl(app->m_helper, m_reload, m_reloadButtonArea.m_bounds);
    }
}

void GamePlayHUD::DrawWeaponButton(CDH_Weapon* weapon)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (weapon == NULL || !app->m_settings->m_field1f) {
        return;
    }
    CDH_BasicGameData* data = app->m_helper->m_gamePlayManager.GetGameData();
    ICRenderSurface* image = NULL;
    if (weapon == m_weaponImagesFor[0]) {
        image = m_weaponImages[0];
    } else if (weapon == m_weaponImagesFor[1]) {
        image = m_weaponImages[1];
    } else {
        image = weapon->getGameImage();
    }
    int imageRight = app->m_helper->m_screenWidth - 5 * App::Scale();
    if (!m_scoped) {
        CDH_Weapon* first = data->GetWeapon01(true);
        CDH_Weapon* second = data->GetWeapon02(true);
        if (first != NULL && second != NULL && first != second) {
            DrawControl(app->m_helper, m_weaponSwitch, m_weaponBounds);
        }
        if (image != NULL) {
            app->m_helper->DrawImage(image, imageRight, 2, 1.0f, 0, 0, 0.0f, DGHelper::DRAW_RIGHT);
        }
    }

    const CAmmo* ammo = weapon->GetActiveAmmo();
    int available = CBH_Player::GetInstance()->GetAvailableAmmo(ammo, true);
    XString text(L"\u221e");
    if (ammo != NULL && available != 0xfffffff) {
        int reserve = available - weapon->m_ammo;
        if (reserve < 0) {
            reserve = 0;
        }
        text = XString::Format(L"%d / %d ", weapon->m_ammo, reserve);
    }
    CFont* font = CFontMgr::GetInstance()->GetFont(8);
    if (font == NULL) {
        return;
    }
    int x = imageRight - 6 * App::Scale() - font->MeasureTextWidth(text.c_str(), -1, -1, NULL);
    int y = ImageHeight(image) + 4;
    font->PaintText(text.c_str(), -1, x, y, -1, -1);
    int bulletWidth = ImageWidth(weapon->getBulletOnImage());
    int fontHeight = font->GetFontHeight();
    const unsigned int centered = DGHelper::DRAW_HCENTER | DGHelper::DRAW_VCENTER;
    if (m_bulletIcon != NULL) {
        app->m_helper->DrawImage(m_bulletIcon, x - bulletWidth, y + fontHeight / 2,
                                 1.0f, 0, 0, 0.0f, centered);
    }
    int height = ImageHeight(m_bulletIcon);
    if (height < fontHeight) {
        height = fontHeight;
    }
    int bottom = y + height;
    int aimAssistX = x - 4 * bulletWidth;
    if (weapon->m_silencerAvailable && weapon->isSilent() && m_silencerIcon != NULL) {
        int silencerY = y + fontHeight;
        app->m_helper->DrawImage(m_silencerIcon, aimAssistX, silencerY, 1.0f, 0, 0, 0.0f, centered);
        XString count(CBH_Player::GetInstance()->m_silentShotsCount);
        int countWidth = font->MeasureTextWidth(count.c_str(), -1, -1, NULL);
        int silencerHeight = ImageHeight(m_silencerIcon);
        int countX = x - 5 * bulletWidth - countWidth + ImageWidth(m_silencerIcon) / 2;
        font->PaintText(count.c_str(), -1, countX, silencerY - silencerHeight, -1, -1);
        int silencerBottom = silencerY + silencerHeight / 2;
        if (silencerBottom > bottom) {
            bottom = silencerBottom;
        }
        const SScreenAdaptingConstant gap(20, 40);
        aimAssistX = countX - gap();
    }
    if (weapon->m_cyberScopeEquipped && m_aimAssistIcon != NULL) {
        const SScreenAdaptingConstant aimAssistY(50, 100);
        app->m_helper->DrawImage(m_aimAssistIcon, aimAssistX, aimAssistY(), 1.0f, 0, 0, 0.0f, centered);
    }

    int reloadWidth = m_reloadButtonArea.m_bounds.m_width;
    m_reloadButtonArea.SetPosition(app->m_screenWidth - reloadWidth, bottom + App::Scale() * 2);
    m_reloadTouchArea.SetPosition(app->m_screenWidth - reloadWidth * 2, 0);
}

void GamePlayHUD::UpdateAmmoIcon(const CAmmo* ammo)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CleanBulletIcon();
    if (ammo != NULL) {
        XString::AnsiString icon(ammo->m_bulletIcon.c_str());
        m_bulletIcon = app->m_surfaceManager->CreateSurface(icon, true);
    }
    if (m_bulletIcon == NULL) {
        m_bulletIcon = app->m_surfaceManager->CreateSurface("IDB_HUD_HANDGUN_BULLET_ICON", true);
    }
    m_displayedAmmo = ammo;
}

void GamePlayHUD::CleanBulletIcon()
{
    if (m_bulletIcon != NULL) {
        App* app = static_cast<App*>(WindowApp::m_instance);
        app->m_surfaceManager->ReleaseSurface(m_bulletIcon);
        m_bulletIcon = NULL;
    }
    m_displayedAmmo = NULL;
}

void GamePlayHUD::CleanWeaponImage()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    for (int slot = 0; slot < 2; ++slot) {
        if (m_weaponImages[slot] != NULL) {
            app->m_surfaceManager->ReleaseSurface(m_weaponImages[slot]);
            m_weaponImages[slot] = NULL;
        }
        m_weaponImagesFor[slot] = NULL;
    }
}

void GamePlayHUD::ReInitWeaponButton()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_BasicGameData* data = app->m_helper->m_gamePlayManager.GetGameData();
    CDH_Weapon* weapons[] = { data->GetWeapon01(true), data->GetWeapon02(true) };
    m_hasAlternateWeapon = weapons[0] != weapons[1];
    CleanWeaponImage();
    for (int slot = 0; slot < 2; ++slot) {
        CDH_Weapon* weapon = weapons[slot];
        m_weaponImagesFor[slot] = weapon;
        if (weapon != NULL && weapon->m_gameImageId != NULL) {
            m_weaponImages[slot] = app->m_surfaceManager->CreateSurface(weapon->m_gameImageId, true);
        }
    }
}

void GamePlayHUD::switchWeapons()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon()->unfetchWeaponModel();
    m_modelAnimationState = WeaponLeaving;
    CBH_Player::GetInstance()->m_gameStats.AddInMIssionWeaponChanges(1);
}

void GamePlayHUD::CheckForAutomaticWeaponSwitch(CDH_Weapon* weapon)
{
    if (!m_enabled || IsModelAnimationActive() || !weapon->IsReadyForUse() || weapon->m_ammo > 0) {
        return;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_BasicGameData* data = app->m_helper->m_gamePlayManager.GetGameData();
    CDH_Weapon* alternate = data->GetWeapon01(true);
    if (alternate == weapon) {
        alternate = data->GetWeapon02(true);
    }
    if (alternate == NULL || alternate == weapon
        || CBH_Player::GetInstance()->GetAvailableAmmo(alternate->GetActiveAmmo(), true) <= 0
        || weapon->CheckForReload(false)) {
        return;
    }
    if (m_scoped) {
        switchScope();
    } else {
        ResetFireButton();
        weapon->ResetAutoShot();
        switchWeapons();
    }
}

void GamePlayHUD::switchScope()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CSwerveGame* game = app->m_helper->m_swerveGame;
    CDH_Weapon* weapon = app->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
    if (m_scoped) {
        if (!weapon->m_scopeAttached) {
            game->m_camera->restoreView(true);
        }
        weapon->ScopeOut();
    } else {
        if (weapon->m_scopeTime <= 0) {
            weapon->m_scopeTime = 1;
        }
        weapon->ScopeIn();
    }
    m_modelAnimationState = ScopeTransition;
}

void GamePlayHUD::Update()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CSwerveGame* game = app->m_helper->m_swerveGame;
    bool controlsVisible = m_enabled && !game->IsIntroTime();
    float alphaStep = app->m_updateFrameTime * 0.001f;
    if (!m_enabled) {
        alphaStep = -alphaStep;
    }
    m_visibilityAlpha += alphaStep;
    if (m_visibilityAlpha < 0.0f) {
        m_visibilityAlpha = 0.0f;
    } else if (m_visibilityAlpha > 1.0f) {
        m_visibilityAlpha = 1.0f;
    }
    UpdateHUDsVisibility();
    if (m_steadyActive && controlsVisible && app->m_timeScale != 0.0f) {
        m_steadyRemainingMs -= app->m_updateFrameTime;
        if (m_steadyRemainingMs <= 0) {
            m_steadyRemainingMs = 0;
            m_steadyActive = false;
        }
    }
    if (controlsVisible && app->m_settings->m_field1f) {
        m_pauseButton->SetFlags(FLAG_VISIBLE | FLAG_ENABLED);
    } else {
        m_pauseButton->ClearFlags(FLAG_VISIBLE | FLAG_ENABLED);
    }
    if (controlsVisible && !game->IsGamePaused() && !game->m_finishing && !game->m_inGameShopOpen
        && !game->m_waitingForAmmo && app->m_settings->m_field1f) {
        m_ingameShop->SetFlags(FLAG_VISIBLE | FLAG_ENABLED);
    } else {
        m_ingameShop->ClearFlags(FLAG_VISIBLE | FLAG_ENABLED);
    }
    bool coverVisible = controlsVisible && game->HasCoverAnimation() && app->m_settings->m_field1f;
    SetDockCoverButtonVisibility(!coverVisible);
    EnableDockCoverButton(coverVisible && !game->m_player->IsInProgress());
    UpdateButtonPosition();

    CDH_BasicGameData* data = app->m_helper->m_gamePlayManager.GetGameData();
    if (m_weaponImagesFor[0] != data->GetWeapon01(true) || m_weaponImagesFor[1] != data->GetWeapon02(true)) {
        ReInitWeaponButton();
    }
    CDH_Weapon* weapon = data->GetActiveWeapon();
    if (weapon != NULL) {
        const CAmmo* ammo = weapon->GetActiveAmmo();
        if (ammo != m_displayedAmmo) {
            UpdateAmmoIcon(ammo);
        }
    }

    if (m_zoomSliderControl != NULL && m_zoomedScope != NULL && m_zoomedScope->m_zoomEnabled) {
        if (!m_zoomedScope->m_shooting && m_zoomedScope->m_zoom != NULL) {
            float position = 1.0f - m_zoomSliderControl->GetValue();
            if (position < 0.0f) {
                position = 0.0f;
            }
            m_zoomedScope->m_zoomPosition = m_zoomedScope->m_zoom->m_durationSec * position * 0.5f;
        }
        m_zoomedScope->Update();
    }
    CBH_Player* player = game->m_player;
    if (m_healthBar != NULL) {
        m_healthBar->SetMaxValue(player->GetMaxHp() / 1000);
        m_healthBar->SetCurrValue(static_cast<int>(MathLib::Ceil(player->m_hp / 1000.0)));
    }
    if (m_armorBar != NULL) {
        if (player->IsArmorAvailable() && !(m_armorBar->m_flags & FLAG_VISIBLE) && app->m_settings->m_field1f) {
            m_armorBar->SetMaxValue(player->m_armorDamageProofMax / 1000);
            m_armorBar->SetCurrValue(player->m_armorDamageProof / 1000);
            m_armorBar->SetFlags(FLAG_VISIBLE | FLAG_ENABLED);
        } else if (!player->IsArmorAvailable() && (m_armorBar->m_flags & FLAG_VISIBLE)) {
            m_armorBar->ClearFlags(FLAG_VISIBLE | FLAG_ENABLED);
        }
        if (player->m_armorDamageProof < 1000) {
            m_armorBar->ClearFlags(FLAG_VISIBLE);
        } else {
            m_armorBar->SetCurrValue(player->m_armorDamageProof / 1000);
            m_armorBar->SetMaxValue(player->m_armorDamageProofMax / 1000);
        }
    }
    if (m_modelAnimationState != ModelReady) {
        processModelAnimation();
    }
    if (m_currentHint != NULL) {
        m_currentHint->m_elapsed += app->m_updateFrameTime;
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
    if (!game->IsGamePaused() && weapon != NULL && m_hasAlternateWeapon) {
        CheckForAutomaticWeaponSwitch(weapon);
    }
}

void GamePlayHUD::UpdateHUDsVisibility()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    bool visible = app->m_settings->m_field1f;
    Window* bars[] = { m_healthBar, m_armorBar, m_missionTimer };
    for (unsigned int index = 0; index < sizeof(bars) / sizeof(bars[0]); ++index) {
        Window* bar = bars[index];
        if (bar != NULL && visible) {
            bar->SetFlags(FLAG_VISIBLE);
        } else if (bar != NULL) {
            bar->ClearFlags(FLAG_VISIBLE);
        }
    }
    if (m_multiplierBar != NULL) {
        if (visible && m_showCountdownTimer) {
            m_multiplierBar->SetFlags(FLAG_VISIBLE);
        } else {
            m_multiplierBar->ClearFlags(FLAG_VISIBLE);
        }
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
    WoundingWindow* window = m_woundingWindow;
    if (armor) {
        window = m_armorHitWindow;
    }
    if (window != NULL) {
        window->StartEffect(angle);
    }
}

void GamePlayHUD::WoundingWindow::StartEffect(float angle)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (app->m_settings->m_field1f && m_arrow != NULL) {
        m_remainingMs = WoundingDurationMs;
        SwerveHelper::Transform rotation = SwerveHelper::NewTransform();
        const float angleAxis[4] = { angle, 0.0f, 0.0f, 1.0f };
        rotation.postRotate(angleAxis);
        m_arrow.setTransform(rotation);
    }
}

GamePlayHUD::WoundingWindow::WoundingWindow(bool armor)
    : ViewWindow(World()), m_remainingMs(0)
{
    typedef char WoundingWindowSizeCheck[sizeof(WoundingWindow) == 0xb4 ? 1 : -1];
    if (armor) {
        m_world = World(App::LoadObject3D("BIN_ARMOR_HIT"));
    } else {
        m_world = World(App::LoadObject3D("BIN_WOUNDING"));
    }
    if (m_world != NULL) {
        m_screen = SwerveHelper::Group(m_world.find(DGHelper::getSwerveID("injure_screenTransform")));
        m_arrow = SwerveHelper::Group(m_world.find(DGHelper::getSwerveID("arrowTransform")));
    }
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
            m_screen.animate(WoundingDurationMs - m_remainingMs);
        }
        m_arrow.animate(WoundingDurationMs - m_remainingMs);
        if (m_remainingMs <= 0) {
            StopEffect();
        }
    }
}

void GamePlayHUD::PlayReviveEffect(float)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_Weapon* weapon = app->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
    if (weapon == NULL) {
        return;
    }
    // The original neither resets the fire button nor leaves the scope here.
    weapon->ResetAutoShot();
    weapon->unfetchWeaponModel();
    m_modelAnimationState = ReviveWeaponLeaving;
}

void GamePlayHUD::processModelAnimation()
{
    if (m_modelAnimationState == ModelReady) {
        return;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    DGHelper* helper = app->m_helper;
    CSwerveGame* game = helper->m_swerveGame;
    CDH_BasicGameData* data = helper->GetGameData();
    CDH_Weapon* weapon = data->GetActiveWeapon();
    if (weapon == NULL) {
        m_modelAnimationState = ModelReady;
        return;
    }
    bool waitingForModel = m_modelAnimationState != RevivePlaying && weapon->m_animation != NULL
        && !weapon->isModelAnimationComplete();
    if (waitingForModel) {
        return;
    }
    switch (m_modelAnimationState) {
    case WeaponLeaving:
        game->OnWeaponHided();
        helper->m_gamePlayManager.SwitchWeapons();
        weapon = data->GetActiveWeapon();
        if (weapon != NULL) {
            weapon->fetchWeaponModel();
            m_modelAnimationState = WeaponEntering;
        } else {
            m_modelAnimationState = ModelReady;
        }
        break;
    case WeaponEntering:
        game->OnWeaponShown();
        m_modelAnimationState = ModelReady;
        break;
    case ReviveWeaponLeaving:
        game->OnWeaponHided();
        if (app->m_settings->m_field1f) {
            if (m_reviveWindow == NULL) {
                m_reviveWindow = new ReviveWindow();
            }
            if (m_reviveWindow->m_world != NULL) {
                m_reviveWindow->m_remainingMs = ReviveDurationMs;
                m_reviveWindow->m_soundPlayed = false;
            }
        }
        m_modelAnimationState = RevivePlaying;
        break;
    case RevivePlaying: {
        if (m_reviveWindow != NULL) {
            m_reviveWindow->Update();
            if (m_reviveWindow->m_remainingMs > 0) {
                return;
            }
        }
        int ammo = weapon->m_ammo;
        weapon->fetchWeaponModel();
        weapon->m_ammo = ammo;
        m_modelAnimationState = WeaponEntering;
        break;
    }
    }
}

bool GamePlayHUD::PaintReviveEffect(Graphics3D& graphics)
{
    if (m_modelAnimationState != RevivePlaying || m_reviveWindow == NULL || m_reviveWindow->m_remainingMs <= 0) {
        return false;
    }
    m_reviveWindow->Paint3D(graphics);
    return true;
}

bool GamePlayHUD::IsModelAnimationActive() const
{
    return m_modelAnimationState != ModelReady;
}

// The provisional declaration only exposed +0xe1 to SetZoomSliderEnable.
// The full layout follows the constructor at 0x14f1e0 (size 0xe4).
GamePlayHUD::CZoomedScopeWindow::CZoomedScopeWindow()
    : ViewWindow(World()),
      m_shoot(NULL), m_zoom(NULL), m_zoomPosition(0.0f), m_shotPosition(0.0f), m_zoomRecoil(0.0f),
      m_weaponId(0), m_reservedD4(0), m_worldTime(0), m_shotTime(0), m_shooting(false),
      m_zoomEnabled(false)
{
    typedef char ScopeWindowSizeCheck[sizeof(CZoomedScopeWindow) == 0xe4 ? 1 : -1];
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_Weapon* weapon = app->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
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
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_Weapon* weapon = app->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
    if (weapon != NULL && weapon->GetId() != m_weaponId) {
        InitResources(weapon);
    }
    if (m_shoot == NULL || m_zoom == NULL) {
        return;
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
        m_shotTime += app->m_updateFrameTime;
        if (m_shotPosition > m_shoot->m_durationSec) {
            m_shotTime = 0;
            m_shooting = false;
            m_shotPosition = 0.0f;
        }
    }
    m_worldTime += app->m_updateFrameTime;
    m_world.animate(m_worldTime);
}

void GamePlayHUD::CZoomedScopeWindow::Paint3D(Graphics3D& graphics)
{
    if (m_zoomEnabled) {
        ViewWindow::Paint3D(graphics);
    }
}

void GamePlayHUD::SetZoomSliderEnable(bool enable)
{
    m_zoomSliderControl->Enable(enable);
    m_zoomedScope->m_zoomEnabled = enable;
}

float GamePlayHUD::GetZoomSliderProgress() const
{
    if (m_zoomSliderControl != NULL) {
        return m_zoomSliderControl->GetValue();
    }
    return 0.0f;
}

void GamePlayHUD::ReInitBulletIcon(CDH_Weapon* weapon)
{
    if (weapon != NULL) {
        const CAmmo* ammo = weapon->GetActiveAmmo();
        if (ammo != NULL) {
            App* app = static_cast<App*>(WindowApp::m_instance);
            XString icon = ammo->m_bulletIcon;
            m_bulletIcon = app->m_surfaceManager->CreateSurface(XString::AnsiString(icon.c_str()), true);
            m_displayedAmmo = ammo;
        }
    }
}

bool GamePlayHUD::isWeaponModelVisible() const
{
    // Outside the scope the model is always shown; in the scope only while it animates.
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
        if (hide) {
            m_coverButton->ClearFlags(FLAG_VISIBLE | FLAG_ENABLED);
        } else {
            m_coverButton->SetFlags(FLAG_VISIBLE | FLAG_ENABLED);
        }
    }
}

void GamePlayHUD::ChangeDockCoverButtonState()
{
    if (m_coverButton != NULL) {
        m_coverButton->ChangeState();
    }
}

bool GamePlayHUD::ContainsFireButton(int x, int y) const
{
    return m_fireHitArea.checkCollision(x, y);
}

void GamePlayHUD::OnCommand(Event& event)
{
    if (event.m_id != 0x2ebf5358 && event.m_id != static_cast<int>(0x811575c5u)) {
        return;
    }
    if (!m_enabled) {
        event.Clear();
        return;
    }
    // The game owns the mission-state checks and the shop window's lifetime.
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_helper->m_swerveGame->OnCommand(event);
}

int GamePlayHUD::GetActiveWeaponIndex()
{
    CDH_Weapon* weapon01 = static_cast<App*>(WindowApp::m_instance)->m_helper->m_gamePlayManager.GetGameData()->GetWeapon01(true);
    // The second weapon is fetched but not compared (original behaviour).
    CDH_Weapon* weapon02 = static_cast<App*>(WindowApp::m_instance)->m_helper->m_gamePlayManager.GetGameData()->GetWeapon02(true);
    CDH_Weapon* active = static_cast<App*>(WindowApp::m_instance)->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
    if (active == weapon01) {
        return 0;
    }
    return 1;
}

int GamePlayHUD::PointerPressed(int x, int y, int pointer)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (!m_enabled) {
        return -1;
    }
    CSwerveGame* game = app->m_helper->m_swerveGame;
    CBH_Player* player = game->m_player;
    if (player->IsInHide() || player->IsInProgress()) {
        return -1;
    }
    if (app->m_settings->IsFireButtonEnabled() && ContainsFireButton(x, y)) {
        m_fireHeld = true;
        m_firePointer = pointer;
        return 2;
    }
    bool hudVisible = app->m_settings->m_field1f;
    if (hudVisible && player->CanRestoreHealth() && player->m_hp < player->GetMaxHp()
        && Contains(m_medkitArea.m_bounds, x, y)) {
        player->RestoreHealth();
        return 4;
    }
    if (hudVisible && m_modelAnimationState == ModelReady && m_steadyRemainingMs > 0
        && !m_steadyActive && Contains(m_steadyBounds, x, y)) {
        CDH_Weapon* weapon = app->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
        if (weapon != NULL) {
            game->m_usedSteadyAim = true;
            m_steadyUsed = true;
            m_steadyActive = true;
            CGameAnalytics::logSteadyActivated(app->m_huntingInfo->m_assignedData.m_missionName, weapon->GetId());
            return 0;
        }
    }
    if (hudVisible && player->IsSpeedModeAvailable() && Contains(m_reactixArea.m_bounds, x, y)) {
        if (player->GetReactixControlTimeSec() < 1) {
            player->ConsumeGoPill();
            CGameAnalytics::logItemUsed(XString("reactix"));
        } else if (player->IsSpeedMode()) {
            player->StopConsumeReactixControl();
        } else {
            player->StartConsumeReactixControl();
            CGameAnalytics::logItemUsed(XString("reactix_control"));
        }
        return 1;
    }
    if (hudVisible && player->m_ultrasoundAttemptCount > 0 && Contains(m_ultrasoundArea.m_bounds, x, y)) {
        player->ConsumeUltrasoundEmitter();
        game->AddStunTimerMS(10000);
        return 5;
    }
    CDH_BasicGameData* data = app->m_helper->m_gamePlayManager.GetGameData();
    CDH_Weapon* weapon = data->GetActiveWeapon();
    if (weapon == NULL) {
        return -1;
    }
    if ((weapon->m_scopeAttached || weapon->m_zoomWithoutScope) && Contains(m_scopeBounds, x, y)) {
        if (weapon->IsReadyForUse()) {
            game->m_usedHudButton = true;
            switchScope();
        }
        return 6;
    }
    if (!m_scoped && Contains(m_weaponBounds, x, y)) {
        CDH_Weapon* first = data->GetWeapon01(true);
        CDH_Weapon* second = data->GetWeapon02(true);
        if (first != NULL && second != NULL && first != second) {
            if (weapon->IsReadyForUse()) {
                ResetFireButton();
                weapon->ResetAutoShot();
                switchWeapons();
            }
            return 6;
        }
    }
    if (!m_scoped && m_reloadTouchArea.checkCollision(x, y)) {
        const CAmmo* ammo = weapon->GetActiveAmmo();
        if (ammo != NULL && weapon->IsReadyForUse() && !weapon->IsReloading()
            && weapon->m_ammo < weapon->m_clipSize
            && weapon->m_ammo < CBH_Player::GetInstance()->GetAvailableAmmo(ammo, true)) {
            weapon->CheckForReload(true);
        }
        return 1;
    }
    return -1;
}

void GamePlayHUD::CheckContollStateOnPointerReleased(int, int, int pointer)
{
    if (IsFirePointer(pointer)) {
        ResetFireButton();
    }
}

void GamePlayHUD::CheckContollStateOnPointerMove(int x, int y, int, int, int pointer)
{
    if (IsFirePointer(pointer) && !ContainsFireButton(x, y)) {
        ResetFireButton();
    }
}

void GamePlayHUD::ResetFireButton()
{
    m_fireHeld = false;
    m_firePointer = -1;
}

bool GamePlayHUD::IsFirePointer(int pointer) const
{
    return m_fireHeld && m_firePointer == pointer;
}

bool GamePlayHUD::IsFireHeld() const
{
    return m_fireHeld;
}

bool GamePlayHUD::IsSteadyAimActive() const
{
    return m_steadyActive;
}

bool GamePlayHUD::HasUsedSteadyAim() const
{
    return m_steadyUsed;
}

void GamePlayHUD::SetEnabled(bool enabled)
{
    if (enabled) {
        Show();
    } else {
        Hide(false);
        ResetFireButton();
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

typedef char AssertGamePlayHUDSize[sizeof(GamePlayHUD) == 0x330 ? 1 : -1];
