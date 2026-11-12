// CHitExecutor.h - one reaction animation for an impact.
#ifndef CHITEXECUTOR_H
#define CHITEXECUTOR_H

#include "IExecutor.h"

class CHitExecutor : public IExecutor {
public:
    CHitExecutor(CUnitMind* mind, bool explosive) : IExecutor(mind), m_explosive(explosive) {}
    void OnStart();
    virtual void OnAnimationComplete(EPersonAnimType type, const char* name);
    virtual const wchar_t* GetName() const { return L"CHitExecutor"; }

private:
    bool m_explosive; // 0x0c
};

void AddHitExecutor(CUnitMind* mind, bool explosive);

#endif
