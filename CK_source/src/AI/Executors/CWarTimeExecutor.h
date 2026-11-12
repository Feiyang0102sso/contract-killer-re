#ifndef CWARTIMEEXECUTOR_H
#define CWARTIMEEXECUTOR_H

#include "IExecutor.h"

class CWarTimeExecutor : public IExecutor {
public:
    explicit CWarTimeExecutor(CUnitMind* mind);
    virtual void Tick(int elapsed);
    virtual void OnChildFinished(EExecutosResult result);
    virtual const wchar_t* GetName() const;
    void MoveToAttackPoint();
    void StartAttack();
    void DefinePointCharacteristics(int camera);

private:
    int m_camera;
    bool m_safe;
    bool m_retryBoss;
};

void AddWarTimeExecutor(CUnitMind* mind);

#endif
