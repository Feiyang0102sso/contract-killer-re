// CDeathExecutor.cpp - mark the unit neutralized after the animation ends.
#include "CDeathExecutor.h"

#include "App.h"
#include "CSwerveGame.h"
#include "CUnit.h"
#include "DGHelper.h"

typedef char DeathExecutorSizeCheck[sizeof(CDeathExecutor) == 0x1c ? 1 : -1];

void CDeathExecutor::OnAnimationComplete(EPersonAnimType, const char* name)
{
    if (!IsAnimationTheSame(name)) {
        return;
    }
    CUnit* unit = m_mind->GetUnit();
    if (unit == NULL) {
        return;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (unit->m_type == UnitBoss) {
        app->m_helper->m_swerveGame->SetSlowMode(false);
    }
    if (m_death) {
        unit->m_dead = true;
    } else {
        unit->m_tranquillized = true;
    }
    app->m_helper->m_swerveGame->ShowUnitInfo(m_mind->GetUnit(), 0);
    m_mind->CancelAllExecutors();
}

void CDeathExecutor::OnStart()
{
    CUnit* unit = m_mind->GetUnit();
    // the original checks both the unit and the body pointer
    CUnitBody* body = NULL;
    if (unit != NULL) {
        body = unit->GetBody();
    }
    if (body != NULL) {
        body->OnNeutralization();
        if (m_death) {
            m_activeAnimation = body->PlayDeathAnimation(m_direction);
        } else {
            m_activeAnimation = body->PlayTranquillizeAnimation();
        }
    }
}

void AddDeathExecutorExecutor(CUnitMind* mind, vec3 direction)
{
    if (mind != NULL) {
        CDeathExecutor* executor = new CDeathExecutor(mind, true, direction);
        mind->AddExecutor(executor);
        executor->OnStart();
    }
}

void AddTranquillizeExecutorExecutor(CUnitMind* mind)
{
    if (mind != NULL) {
        CDeathExecutor* executor = new CDeathExecutor(mind, false, vec3());
        mind->AddExecutor(executor);
        executor->OnStart();
    }
}
