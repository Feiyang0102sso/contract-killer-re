#include "CChallengeStats.h"

#include "App.h"
#include "CAmmo.h"
#include "CDH_Weapon.h"
#include "CDH_WeaponsManager.h"
#include "CSwerveGame.h"
#include "CUnit.h"
#include "CXmlHelper.h"
#include "DGHelper.h"

CChallengeStats::CChallengeStats()
    : m_killsByWeapon(8), m_killsByCategory(8), m_headshotsByWeapon(8), m_headshotsByCategory(8),
      m_missionsByWeapon(8), m_missionsByCategory(8), m_headshotsByLocation(8)
{
    ResetStats();
}

void CChallengeStats::ResetStats()
{
    m_missionsWithoutScope = 0;
    m_headshotsInRowWithCategory = 0;
    m_targetsKilled = 0;
    m_huntKills = 0;
    m_headshots = 0;
    m_killsWithoutScope = 0;
    m_missionsWithoutMedkits = 0;
    m_completedBounties = 0;
    m_headshotsInRow = 0;
    m_missionsWithoutCover = 0;
    m_bossHeadshotsWithoutScope = 0;
    m_oneShots = 0;
    m_oneShotsInRow = 0;
    m_missionsWithoutEnemyCover = 0;
    m_missionsWithoutInjury = 0;
    m_huntHeadshots = 0;
    m_missionsWithHeadshotsOnly = 0;
    m_missionsWithKillsWithoutCover = 0;
    m_killsBeforePlayerCover = 0;
    m_ambushWithoutMisses = 0;
    m_missionsCompletedInRow = 0;
    m_escapingBossHeadshotsWithoutScope = 0;
    m_missionsWithin30Seconds = 0;
    m_killsOutOfRange = 0;
    m_silentHeadshots = 0;
    m_missionsWithExplosiveAmmo = 0;
    m_missionsWithIncendiaryAmmo = 0;
    m_movingCameraWins = 0;
    m_bossKillsByExplosiveObject = 0;
    m_weaponGatedWins = 0;
    m_missionsWithoutAlert = 0;
    m_revives = 0;
    m_multiKills = 0;
    m_missionsWithAimAssist = 0;
    m_lastHeadshotCategory = -1;
    m_onlyHeadshots = false;
    m_huntActive = false;
    m_medkitFreeRecorded = false;
    m_noScope = false;
    m_onlyExplosiveAmmo = true;
    m_onlyIncendiaryAmmo = true;
    m_onlyAimAssist = true;
    m_fastestMissionSeconds = 0x0d286880;
    m_escapingUnitId = -1;
    m_killsByWeapon.Clear();
    m_killsByCategory.Clear();
    m_headshotsByWeapon.Clear();
    m_headshotsByCategory.Clear();
    m_missionsByWeapon.Clear();
    m_missionsByCategory.Clear();
    m_headshotsByLocation.Clear();
}

void CChallengeStats::RegisterMissionCompletionTime(unsigned int seconds)
{
    if (seconds < static_cast<unsigned int>(m_fastestMissionSeconds)) {
        m_fastestMissionSeconds = seconds;
    }
    if (seconds < 30) {
        ++m_missionsWithin30Seconds;
    }
}

