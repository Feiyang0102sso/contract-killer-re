// CPlayerGameStats.cpp - read, save and update the CK bounty statistics.
#include "CPlayerGameStats.h"

#include <stddef.h>
#include <string.h>
#include "App.h"
#include "CAmmo.h"
#include "CDH_Weapon.h"
#include "CLocationManager.h"
#include "CMiscItemManager.h"
#include "CStdUtil_Cocoa.h"
#include "CXmlHelper.h"

typedef char CPlayerGameStats_ARMv7_Size[(sizeof(CPlayerGameStats) == 0x114) ? 1 : -1];

namespace {
    struct StatAttribute {
        const char* name;
        int CPlayerGameStats::* value;
    };

    const StatAttribute kStatAttributes[] = {
        { "m_successBounties", &CPlayerGameStats::m_successBounties },
        { "m_totalBounties", &CPlayerGameStats::m_totalBounties },
        { "m_targetsKilled", &CPlayerGameStats::m_targetsKilled },
        { "m_targetsEscaped", &CPlayerGameStats::m_targetsEscaped },
        { "m_civiliansKilled", &CPlayerGameStats::m_civiliansKilled },
        { "m_shotsNumber", &CPlayerGameStats::m_shotsNumber },
        { "m_hitsNumber", &CPlayerGameStats::m_hitsNumber },
        { "m_headshotsNumber", &CPlayerGameStats::m_headshotsNumber },
        { "m_locationsUnlocked", &CPlayerGameStats::m_locationsUnlocked },
        { "m_moneyEarned", &CPlayerGameStats::m_moneyEarned },
        { "m_moneySpent", &CPlayerGameStats::m_moneySpent },
        { "m_weaponsEarned", &CPlayerGameStats::m_weaponsEarned },
        { "m_enemyKilled", &CPlayerGameStats::m_enemyKilled },
        { "m_enemyTranquillized", &CPlayerGameStats::m_enemyTranquillized },
        { "m_enemyEscaped", &CPlayerGameStats::m_enemyEscaped },
        { "m_woundedFailBounties", &CPlayerGameStats::m_woundedFailBounties },
        { "m_noAmmoFailBounties", &CPlayerGameStats::m_noAmmoFailBounties },
        { "m_bossEscapedFailBounties", &CPlayerGameStats::m_bossEscapedFailBounties },
        { "m_x3MultiplierKills", &CPlayerGameStats::m_x3MultiplierKills },
        { "m_x2MultiplierKills", &CPlayerGameStats::m_x2MultiplierKills },
        { "m_medkitsUsed", &CPlayerGameStats::m_medkitsUsed },
        { "m_reviveUsed", &CPlayerGameStats::m_reviveUsed },
        { "m_coversUsed", &CPlayerGameStats::m_coversUsed },
        { "m_energyBought", &CPlayerGameStats::m_energyBought },
        { "m_mapDrags", &CPlayerGameStats::m_mapDrags },
        { "m_informantsUnlocked", &CPlayerGameStats::m_informantsUnlocked },
        { "m_headshotsInRunning", &CPlayerGameStats::m_headshotsInRunning },
        { "m_bulletsEarned", &CPlayerGameStats::m_bulletsEarned },
        { "m_killsWithoutMisses", &CPlayerGameStats::m_killsWithoutMisses },
        { "m_killsWithoutMissesBest", &CPlayerGameStats::m_killsWithoutMissesBest },
        { "m_pistolHeadshots", &CPlayerGameStats::m_pistolHeadshots },
        { "m_pacifistMissions", &CPlayerGameStats::m_pacifistMissions },
        { "m_outOfEnergyTimes", &CPlayerGameStats::m_outOfEnergyTimes },
        { "m_pistolHitsOnePersonBest", &CPlayerGameStats::m_pistolHitsOnePersonBest },
        { "m_killsIn15SecBest", &CPlayerGameStats::m_killsIn15SecBest },
        { "m_killsIn25SecBest", &CPlayerGameStats::m_killsIn25SecBest },
        { "m_killsIn40SecBest", &CPlayerGameStats::m_killsIn40SecBest },
        { "m_killsWithOneShotBest", &CPlayerGameStats::m_killsWithOneShotBest },
        { "m_totalKillsCountByExplosion", &CPlayerGameStats::m_totalKillsCountByExplosion },
        { "m_maxKillsCountByOneExplosion", &CPlayerGameStats::m_maxKillsCountByOneExplosion },
        { "m_outOfRangeKillsCount", &CPlayerGameStats::m_outOfRangeKillsCount },
        { "m_escapingKillsCount", &CPlayerGameStats::m_escapingKillsCount },
        { "m_pillUsedTimeMS", &CPlayerGameStats::m_pillUsedTimeMS },
        { "m_pillsUsed", &CPlayerGameStats::m_pillsUsed },
        { "m_armorConsumeDamage", &CPlayerGameStats::m_armorConsumeDamage },
        { "m_onBoatHeadshotsCount", &CPlayerGameStats::m_onBoatHeadshotsCount },
        { "m_incendiaryAmmoKills", &CPlayerGameStats::m_incendiaryAmmoKills },
        { "m_explosiveAmmoKills", &CPlayerGameStats::m_explosiveAmmoKills },
        { "m_movingMissionsCompleted", &CPlayerGameStats::m_movingMissionsCompleted },
        { "m_coopsCompleted", &CPlayerGameStats::m_coopsCompleted },
        { "m_minigunKills", &CPlayerGameStats::m_minigunKills },
        { "m_purchasedCyberScopes", &CPlayerGameStats::m_purchasedCyberScopes },
        { "m_resurrects", &CPlayerGameStats::m_resurrects },
    };
}

