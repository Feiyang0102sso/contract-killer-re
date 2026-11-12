// CBH_Player.h - CK ARMv7 player profile and inventory.
// The prior CKZ layout and its comments are preserved in recovery/legacy-game/CBH_Player.h.
#ifndef CBH_PLAYER_H
#define CBH_PLAYER_H

#include "CPlayerGameStats.h"
#include "CStrWChar.h"
#include "CSingleton.h"
#include "HashTable.h"
#include "CDH_Weapon.h"

class CAmmo;
class CAvatar;
class CChallengeStats;
class CDH_Weapon;
class COverrideManager;
class CUnit;

enum EGainSource {
    GAIN_SOURCE_MISSION = 0,
    GAIN_SOURCE_ENEMY = 1,
    GAIN_SOURCE_CHALLENGE = 2,
    GAIN_SOURCE_IAP = 3
};

class CBH_Player : public CSingleton {
public:
    enum { ClassId = 0xda6a6154 };

    CBH_Player();
    virtual ~CBH_Player();
    static CBH_Player* GetInstance();

    void Init();
    void Load();
    void Save();
    void Save(bool force);
    void Update();
    void ReleaseResources();

    int GetLevel() const { return m_level; }
    int GetTutorialState() const { return m_tutorialState; }
    int GetDailyBonusesInRow() const { return m_dailyBonusesInRow; }
    void SetTutorialState(int state);
    int& FieldAt(int offset) { return *reinterpret_cast<int*>(reinterpret_cast<char*>(this) + offset); }

    void AddCredits(int amount);
    void AddCredits(int amount, const wchar_t*) { AddCredits(amount); }
    void ReduceCredits(int amount);
    void AddMoney(int amount) { AddMoney(amount, GAIN_SOURCE_MISSION); }
    void AddMoney(int amount, EGainSource source);
    void ReduceMoney(int amount);
    int GetOverride(COverrideManager& manager, const XString& group, const XString& name);
    void AddEnergy(int amount, bool allowOverflow);
    void ReduceEnergy(int amount);
    bool IsEnegyEnabled();
    int GetEnergyCooldownTimeLeft();
    int GetMaxEnergy();
    int GetMaxEnergyForLevel(int level);
    int GetMaxHp();
    int GetMaxHpForLevel(int level);
    int GetXpForNextLvl();
    int GetXpForCurrentLvl();
    bool IsPlayerLevelMax();
    int AddXp(int amount, EGainSource source);
    void AddXp(int amount) { AddXp(amount, GAIN_SOURCE_MISSION); }

    void SetPlayerStartParams();
    void InitFactionAttitude();
    void OnEnemyKilled();
    void OnEnemyTranquillized();
    void OnEnemyEscaped();
    void OnMissionFinished(int id);
    bool IsMissionCompleted(int id);
    void MissionCompleted();
    bool TryMakeFiveDaysBonus(bool* newDay);
    bool TryMakeFiveDaysIncentiveBonus();
    bool CanShowFiveDaysIncentiveBonusExpirationMessage();
    void RegisterFiveDaysIncentiveBonusExpirationMessageShown();
    int GetReputation(const XString& name);
    int GetReputationLimit();
    void SetAvatar(const CAvatar* avatar);
    int GetFriendGiftLastDay(int socialId);
    void SetFriendGiftLastDay(int socialId, int day);
    void CancelFriendInvitation(int socialId);
    int GetCurrentFriendGatedMissionID() const;
    void SetNextFriendGatedMissionID();

    void SetAmmoForWeaponId(int id, int packs);
    void IncAmmo(const CAmmo* ammo, int packs);
    void IncAmmoByBulletsCount(const CAmmo* ammo, int count);
    void SetAmmoCount(const CAmmo* ammo, int count);
    int GetAvailableAmmo(const CAmmo* ammo, bool estimate);
    void UseAmmo(const CAmmo* ammo);
    void SetEstimatedWeapon(CDH_Weapon* weapon, int count);
    bool HasAnyAmmo();
    bool HasAmmoInBothWeapons();
    bool HasTranqWeapon();
    bool HasTranqWeaponWithin();
    bool HasAmmoForTranqWeaponWithin();
    bool HasWeaponWithRange(float range);
    bool HasWeaponWithRangeInHands(float range);
    int WeaponsWithCategoryCount(CDH_Weapon::EWeaponCategory category);

    void ResetBeforeHunting();
    void StartCover();
    void EndCover();
    bool IsNeedHide() const;
    bool IsInHide() const;
    bool IsInProgress() const;
    void TryShotPlayer(CUnit* unit);
    bool IsArmorAvailable() const;
    void SetArmor(int id, int damageProof);
    void AddMedkits(int amount);
    bool CanRestoreHealth();
    void RestoreHealth();
    void AddHp(int amount);
    int GetPillsCount() const;
    void AddGoPills(int amount);
    bool IsSpeedModeAvailable() const;
    int GetRemainingPillCount() const;
    bool IsSpeedMode() const;
    int GetSpeedTimeMS() const;
    void ConsumeGoPill();
    void AddRevive(int amount);
    bool CanRevive();
    void Revive();
    void GamePlayTick(int elapsed);
    void StopConsumeReactixControl();
    bool IsGameVersionNew();
    bool IsMissionVersionNew();
    void ChangeGameVersion();
    void ChangeMissionVersion();
    void ReduceSilentShotsCount(int count);
    void AddReactixControlTimeSec(int seconds);
    int GetReactixControlTimeSec();
    void StartConsumeReactixControl();
    void ConsumeUltrasoundEmitter();
    void AddUltrasoundEmitterAttempt(int count);
    void AddCyberScopeCount(int count);
    void ReduceCyberScopeCount(int count);
    void AddSilentShotsCount(int count);

