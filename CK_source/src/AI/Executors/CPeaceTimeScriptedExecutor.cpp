#include "CPeaceTimeScriptedExecutor.h"

#include "App.h"
#include "CHuntingInfo.h"
#include "CMovementExecutor.h"
#include "CPeaceIdleExecutor.h"
#include "CScriptType.h"
#include "CUnit.h"

CPeaceTimeScriptedExecutor::CPeaceTimeScriptedExecutor(CUnitMind* mind)
    : CPeaceTimeExecutor(mind), m_waypoint(0)
{
    m_idleNext = false;
}

void CPeaceTimeScriptedExecutor::MoveToNewPoint()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    const CScriptType* script = app->m_huntingInfo->m_assignedData.m_scriptType;
    if (script == NULL) {
        Finish(EXECUTOR_RESULT_NO_BODY);
        return;
    }
    CUnit* unit = m_mind->GetUnit();
    if (unit != NULL) {
        AddMovementExecutor(m_mind, unit->m_body.m_cellId, script->GetWayPointID(m_waypoint), false);
    }
}

void CPeaceTimeScriptedExecutor::SetPeaceIdleBehaviour()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    const CScriptType* script = app->m_huntingInfo->m_assignedData.m_scriptType;
    if (script == NULL) {
        Finish(EXECUTOR_RESULT_NO_BODY);
        return;
    }
    int behaviour = script->GetEldeBehaviorID(m_waypoint);
    AddCurrentPeaceIdleExecutor(m_mind, behaviour);
    MakeNextStep();
}

void CPeaceTimeScriptedExecutor::MakeNextStep()
{
    ++m_waypoint;
    App* app = static_cast<App*>(WindowApp::m_instance);
    const CScriptType* script = app->m_huntingInfo->m_assignedData.m_scriptType;
    if (script != NULL && m_waypoint >= script->m_waypoints.size()) {
        if (script->m_looped) {
            m_waypoint = 0;
        } else {
            Finish(EXECUTOR_RESULT_SUCCESS);
        }
    }
}

const wchar_t* CPeaceTimeScriptedExecutor::GetName() const
{
    return L"CPeaceTimeScriptedExecutor";
}

void AddPeaceTimeScriptedExecutor(CUnitMind* mind)
{
    if (mind != NULL) {
        CPeaceTimeScriptedExecutor* executor = new CPeaceTimeScriptedExecutor(mind);
        mind->AddExecutor(executor);
        executor->OnStart();
    }
}

typedef char AssertPeaceTimeScriptedSize[sizeof(CPeaceTimeScriptedExecutor) == 0x1c ? 1 : -1];
