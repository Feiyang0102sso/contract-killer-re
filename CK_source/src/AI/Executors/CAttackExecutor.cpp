#include "CAttackExecutor.h"

#include "CAuxRenderText.h"
#include "CBH_Player.h"
#include "CSwerveGame.h"
#include "CUnit.h"

void CAttackExecutor::DebugRender(CAuxRender*)
{
    switch (m_state) {
    case Hide:
        CAuxRenderText::GetInstance()->AddTextInfo(L"Unit hides");
        break;
    case ReloadWeapon:
        CAuxRenderText::GetInstance()->AddTextInfo(L"Unit reloads");
        break;
    case Idle:
        CAuxRenderText::GetInstance()->AddTextInfo(L"Unit idles");
        break;
    case Unhide:
        CAuxRenderText::GetInstance()->AddTextInfo(L"Unit unhides");
        break;
    case Shoot:
        CAuxRenderText::GetInstance()->AddTextInfo(L"Unit shots");
        break;
    }
}

CAttackExecutor::CAttackExecutor(CUnitMind* mind, int state)
    : IExecutor(mind), m_camera(-1), m_waitMs(0), m_waiting(false), m_state(state), m_turnElapsed(0)
{
}

CAttackExecutor::~CAttackExecutor()
{
    CUnit* unit = m_mind->GetUnit();
    if (unit != NULL) {
        unit->m_body.ShowWeapon(false);
    }
}

void CAttackExecutor::Tick(int elapsed)
{
    m_turnElapsed += elapsed;
    if (m_turnElapsed > 500) {
        m_turnElapsed = 0;
        OrientBodyToCamera();
    }
    if (m_waiting) {
        m_waitMs -= elapsed;
        if (m_waitMs > 0) {
            if (IsOnCover()) {
                m_mind->GetUnit()->OnTakeCover();
            }
            return;
        }
        m_waiting = false;
        m_waitMs = 0;
    } else if (m_activeAnimation != NULL) {
        return;
    }
    StopActiveAnimation();
    ChooseNextState();
    ProcessState();
}

void CAttackExecutor::OrientBodyToCamera()
{
    CUnit* unit = m_mind->GetUnit();
    if (unit != NULL) {
        unit->m_body.SetOrientationToPoint(CSwerveGame::m_pInstance->GetCurrentCameraPos(), true);
    }
}

void CAttackExecutor::OnAnimationComplete(EPersonAnimType, const char* name)
{
    if (!m_waiting && IsAnimationTheSame(name)) {
        StopActiveAnimation();
        ChooseNextState();
        ProcessState();
    }
}

void CAttackExecutor::OnStart(int camera)
{
    m_camera = camera;
    m_mind->GetUnit()->m_body.SetExactlyToPoint();
    OrientBodyToCamera();
    GetWeapon();
}

void CAttackExecutor::GetWeapon()
{
    CUnitBody& body = m_mind->GetUnit()->m_body;
    m_activeAnimation = body.PlayGetWeaponAnimation();
    body.ShowWeapon(true);
}

void CAttackExecutor::ProcessState()
{
    CUnitBody& body = m_mind->GetUnit()->m_body;
    switch (m_state) {
    case Hide:
        m_activeAnimation = body.PlayHideAnimation();
        break;
    case ReloadWeapon:
        Reload();
        break;
    case Idle:
        m_activeAnimation = body.StartAnimation(PERSON_ANIM_TYPE_DEFAULT, false);
        break;
    case Unhide:
        m_activeAnimation = body.PlayUnhideAnimation();
        break;
    case Shoot:
        Shot();
        break;
    }
}

void CAttackExecutor::Reload()
{
    bool available;
    m_activeAnimation = m_mind->GetUnit()->m_body.PlayReloadAnimation(&available);
}

void CAttackExecutor::Shot()
{
    CUnit* unit = m_mind->GetUnit();
    m_activeAnimation = unit->m_body.PlayShotAnimation();
    unit->m_body.PlayWeaponSound();
    CSwerveGame::m_pInstance->m_missionAlert = true;
    CBH_Player::GetInstance()->TryShotPlayer(unit);
}

CAttackSafeExecutor::CAttackSafeExecutor(CUnitMind* mind) : CAttackExecutor(mind, Shoot)
{
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

CAttackUnsafeExecutor::CAttackUnsafeExecutor(CUnitMind* mind) : CAttackExecutor(mind, Idle)
{
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
