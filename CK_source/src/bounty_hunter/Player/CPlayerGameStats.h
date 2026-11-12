// CPlayerGameStats.h - CK bounty statistics and their saved attributes.
// The previous CKZ declarations and their notes are in recovery/legacy-game/CPlayerGameStats.h.
#ifndef CPLAYERGAMESTATS_H
#define CPLAYERGAMESTATS_H

#include "Vector_ss.h"
#include "xString.h"

class TiXmlElement;
class CDH_Weapon;
class CUnit;

class CPlayerGameStats {
public:
    CPlayerGameStats();
    ~CPlayerGameStats();
    void Save(TiXmlElement* parent);
    void Load(TiXmlElement* parent);

    void AddSuccessBounties(int count);
    void AddTotalBounties(int count);
    void AddTargetsKilled(int count);
    void AddCiviliansKilled(int count);
    void AddShotsNumber(int count);
    void AddMoneyEarned(int count);
    void AddMoneySpent(int count);
    void AddWeaponsEarned(int count);
    void AddEnemyKilled(int count);
    void AddEnemyTranquillized(int count);
    void AddEnemyEscaped(int count);
    void AddX2MultiplierKills(int count);
    void AddX3MultiplierKills(int count);
    void AddMedkitsUsed(int count);
    void AddCoversUsed(int count);
    void AddInMIssionWeaponChanges(int count);
    void AddEnergyBought(int count);
    void AddMapDrags(int count);
    void AddWoundedFailBounties(int count);
    void AddNoAmmoFailBounties(int count);
    void AddBossEscapedFailBounties(int count);
    void AddInformantsUnlocked(int count);
    void AddHeadshotsInRunning(int count);
    void AddBulletsEarned(int count);
    void AddOutOfEnergyTimes(int count);
    void SetAllAmmoTypesOpen(bool open);
    void RegisterHuntStart();
    void RegisterEnemyKill();
    void UpdateKillLogsStats();
    void RegisterShootMissed();
    void RegisterHeadshot(const CDH_Weapon* weapon);
    void RegisterWarTimerEnd();
    void RegisterHit(const CUnit* unit, const CDH_Weapon* weapon);
    void RegisterMultiKill(int count);
    int GetKillsInIntervalSec(int seconds);
    void RegisterKillsByExplosion(int count);
    void RegisterOutOfRangeKill();
    void RegisterEscapingKill();
    void RegisterPillAddTime(int milliseconds);
    void RegisterArmorConsumeDamage(int damage);
    void RegisterHeadshotsOnBoat(int count);
    void RegisterCompletedMissionAt(XString location);
    bool CompletedMissionsAtEveryLocations() const;
    void RegisterMiscPurchase(int id);
    bool HasPurchasedVisorAndCyberScope() const;

    struct STrophy {
        XString m_name;
        int m_field04;
        int m_field08;
        int m_field0c;
        XString m_value;
    };

    int m_successBounties; // 0x00
    int m_totalBounties; // 0x04
    int m_targetsKilled; // 0x08
    int m_targetsEscaped; // 0x0c
    int m_civiliansKilled; // 0x10
    int m_shotsNumber; // 0x14
    int m_hitsNumber; // 0x18
    int m_headshotsNumber; // 0x1c
    int m_locationsUnlocked; // 0x20
    int m_moneyEarned; // 0x24
    int m_moneySpent; // 0x28
    int m_weaponsEarned; // 0x2c
    int m_enemyKilled; // 0x30
    int m_enemyTranquillized; // 0x34
    int m_enemyEscaped; // 0x38
    int m_woundedFailBounties; // 0x3c
    int m_noAmmoFailBounties; // 0x40
    int m_bossEscapedFailBounties; // 0x44
    int m_x3MultiplierKills; // 0x48
    int m_x2MultiplierKills; // 0x4c
    int m_medkitsUsed; // 0x50
    int m_reviveUsed; // 0x54
    int m_coversUsed; // 0x58
    int m_energyBought; // 0x5c
    int m_mapDrags; // 0x60
    int m_informantsUnlocked; // 0x64
    int m_headshotsInRunning; // 0x68
    int m_bulletsEarned; // 0x6c
    int m_killsWithoutMisses; // 0x70
    int m_killsWithoutMissesBest; // 0x74
    int m_pistolHeadshots; // 0x78
    int m_pacifistMissions; // 0x7c
    int m_outOfEnergyTimes; // 0x80
    int m_pistolHitsOnePersonBest; // 0x84
    int m_killsIn15SecBest; // 0x88
    int m_killsIn25SecBest; // 0x8c
    int m_killsIn40SecBest; // 0x90
    int m_killsWithOneShotBest; // 0x94
    bool m_allAmmoTypesOpen; // 0x98
    int m_totalKillsCountByExplosion; // 0x9c
    int m_maxKillsCountByOneExplosion; // 0xa0
    int m_outOfRangeKillsCount; // 0xa4
    int m_escapingKillsCount; // 0xa8
    int m_pillUsedTimeMS; // 0xac
    int m_pillsUsed; // 0xb0
    int m_armorConsumeDamage; // 0xb4
    int m_onBoatHeadshotsCount; // 0xb8
    int m_incendiaryAmmoKills; // 0xbc
    int m_explosiveAmmoKills; // 0xc0
    int m_movingMissionsCompleted; // 0xc4
    Vector<XString> m_completedMissionsAtLocations; // 0xc8
    int m_coopsCompleted; // 0xd8
    int m_minigunKills; // 0xdc
    int m_purchasedCyberScopes; // 0xe0
    int m_resurrects; // 0xe4
    int m_inMIssionWeaponChanges; // 0xe8
    int m_killsInCurrentHunt; // 0xec
    int m_pistolHitsOnTarget; // 0xf0
    Vector<unsigned int> m_killTimes; // 0xf4
    Vector<STrophy> m_trophies; // 0x104
};

#endif
