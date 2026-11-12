#ifndef CBOSSLOGICEXECUTOR_H
#define CBOSSLOGICEXECUTOR_H

#include "CWarriorLogicExecutor.h"

class CBossLogicExecutor : public CWarriorLogicExecutor {
public:
    CBossLogicExecutor(CUnitMind* mind, bool scripted);
    virtual void Tick(int elapsed);
    virtual void OnReceiveDamage();
    virtual void OnTranquillize();
    virtual const wchar_t* GetName() const;

private:
    int m_field10;
};

void AddBossLogicExecutor(CUnitMind* mind, bool scripted);

#endif
