#include "CRunAwayExecutor.h"

#include "App.h"
#include "CBH_Player.h"
#include "CMovementExecutor.h"
#include "CTrickShotManager.h"
#include "CUnit.h"

void CRunAwayExecutor::OnReceiveDamage()
{
    CUnit* unit = m_mind->GetUnit();
    if (unit != NULL && unit->m_type == UnitBoss && unit->m_fatalDamage) {
        App* app = static_cast<App*>(WindowApp::m_instance);
        app->m_trickShotManager->RegisterEvent(CTrickShotManager::FastShot);
        CBH_Player::GetInstance()->m_gameStats.RegisterEscapingKill();
    }
}

void CRunAwayExecutor::OnChildFinished(EExecutosResult)
{
    CUnit* unit = m_mind->GetUnit();
    if (unit != NULL && !unit->m_removed) {
        unit->m_body.UnbindFromWorld();
        unit->MarkAsRemoved();
    }
}

void CRunAwayExecutor::OnStart()
{
    if (m_mind != NULL && m_mind->GetUnit() != NULL) {
        MoveToExitPoint();
    }
}

void CRunAwayExecutor::MoveToExitPoint()
{
    CUnit* unit = m_mind->GetUnit();
    if (unit != NULL) {
        AddMovementExecutorToExitPoint(m_mind, unit->m_body.m_cellId);
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
