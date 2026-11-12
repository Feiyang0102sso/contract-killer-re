#ifndef CTRICKSHOTMANAGER_H
#define CTRICKSHOTMANAGER_H

#include "xString.h"

class CTrickShotManager {
public:
    enum EEvent {
        RunningRed, BullsEye, NapTime, Revenge, LongShot, SeekingBullet, FastShot,
        DoubleKill, TripleKill, DoubleExplosionKill, TripleExplosionKill, EventCount
    };

    CTrickShotManager();
    void ResetEvents();
    void SetKillCount(int count);
    void SetKillByExplosionCount(int count);
    void RegisterEvent(EEvent event);
    int GetMultiplier();

private:
    XString GetEventName(EEvent event);
    void ShowMessage(const XString& name, int multiplier);

    bool m_events[EventCount];
    int m_killCount;
    int m_explosionKillCount;
};

#endif
