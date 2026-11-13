#include "CWarriorLogicExecutor.h"

#include "CRunAwayExecutor.h"
#include "App.h"
#include "CSwerveGame.h"
#include "DGHelper.h"
#include "CWarTimeExecutor.h"

CWarriorLogicExecutor::CWarriorLogicExecutor(CUnitMind* mind, bool scripted, bool inSquad)
    : CHumanLogicExecutor(mind, scripted, inSquad)
{
}

void CWarriorLogicExecutor::OnTranquillize()
{
    // the original reaches the game through App -> m_helper and reloads it per access
    if (static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->m_peaceTime) {
        static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->m_peaceTime = false;
        static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->m_missionAlert = true;
    }
    CHumanLogicExecutor::OnTranquillize();
}

void CWarriorLogicExecutor::OnChildFinished(EExecutosResult result)
{
    if (result == EXECUTOR_RESULT_STUN_OVER) {
        AddWarTimeExecutor(m_mind);
    } else if (!m_reacted) {
        AddRunAwayExecutor(m_mind);
        m_reacted = true;
    }
}

const wchar_t* CWarriorLogicExecutor::GetName() const
{
    return L"CWarriorLogicExecutor";
}

void AddWarriorLogicExecutor(CUnitMind* mind, bool inSquad)
{
    if (mind != NULL) {
        CWarriorLogicExecutor* executor = new CWarriorLogicExecutor(mind, false, inSquad);
        mind->AddExecutor(executor);
        executor->OnStart();
    }
}
