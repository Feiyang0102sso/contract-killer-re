#ifndef CWARRIORLOGICEXECUTOR_H
#define CWARRIORLOGICEXECUTOR_H

#include "CHumanLogicExecutor.h"

class CWarriorLogicExecutor : public CHumanLogicExecutor {
public:
    CWarriorLogicExecutor(CUnitMind* mind, bool scripted, bool inSquad);
    virtual void OnTranquillize();
    virtual void OnChildFinished(EExecutosResult result);
    virtual const wchar_t* GetName() const;
};

void AddWarriorLogicExecutor(CUnitMind* mind, bool inSquad);

#endif
