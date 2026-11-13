#include "CWarTimeExecutor.h"

#include "App.h"
#include "CAttackExecutor.h"
#include "CMovementExecutor.h"
#include "CRunAwayExecutor.h"
#include "CSwerveGame.h"
#include "CUnit.h"
#include "DGHelper.h"

CWarTimeExecutor::CWarTimeExecutor(CUnitMind* mind)
    : IExecutor(mind), m_camera(-1), m_safe(false), m_retryBoss(true)
{
}

void CWarTimeExecutor::Tick(int)
{
    // the original reaches the game through the application
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (app->m_helper->m_swerveGame->GetCurrCamId() != m_camera) {
        CancelChildren();
        MoveToAttackPoint();
    }
}

void CWarTimeExecutor::MoveToAttackPoint()
{
    // the original checks unit and body, queues the move first and reads the camera through the application
    CUnit* unit = m_mind->GetUnit();
    if (unit == NULL) {
        return;
    }
    CUnitBody* body = unit->GetBody();
    if (body == NULL) {
        return;
    }
    m_safe = true;
    AddMovementExecutorToAttackPoint(m_mind, body->m_cellId, true);
    App* app = static_cast<App*>(WindowApp::m_instance);
    m_camera = app->m_helper->m_swerveGame->GetCurrCamId();
}

void CWarTimeExecutor::OnChildFinished(EExecutosResult result)
{
    if (result == EXECUTOR_RESULT_SUCCESS) {
        StartAttack();
        return;
    }
    // Original 0xd1d84: the camera id is taken after a new attack point is requested; running away
    // does not mark the unit here.
    CUnitMind* mind = m_mind;
    CUnit* unit = mind->GetUnit();
    if (unit == NULL || unit->GetBody() == NULL) {
        return;
    }
    if (result == EXECUTOR_RESULT_FAILED && m_safe) {
        m_safe = false;
        AddMovementExecutorToAttackPoint(mind, unit->m_body.m_cellId, false);
        m_camera = static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->GetCurrCamId();
    } else if (unit->m_type == UnitBoss && m_retryBoss) {
        if (CSwerveGame::m_pInstance != NULL) {
            CSwerveGame::m_pInstance->UnregisterAllDeadBodies();
            mind = m_mind;
        }
        m_retryBoss = false;
        AddMovementExecutorToAttackPoint(mind, unit->m_body.m_cellId, m_safe);
        m_camera = static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->GetCurrCamId();
    } else {
        AddRunAwayExecutor(mind);
    }
}

void CWarTimeExecutor::StartAttack()
{
    int camera = static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->GetCurrCamId();
    if (m_safe) {
        DefinePointCharacteristics(camera);
        AddSafeAttackExecutor(m_mind, camera);
        return;
    }
    CUnit* unit = m_mind->GetUnit();
    if (unit == NULL) {
        return;
    }
    CUnitBody* body = unit->GetBody();
    if (body == NULL) {
        return;
    }
    body->SetAttitude(PERSON_SHOT_STAND);
    AddUnsafeAttackExecutor(m_mind, camera);
}

void CWarTimeExecutor::DefinePointCharacteristics(int camera)
{
    // the original checks the unit, then the body pointer and its map
    CUnit* unit = m_mind->GetUnit();
    if (unit == NULL) {
        return;
    }
    CUnitBody* body = unit->GetBody();
    if (body == NULL || body->m_map == NULL) {
        return;
    }
    int cover = body->m_map->GetHideBehaviour(body->m_pointId, camera);
    switch (cover) {
    case 1:
        body->SetAttitude(PERSON_SHOT_WALL_LEFT);
        break;
    case 2:
        body->SetAttitude(PERSON_SHOT_WALL_RIGHT);
        break;
    case 0:
    case 3:
        body->SetAttitude(PERSON_SHOT_CROUCH);
        break;
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
