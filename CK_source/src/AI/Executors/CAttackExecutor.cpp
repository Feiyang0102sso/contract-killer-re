#include "CAttackExecutor.h"

#include "App.h"
#include "CAuxRenderText.h"
#include "CBH_Player.h"
#include "CSwerveGame.h"
#include "CUnit.h"
#include "DGHelper.h"

static inline __attribute__((always_inline)) CAuxRenderText* GetAttackDebugText()
{
    void* found = NULL;
    CApplet::m_pApp->m_singletons->Find(CAuxRenderText::SingletonId, &found);
    CAuxRenderText* text = static_cast<CAuxRenderText*>(found);
    if (text == NULL) {
        text = new CAuxRenderText();
    }
    return text;
}

void CAttackExecutor::DebugRender(CAuxRender*)
{
    switch (m_state) {
    case Hide:
        GetAttackDebugText()->AddTextInfo(L"Unit hides");
        break;
    case ReloadWeapon:
        GetAttackDebugText()->AddTextInfo(L"Unit reloads");
        break;
    case Idle:
        GetAttackDebugText()->AddTextInfo(L"Unit idles");
        break;
    case Unhide:
        GetAttackDebugText()->AddTextInfo(L"Unit unhides");
        break;
    case Shoot:
        GetAttackDebugText()->AddTextInfo(L"Unit shots");
        break;
    }
}

CAttackExecutor::CAttackExecutor(CUnitMind* mind)
    : IExecutor(mind), m_camera(-1), m_waitMs(0), m_waiting(false), m_turnElapsed(0)
{
}

CAttackExecutor::~CAttackExecutor()
{
    // the original checks the body pointer it gets from the unit
    CUnitBody* body = m_mind->GetUnit()->GetBody();
    if (body != NULL) {
        body->ShowWeapon(false);
    }
}

void CAttackExecutor::Tick(int elapsed)
{
    // the original only ticks while waiting and always keeps the unit in cover
    if (!m_waiting) {
        return;
    }
    m_turnElapsed += elapsed;
    if (m_turnElapsed > 500) {
        m_turnElapsed = 0;
        OrientBodyToCamera();
    }
    m_waitMs -= elapsed;
    if (m_waitMs <= 0) {
        StopActiveAnimation();
        ChooseNextState();
        ProcessState();
        m_waiting = false;
        m_waitMs = 0;
    }
    m_mind->GetUnit()->OnTakeCover();
}

void CAttackExecutor::OrientBodyToCamera()
{
    // the original checks the body pointer and reaches the game through the application
    CUnitBody* body = m_mind->GetUnit()->GetBody();
    if (body != NULL) {
        App* app = static_cast<App*>(WindowApp::m_instance);
        body->SetOrientationToPoint(app->m_helper->m_swerveGame->GetCurrentCameraPos(), true);
    }
}

void CAttackExecutor::OnAnimationComplete(EPersonAnimType, const char* name)
{
    if (IsAnimationTheSame(name) && !m_waiting) {
        StopActiveAnimation();
        ChooseNextState();
        ProcessState();
    }
}

__attribute__((noinline)) void CAttackExecutor::OnStart(int camera)
{
    m_camera = camera;
    // the original only returns when the body pointer is NULL (no Finish call)
    CUnitBody* body = m_mind->GetUnit()->GetBody();
    if (body == NULL) {
        return;
    }
    body->SetExactlyToPoint();
    OrientBodyToCamera();
    GetWeapon();
}

__attribute__((noinline)) void CAttackExecutor::GetWeapon()
{
    // the original checks the body pointer it gets from the unit
    CUnitBody* body = m_mind->GetUnit()->GetBody();
    if (body != NULL) {
        m_activeAnimation = body->PlayGetWeaponAnimation();
        body->ShowWeapon(true);
    }
}

