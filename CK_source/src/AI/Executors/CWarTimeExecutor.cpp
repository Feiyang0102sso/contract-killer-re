#include "CWarTimeExecutor.h"

#include "CAttackExecutor.h"
#include "CMovementExecutor.h"
#include "CRunAwayExecutor.h"
#include "CSwerveGame.h"
#include "CUnit.h"

CWarTimeExecutor::CWarTimeExecutor(CUnitMind* mind)
    : IExecutor(mind), m_camera(-1), m_safe(false), m_retryBoss(true)
{
}

void CWarTimeExecutor::Tick(int)
{
    if (CSwerveGame::m_pInstance->GetCurrCamId() != m_camera) {
        CancelChildren();
        MoveToAttackPoint();
    }
}

void CWarTimeExecutor::MoveToAttackPoint()
{
    CUnit* unit = m_mind->GetUnit();
    if (unit != NULL) {
        m_camera = CSwerveGame::m_pInstance->GetCurrCamId();
        m_safe = true;
        AddMovementExecutorToAttackPoint(m_mind, unit->m_body.m_cellId, true);
    }
}

void CWarTimeExecutor::OnChildFinished(EExecutosResult result)
{
    if (result == EXECUTOR_RESULT_SUCCESS) {
        StartAttack();
        return;
    }
    CUnit* unit = m_mind->GetUnit();
    if (unit == NULL) {
        return;
    }
    m_camera = CSwerveGame::m_pInstance->GetCurrCamId();
    if (result == EXECUTOR_RESULT_FAILED && m_safe) {
        m_safe = false;
        AddMovementExecutorToAttackPoint(m_mind, unit->m_body.m_cellId, false);
    } else if (unit->m_type == UnitBoss && m_retryBoss) {
        CSwerveGame::m_pInstance->UnregisterAllDeadBodies();
        m_retryBoss = false;
        AddMovementExecutorToAttackPoint(m_mind, unit->m_body.m_cellId, m_safe);
    } else {
        unit->MarkAsRunAway();
        AddRunAwayExecutor(m_mind);
    }
}

void CWarTimeExecutor::StartAttack()
{
    int camera = CSwerveGame::m_pInstance->GetCurrCamId();
    if (m_safe) {
        DefinePointCharacteristics(camera);
        AddSafeAttackExecutor(m_mind, camera);
    } else {
        m_mind->GetUnit()->m_body.SetAttitude(PERSON_SHOT_STAND);
        AddUnsafeAttackExecutor(m_mind, camera);
    }
}

void CWarTimeExecutor::DefinePointCharacteristics(int camera)
{
    CUnit* unit = m_mind->GetUnit();
    if (unit != NULL && unit->m_body.m_map != NULL) {
        int cover = unit->m_body.m_map->GetHideBehaviour(unit->m_body.m_pointId, camera);
        switch (cover) {
        case 0:
        case 3:
            unit->m_body.SetAttitude(PERSON_SHOT_CROUCH);
            break;
        case 1:
            unit->m_body.SetAttitude(PERSON_SHOT_WALL_LEFT);
            break;
        case 2:
            unit->m_body.SetAttitude(PERSON_SHOT_WALL_RIGHT);
            break;
        }
    }
}

const wchar_t* CWarTimeExecutor::GetName() const
{
    return L"CWarTimeExecutor";
}

void AddWarTimeExecutor(CUnitMind* mind)
{
    if (mind != NULL) {
        CWarTimeExecutor* executor = new CWarTimeExecutor(mind);
        mind->AddExecutor(executor);
        executor->MoveToAttackPoint();
    }
}

typedef char AssertWarTimeExecutorSize[sizeof(CWarTimeExecutor) == 0x14 ? 1 : -1];
