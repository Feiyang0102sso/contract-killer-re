// CRunAwayExecutor.h - leaves the squad and walks to the exit point of the unit.
#ifndef CRUNAWAYEXECUTOR_H
#define CRUNAWAYEXECUTOR_H

#include "IExecutor.h"

class CRunAwayExecutor : public IExecutor {
public:
    CRunAwayExecutor(CUnitMind* mind)
        : IExecutor(mind)
    {
    }

    virtual void OnReceiveDamage();
    virtual void OnChildFinished(EExecutosResult result);

    virtual const wchar_t* GetName() const
    {
        return L"CRunAwayExecutor";
    }

    void OnStart();
    void MoveToExitPoint();
};

/** Push a run-away executor on `mind` (ignored when NULL). */
void AddRunAwayExecutor(CUnitMind* mind);

#endif