void CChallengeStats::RegisterKill(const CUnit* unit, const CDH_Weapon* weapon, bool headshot,
                                  bool beforeEnemyCover, int)
{
    if (unit->m_type == UnitCivilian) {
        m_headshotsInRow = 0;
        m_headshotsInRowWithCategory = 0;
        return;
    }
    if (unit->m_runningAway) {
        m_escapingUnitId = unit->m_guid;
    }
    ++m_targetsKilled;
    ++m_huntKills;
    if (headshot == true) {
        if (weapon != NULL) {
            App* app = static_cast<App*>(WindowApp::m_instance);
            if (!app->m_helper->m_swerveGame->m_missionAlert) {
                ++m_silentHeadshots;
            }
            AddHeadshotsNumber(weapon, 1);
            if (unit->m_guid == m_escapingUnitId && !weapon->m_fetched && unit->m_type == UnitBoss) {
                ++m_escapingBossHeadshotsWithoutScope;
            }
        }
        ++m_headshotsInRow;
        int* locationCount = m_headshotsByLocation.Get(m_location);
        if (locationCount != NULL) {
            m_headshotsByLocation.Set(m_location, *locationCount + 1);
        } else {
            m_headshotsByLocation.Add(m_location, 1);
        }
        ++m_huntHeadshots;
    } else {
        m_onlyHeadshots = false;
        m_headshotsInRow = 0;
    }
    if (weapon != NULL && !weapon->m_fetched) {
        ++m_killsWithoutScope;
        if (unit->m_type == UnitBoss && headshot) {
            ++m_bossHeadshotsWithoutScope;
        }
    }
    if (beforeEnemyCover == true) {
        ++m_oneShots;
        ++m_oneShotsInRow;
    } else {
        m_oneShotsInRow = 0;
    }
    if (weapon != NULL) {
        int category = weapon->m_category;
        if (m_lastCategory >= 0 && m_lastCategory != category) {
            m_singleCategory = false;
        }
        m_lastCategory = category;
        int weaponId = weapon->GetId();
        if (m_lastWeapon >= 0 && m_lastWeapon != weaponId) {
            m_singleWeapon = false;
        }
        m_lastWeapon = weaponId;
        int* weaponKills = m_killsByWeapon.Get(HashTableKey<int>(weaponId));
        if (weaponKills != NULL) {
            int newCount = *weaponKills;
            m_killsByWeapon.Set(HashTableKey<int>(weaponId), ++newCount);
        } else {
            m_killsByWeapon.Add(HashTableKey<int>(weaponId), 1);
        }
        int* categoryKills = m_killsByCategory.Get(HashTableKey<int>(category));
        if (categoryKills != NULL) {
            int newCount = *categoryKills;
            m_killsByCategory.Set(HashTableKey<int>(category), ++newCount);
        } else {
            m_killsByCategory.Add(HashTableKey<int>(category), 1);
        }
    } else {
        // No weapon: mark both trackers with an id no real weapon uses.
        m_lastWeapon = 1000;
        m_lastCategory = 1000;
    }
}

void CChallengeStats::AddHeadshotsNumber(const CDH_Weapon* weapon, int count)
{
    if (weapon == NULL) {
        return;
    }
    m_headshots += count;
    int weaponId = weapon->GetId();
    int* weaponHeadshots = m_headshotsByWeapon.Get(HashTableKey<int>(weaponId));
    if (weaponHeadshots != NULL) {
        int newCount = *weaponHeadshots;
        m_headshotsByWeapon.Set(HashTableKey<int>(weaponId), ++newCount);
    } else {
        m_headshotsByWeapon.Add(HashTableKey<int>(weaponId), 1);
    }
    int category = weapon->m_category;
    int* categoryHeadshots = m_headshotsByCategory.Get(HashTableKey<int>(category));
    if (categoryHeadshots != NULL) {
        int newCount = *categoryHeadshots;
        m_headshotsByCategory.Set(HashTableKey<int>(category), ++newCount);
    } else {
        m_headshotsByCategory.Add(HashTableKey<int>(category), 1);
    }
    if (m_lastHeadshotCategory == category) {
        ++m_headshotsInRowWithCategory;
    } else {
        m_headshotsInRowWithCategory = 0;
    }
    m_lastHeadshotCategory = category;
}

int CChallengeStats::GetHeadshotsInRowWithWeaponCategory(int)
{
    if (m_lastHeadshotCategory != 0) {
        return m_headshotsInRowWithCategory;
    }
    return 0;
}

int CChallengeStats::GetKillsByWeaponId(int weaponId)
{
    int zero = 0;
    int* kills = m_killsByWeapon.Get(HashTableKey<int>(weaponId));
    // ?: keeps the original pointer select (movne) before the single load.
    return *(kills != NULL ? kills : &zero);
}

