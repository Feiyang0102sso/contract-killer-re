#include "CHumanLogicExecutor.h"

#include "App.h"
#include "CBH_Player.h"
#include "CBurnExecutor.h"
#include "CDeathExecutor.h"
#include "CHitExecutor.h"
#include "CHuntingInfo.h"
#include "CPeaceTimeExecutor.h"
#include "CPeaceTimeSquadExecutor.h"
#include "CPeaceTimeScriptedExecutor.h"
#include "CProgressSystem.h"
#include "CStunExecutor.h"
#include "CSquad.h"
#include "CSwerveGame.h"
#include "CUnit.h"

CHumanLogicExecutor::CHumanLogicExecutor(CUnitMind* mind, bool scripted, bool inSquad)
    : IExecutor(mind), m_reacted(false), m_scripted(scripted), m_inSquad(inSquad)
{
}

void CHumanLogicExecutor::OnStart()
{
    if (m_inSquad) {
        AddPeaceTimeSquadExecutor(m_mind);
    } else if (m_scripted) {
        AddPeaceTimeScriptedExecutor(m_mind);
    } else {
        AddPeaceTimeExecutor(m_mind);
    }
}

void CHumanLogicExecutor::OnTranquillize()
{
    RegisterAsTranquillized();
    CancelChildren();
    AddTranquillizeExecutorExecutor(m_mind);
}

void CHumanLogicExecutor::Tick(int)
{
    CUnit* unit = m_mind->GetUnit();
    if (unit != NULL && CSwerveGame::m_pInstance->m_stunMs > 0 && !unit->m_invulnerable
        && m_mind->GetExecutor(L"CStunExecutor") == NULL) {
        CancelChildren();
        AddStunExecutor(m_mind);
    }
}

void CHumanLogicExecutor::OnBurnt(int duration, int damage)
{
    if (m_mind != NULL) {
        CBurnExecutor* burn = static_cast<CBurnExecutor*>(m_mind->GetExecutor(L"CBurnExecutor"));
        if (burn != NULL) {
            burn->SetDamageAndTime(duration, damage);
        } else {
            CancelChildren();
            AddBurnExecutor(m_mind, duration, damage);
        }
    }
}

void CHumanLogicExecutor::OnHit(bool explosive)
{
    CancelChildren();
    AddHitExecutor(m_mind, explosive);
}

void CHumanLogicExecutor::RegisterAsTranquillized()
{
    CUnit* unit = m_mind->GetUnit();
    if (unit == NULL || !unit->CanBeHandled()) {
        return;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    CSwerveGame::m_pInstance->RegisterUnitNeutralization(unit->m_type);
    if (unit->m_type == UnitBoss) {
        app->m_huntingInfo->OnBossTranquillized();
    }
    if (unit->m_type != UnitCivilian) {
        app->InitProgressSystem()->RegisterGameEvent(CProgressSystem::EVENT_ENEMY_TRANQUILLIZED,
                                                   CProgressSystem::CalcXpForKilledUnit(unit), -1);
    }
    unit->m_tranquillized = true;
    RegisterAsNeutralized();
}

void CHumanLogicExecutor::OnDeath(const vec3& direction)
{
    RegisterAsDead();
    CancelChildren();
    AddDeathExecutorExecutor(m_mind, direction);
}

void CHumanLogicExecutor::RegisterAsDead()
{
    RegisterAsNeutralized();
}

void CHumanLogicExecutor::RegisterAsNeutralized()
{
    CUnit* unit = m_mind->GetUnit();
    if (unit != NULL && unit->m_type != UnitCivilian) {
        unit->m_invulnerable = true;
        if (unit->m_squad != NULL) {
            unit->m_squad->LeaveSquad(unit);
        }
    }
}
