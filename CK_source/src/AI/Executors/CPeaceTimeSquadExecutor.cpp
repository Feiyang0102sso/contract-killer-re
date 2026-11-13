#include "CPeaceTimeSquadExecutor.h"

#include "CSquad.h"
#include "CUnit.h"

CPeaceTimeSquadExecutor::CPeaceTimeSquadExecutor(CUnitMind* mind)
    : CPeaceTimeExecutor(mind), m_squadMind(mind), m_waiting(false)
{
}

void CPeaceTimeSquadExecutor::Tick(int elapsed)
{
    MoveToSquadPoint();
    CPeaceTimeExecutor::Tick(elapsed);
}

void CPeaceTimeSquadExecutor::MoveToSquadPoint()
{
    CUnit* unit = m_squadMind->GetUnit();
    if (unit == NULL || unit->m_squad == NULL) {
        return;
    }
    CUnitBody* body = unit->GetBody();
    vec3 target = unit->m_squad->GetLocalSquadPosition(unit);
    vec3 difference = target - body->m_position;
    m_waiting = difference.LengthSqr() < 0.01f || !unit->m_squad->m_hasLeader;
    if (m_waiting) {
        body->SetMovementParam(body->m_position, 0.1f);
    } else {
        float speed;
        EPersonAnimType animation;
        unit->GetMovementAnimParam(&speed, &animation, false);
        body->SetMovementParam(target, speed);
    }
}

void CPeaceTimeSquadExecutor::OnAnimationComplete(EPersonAnimType, const char* animation)
{
    if (IsAnimationTheSame(animation)) {
        if (m_waiting) {
            PlayIdleBehaviour();
        } else {
            PlayMoveBehaviour();
        }
    }
}

__attribute__((noinline)) void CPeaceTimeSquadExecutor::PlayIdleBehaviour()
{
    // the original also checks the squad mind
    if (m_squadMind == NULL) {
        return;
    }
    CUnit* unit = m_squadMind->GetUnit();
    if (unit != NULL) {
        m_activeAnimation = unit->m_body.StartAnimation(PERSON_ANIM_TYPE_DEFAULT, false);
    }
}

void CPeaceTimeSquadExecutor::PlayMoveBehaviour()
{
    // the original also checks the squad mind
    if (m_squadMind == NULL) {
        return;
    }
    CUnit* unit = m_squadMind->GetUnit();
    if (unit != NULL) {
        float speed;
        EPersonAnimType animation;
        unit->GetMovementAnimParam(&speed, &animation, false);
        m_activeAnimation = unit->m_body.StartAnimation(animation, false);
    }
}

void CPeaceTimeSquadExecutor::OnStart()
{
    CUnit* unit = m_squadMind->GetUnit();
    if (unit != NULL && unit->m_squad != NULL && unit->m_squad->m_hasLeader) {
        unit->m_body.m_position = unit->m_squad->GetLocalSquadPosition(unit);
        unit->m_body.m_moved = true;
        PlayMoveBehaviour();
    }
}

void AddPeaceTimeSquadExecutor(CUnitMind* mind)
{
    if (mind != NULL) {
        CPeaceTimeSquadExecutor* executor = new CPeaceTimeSquadExecutor(mind);
        mind->AddExecutor(executor);
        executor->OnStart();
    }
}

typedef char AssertPeaceTimeSquadSize[sizeof(CPeaceTimeSquadExecutor) == 0x20 ? 1 : -1];
