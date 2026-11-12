#ifndef CMOVEMENTEXECUTOR_H
#define CMOVEMENTEXECUTOR_H

#include "CPath.h"
#include "IExecutor.h"

class CMovementExecutor : public IExecutor {
public:
    CMovementExecutor(CUnitMind* mind, float speed, EPersonAnimType animation);
    virtual void Tick(int elapsed);
    virtual void DebugRender(CAuxRender* render);
    virtual void OnPathComplete();
    virtual void OnReceiveDamage();
    virtual void OnCancel();
    virtual const wchar_t* GetName() const;

    void StartAnimation();
    void OnStartFromPointToPoint(int from, int to);
    void OnStartToExitPoint(int from);
    void OnStartToAttackPoint(int from, bool safeOnly);

private:
    CPath m_path;                        // 0x0c
    EPersonAnimType m_animation;
    int m_field4030;
};

void AddMovementExecutor(CUnitMind* mind, int from, int to, bool run);
void AddMovementExecutorToExitPoint(CUnitMind* mind, int from);
void AddMovementExecutorToAttackPoint(CUnitMind* mind, int from, bool safeOnly);

#endif
