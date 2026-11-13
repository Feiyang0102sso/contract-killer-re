#ifndef CATTACKEXECUTOR_H
#define CATTACKEXECUTOR_H

#include "IExecutor.h"

class CAttackExecutor : public IExecutor {
public:
    /** The derived constructors set m_state in their bodies (after the vtable store, as the original). */
    explicit CAttackExecutor(CUnitMind* mind);
    virtual ~CAttackExecutor();
    virtual void Tick(int elapsed);
    virtual void DebugRender(CAuxRender* render);
    virtual void OnAnimationComplete(EPersonAnimType type, const char* name);
    virtual void ChooseNextState() = 0;
    virtual bool IsOnCover() = 0;

    void OnStart(int camera);
    void OrientBodyToCamera();
    void GetWeapon();
    void ProcessState();
    void Reload();
    void Shot();

protected:
    enum State { Hide, ReloadWeapon, Idle, Unhide, Shoot };
    int m_camera;          // 0x0c
    int m_waitMs;
    bool m_waiting;
    int m_state;
    int m_turnElapsed;
};

class CAttackSafeExecutor : public CAttackExecutor {
public:
    explicit CAttackSafeExecutor(CUnitMind* mind);
    virtual bool IsOnCover();
    virtual void ChooseNextState();
    virtual const wchar_t* GetName() const;
};

class CAttackUnsafeExecutor : public CAttackExecutor {
public:
    explicit CAttackUnsafeExecutor(CUnitMind* mind);
    virtual bool IsOnCover();
    virtual void ChooseNextState();
    virtual const wchar_t* GetName() const;
};

void AddSafeAttackExecutor(CUnitMind* mind, int camera);
void AddUnsafeAttackExecutor(CUnitMind* mind, int camera);

#endif
