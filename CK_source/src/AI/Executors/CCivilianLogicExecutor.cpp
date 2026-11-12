// CCivilianLogicExecutor.cpp - civilian logic.
#include "CCivilianLogicExecutor.h"
#include "CRunAwayExecutor.h"
#include "CUnitMind.h"

void CCivilianLogicExecutor::OnChildFinished(EExecutosResult result)
{
    if (m_reacted) {
        return;
    }
    AddRunAwayExecutor(m_mind);
    m_reacted = true;
}

void AddCivilianLogicExecutor(CUnitMind* mind)
{
    if (mind == NULL) {
        return;
    }
    CCivilianLogicExecutor* executor = new CCivilianLogicExecutor(mind);
    mind->AddExecutor(executor);
    executor->OnStart();
}
