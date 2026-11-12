// CPeaceIdleExecutor.h - plays one idle animation in peace time.
#ifndef CPEACEIDLEEXECUTOR_H
#define CPEACEIDLEEXECUTOR_H

#include "IExecutor.h"

class CPeaceIdleExecutor : public IExecutor {
public:
    CPeaceIdleExecutor(CUnitMind* mind)
        : IExecutor(mind)
    {
        m_idle = 0;
    }

    virtual void Tick(int deltaTime);
    virtual void OnAnimationComplete(EPersonAnimType animType, const char* animName);

    virtual const wchar_t* GetName() const
    {
        return L"CPeaceIdleExecutor";
    }

    void OnStart();

    void SetIdle(int idle)
    {
        m_idle = idle;
    }

private:
    int m_idle;     // 0x0c 0 default animation, n > 0 idle animation n - 1, negative finishes at once
};

/** Push an idle executor with the default animation on `mind` (ignored when NULL). */
void AddPeaceIdleExecutor(CUnitMind* mind);
/** Push an idle executor playing idle `idle` (0 is the default animation). */
void AddCurrentPeaceIdleExecutor(CUnitMind* mind, int idle);

#endif
