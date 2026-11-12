#include "DGScopeCursor.h"

#include "App.h"
#include "AppSettings.h"
#include "CBH_Player.h"
#include "CBH_SurfaceManager.h"
#include "CDH_BasicGameData.h"
#include "CDH_Weapon.h"
#include "CSwerveGame.h"
#include "DGHelper.h"
#include "GamePlayHUD.h"
#include "dgCamera.h"
#include "ICResourceManager.h"

DGScopeCursor::DGScopeCursor()
    : m_wideFov(30.0f), m_narrowFov(30.0f), m_resourcesLoaded(false), m_field60(0),
      m_pin(NULL), m_dot(NULL), m_arrows(NULL), m_pressed(false), m_moved(false),
      m_motionDelayMs(-1), m_fov(30.0f), m_fired(false)
{
    for (int index = 0; index < 4; ++index) {
        m_fieldsBC[index] = 0;
    }
}

DGScopeCursor::~DGScopeCursor()
{
    DeInit();
}

int DGScopeCursor::GetAimX() const
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    return app->m_helper->m_screenWidth / 2;
}

int DGScopeCursor::GetAimY() const
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    return app->m_helper->m_screenHeight / 2;
}

void DGScopeCursor::onWeaponSwitch()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_helper->m_swerveGame->m_camera->SetWanderngByActiveWeapon();
}

void DGScopeCursor::onFire()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_Weapon* weapon = app->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
    CBH_Player* player = CBH_Player::GetInstance();
    if (weapon != NULL && player->IsNeedHide() && !player->IsInProgress()
        && weapon->IsReadyForShot() && weapon->m_ammo > 0) {
        app->m_helper->m_swerveGame->Shot(GetAimX(), GetAimY());
        app->m_helper->m_swerveGame->m_hud->Shot();
        m_fired = true;
    }
}

void DGScopeCursor::onPointerMove(int, int, int deltaX, int deltaY)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (!app->m_settings->IsAccelerationEnabled()) {
        m_moved = true;
    }
    if (m_motionDelayMs < 0) {
        setTargetAndTime(m_targetX + deltaX, m_targetY + deltaY, 1);
        DGHelper* helper = app->m_helper;
        helper->m_swerveGame->m_camera->SetTargetPoint(0.5f + (float)deltaX / helper->m_screenWidth,
                                                        0.5f + (float)deltaY / helper->m_screenHeight);
    }
}

void DGScopeCursor::onPointerBreak()
{
    m_pressed = false;
    m_moved = false;
}

void DGScopeCursor::onPointerPressed(int, int)
{
    m_pressed = true;
    m_moved = false;
}

void DGScopeCursor::onPointerReleased()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (!app->m_settings->IsFireButtonEnabled() && m_pressed && !m_moved) {
        onFire();
    }
    m_pressed = false;
    setPosition(0, 0, true);
}

void DGScopeCursor::DrawImpl()
{
}

void DGScopeCursor::onConstraintReached(int)
{
}

void DGScopeCursor::UpdateImpl(int elapsedMs)
{
    if (m_motionDelayMs >= 0) {
        m_motionDelayMs += elapsedMs;
        if (m_motionDelayMs > 100) {
            m_motionDelayMs = -1;
        }
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    CSwerveGame* game = app->m_helper->m_swerveGame;
    float progress = game->m_hud->GetZoomSliderProgress();
    m_fov = m_narrowFov + (m_wideFov - m_narrowFov) * progress;
    game->m_camera->SetFOV(m_fov);
}

void DGScopeCursor::CreateResources()
{
    if (!m_resourcesLoaded) {
        static_cast<App*>(WindowApp::m_instance)->m_helper->LoadImageSurface(&m_pin, "SUR_AIM_SCOPE_PIN", false);
        static_cast<App*>(WindowApp::m_instance)->m_helper->LoadImageSurface(&m_dot, "SUR_AIM_SCOPE_DOT", false);
        static_cast<App*>(WindowApp::m_instance)->m_helper->LoadImageSurface(&m_arrows, "SUR_AIM_SCOPE_ARROWS", false);
        m_resourcesLoaded = true;
    }
}

void DGScopeCursor::ReleaseResources()
{
    if (m_resourcesLoaded) {
        CResourceManager* resources = CApplet::m_pApp->m_resourceManager;
        resources->ReleaseResource("SUR_AIM_SCOPE_PIN");
        resources->ReleaseResource("SUR_AIM_SCOPE_DOT");
        resources->ReleaseResource("SUR_AIM_SCOPE_ARROWS");
        m_resourcesLoaded = false;
    }
}

void DGScopeCursor::InitFovValues()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_Weapon* weapon = app->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
    if (weapon != NULL) {
        m_wideFov = weapon->m_zoomFovMax;
        m_narrowFov = weapon->m_zoomFovMin;
        m_fov = m_wideFov;
    }
}

void DGScopeCursor::InitImpl()
{
    CreateResources();
    InitFovValues();
    App* app = static_cast<App*>(WindowApp::m_instance);
    DGHelper* helper = app->m_helper;
    DGCamera* camera = helper->m_swerveGame->m_camera;
    camera->SetViewPortWidthAndHeight(257.0f / helper->m_screenWidth, 257.0f / helper->m_screenHeight);
    camera->SetWanderngByActiveWeapon();
    camera->SetFOV(m_fov);
}

void DGScopeCursor::DeInitImpl()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_helper->m_swerveGame->m_camera->restoreView(true);
    ReleaseResources();
}

typedef char AssertScopeCursorSize[sizeof(DGScopeCursor) == 0xd4 ? 1 : -1];
