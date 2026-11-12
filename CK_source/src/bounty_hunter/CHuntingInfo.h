// CHuntingInfo.h - CK ARMv7 mission assignment and post-mission counters.
#ifndef CHUNTINGINFO_H
#define CHUNTINGINFO_H

#include "xString.h"

class CBH_PersonType;
class CScriptType;
class CSquadType;

enum EBountyResult {
    BountyResultPending = 0,
    BountyResultLose = 1,
    BountyResultKilledInsteadOfTranquillized = 2,
    BountyResultBossTranquillizedInsteadOfKilled = 3,
    BountyResultHenchmanTranquillizedInsteadOfKilled = 4,
    BountyResultBossEscaped = 5,
    BountyResultCivilianKilled = 6,
    BountyResultNoAmmo = 7,
    BountyResultCameraTimeOut = 8,
    BountyResultRuinedSilence = 9,
    BountyResultWin = 10,
    BountyResultCapture = 11,
    BountyResultNoDelayedResult = 12
};

class CHuntingInfo {
public:
    struct SAssignedData {
        SAssignedData();

        XString m_bossName;             // 0x00
        int m_reputationChanges[5];     // 0x04
        int m_field18[2];              // 0x18
        CBH_PersonType* m_bossType;     // 0x20
        CSquadType* m_squadType;
        CScriptType* m_scriptType;
        int m_civilianCount;
        int m_experienceReward;        // 0x30
        int m_moneyReward;             // 0x34
        int m_experiencePenalty;       // 0x38
        int m_moneyPenalty;
        XString m_locationName;        // 0x40
        const char* m_ambientSound;    // 0x44
        int m_missionId;               // 0x48
        int m_field4c;
        int m_cameraIndex;
        float m_cameraHorizontalLimit;
        float m_cameraVerticalLimit;
        int m_introTimeMs;
        int m_cameraDurationMs;        // 0x60
        XString m_cameraHUDModel;      // 0x64
        int m_additionalMinionCount;
        XString m_field6c;
        bool m_field70;
        bool m_hasTarget;
        bool m_revenge;
        bool m_friendGated;            // 0x73
        bool m_weaponGated;            // 0x74
        int m_weaponCategory;          // 0x78
        int m_requiredWeaponId;
        int m_randomWeaponId;
        int m_field84;                 // 0x84
        int m_alertSeconds;
        int m_escapeSeconds;
        int m_tripleMultiplierSeconds; // 0x90
        int m_doubleMultiplierSeconds; // 0x94
        bool m_ambush;                 // 0x98
        bool m_tranquillize;
        bool m_multiplierCountdown;
        bool m_field9b;
        bool m_field9c;
        bool m_bossFirst;
        bool m_bossWins;
        bool m_bossSpawnsLast;
        bool m_silentKill;             // 0xa0
        bool m_fielda1;
        bool m_hasFakeBoss;
        XString m_missionName;         // 0xa4 analytics identifier
    };

    CHuntingInfo();
    void SetHuntingData(SAssignedData data);
    void CleanHuntingStatistic();
    void SetBountyResult(EBountyResult result);
    bool IsPlayerWin() const;
    bool IsAmbush() const;
    bool SilentKill() const;
    void ReduceRewardMoney(int amount);
    void OnReceivedXP(int amount);
    void OnEnemyKilled();
    void OnEnemyTranqullized();
    void OnEnemyEscaped();
    void OnBossTranquillized();
    void AfterSceneActivity();
    bool UpdateLeaderboards();

    SAssignedData m_assignedData;  // 0x00
    EBountyResult m_result;        // 0xa8
    bool m_fieldac;
    bool m_bossTranquillized;      // 0xad
    int m_experienceEarned;        // 0xb0
    int m_enemiesKilled;          // 0xb4
    int m_enemiesTranquillized;    // 0xb8
    int m_enemiesEscaped;          // 0xbc
    bool m_fieldc0;
    int m_primaryHits;            // 0xc4
    int m_secondaryHits;          // 0xc8
    int m_primaryKills;           // 0xcc
    int m_secondaryKills;         // 0xd0
    int m_headshots;              // 0xd4
};

#endif