CPlayerGameStats::CPlayerGameStats()
{
    memset(&m_successBounties, 0, offsetof(CPlayerGameStats, m_completedMissionsAtLocations));
    m_weaponsEarned = 1;
    m_coopsCompleted = 0;
    m_minigunKills = 0;
    m_purchasedCyberScopes = 0;
    m_resurrects = 0;
    m_inMIssionWeaponChanges = 0;
    m_killsInCurrentHunt = 0;
    m_pistolHitsOnTarget = 0;
    m_killTimes.ensureCapacity(20);
}

CPlayerGameStats::~CPlayerGameStats()
{
}

void CPlayerGameStats::Save(TiXmlElement* parent)
{
    TiXmlElement* element = CXmlHelper::ObtainElement(parent, XString("game_stats"));
    element->Clear();
    for (unsigned int index = 0; index < sizeof(kStatAttributes) / sizeof(kStatAttributes[0]); ++index) {
        const StatAttribute& attribute = kStatAttributes[index];
        element->SetAttribute(attribute.name, this->*attribute.value);
    }
    element->SetAttribute("m_allAmmoTypesOpen", m_allAmmoTypesOpen);

    XString locations;
    for (int index = 0; index < m_completedMissionsAtLocations.size(); ++index) {
        if (index != 0) {
            locations = locations + L",";
        }
        locations = locations + m_completedMissionsAtLocations[index];
    }
    XString::AnsiString encoded(locations.c_str());
    element->SetAttribute("m_completedMissionsAtLocations", encoded);
}

void CPlayerGameStats::Load(TiXmlElement* parent)
{
    const TiXmlElement* element = parent->FirstChildElement("game_stats");
    if (element == NULL) {
        return;
    }
    for (unsigned int index = 0; index < sizeof(kStatAttributes) / sizeof(kStatAttributes[0]); ++index) {
        const StatAttribute& attribute = kStatAttributes[index];
        const char* text = element->Attribute(attribute.name);
        if (text != NULL) {
            this->*attribute.value = XString(text).ToInt();
        }
    }
    m_allAmmoTypesOpen = CXmlHelper::GetAttributeValueOptional(element, "m_allAmmoTypesOpen").ToBool();
    const XString locations = CXmlHelper::GetAttributeValueOptional(element, "m_completedMissionsAtLocations");
    m_completedMissionsAtLocations.setSize(0);
    if (locations.Length() != 0) {
        locations.Split(m_completedMissionsAtLocations, L",");
    }
    if (m_pillUsedTimeMS == 0) {
        m_pillUsedTimeMS = m_pillsUsed * 10000;
    }
    m_trophies.removeAllElements();
}

