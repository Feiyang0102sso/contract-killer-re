#ifndef CPEACETIMESCRIPTEDEXECUTOR_H
#define CPEACETIMESCRIPTEDEXECUTOR_H

#include "CPeaceTimeExecutor.h"

class CPeaceTimeScriptedExecutor : public CPeaceTimeExecutor {
public:
    explicit CPeaceTimeScriptedExecutor(CUnitMind* mind);
    virtual void MoveToNewPoint();
    virtual void SetPeaceIdleBehaviour();
    virtual const wchar_t* GetName() const;
    void MakeNextStep();

private:
    int m_waypoint;
};

void AddPeaceTimeScriptedExecutor(CUnitMind* mind);

#endif
