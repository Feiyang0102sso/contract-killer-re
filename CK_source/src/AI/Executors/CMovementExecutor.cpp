#include "CMovementExecutor.h"

#include "CBH_Player.h"
#include "CPathCalculator.h"
#include "CSwerveGame.h"
#include "CUnit.h"

CMovementExecutor::CMovementExecutor(CUnitMind* mind, float speed, EPersonAnimType animation)
    : IExecutor(mind), m_animation(animation), m_field4030(0)
{
    m_path.m_speed = speed;
}

void CMovementExecutor::Tick(int elapsed)
{
    m_path.Tick(elapsed);
}

void CMovementExecutor::DebugRender(CAuxRender* render)
{
    m_path.Render(render);
}

void CMovementExecutor::OnPathComplete()
{
    Finish(EXECUTOR_RESULT_SUCCESS);
}

void CMovementExecutor::OnReceiveDamage()
{
    CUnit* unit = m_mind->GetUnit();
    if (unit == NULL) {
        return;
    }
    CUnitBody* body = unit->GetBody();
    if (unit->m_hp == 0 && body->IsRunning()) {
        EHitType hit;
        unit->GetPickInfo(&hit);
        if (hit == HitHead) {
            CBH_Player::GetInstance()->m_gameStats.AddHeadshotsInRunning(1);
        }
        return;
    }
    if (unit->m_hp < unit->m_maxHp) {
        unit->GetMovementAnimParam(&m_path.m_speed, &m_animation, true);
        StopActiveAnimation();
        StartAnimation();
    }
}

void CMovementExecutor::OnCancel()
{
    StopActiveAnimation();
    if (m_path.m_body != NULL) {
        m_path.m_body->m_speed = 0.0f;
    }
}

const wchar_t* CMovementExecutor::GetName() const
{
    return L"CMovementExecutor";
}

void CMovementExecutor::StartAnimation()
{
    m_activeAnimation = m_mind->GetUnit()->GetBody()->StartAnimation(m_animation, true);
}

void CMovementExecutor::OnStartFromPointToPoint(int from, int to)
{
    m_path.SetBody(m_mind->GetUnit()->GetBody());
    if (!m_mind->m_pathCalculator->FindPath(&m_path, from, to)) {
        Finish(EXECUTOR_RESULT_FAILED);
        return;
    }
    StartAnimation();
}

void CMovementExecutor::OnStartToExitPoint(int from)
{
    m_path.SetBody(m_mind->GetUnit()->GetBody());
    if (!m_mind->m_pathCalculator->FindPathToNearestExitPoint(&m_path, from)) {
        Finish(EXECUTOR_RESULT_FAILED);
        return;
    }
    StartAnimation();
}

void CMovementExecutor::OnStartToAttackPoint(int from, bool safeOnly)
{
    m_path.SetBody(m_mind->GetUnit()->GetBody());
    int camera = CSwerveGame::m_pInstance->GetCurrCamId();
    if (!m_mind->m_pathCalculator->FindPathToNearestAttackPoint(&m_path, from, camera, safeOnly)) {
        Finish(EXECUTOR_RESULT_FAILED);
        return;
    }
    StartAnimation();
}

void AddMovementExecutor(CUnitMind* mind, int from, int to, bool run)
{
    if (mind != NULL && mind->GetUnit() != NULL) {
        float speed;
        EPersonAnimType animation;
        mind->GetUnit()->GetMovementAnimParam(&speed, &animation, run);
        CMovementExecutor* executor = new CMovementExecutor(mind, speed, animation);
        mind->AddExecutor(executor);
        executor->OnStartFromPointToPoint(from, to);
    }
}

void AddMovementExecutorToExitPoint(CUnitMind* mind, int from)
{
    if (mind != NULL && mind->GetUnit() != NULL) {
        float speed;
        EPersonAnimType animation;
        mind->GetUnit()->GetMovementAnimParam(&speed, &animation, true);
        CMovementExecutor* executor = new CMovementExecutor(mind, speed, animation);
        mind->AddExecutor(executor);
        executor->OnStartToExitPoint(from);
    }
}

void AddMovementExecutorToAttackPoint(CUnitMind* mind, int from, bool safeOnly)
{
    if (mind != NULL && mind->GetUnit() != NULL) {
        float speed;
        EPersonAnimType animation;
        mind->GetUnit()->GetMovementAnimParam(&speed, &animation, true);
        CMovementExecutor* executor = new CMovementExecutor(mind, speed, animation);
        mind->AddExecutor(executor);
        executor->OnStartToAttackPoint(from, safeOnly);
    }
}

typedef char AssertMovementExecutorSize[sizeof(CMovementExecutor) == 0x4034 ? 1 : -1];
