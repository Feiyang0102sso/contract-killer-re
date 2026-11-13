#include "CBossLogicExecutor.h"

#include "App.h"
#include "DGHelper.h"
#include "CHuntingInfo.h"
#include "CRunAwayExecutor.h"
#include "CSwerveGame.h"
#include "CUnit.h"

CBossLogicExecutor::CBossLogicExecutor(CUnitMind* mind, bool scripted)
    : CWarriorLogicExecutor(mind, scripted, false), m_field10(0)
{
}

void CBossLogicExecutor::Tick(int elapsed)
{
    // Original 0xd02d4: the escape check runs before the human logic tick (no stun check).
    CUnit* unit = m_mind->GetUnit();
    if (unit == NULL) {
        return;
    }
    if (static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->m_bossMustEscape && !m_reacted
        && !unit->m_invulnerable) {
        CancelChildren();
        AddRunAwayExecutor(m_mind);
        m_reacted = true;
        unit->MarkAsRunAway();
    }
    CHumanLogicExecutor::Tick(elapsed);
}

void CBossLogicExecutor::OnReceiveDamage()
{
    if (static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->IsAmbush() || m_reacted) {
        return;
    }
    CUnit* unit = m_mind->GetUnit();
    if (unit != NULL && unit->m_wounded && unit->m_hp > 0) {
        CancelChildren();
        AddRunAwayExecutor(m_mind);
        m_reacted = true;
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
