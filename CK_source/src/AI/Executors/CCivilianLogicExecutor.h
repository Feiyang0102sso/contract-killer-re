// CCivilianLogicExecutor.h - civilian: runs away once the peace-time behaviour ends.
#ifndef CCIVILIANLOGICEXECUTOR_H
#define CCIVILIANLOGICEXECUTOR_H

#include "CHumanLogicExecutor.h"

class CCivilianLogicExecutor : public CHumanLogicExecutor {
public:
    CCivilianLogicExecutor(CUnitMind* mind)
        : CHumanLogicExecutor(mind, false, false)
    {
    }

    virtual void OnChildFinished(EExecutosResult result);

    virtual const wchar_t* GetName() const
    {
        return L"CCivilianLogicExecutor";
    }
};

/** Push a civilian logic executor on `mind` (ignored when NULL). */
void AddCivilianLogicExecutor(CUnitMind* mind);

#endif
