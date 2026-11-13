// CAlertExecutor.h - plays the afraid animation once.
#ifndef CALERTEXECUTOR_H
#define CALERTEXECUTOR_H

#include "IExecutor.h"

class CAlertExecutor : public IExecutor {
public:
    CAlertExecutor(CUnitMind* mind)
        : IExecutor(mind)
    {
    }

    virtual void OnAnimationComplete(EPersonAnimType animType, const char* animName);

    virtual const wchar_t* GetName() const
    {
        return L"CAlertExecutor";
    }

    void OnStart();
    bool HasActiveAnimation() const { return m_activeAnimation != NULL; }
};

/** Push an alert executor on `mind` (ignored when NULL). */
void AddAlertExecutor(CUnitMind* mind);

#endif