void CPlayerGameStats::AddSuccessBounties(int count)
{
    m_successBounties += count;
}

void CPlayerGameStats::AddTotalBounties(int count)
{
    m_totalBounties += count;
}

void CPlayerGameStats::AddTargetsKilled(int count)
{
    m_targetsKilled += count;
}

void CPlayerGameStats::AddCiviliansKilled(int count)
{
    m_civiliansKilled += count;
}

void CPlayerGameStats::AddShotsNumber(int count)
{
    m_shotsNumber += count;
}

void CPlayerGameStats::AddMoneyEarned(int count)
{
    m_moneyEarned += count;
}

void CPlayerGameStats::AddMoneySpent(int count)
{
    m_moneySpent += count;
}

void CPlayerGameStats::AddWeaponsEarned(int count)
{
    m_weaponsEarned += count;
}

void CPlayerGameStats::AddEnemyKilled(int count)
{
    m_enemyKilled += count;
}

void CPlayerGameStats::AddEnemyTranquillized(int count)
{
    m_enemyTranquillized += count;
}

void CPlayerGameStats::AddEnemyEscaped(int count)
{
    m_enemyEscaped += count;
}

void CPlayerGameStats::AddX2MultiplierKills(int count)
{
    m_x2MultiplierKills += count;
}

void CPlayerGameStats::AddX3MultiplierKills(int count)
{
    m_x3MultiplierKills += count;
}

void CPlayerGameStats::AddMedkitsUsed(int count)
{
    m_medkitsUsed += count;
}

void CPlayerGameStats::AddCoversUsed(int count)
{
    m_coversUsed += count;
}

void CPlayerGameStats::AddInMIssionWeaponChanges(int count)
{
    m_inMIssionWeaponChanges += count;
}

void CPlayerGameStats::AddEnergyBought(int count)
{
    m_energyBought += count;
}

void CPlayerGameStats::AddMapDrags(int count)
{
    m_mapDrags += count;
}

void CPlayerGameStats::AddWoundedFailBounties(int count)
{
    m_woundedFailBounties += count;
}

void CPlayerGameStats::AddNoAmmoFailBounties(int count)
{
    m_noAmmoFailBounties += count;
}

void CPlayerGameStats::AddBossEscapedFailBounties(int count)
{
    m_bossEscapedFailBounties += count;
}

void CPlayerGameStats::AddInformantsUnlocked(int count)
{
    m_informantsUnlocked += count;
}

void CPlayerGameStats::AddHeadshotsInRunning(int count)
{
    m_headshotsInRunning += count;
}

void CPlayerGameStats::AddBulletsEarned(int count)
{
    m_bulletsEarned += count;
}

void CPlayerGameStats::AddOutOfEnergyTimes(int count)
{
    m_outOfEnergyTimes += count;
}

void CPlayerGameStats::SetAllAmmoTypesOpen(bool open)
{
    m_allAmmoTypesOpen = open;
}

void CPlayerGameStats::RegisterHuntStart()
{
    m_killsInCurrentHunt = 0;
    m_pistolHitsOnTarget = 0;
    m_killTimes.removeAllElements();
}

void CPlayerGameStats::RegisterEnemyKill()
{
    const unsigned int now = CStdUtil_Cocoa::GetUpTimeMS();
    m_killTimes.addElement(now);
    ++m_killsInCurrentHunt;
    ++m_killsWithoutMisses;
    if (m_killsWithoutMissesBest < m_killsWithoutMisses) {
        m_killsWithoutMissesBest = m_killsWithoutMisses;
    }
    UpdateKillLogsStats();
}

