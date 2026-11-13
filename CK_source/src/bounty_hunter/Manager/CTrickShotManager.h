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

    /** Multiplier of one event: 1 unless it was registered (original inline switch). */
    int GetEventMultiplier(unsigned int event)
    {
        if (!m_events[event] || event > 10) {
            return 1;
        }
        switch (event) {
        case RunningRed:
        case NapTime:
        case Revenge:
        case SeekingBullet:
        case DoubleKill:
        case DoubleExplosionKill:
            return 2;
        case BullsEye:
        case LongShot:
        case FastShot:
        case TripleKill:
        case TripleExplosionKill:
            return 3;
        default:
            return 1;
        }
    }

    bool m_events[EventCount];
    int m_killCount;
    int m_explosionKillCount;
};

#endif
