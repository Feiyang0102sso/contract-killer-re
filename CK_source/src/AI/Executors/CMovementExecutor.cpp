#include "CMovementExecutor.h"

#include "App.h"
#include "CBH_Player.h"
#include "DGHelper.h"
#include "CPathCalculator.h"
#include "CSwerveGame.h"
#include "CUnit.h"
#include "CVisualCharacterType.h"

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
    if (body == NULL) {
        return;
    }
    if (unit->m_hp == 0 && body->IsRunning()) {
        EHitType hit;
        unit->GetPickInfo(&hit);
        if (hit == HitHead) {
            CBH_Player::GetInstance()->m_gameStats.AddHeadshotsInRunning(1);
        }
        return;
    }
    if (unit->m_hp < unit->m_maxHp) {
        // original: wounded run animation (7) at the shared wounded running speed
        float speed = CVisualCharacterType::m_runWoundSpeed;
        m_animation = static_cast<EPersonAnimType>(7);
        StopActiveAnimation();
        StartAnimation();
        m_path.m_speed = speed;
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

__attribute__((noinline)) void CMovementExecutor::StartAnimation()
{
    // the original checks the body pointer it gets from the unit
    CUnitBody* body = m_mind->GetUnit()->GetBody();
    if (body != NULL) {
        m_activeAnimation = body->StartAnimation(m_animation, true);
    }
}

void CMovementExecutor::OnStartFromPointToPoint(int from, int to)
{
    // the original only hands over a non-NULL body
    CUnitBody* body = m_mind->GetUnit()->GetBody();
    if (body != NULL) {
        m_path.SetBody(body);
    }
    if (!m_mind->m_pathCalculator->FindPath(&m_path, from, to)) {
        Finish(EXECUTOR_RESULT_FAILED);
        return;
    }
    StartAnimation();
}

void CMovementExecutor::OnStartToExitPoint(int from)
{
    // the original only hands over a non-NULL body
    CUnitBody* body = m_mind->GetUnit()->GetBody();
    if (body != NULL) {
        m_path.SetBody(body);
    }
    if (!m_mind->m_pathCalculator->FindPathToNearestExitPoint(&m_path, from)) {
        Finish(EXECUTOR_RESULT_FAILED);
        return;
    }
    StartAnimation();
}

void CMovementExecutor::OnStartToAttackPoint(int from, bool safeOnly)
{
    // the original only hands over a non-NULL body
    CUnitBody* body = m_mind->GetUnit()->GetBody();
    if (body != NULL) {
        m_path.SetBody(body);
    }
    // original: calculator read before the camera id, the game taken through App -> helper
    CPathCalculator* calculator = m_mind->m_pathCalculator;
    int camera = static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->GetCurrCamId();
    if (!calculator->FindPathToNearestAttackPoint(&m_path, from, camera, safeOnly)) {
        Finish(EXECUTOR_RESULT_FAILED);
    } else {
        StartAnimation();
    }
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
