// CStunExecutor.h - plays the stun animation until it ends or 12 seconds pass.
#ifndef CSTUNEXECUTOR_H
#define CSTUNEXECUTOR_H

#include "IExecutor.h"

class CStunExecutor : public IExecutor {
public:
    CStunExecutor(CUnitMind* mind)
        : IExecutor(mind)
    {
        m_elapsed = 0;
    }

    virtual void Tick(int deltaTime);
    virtual void OnAnimationComplete(EPersonAnimType animType, const char* animName);

    virtual const wchar_t* GetName() const
    {
        return L"CStunExecutor";
    }

    void OnStart();

private:
    int m_elapsed;      // 0x0c milliseconds since the start
};

/** Push a stun executor on `mind` (ignored when NULL). */
void AddStunExecutor(CUnitMind* mind);

#endif
