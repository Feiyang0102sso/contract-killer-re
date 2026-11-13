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
#include "DGHelper.h"

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
    // the original checks the mind first and reads the unit without a NULL check
    if (m_mind != NULL && CSwerveGame::m_pInstance->m_stunMs > 0 && !m_mind->GetUnit()->m_invulnerable
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
    app->m_helper->m_swerveGame->RegisterUnitNeutralization(unit->m_type);
    if (unit->m_type != UnitCivilian) {
        int previousExperience = app->m_huntingInfo->m_experienceEarned;
        int experience = CProgressSystem::CalcXpForKilledUnit(unit);
        app->InitProgressSystem()->RegisterGameEvent(CProgressSystem::EVENT_ENEMY_TRANQUILLIZED, experience, -1);
        unit->m_field1f0 = app->m_huntingInfo->m_experienceEarned - previousExperience;
        RegisterAsNeutralized();
    }
    unit->m_tranquillized = true;
}

void CHumanLogicExecutor::OnDeath(const vec3& direction)
{
    RegisterAsDead();
    CancelChildren();
    CSwerveGame* game = CSwerveGame::m_pInstance;
    if (game == NULL || game->m_specialWeapon == NULL) {
        AddDeathExecutorExecutor(m_mind, direction);
        return;
    }
    CUnit* unit = m_mind->GetUnit();
    if (unit != NULL) {
        game->PlayInternalBodyBlast(unit->m_body.m_position);
        unit->m_dead = true;
        unit->MarkAsRemoved();
        m_mind->CancelAllExecutors();
    }
}

__attribute__((noinline)) void CHumanLogicExecutor::RegisterAsDead()
{
    if (m_mind == NULL || m_mind->GetUnit() == NULL) {
        return;
    }
    CUnit* unit = m_mind->GetUnit();
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_helper->m_swerveGame->RegisterUnitNeutralization(unit->m_type);
    if (unit->m_type != UnitCivilian) {
        EHitType hitType;
        unit->GetPickInfo(&hitType);
        int experience = CProgressSystem::CalcXpForKilledUnit(unit);
        app->InitProgressSystem()->RegisterGameEvent(CProgressSystem::EVENT_ENEMY_KILLED, experience,
                                                   hitType == HitHead);
        RegisterAsNeutralized();
    }
}

void CHumanLogicExecutor::RegisterAsNeutralized()
{
    // the original leaves the squad first and does not skip civilians
    CUnit* unit = m_mind->GetUnit();
    if (unit == NULL) {
        return;
    }
    if (unit->m_squad != NULL) {
        unit->m_squad->LeaveSquad(unit);
    }
    unit->m_invulnerable = true;
}
