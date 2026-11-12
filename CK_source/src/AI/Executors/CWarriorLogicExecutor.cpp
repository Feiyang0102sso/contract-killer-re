#include "CWarriorLogicExecutor.h"

#include "CRunAwayExecutor.h"
#include "CSwerveGame.h"
#include "CWarTimeExecutor.h"

CWarriorLogicExecutor::CWarriorLogicExecutor(CUnitMind* mind, bool scripted, bool inSquad)
    : CHumanLogicExecutor(mind, scripted, inSquad)
{
}

void CWarriorLogicExecutor::OnTranquillize()
{
    CSwerveGame* game = CSwerveGame::m_pInstance;
    if (game->m_peaceTime) {
        game->m_peaceTime = false;
        game->m_missionAlert = true;
    }
    CHumanLogicExecutor::OnTranquillize();
}

void CWarriorLogicExecutor::OnChildFinished(EExecutosResult result)
{
    if (result == EXECUTOR_RESULT_STUN_OVER) {
        AddWarTimeExecutor(m_mind);
    } else if (!m_reacted) {
        m_reacted = true;
        AddRunAwayExecutor(m_mind);
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
