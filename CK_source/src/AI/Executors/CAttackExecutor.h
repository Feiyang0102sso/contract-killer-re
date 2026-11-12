#ifndef CATTACKEXECUTOR_H
#define CATTACKEXECUTOR_H

#include "IExecutor.h"

class CAttackExecutor : public IExecutor {
public:
    CAttackExecutor(CUnitMind* mind, int state);
    virtual ~CAttackExecutor();
    virtual void Tick(int elapsed);
    virtual void DebugRender(CAuxRender* render);
    virtual void OnAnimationComplete(EPersonAnimType type, const char* name);
    virtual bool IsOnCover() = 0;
    virtual void ChooseNextState() = 0;

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
