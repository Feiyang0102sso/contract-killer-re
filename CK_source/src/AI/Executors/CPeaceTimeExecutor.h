#ifndef CPEACETIMEEXECUTOR_H
#define CPEACETIMEEXECUTOR_H

#include "IExecutor.h"

class CPeaceTimeExecutor : public IExecutor {
public:
    explicit CPeaceTimeExecutor(CUnitMind* mind);
    virtual void Tick(int elapsed);
    virtual void OnReceiveDamage();
    virtual void OnSeeUnit(const CUnit* unit);
    virtual void OnChildFinished(EExecutosResult result);
    virtual const wchar_t* GetName() const;
    virtual void MoveToNewPoint();
    virtual void SetPeaceIdleBehaviour();

    void OnStart();
    void ChooseFutherAction();
    void PlayAlertSound();

protected:
    unsigned int m_alertTime;
    bool m_idleNext;
    bool m_alerted;
    bool m_tryAttackPoint;
    int m_ticks;
    static bool m_alertSoundPlayed;
};

void AddPeaceTimeExecutor(CUnitMind* mind);

#endif