void CChallengeStats::RegisterHuntStart(XString location)
{
    m_noScope = true;
    m_noPlayerCover = true;
    m_onlyHeadshots = true;
    m_onlyExplosiveAmmo = true;
    m_onlyIncendiaryAmmo = true;
    m_onlyAimAssist = true;
    m_huntActive = true;
    m_huntHeadshots = 0;
    m_huntKills = 0;
    m_lastWeapon = -1;
    m_lastCategory = -1;
    m_singleWeapon = true;
    m_singleCategory = true;
    m_location = location;
    m_medkitFreeRecorded = false;
    m_escapingUnitId = -1;
}

void CChallengeStats::RegisterHuntEnd(EBountyResult result)
{
    if (result == BountyResultWin || result == BountyResultCapture) {
        if (m_singleCategory) {
            AddMissionsWithSameWeaponCategory(m_lastWeapon, 1);
        }
        if (m_singleWeapon) {
            if (m_lastWeapon == 34) {
                ++m_missionsWithAimAssist;
            }
            AddMissionsWithSameWeapon(m_lastWeapon, 1);
        }
        if (m_huntKills == m_huntHeadshots) {
            ++m_missionsWithHeadshotsOnly;
        }
        if (m_noPlayerCover) {
            ++m_missionsWithKillsWithoutCover;
        }
        if (m_noScope) {
            ++m_missionsWithoutScope;
        }
        if (m_onlyExplosiveAmmo) {
            ++m_missionsWithExplosiveAmmo;
        }
        if (m_onlyIncendiaryAmmo) {
            ++m_missionsWithIncendiaryAmmo;
        }
        if (m_onlyAimAssist) {
            ++m_missionsWithAimAssist;
        }
        ++m_missionsCompletedInRow;
    } else {
        m_missionsCompletedInRow = 0;
    }
    m_huntActive = false;
}

void CChallengeStats::AddMissionsWithSameWeaponCategory(int weaponId, int count)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_Weapon* weapon = app->InitWeaponManager()->GetWeaponById(weaponId);
    if (weapon != NULL) {
        int category = weapon->m_category;
        int* missions = m_missionsByCategory.Get(HashTableKey<int>(category));
        if (missions != NULL) {
            int current = *missions;
            m_missionsByCategory.Set(HashTableKey<int>(category), current + count);
        } else {
            m_missionsByCategory.Add(HashTableKey<int>(category), count);
        }
    }
}

void CChallengeStats::AddMissionsWithSameWeapon(int weaponId, int)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (app->InitWeaponManager()->GetWeaponById(weaponId) != NULL) {
        int missionCount;  // declared before `zero`: the original keeps it higher on the stack
        int zero = 0;
        int* missions = m_missionsByWeapon.Get(HashTableKey<int>(weaponId));
        missionCount = *(missions != NULL ? missions : &zero) + 1;
        // The original always calls Add (never Set) and ignores the count argument.
        m_missionsByWeapon.Add(HashTableKey<int>(weaponId), missionCount);
    }
}

void CChallengeStats::RegisterPlayerTakeCover()
{
    m_noPlayerCover = false;
}

void CChallengeStats::AddMissionsWithoutMedkitsUsed(int count)
{
    m_medkitFreeRecorded = true;
    m_missionsWithoutMedkits += count;
}

void CChallengeStats::RegisterPlayerEnterScope()
{
    // The original stores true here as well (RegisterHuntEnd still counts it as a no-scope mission).
    m_noScope = true;
}

int CChallengeStats::GetKillsByWeaponType(int category)
{
    int* kills = m_killsByCategory.Get(HashTableKey<int>(category));
    if (kills != NULL) {
        return *kills;
    }
    return 0;
}

int CChallengeStats::GetHeadshotsByWeaponType(int category)
{
    int* headshots = m_headshotsByCategory.Get(HashTableKey<int>(category));
    if (headshots != NULL) {
        return *headshots;
    }
    return 0;
}

