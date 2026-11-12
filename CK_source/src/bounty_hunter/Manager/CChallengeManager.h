#ifndef CCHALLENGEMANAGER_H
#define CCHALLENGEMANAGER_H

#include "Vector_ss.h"

class CChallenge;

class CChallengeManager {
public:
    CChallengeManager();
    ~CChallengeManager();
    void Update();
    Vector<CChallenge*> GetActiveChallenges();
    CChallenge* GetChallengeById(int id);
    bool IsChallengeActive(int id);
    int GetCurrentChallengeDay();
    int GetChallengesFinishTime();
    static int GetTimeSec();
    void Save();
    friend class CChallengesWindow;  // original window calls CheckChallengesFinishByTime

private:
    void ParseConfig();
    void Load();
    void CheckChallengesFinishByTime();
    void StartNewChallenges(int time);
    void GetChallengesForDay(int day, Vector<CChallenge*>& challenges);
    int DayForTime(int time) const;

    unsigned int m_lastUpdateSecond;
    int m_activeCount;
    int m_reserved;
    int m_finishTime;
    int m_lastDay;
    Vector<CChallenge*> m_challenges;
};

#endif
