#include "CBossLogicExecutor.h"

#include "App.h"
#include "CHuntingInfo.h"
#include "CRunAwayExecutor.h"
#include "CSquad.h"
#include "CSwerveGame.h"
#include "CUnit.h"

CBossLogicExecutor::CBossLogicExecutor(CUnitMind* mind, bool scripted)
    : CWarriorLogicExecutor(mind, scripted, false), m_field10(0)
{
}

void CBossLogicExecutor::Tick(int elapsed)
{
    CHumanLogicExecutor::Tick(elapsed);
    if (m_mind->GetExecutor(L"CStunExecutor") != NULL) {
        return;
    }
    CUnit* unit = m_mind->GetUnit();
    if (unit != NULL && CSwerveGame::m_pInstance->m_bossMustEscape && !m_reacted && !unit->m_invulnerable) {
        CancelChildren();
        m_reacted = true;
        unit->MarkAsRunAway();
        if (unit->m_squad != NULL) {
            unit->m_squad->LeaveSquad(unit);
        }
        AddRunAwayExecutor(m_mind);
    }
}

void CBossLogicExecutor::OnReceiveDamage()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CUnit* unit = m_mind->GetUnit();
    if (!app->m_huntingInfo->IsAmbush() && !m_reacted && unit != NULL && unit->m_wounded && unit->m_hp > 0) {
        CancelChildren();
        m_reacted = true;
        if (unit->m_squad != NULL) {
            unit->m_squad->LeaveSquad(unit);
        }
        AddRunAwayExecutor(m_mind);
    }
}

void CBossLogicExecutor::OnTranquillize()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_huntingInfo->OnBossTranquillized();
    CWarriorLogicExecutor::OnTranquillize();
}

const wchar_t* CBossLogicExecutor::GetName() const
{
    return L"CBossLogicExecutor";
}

void AddBossLogicExecutor(CUnitMind* mind, bool scripted)
{
    if (mind != NULL) {
        CBossLogicExecutor* executor = new CBossLogicExecutor(mind, scripted);
        mind->AddExecutor(executor);
        executor->OnStart();
    }
}

typedef char AssertBossLogicSize[sizeof(CBossLogicExecutor) == 0x14 ? 1 : -1];
