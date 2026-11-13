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
    if (m_squadMind == NULL) {
        return;
    }
    CUnit* unit = m_squadMind->GetUnit();
    if (unit == NULL || unit->m_squad == NULL) {
        return;
    }
    CSquad* squad = unit->m_squad;
    CUnitBody* body = unit->GetBody();
    if (body == NULL) {
        return;
    }
    vec3 target = squad->GetLocalSquadPosition(unit);
    vec3 difference = target - body->m_position;
    float distanceSquared = difference.LengthSqr();
    float distance = MathLib::InvSqrt(distanceSquared) * distanceSquared;
    if (distance < 0.1f) {
        body->SetMovementParam(body->m_position, 0.0f);
        m_waiting = true;
        return;
    }
    m_waiting = false;
    if (!squad->m_hasLeader) {
        body->SetMovementParam(body->m_position, 0.0f);
        m_waiting = true;
        return;
    }
    float speed;
    EPersonAnimType animation;
    unit->GetMovementAnimParam(&speed, &animation, false);
    body->SetMovementParam(target, speed);
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
    if (m_squadMind == NULL) {
        return;
    }
    CUnit* unit = m_squadMind->GetUnit();
    if (unit == NULL || unit->m_squad == NULL) {
        return;
    }
    CUnitBody* body = unit->GetBody();
    if (body == NULL) {
        return;
    }
    body->m_position = unit->m_squad->GetLocalSquadPosition(unit);
    PlayMoveBehaviour();
}

void AddPeaceTimeSquadExecutor(CUnitMind* mind)
{
    if (mind != NULL) {
        CPeaceTimeSquadExecutor* executor = new CPeaceTimeSquadExecutor(mind);
        mind->AddExecutor(executor);
        executor->OnStart();
    }
}

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AssertPeaceTimeSquadSize[sizeof(CPeaceTimeSquadExecutor) == 0x20 ? 1 : -1];
#endif
