#include "CFpsCursor.h"

#include "App.h"
#include "AppSettings.h"
#include "CBH_Player.h"
#include "CDH_BasicGameData.h"
#include "CDH_Weapon.h"
#include "CFont.h"
#include "CFontMgr.h"
#include "CSwerveGame.h"
#include "DGHelper.h"
#include "Fixed.h"
#include "SG_Presenter.h"
#include "dgCamera.h"

CFPSCursor::CFPSCursor()
    : m_cross(NULL), m_downX(0), m_downY(0), m_angleV(0.0f), m_angleH(0.0f),
      m_pressed(false), m_moved(false), m_shotRequested(false), m_shotAnimation(false), m_showCross(true)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    m_aimX = app->m_helper->m_screenWidth / 2;
    m_aimY = app->m_helper->m_screenHeight / 2;
    m_cross = new SG_Presenter(9, 0);
    m_cross->SetSGInstance(app->m_sgInstance);
    m_cross->SetAnimation(0, true);
}

CFPSCursor::~CFPSCursor()
{
    if (m_cross != NULL) {
        delete m_cross;
        m_cross = NULL;
    }
}

int CFPSCursor::GetAimX() const
{
    return m_aimX;
}

int CFPSCursor::GetAimY() const
{
    return m_aimY;
}

void CFPSCursor::ShowCross(bool show)
{
    m_showCross = show;
}

void CFPSCursor::onWeaponSwitch()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_helper->m_swerveGame->m_camera->SetWanderngByActiveWeapon();
}

void CFPSCursor::onFire()
{
    HandleShot();
}

void CFPSCursor::HandleShot()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_Weapon* weapon = app->m_helper->GetGameData()->GetActiveWeapon();
    if (weapon == NULL) {
        return;
    }
    if (weapon->IsReadyForShot() && weapon->m_ammo > 0) {
        app->m_helper->m_swerveGame->Shot(GetAimX(), GetAimY());
        m_cross->SetAnimation(1, false);
        m_shotAnimation = true;
    }
    m_shotRequested = false;
}

void CFPSCursor::onPointerMove(int, int, int deltaX, int deltaY)
{
    m_moved = true;
    App* app = static_cast<App*>(WindowApp::m_instance);
    DGCamera* camera = app->m_helper->m_swerveGame->m_camera;
    if (camera != NULL) {
        // the original does not look at the acceleration setting
        float moveX = static_cast<float>(deltaX);
        float moveY = static_cast<float>(deltaY);
        float angleV = camera->m_field1c;
        m_angleV = angleV;
        float angleH = camera->m_field0c;
        m_angleH = angleH;
        camera->SetAngleV(angleV - moveY * 0.1f, false);
        camera->SetAngleH(angleH - moveX * 0.1f, false);
    }
}

void CFPSCursor::onPointerBreak()
{
    m_pressed = false;
    m_moved = false;
    m_shotRequested = false;
}

void CFPSCursor::onPointerPressed(int x, int y)
{
    m_pressed = true;
    m_downX = x;
    m_downY = y;
    App* app = static_cast<App*>(WindowApp::m_instance);
    DGCamera* camera = app->m_helper->m_swerveGame->m_camera;
    if (camera != NULL) {
        m_angleV = camera->m_field1c;
        m_angleH = camera->m_field0c;
    }
}

void CFPSCursor::onPointerReleased()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    m_shotRequested = !app->m_settings->IsFireButtonEnabled() && m_pressed && !m_moved;
    m_pressed = false;
    m_moved = false;
}

void CFPSCursor::DrawImpl()
{
    DrawCross();
    if (App::m_platformType == 0) {
        DrawReloadingMessage();
    }
}

void CFPSCursor::DrawCross()
{
    if (m_showCross) {
        m_cross->Draw(GetAimX(), GetAimY());
    }
}

void CFPSCursor::DrawReloadingMessage()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_Weapon* weapon = app->m_helper->GetGameData()->GetActiveWeapon();
    if (weapon == NULL || !weapon->DoesShowReloadingMessage()) {
        return;
    }
    CFont* font = CFontMgr::GetInstance()->GetFont(1);
    if (font != NULL) {
        XString text = Window::ResString("IDS_RELOADING");
        int width = font->MeasureTextWidth(text.c_str(), -1, -1, NULL);
        font->PaintText(text.c_str(), text.Length(), GetAimX() - width / 2,
                        GetAimY() - font->GetFontHeight() / 2, -1, -1);
    }
}

void CFPSCursor::UpdateImpl(int)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (m_shotRequested && !app->m_settings->IsFireButtonEnabled()) {
        CBH_Player* player = CBH_Player::GetInstance();
        if (player->IsNeedHide() && !player->IsInProgress()) {
            HandleShot();
        } else {
            m_shotRequested = false;
        }
    }
    if (m_shotAnimation && m_cross->HasFinished()) {
        m_cross->SetAnimation(0, true);
        m_shotAnimation = false;
    }
    m_cross->Update(app->m_updateFrameTime);
}

void CFPSCursor::InitImpl()
{
    onWeaponSwitch();
}

void CFPSCursor::DeInitImpl()
{
}

typedef char AssertFPSCursorSize[sizeof(CFPSCursor) == 0x78 ? 1 : -1];
