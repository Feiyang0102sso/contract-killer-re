#ifndef CPEACETIMESQUADEXECUTOR_H
#define CPEACETIMESQUADEXECUTOR_H

#include "CPeaceTimeExecutor.h"

class CPeaceTimeSquadExecutor : public CPeaceTimeExecutor {
public:
    explicit CPeaceTimeSquadExecutor(CUnitMind* mind);
    virtual void Tick(int elapsed);
    virtual void OnAnimationComplete(EPersonAnimType type, const char* animation);
    virtual const wchar_t* GetName() const
    {
        return L"CPeaceTimeSquadExecutor";
    }
    // Squad members follow the squad point instead of picking their own.
    virtual void MoveToNewPoint()
    {
    }
    virtual void SetPeaceIdleBehaviour()
    {
    }
    void OnStart();

private:
    void MoveToSquadPoint();
    void PlayIdleBehaviour();
    void PlayMoveBehaviour();

    CUnitMind* m_squadMind;
    bool m_waiting;
};

void AddPeaceTimeSquadExecutor(CUnitMind* mind);

#endif