void CAttackExecutor::ProcessState()
{
    // Original 0xcfefc: each animation case checks the body pointer it takes from the unit.
    switch (m_state) {
    case Hide: {
        CUnitBody* body = m_mind->GetUnit()->GetBody();
        if (body != NULL) {
            m_activeAnimation = body->PlayHideAnimation();
        }
        break;
    }
    case ReloadWeapon:
        Reload();
        break;
    case Idle: {
        CUnitBody* body = m_mind->GetUnit()->GetBody();
        if (body != NULL) {
            m_activeAnimation = body->PlayIdleAnimation(0);
        }
        break;
    }
    case Unhide: {
        CUnitBody* body = m_mind->GetUnit()->GetBody();
        if (body != NULL) {
            m_activeAnimation = body->PlayUnhideAnimation();
        }
        break;
    }
    case Shoot:
        Shot();
        break;
    }
}

__attribute__((noinline)) void CAttackExecutor::Reload()
{
    // the original checks the body pointer it gets from the unit
    CUnitBody* body = m_mind->GetUnit()->GetBody();
    if (body == NULL) {
        return;
    }
    bool available = false;
    const char* animation = body->PlayReloadAnimation(&available);
    if (available) {
        m_activeAnimation = animation;
    } else if (!m_waiting) {
        StopActiveAnimation();
        ChooseNextState();
        ProcessState();
    }
}

__attribute__((noinline)) void CAttackExecutor::Shot()
{
    // the original checks the body pointer it gets from the unit
    CUnitBody* body = m_mind->GetUnit()->GetBody();
    if (body == NULL) {
        return;
    }
    m_activeAnimation = body->PlayShotAnimation();
    body->PlayWeaponSound();
    body->ShowBulletMuzzleFlash();
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_helper->m_swerveGame->m_missionAlert = true;
    CBH_Player::GetInstance()->TryShotPlayer(body->m_unit);
}

CAttackSafeExecutor::CAttackSafeExecutor(CUnitMind* mind) : CAttackExecutor(mind)
{
    m_state = Shoot;
}

bool CAttackSafeExecutor::IsOnCover()
{
    return m_state == ReloadWeapon;
}

void CAttackSafeExecutor::ChooseNextState()
{
    switch (m_state) {
    case Hide:
        m_state = ReloadWeapon;
        m_waiting = true;
        m_waitMs = 2000;
        break;
    case ReloadWeapon:
        m_state = Unhide;
        break;
    case Unhide:
        m_state = Shoot;
        break;
    case Shoot:
        m_state = Hide;
        m_mind->GetUnit()->OnTakeCover();
        break;
    }
}

const wchar_t* CAttackSafeExecutor::GetName() const
{
    return L"CAttackSafeExecutor";
}

CAttackUnsafeExecutor::CAttackUnsafeExecutor(CUnitMind* mind) : CAttackExecutor(mind)
{
    m_state = Idle;
}

bool CAttackUnsafeExecutor::IsOnCover()
{
    return false;
}

void CAttackUnsafeExecutor::ChooseNextState()
{
    if (m_state == Shoot) {
        m_state = ReloadWeapon;
    } else if (m_state == Idle) {
        m_state = Shoot;
    } else if (m_state == ReloadWeapon) {
        m_state = Idle;
        m_waiting = true;
        m_waitMs = 2000;
    }
}

const wchar_t* CAttackUnsafeExecutor::GetName() const
{
    return L"CAttackUnsafeExecutor";
}

void AddSafeAttackExecutor(CUnitMind* mind, int camera)
{
    if (mind != NULL) {
        CAttackSafeExecutor* executor = new CAttackSafeExecutor(mind);
        mind->AddExecutor(executor);
        executor->OnStart(camera);
    }
}

void AddUnsafeAttackExecutor(CUnitMind* mind, int camera)
{
    if (mind != NULL) {
        CAttackUnsafeExecutor* executor = new CAttackUnsafeExecutor(mind);
        mind->AddExecutor(executor);
        executor->OnStart(camera);
    }
}

typedef char AssertAttackExecutorSize[sizeof(CAttackSafeExecutor) == 0x20 ? 1 : -1];
