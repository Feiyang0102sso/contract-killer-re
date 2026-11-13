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
    if (m_mind == NULL) {
        return;
    }
    static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->RegisterUnitNeutralization(
        m_mind->GetUnit()->m_type);
    if (m_mind->GetUnit()->m_type != UnitCivilian) {
        int previousExperience = static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_experienceEarned;
        int experience = CProgressSystem::CalcXpForKilledUnit(m_mind->GetUnit());
        static_cast<App*>(WindowApp::m_instance)->InitProgressSystem()->RegisterGameEvent(
            CProgressSystem::EVENT_ENEMY_TRANQUILLIZED, experience, -1);
        if (m_mind != NULL && m_mind->GetUnit() != NULL) {
            m_mind->GetUnit()->m_field1f0 =
                static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_experienceEarned - previousExperience;
        }
        RegisterAsNeutralized();
    }
}

void CHumanLogicExecutor::OnDeath(const vec3& direction)
{
    RegisterAsDead();
    CancelChildren();
    CSwerveGame* game = CSwerveGame::m_pInstance;
    if (game != NULL && game->m_specialWeapon != NULL) {
        if (m_mind != NULL) {
            game->PlayInternalBodyBlast(m_mind->GetUnit()->m_body.m_position);
            m_mind->GetUnit()->m_dead = true;
            m_mind->GetUnit()->MarkAsRemoved();
            m_mind->CancelAllExecutors();
        }
    } else {
        AddDeathExecutorExecutor(m_mind, direction);
    }
}

__attribute__((noinline)) void CHumanLogicExecutor::RegisterAsDead()
{
    if (m_mind == NULL) {
        return;
    }
    static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->RegisterUnitNeutralization(
        m_mind->GetUnit()->m_type);
    if (m_mind->GetUnit()->m_type != UnitCivilian) {
        EHitType hitType;
        m_mind->GetUnit()->GetPickInfo(&hitType);
        EHitType snapshot = hitType;
        int experience = CProgressSystem::CalcXpForKilledUnit(m_mind->GetUnit());
        static_cast<App*>(WindowApp::m_instance)->InitProgressSystem()->RegisterGameEvent(
            CProgressSystem::EVENT_ENEMY_KILLED, experience, snapshot == HitHead);
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
