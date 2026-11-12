// CProgressSystem.h - CK level thresholds and per-level game parameters.
#ifndef CPROGRESSSYSTEM_H
#define CPROGRESSSYSTEM_H

#include "Vector_ss.h"

class TiXmlNode;
class CUnit;

class CProgressSystem {
public:
    enum EGameEvent {
        EVENT_ENEMY_KILLED = 0,
        EVENT_ENEMY_TRANQUILLIZED = 1,
        EVENT_ENEMY_ESCAPED = 2
    };

    struct SLevelInfo {
        int m_xpThreshold;
        int m_maxEnergy;
        int m_civilianCount;
        float m_damageCoefficient;
    };

    CProgressSystem();
    void ParseLevels(TiXmlNode* root);
    int GetMaxLevel() const;
    int GetMaxXPForLevel(int level) const;
    int GetMaxXP() const;
    int GetMaxEnergyForLevel(int level) const;
    int GetLevel(int experience) const;
    int GetCivilianCount(int level) const;
    float GetRandValue() const;
    void SetMissionDifficulty(float difficulty) { m_field10 = difficulty; }
    float GetLevelCoef(bool allowAlternate) const;
    float GetDmgCoeff() const;
    float GetLevelForkFormulaCoef(bool allowAlternate) const;
    static int CalcXpForKilledUnit(CUnit* unit);
    int GetTaggingTimeMS();
    void RegisterGameEvent(EGameEvent event, int experience, int headshot);
    void AddXPForUnit(int experience, bool headshot);

private:
    Vector<SLevelInfo> m_levels;
    float m_field10;
};

#endif
