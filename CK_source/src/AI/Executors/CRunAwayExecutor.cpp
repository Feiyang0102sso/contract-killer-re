#include "CRunAwayExecutor.h"

#include "App.h"
#include "CBH_Player.h"
#include "CMovementExecutor.h"
#include "CSquad.h"
#include "CTrickShotManager.h"
#include "CUnit.h"

void CRunAwayExecutor::OnReceiveDamage()
{
    CUnit* unit = m_mind->GetUnit();
    if (unit != NULL && unit->m_type == UnitBoss && unit->m_fatalDamage) {
        // the original only registers the trick shot here
        static_cast<App*>(WindowApp::m_instance)->m_trickShotManager->RegisterEvent(CTrickShotManager::FastShot);
    }
}

void CRunAwayExecutor::OnChildFinished(EExecutosResult)
{
    // the original only marks the unit as removed
    CUnit* unit = m_mind->GetUnit();
    if (unit != NULL) {
        unit->MarkAsRemoved();
    }
}

void CRunAwayExecutor::OnStart()
{
    if (m_mind == NULL) {
        return;
    }
    CUnit* unit = m_mind->GetUnit();
    if (unit == NULL) {
        return;
    }
    if (unit->m_squad != NULL) {
        unit->m_squad->LeaveSquad(unit);
    }
    MoveToExitPoint();
}

__attribute__((noinline)) void CRunAwayExecutor::MoveToExitPoint()
{
    CUnit* unit = m_mind->GetUnit();
    if (unit == NULL) {
        return;
    }
    // the original also checks the body pointer
    CUnitBody* body = unit->GetBody();
    if (body != NULL) {
        AddMovementExecutorToExitPoint(m_mind, body->m_cellId);
    }
}

void AddRunAwayExecutor(CUnitMind* mind)
{
    if (mind != NULL) {
        CRunAwayExecutor* executor = new CRunAwayExecutor(mind);
        mind->AddExecutor(executor);
        executor->OnStart();
    }
}
