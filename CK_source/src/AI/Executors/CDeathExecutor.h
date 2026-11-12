// CDeathExecutor.h - run a unit's neutralization animation to completion.
#ifndef CDEATHEXECUTOR_H
#define CDEATHEXECUTOR_H

#include "IExecutor.h"
#include "MathLib.h"

class CDeathExecutor : public IExecutor {
public:
    CDeathExecutor(CUnitMind* mind, bool death, const vec3& direction)
        : IExecutor(mind), m_death(death), m_direction(direction) {}

    void OnStart();
    virtual void OnAnimationComplete(EPersonAnimType type, const char* name);
    virtual const wchar_t* GetName() const { return L"CDeathExecutor"; }

private:
    bool m_death;       // 0x0c
    vec3 m_direction;   // 0x10
};

void AddDeathExecutorExecutor(CUnitMind* mind, vec3 direction);
void AddTranquillizeExecutorExecutor(CUnitMind* mind);

#endif