    void SaveStats(TiXmlElement* element);
    void SaveReputation(TiXmlElement* element);
    void SaveMissionInfo(TiXmlElement* element);
    void SaveAmmo(TiXmlElement* element);
    void SaveTjPoints(TiXmlElement* element);
    void SaveFriendGiftLasDay(TiXmlElement* element);
    void LoadStats(TiXmlElement* element);
    void LoadMissionInfo(TiXmlElement* element);
    void LoadAmmo(TiXmlElement* element);
    void LoadAvatar(TiXmlElement* element);
    void LoadTjPoints(TiXmlElement* element);
    void LoadFriendGiftLasDay(TiXmlElement* element);

    struct SCompletedMissionInfo {
        int m_id;
        bool m_tranquillized;
    };

    Vector<int> m_friendInvitations;         // 0x08
    int m_energy;                            // 0x18
    int m_cash;                              // 0x1c
    int m_credits;                           // 0x20
    int m_experience;                        // 0x24
    int m_level;                             // 0x28
    int m_hp;                                // 0x2c
    int m_medkits;                           // 0x30
    int m_reviveCount;                       // 0x34
    bool m_energyEnabled;                    // 0x38
    bool m_field39;
    int m_field3c;
    int m_tutorialState;                     // 0x40
    unsigned int m_lastEnergyUpdateTime;     // 0x44
    int m_lastHuntItem;                      // 0x48
    int m_lastHuntResult;                    // 0x4c
    unsigned int m_lastDailyBonusTime;       // 0x50
    int m_dailyBonusesInRow;                 // 0x54
    unsigned int m_lastFiveDaysBonusTime;    // 0x58
    int m_activeFiveDaysBonusDay;             // 0x5c
    int m_lastFiveDaysBonusTeaserDay;
    int m_lastFiveDaysIncentiveBonusTimestamp; // 0x64
    int m_activeFiveDaysIncentiveBonusDay;   // 0x68
    bool m_fiveDayIncentiveBonusForbidden;   // 0x6c
    bool m_receivedFiveDaysIncentiveBonus;
    bool m_incentiveBonusExpirationShown;
    bool m_appRated;                         // 0x6f
    int m_silentShotsCount;                  // 0x70
    int m_cyberScopesCount;                  // 0x74
    int m_lastGameVersionPlayed;            // 0x78
    int m_lastMissionVersionPlayed;         // 0x7c
    bool m_newPlayer;                        // 0x80
    bool m_newMissionVersion;
    int m_completedMissionCount;             // 0x84
    HashTable<XString, int> m_tapjoyPoints;   // 0x88
    HashTable<XString, int> m_friendGiftDays; // 0x94
    int m_currentFriendGatedMissionId;        // 0xa0
    int m_fielda4;
    const CAvatar* m_avatar;                 // 0xa8
    bool m_authenticationStarted;           // 0xac
    bool m_playerNameReady;
    XString m_playerName;                    // 0xb0
    int m_fieldb4;
    unsigned int m_energyRestoreSeconds;    // 0xb8
    unsigned int m_energyRemainingSeconds;
    bool m_fieldc0;
    bool m_hitechVisorEquipped;
    Vector<SCompletedMissionInfo> m_completedMissions; // 0xc4
    Vector<int> m_fieldd4;
    HashTable<XString, int> m_reputation;     // 0xe4
    CPlayerGameStats m_gameStats;            // 0xf0
    CChallengeStats* m_challengeStats;       // 0x204
    int m_field208;
    const char* m_missionBriefImage;
    XString m_missionBriefTitle;
    XString m_missionBriefText;
    int m_missionBriefEnergy;
    int m_field21c;
    int m_field220;
    int m_targetFriendId;                    // 0x224
    CStrWChar m_missionBriefTargetFB;
    int m_field238;
    int m_field23c;
    int m_field240;
    int m_speedTimeMS;                       // 0x244
    int m_speedModeAvailableTimeMS;          // 0x248
    bool m_speedModeWasActive;
    bool m_ultrasoundWasActive;
    bool m_consumingReactixControl;
    int m_reactixControlTime;
    int m_ultrasoundAttemptCount;
    int m_armorId;
    int m_armorDamageProof;
    int m_armorDamageProofMax;
    HashTable<HashTableKey<const CAmmo*>, int> m_ammoCounts; // 0x264
    Vector<unsigned int> m_field270;
    int m_field280;
    CDH_Weapon* m_estimatedWeapon;
    int m_estimatedAmmo;
};

#endif