int CChallengeStats::GetHeadshotsOnLocation(const XString&)
{
    // The original ignores the argument and looks up the current location converted to a number.
    int* headshots = m_headshotsByLocation.Get(XString(m_location.ToInt()));
    if (headshots != NULL) {
        return *headshots;
    }
    return 0;
}

void CChallengeStats::Save(TiXmlElement* parent)
{
    TiXmlElement* section = CXmlHelper::ObtainElement(parent, XString("challenge_stat"));
    section->Clear();
    section->SetAttribute("targetsKilled", m_targetsKilled);
    section->SetAttribute("headShotsCount", m_headshots);
    section->SetAttribute("targetKilledWithoutScope", m_killsWithoutScope);
    section->SetAttribute("missionsWithoutMedkitsUsed", m_missionsWithoutMedkits);
    section->SetAttribute("completedBounties", m_completedBounties);
    section->SetAttribute("headShotsInRow", m_headshotsInRow);
    section->SetAttribute("missionsWithoutTakeCoverInMission", m_missionsWithoutCover);
    section->SetAttribute("bossKillsWithHeadshotsWOScope", m_bossHeadshotsWithoutScope);
    section->SetAttribute("oneShots", m_oneShots);
    section->SetAttribute("oneShotsInRow", m_oneShotsInRow);
    section->SetAttribute("missionsWithoutGettingCover", m_missionsWithoutEnemyCover);
    section->SetAttribute("missionsWithoutInjure", m_missionsWithoutInjury);
    section->SetAttribute("missionsWithHeadShotsOnly", m_missionsWithHeadshotsOnly);
    section->SetAttribute("missionsWithKillsWithoutPlayerTakeCover", m_missionsWithKillsWithoutCover);
    section->SetAttribute("killsBeforeTakeCoverInMission", m_killsBeforePlayerCover);
    section->SetAttribute("missionsWithoutUsingScope", m_missionsWithoutScope);
    section->SetAttribute("ambushWithoutMisses", m_ambushWithoutMisses);
    section->SetAttribute("missionsCompletedInRow", m_missionsCompletedInRow);
    section->SetAttribute("noScopeBossHeadshotsWhileEscaping", m_escapingBossHeadshotsWithoutScope);
    section->SetAttribute("missionsCompletedFor30Seconds", m_missionsWithin30Seconds);
    section->SetAttribute("killsOutOfRange", m_killsOutOfRange);
    section->SetAttribute("headshotsInRowWithSameWeaponCategory", m_headshotsInRowWithCategory);
    section->SetAttribute("lastHeadshotWeaponCategory", m_lastHeadshotCategory);
    section->SetAttribute("silentHeadshotsCount", m_silentHeadshots);
    section->SetAttribute("missionsWithExplosiveAmmo", m_missionsWithExplosiveAmmo);
    section->SetAttribute("missionsWithIncendiaryAmmo", m_missionsWithIncendiaryAmmo);
    section->SetAttribute("movingCameraMissionsWins", m_movingCameraWins);
    section->SetAttribute("bossKillsByExplosiveObject", m_bossKillsByExplosiveObject);
    section->SetAttribute("weaponGatedMissionWins", m_weaponGatedWins);
    section->SetAttribute("missionsWithoutAlert", m_missionsWithoutAlert);
    section->SetAttribute("revivesCount", m_revives);
    section->SetAttribute("multiKillsCount", m_multiKills);
    section->SetAttribute("missionsWithAimAssist", m_missionsWithAimAssist);
    section->SetAttribute("fastestMissionCompletionTimeSec", m_fastestMissionSeconds);
    CXmlHelper::WriteHashtable(m_killsByWeapon, section, XString("killsByWeapon"));
    CXmlHelper::WriteHashtable(m_killsByCategory, section, XString("killsByWeaponCategory"));
    CXmlHelper::WriteHashtable(m_headshotsByWeapon, section, XString("headshotsByWeapon"));
    CXmlHelper::WriteHashtable(m_headshotsByCategory, section, XString("headshotsByWeaponCategory"));
    CXmlHelper::WriteHashtable(m_headshotsByLocation, section, XString("headshotsByLocation"));
    CXmlHelper::WriteHashtable(m_missionsByCategory, section, XString("missionsCompletedWithWeapon"));
    CXmlHelper::WriteHashtable(m_missionsByWeapon, section, XString("missionsCompletedWithWeaponById"));
}

