#ifndef CCHALLENGESTATS_H
#define CCHALLENGESTATS_H

#include "CHuntingInfo.h"
#include "HashTable.h"

class CDH_Weapon;
class CUnit;
class TiXmlElement;

class CChallengeStats {
public:
    CChallengeStats();
    void ResetStats();
    void RegisterMissionCompletionTime(unsigned int seconds);
    void RegisterKill(const CUnit* unit, const CDH_Weapon* weapon, bool headshot, bool beforeEnemyCover, int distance);
    void AddHeadshotsNumber(const CDH_Weapon* weapon, int count);
    void RegisterHuntStart(XString location);
    void RegisterHuntEnd(EBountyResult result);
    void RegisterPlayerTakeCover();
    void RegisterPlayerEnterScope();
    void RegisterShot(CDH_Weapon* weapon);
    void AddMissionsWithoutMedkitsUsed(int count);
    void AddMissionsWithSameWeaponCategory(int weaponId, int count);
    void AddMissionsWithSameWeapon(int weaponId, int count);
    int GetHeadshotsInRowWithWeaponCategory(int category);
    int GetKillsByWeaponId(int weaponId);
    int GetKillsByWeaponType(int category);
    int GetHeadshotsByWeaponType(int category);
    int GetHeadshotsOnLocation(const XString& location);
    int GetMissionsWithSameWeaponCategory(int category);
    int GetMissionsWithSameWeapon(int weaponId);
    int GetTargetsKilled() { return m_targetsKilled; }
    int GetHeadshotsNumber() { return m_headshots; }
    int GetTargetsKilledWithoutScope() { return m_killsWithoutScope; }
    int GetOneShots() { return m_oneShots; }
    int GetMissionsWithoutMedkitsUsed() { return m_missionsWithoutMedkits; }
    int GetFastestMissionCompletionTime() { return m_fastestMissionSeconds; }
    int GetCompletedBounties() { return m_completedBounties; }
    int GetHeadshotsInRow() { return m_headshotsInRow; }
    int GetKillsBeforeTakeCoverInMission() { return m_killsBeforePlayerCover; }
    int GetBossKillsWithHeadshotsWOScope() { return m_bossHeadshotsWithoutScope; }
    int GetOneShotsInRow() { return m_oneShotsInRow; }
    int GetMissionsWithoutInjure() { return m_missionsWithoutInjury; }
    int GetMissionsWithHeadshotsOnly() { return m_missionsWithHeadshotsOnly; }
    int GetMissionsWithoutEnemyGettingCover() { return m_missionsWithoutEnemyCover; }
    int GetMissionsCompleteWithoutPlayerTakeCover() { return m_missionsWithKillsWithoutCover; }
    int GetNonScopeBossHeadshotsWhileEscaping() { return m_escapingBossHeadshotsWithoutScope; }
    int GetAmbushWithoutMiss() { return m_ambushWithoutMisses; }
    int GetKillsOutOfRange() { return m_killsOutOfRange; }
    int GetMissionsCompletedFor30Seconds() { return m_missionsWithin30Seconds; }
    int GetSilentHeadshots() { return m_silentHeadshots; }
    int GetMovingCameraMissionWins() { return m_movingCameraWins; }
    int GetMissionsWithExplosiveAmmoOnly() { return m_missionsWithExplosiveAmmo; }
    int GetMissionsWithIncendiaryAmmoOnly() { return m_missionsWithIncendiaryAmmo; }
    int GetWeaponGatedMissionsWins() { return m_weaponGatedWins; }
    int GetBossKillingByExplosiveObject() { return m_bossKillsByExplosiveObject; }
    int GetMissionsWithoutAlert() { return m_missionsWithoutAlert; }
    int GetRevives() { return m_revives; }
    int GetMissionsCompletedInRow() { return m_missionsCompletedInRow; }
    int GetMultiKills() { return m_multiKills; }
    int GetMissionsWithAimAssist() { return m_missionsWithAimAssist; }
    void AddMissionsWihoutEnemyGettingCover(int count) { m_missionsWithoutEnemyCover += count; }
    void ResetKillsBeforeTakeCoverInMission() { m_killsBeforePlayerCover = 0; }
    void Save(TiXmlElement* parent);
    void Load(TiXmlElement* parent);

    HashTable<HashTableKey<int>, int> m_killsByWeapon;
    HashTable<HashTableKey<int>, int> m_killsByCategory;
    HashTable<HashTableKey<int>, int> m_headshotsByWeapon;
    HashTable<HashTableKey<int>, int> m_headshotsByCategory;
    HashTable<HashTableKey<int>, int> m_missionsByWeapon;
    HashTable<HashTableKey<int>, int> m_missionsByCategory;
    HashTable<XString, int> m_headshotsByLocation;
    int m_targetsKilled;                     // 0x54
    int m_huntKills;
    int m_headshots;
    int m_killsWithoutScope;
    int m_missionsWithoutMedkits;             // 0x64
    int m_completedBounties;
    int m_headshotsInRow;
    int m_missionsWithoutCover;
    int m_bossHeadshotsWithoutScope;
    int m_oneShots;
    int m_oneShotsInRow;
    int m_missionsWithoutEnemyCover;          // 0x80
    int m_missionsWithoutInjury;
    int m_huntHeadshots;
    int m_missionsWithHeadshotsOnly;
    int m_missionsWithKillsWithoutCover;
    int m_killsBeforePlayerCover;
    int m_ambushWithoutMisses;
    int m_missionsCompletedInRow;
    int m_escapingBossHeadshotsWithoutScope;  // 0xa0
    int m_missionsWithin30Seconds;
    int m_killsOutOfRange;
    int m_silentHeadshots;
    int m_missionsWithExplosiveAmmo;
    int m_missionsWithIncendiaryAmmo;
    int m_movingCameraWins;
    int m_bossKillsByExplosiveObject;
    int m_weaponGatedWins;                    // 0xc0
    int m_missionsWithoutAlert;
    int m_revives;
    int m_multiKills;
    int m_missionsWithAimAssist;
    int m_lastHeadshotCategory;
    int m_headshotsInRowWithCategory;
    bool m_noPlayerCover;                    // 0xdc
    bool m_onlyHeadshots;
    bool m_onlyExplosiveAmmo;
    bool m_onlyIncendiaryAmmo;
    bool m_onlyAimAssist;                    // 0xe0
    bool m_huntActive;
    bool m_medkitFreeRecorded;
    int m_missionsWithoutScope;              // 0xe4
    bool m_noScope;                          // 0xe8 set again by RegisterPlayerEnterScope in the original
    bool m_singleWeapon;
    bool m_singleCategory;
    int m_lastWeapon;
    int m_lastCategory;
    int m_fastestMissionSeconds;
    XString m_location;
    int m_escapingUnitId;                    // 0xfc
};

#endif