void CPlayerGameStats::UpdateKillLogsStats()
{
    const int in15 = GetKillsInIntervalSec(15);
    if (m_killsIn15SecBest < in15) {
        m_killsIn15SecBest = in15;
    }
    const int in25 = GetKillsInIntervalSec(25);
    if (m_killsIn25SecBest < in25) {
        m_killsIn25SecBest = in25;
    }
    const int in40 = GetKillsInIntervalSec(40);
    if (m_killsIn40SecBest < in40) {
        m_killsIn40SecBest = in40;
    }
}

void CPlayerGameStats::RegisterShootMissed()
{
    m_killsWithoutMisses = 0;
}

void CPlayerGameStats::RegisterWarTimerEnd()
{
    if (m_killsInCurrentHunt == 0) {
        ++m_pacifistMissions;
    }
}

void CPlayerGameStats::RegisterHeadshot(const CDH_Weapon* weapon)
{
    ++m_headshotsNumber;
    const CAmmo* ammo = weapon->GetActiveAmmo();
    if (ammo != NULL && ammo->m_ammoType == XString("pistol")) {
        ++m_pistolHeadshots;
    }
}

void CPlayerGameStats::RegisterHit(const CUnit*, const CDH_Weapon* weapon)
{
    ++m_hitsNumber;
    if (weapon == NULL) {
        return;
    }
    const CAmmo* ammo = weapon->GetActiveAmmo();
    if (ammo != NULL && ammo->m_ammoType == XString("pistol")) {
        ++m_pistolHitsOnTarget;
        if (m_pistolHitsOnePersonBest < m_pistolHitsOnTarget) {
            m_pistolHitsOnePersonBest = m_pistolHitsOnTarget;
        }
    }
}

void CPlayerGameStats::RegisterMultiKill(int count)
{
    if (m_killsWithOneShotBest < count) {
        m_killsWithOneShotBest = count;
    }
}

int CPlayerGameStats::GetKillsInIntervalSec(int seconds)
{
    const unsigned int limit = seconds * 1000;
    int best = 0;
    int first = 0;
    for (int last = 0; last < m_killTimes.size(); ++last) {
        while (first < last && m_killTimes[last] - m_killTimes[first] > limit) {
            ++first;
        }
        const int count = last - first + 1;
        if (best < count) {
            best = count;
        }
    }
    return best;
}

void CPlayerGameStats::RegisterKillsByExplosion(int count)
{
    m_totalKillsCountByExplosion += count;
    if (m_maxKillsCountByOneExplosion < count) {
        m_maxKillsCountByOneExplosion = count;
    }
}

void CPlayerGameStats::RegisterOutOfRangeKill()
{
    ++m_outOfRangeKillsCount;
}

void CPlayerGameStats::RegisterEscapingKill()
{
    ++m_escapingKillsCount;
}

void CPlayerGameStats::RegisterPillAddTime(int milliseconds)
{
    if (m_pillUsedTimeMS <= 2000000) {
        m_pillUsedTimeMS += milliseconds;
    }
}

void CPlayerGameStats::RegisterArmorConsumeDamage(int damage)
{
    m_armorConsumeDamage += damage;
}

void CPlayerGameStats::RegisterHeadshotsOnBoat(int)
{
    ++m_onBoatHeadshotsCount;
}

void CPlayerGameStats::RegisterCompletedMissionAt(XString location)
{
    if (m_completedMissionsAtLocations.indexOf(location) < 0) {
        m_completedMissionsAtLocations.addElement(location);
    }
}

bool CPlayerGameStats::CompletedMissionsAtEveryLocations() const
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CLocationManager* locations = app->m_locationManager;
    return locations != NULL && locations->m_locations.m_count > 0
        && m_completedMissionsAtLocations.size() >= locations->m_locations.m_count;
}

void CPlayerGameStats::RegisterMiscPurchase(int id)
{
    if (id == 4) {
        ++m_purchasedCyberScopes;
    }
}

bool CPlayerGameStats::HasPurchasedVisorAndCyberScope() const
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CItem* visor = app->InitMiscItemManager()->getItemById(2);
    return visor != NULL && visor->isAvailable() && m_purchasedCyberScopes > 0;
}