void CChallengeStats::Load(TiXmlElement* parent)
{
    TiXmlElement* section = parent->FirstChildElement("challenge_stat");
    if (section == NULL) {
        return;
    }
    m_targetsKilled = CXmlHelper::GetAttributeValue(section, "targetsKilled").ToInt();
    m_headshots = CXmlHelper::GetAttributeValue(section, "headShotsCount").ToInt();
    m_killsWithoutScope = CXmlHelper::GetAttributeValue(section, "targetKilledWithoutScope").ToInt();
    m_missionsWithoutMedkits = CXmlHelper::GetAttributeValue(section, "missionsWithoutMedkitsUsed").ToInt();
    m_completedBounties = CXmlHelper::GetAttributeValue(section, "completedBounties").ToInt();
    m_headshotsInRow = CXmlHelper::GetAttributeValue(section, "headShotsInRow").ToInt();
    m_missionsWithoutCover = CXmlHelper::GetAttributeValue(section, "missionsWithoutTakeCoverInMission").ToInt();
    m_bossHeadshotsWithoutScope = CXmlHelper::GetAttributeValue(section, "bossKillsWithHeadshotsWOScope").ToInt();
    m_oneShots = CXmlHelper::GetAttributeValue(section, "oneShots").ToInt();
    m_oneShotsInRow = CXmlHelper::GetAttributeValue(section, "oneShotsInRow").ToInt();
    m_missionsWithoutEnemyCover = CXmlHelper::GetAttributeValue(section, "missionsWithoutGettingCover").ToInt();
    m_missionsWithoutInjury = CXmlHelper::GetAttributeValue(section, "missionsWithoutInjure").ToInt();
    m_missionsWithHeadshotsOnly = CXmlHelper::GetAttributeValue(section, "missionsWithHeadShotsOnly").ToInt();
    m_missionsWithKillsWithoutCover =
        CXmlHelper::GetAttributeValue(section, "missionsWithKillsWithoutPlayerTakeCover").ToInt();
    m_killsBeforePlayerCover = CXmlHelper::GetAttributeValue(section, "killsBeforeTakeCoverInMission").ToInt();
    m_missionsWithoutScope = CXmlHelper::GetAttributeValue(section, "missionsWithoutUsingScope").ToInt();
    m_ambushWithoutMisses = CXmlHelper::GetAttributeValue(section, "ambushWithoutMisses").ToInt();
    m_missionsCompletedInRow = CXmlHelper::GetAttributeValue(section, "missionsCompletedInRow").ToInt();
    // Counters added later in development fall back to a default when an old save lacks them.
    m_missionsWithin30Seconds =
        CXmlHelper::GetAttributeValueOrUseDefault(section, "missionsCompletedFor30Seconds", XString("0")).ToInt();
    m_headshotsInRowWithCategory =
        CXmlHelper::GetAttributeValueOrUseDefault(section, "headshotsInRowWithSameWeaponCategory", XString("0")).ToInt();
    m_lastHeadshotCategory =
        CXmlHelper::GetAttributeValueOrUseDefault(section, "lastHeadshotWeaponCategory", XString("-1")).ToInt();
    m_silentHeadshots =
        CXmlHelper::GetAttributeValueOrUseDefault(section, "silentHeadshotsCount", XString("0")).ToInt();
    m_missionsWithExplosiveAmmo =
        CXmlHelper::GetAttributeValueOrUseDefault(section, "missionsWithExplosiveAmmo", XString("0")).ToInt();
    m_missionsWithIncendiaryAmmo =
        CXmlHelper::GetAttributeValueOrUseDefault(section, "missionsWithIncendiaryAmmo", XString("0")).ToInt();
    m_movingCameraWins =
        CXmlHelper::GetAttributeValueOrUseDefault(section, "movingCameraMissionsWins", XString("0")).ToInt();
    m_bossKillsByExplosiveObject =
        CXmlHelper::GetAttributeValueOrUseDefault(section, "bossKillsByExplosiveObject", XString("0")).ToInt();
    m_weaponGatedWins =
        CXmlHelper::GetAttributeValueOrUseDefault(section, "weaponGatedMissionWins", XString("0")).ToInt();
    m_escapingBossHeadshotsWithoutScope =
        CXmlHelper::GetAttributeValueOrUseDefault(section, "noScopeBossHeadshotsWhileEscaping", XString("0")).ToInt();
    m_killsOutOfRange =
        CXmlHelper::GetAttributeValueOrUseDefault(section, "killsOutOfRange", XString("0")).ToInt();
    m_missionsWithoutAlert =
        CXmlHelper::GetAttributeValueOrUseDefault(section, "missionsWithoutAlert", XString("0")).ToInt();
    m_revives = CXmlHelper::GetAttributeValueOrUseDefault(section, "revivesCount", XString("0")).ToInt();
    m_multiKills = CXmlHelper::GetAttributeValueOrUseDefault(section, "multiKillsCount", XString("0")).ToInt();
    m_missionsWithAimAssist =
        CXmlHelper::GetAttributeValueOrUseDefault(section, "missionsWithAimAssist", XString("0")).ToInt();
    m_fastestMissionSeconds = CXmlHelper::GetAttributeValue(section, "fastestMissionCompletionTimeSec").ToInt();
    CXmlHelper::ReadHashtable(m_killsByWeapon, section, XString("killsByWeapon"));
    CXmlHelper::ReadHashtable(m_killsByCategory, section, XString("killsByWeaponCategory"));
    CXmlHelper::ReadHashtable(m_headshotsByWeapon, section, XString("headshotsByWeapon"));
    CXmlHelper::ReadHashtable(m_headshotsByCategory, section, XString("headshotsByWeaponCategory"));
    CXmlHelper::ReadHashtable(m_headshotsByLocation, section, XString("headshotsByLocation"));
    CXmlHelper::ReadHashtable(m_missionsByCategory, section, XString("missionsCompletedWithWeapon"));
    CXmlHelper::ReadHashtable(m_missionsByWeapon, section, XString("missionsCompletedWithWeaponById"));
}

int CChallengeStats::GetMissionsWithSameWeaponCategory(int category)
{
    int* missions = m_missionsByCategory.Get(HashTableKey<int>(category));
    if (missions != NULL) {
        return *missions;
    }
    return 0;
}

void CChallengeStats::RegisterShot(CDH_Weapon* weapon)
{
    if (!(XString(weapon->GetActiveAmmo()->m_ammoType) == XString("explosive"))) {
        m_onlyExplosiveAmmo = false;
    }
    if (!(XString(weapon->GetActiveAmmo()->m_ammoType) == XString("incendiary"))) {
        m_onlyIncendiaryAmmo = false;
    }
    if (!weapon->m_autoAim && !weapon->m_cyberScopeEquipped) {
        m_onlyAimAssist = false;
    }
}

int CChallengeStats::GetMissionsWithSameWeapon(int weaponId)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (app->InitWeaponManager()->GetWeaponById(weaponId) != NULL) {
        int zero = 0;
        int* missions = m_missionsByWeapon.Get(HashTableKey<int>(weaponId));
        return *(missions != NULL ? missions : &zero);
    }
    return 0;
}

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char CKChallengeStatsSize[sizeof(CChallengeStats) == 0x100 ? 1 : -1];
#endif
