// CPlayerGameStats.cpp - read, save and update the CK bounty statistics.
#undef memset
#include <stddef.h>
#include <string.h>

#include "CPlayerGameStats.h"
#include "App.h"
#include "CAmmo.h"
#include "CDH_Weapon.h"
#include "CLocationManager.h"
#include "CMiscItemManager.h"
#include "CStdUtil_Cocoa.h"
#include "CXmlHelper.h"

typedef char CPlayerGameStats_ARMv7_Size[(sizeof(CPlayerGameStats) == 0x114) ? 1 : -1];

CPlayerGameStats::CPlayerGameStats()
{
    memset(&m_successBounties, 0, offsetof(CPlayerGameStats, m_weaponsEarned));
    m_weaponsEarned = 1;
    m_inMIssionWeaponChanges = 0;
    m_killsInCurrentHunt = 0;
    m_pistolHitsOnTarget = 0;

    // The first range ends with the bool and leaves its trailing padding untouched.
    const size_t firstCountBytes = offsetof(CPlayerGameStats, m_allAmmoTypesOpen) +
                                   sizeof(m_allAmmoTypesOpen) - offsetof(CPlayerGameStats, m_enemyKilled);
    const size_t secondCountBytes = offsetof(CPlayerGameStats, m_completedMissionsAtLocations) -
                                    offsetof(CPlayerGameStats, m_totalKillsCountByExplosion);
    memset(&m_enemyKilled, 0, firstCountBytes);
    memset(&m_totalKillsCountByExplosion, 0, secondCountBytes);
    m_completedMissionsAtLocations = Vector<XString>();

    m_coopsCompleted = 0;
    m_minigunKills = 0;
    m_purchasedCyberScopes = 0;
    m_resurrects = 0;
    m_killTimes.ensureCapacity(20);
}

CPlayerGameStats::~CPlayerGameStats()
{
}

void CPlayerGameStats::Save(TiXmlElement* parent)
{
    TiXmlElement* element = CXmlHelper::ObtainElement(parent, XString("game_stats"));
    element->Clear();
    element->SetAttribute("m_successBounties", m_successBounties);
    element->SetAttribute("m_totalBounties", m_totalBounties);
    element->SetAttribute("m_targetsKilled", m_targetsKilled);
    element->SetAttribute("m_targetsEscaped", m_targetsEscaped);
    element->SetAttribute("m_civiliansKilled", m_civiliansKilled);
    element->SetAttribute("m_shotsNumber", m_shotsNumber);
    element->SetAttribute("m_hitsNumber", m_hitsNumber);
    element->SetAttribute("m_headshotsNumber", m_headshotsNumber);
    element->SetAttribute("m_locationsUnlocked", m_locationsUnlocked);
    element->SetAttribute("m_moneyEarned", m_moneyEarned);
    element->SetAttribute("m_moneySpent", m_moneySpent);
    element->SetAttribute("m_weaponsEarned", m_weaponsEarned);
    element->SetAttribute("m_enemyKilled", m_enemyKilled);
    element->SetAttribute("m_enemyTranquillized", m_enemyTranquillized);
    element->SetAttribute("m_enemyEscaped", m_enemyEscaped);
    element->SetAttribute("m_x2MultiplierKills", m_x2MultiplierKills);
    element->SetAttribute("m_x3MultiplierKills", m_x3MultiplierKills);
    element->SetAttribute("m_medkitsUsed", m_medkitsUsed);
    element->SetAttribute("m_reviveUsed", m_reviveUsed);
    element->SetAttribute("m_coversUsed", m_coversUsed);
    element->SetAttribute("m_energyBought", m_energyBought);
    element->SetAttribute("m_mapDrags", m_mapDrags);
    element->SetAttribute("m_woundedFailBounties", m_woundedFailBounties);
    element->SetAttribute("m_noAmmoFailBounties", m_noAmmoFailBounties);
    element->SetAttribute("m_bossEscapedFailBounties", m_bossEscapedFailBounties);
    element->SetAttribute("m_informantsUnlocked", m_informantsUnlocked);
    element->SetAttribute("m_headshotsInRunning", m_headshotsInRunning);
    element->SetAttribute("m_bulletsEarned", m_bulletsEarned);
    element->SetAttribute("m_killsWithoutMisses", m_killsWithoutMisses);
    element->SetAttribute("m_killsWithoutMissesBest", m_killsWithoutMissesBest);
    element->SetAttribute("m_pistolHeadshots", m_pistolHeadshots);
    element->SetAttribute("m_pacifistMissions", m_pacifistMissions);
    element->SetAttribute("m_outOfEnergyTimes", m_outOfEnergyTimes);
    element->SetAttribute("m_pistolHitsOnePersonBest", m_pistolHitsOnePersonBest);
    element->SetAttribute("m_killsIn15SecBest", m_killsIn15SecBest);
    element->SetAttribute("m_killsIn25SecBest", m_killsIn25SecBest);
    element->SetAttribute("m_killsIn40SecBest", m_killsIn40SecBest);
    element->SetAttribute("m_killsWithOneShotBest", m_killsWithOneShotBest);
    element->SetAttribute("m_allAmmoTypesOpen", m_allAmmoTypesOpen);
    element->SetAttribute("m_totalKillsCountByExplosion", m_totalKillsCountByExplosion);
    element->SetAttribute("m_maxKillsCountByOneExplosion", m_maxKillsCountByOneExplosion);
    element->SetAttribute("m_outOfRangeKillsCount", m_outOfRangeKillsCount);
    element->SetAttribute("m_escapingKillsCount", m_escapingKillsCount);
    element->SetAttribute("m_pillsUsed", m_pillsUsed);
    element->SetAttribute("m_armorConsumeDamage", m_armorConsumeDamage);
    element->SetAttribute("m_onBoatHeadshotsCount", m_onBoatHeadshotsCount);
    element->SetAttribute("m_incendiaryAmmoKills", m_incendiaryAmmoKills);
    element->SetAttribute("m_explosiveAmmoKills", m_explosiveAmmoKills);
    element->SetAttribute("m_movingMissionsCompleted", m_movingMissionsCompleted);
    element->SetAttribute("m_completedMissionsAtLocations",
                          XString::AnsiString(MergeVector(m_completedMissionsAtLocations, XString(",")).c_str()));
    element->SetAttribute("m_coopsCompleted", m_coopsCompleted);
    element->SetAttribute("m_minigunKills", m_minigunKills);
    element->SetAttribute("m_purchasedCyberScopes", m_purchasedCyberScopes);
    element->SetAttribute("m_resurrects", m_resurrects);
    element->SetAttribute("m_pillUsedTimeMS", m_pillUsedTimeMS);
}

void CPlayerGameStats::Load(TiXmlElement* parent)
{
    TiXmlElement* element = parent->FirstChildElement("game_stats");
    if (element == NULL) {
        return;
    }
    // The original reads every attribute one by one: the first 15 are required, the rest optional.
    m_successBounties = CXmlHelper::GetAttributeValue(element, "m_successBounties").ToInt();
    m_totalBounties = CXmlHelper::GetAttributeValue(element, "m_totalBounties").ToInt();
    m_targetsKilled = CXmlHelper::GetAttributeValue(element, "m_targetsKilled").ToInt();
    m_targetsEscaped = CXmlHelper::GetAttributeValue(element, "m_targetsEscaped").ToInt();
    m_civiliansKilled = CXmlHelper::GetAttributeValue(element, "m_civiliansKilled").ToInt();
    m_shotsNumber = CXmlHelper::GetAttributeValue(element, "m_shotsNumber").ToInt();
    m_hitsNumber = CXmlHelper::GetAttributeValue(element, "m_hitsNumber").ToInt();
    m_headshotsNumber = CXmlHelper::GetAttributeValue(element, "m_headshotsNumber").ToInt();
    m_locationsUnlocked = CXmlHelper::GetAttributeValue(element, "m_locationsUnlocked").ToInt();
    m_moneyEarned = CXmlHelper::GetAttributeValue(element, "m_moneyEarned").ToInt();
    m_moneySpent = CXmlHelper::GetAttributeValue(element, "m_moneySpent").ToInt();
    m_weaponsEarned = CXmlHelper::GetAttributeValue(element, "m_weaponsEarned").ToInt();
    m_enemyKilled = CXmlHelper::GetAttributeValue(element, "m_enemyKilled").ToInt();
    m_enemyTranquillized = CXmlHelper::GetAttributeValue(element, "m_enemyTranquillized").ToInt();
    m_enemyEscaped = CXmlHelper::GetAttributeValue(element, "m_enemyEscaped").ToInt();
    m_x2MultiplierKills = CXmlHelper::GetAttributeValueOptional(element, "m_x2MultiplierKills").ToInt();
    m_x3MultiplierKills = CXmlHelper::GetAttributeValueOptional(element, "m_x3MultiplierKills").ToInt();
    m_medkitsUsed = CXmlHelper::GetAttributeValueOptional(element, "m_medkitsUsed").ToInt();
    m_coversUsed = CXmlHelper::GetAttributeValueOptional(element, "m_coversUsed").ToInt();
    m_energyBought = CXmlHelper::GetAttributeValueOptional(element, "m_energyBought").ToInt();
    m_mapDrags = CXmlHelper::GetAttributeValueOptional(element, "m_mapDrags").ToInt();
    m_woundedFailBounties = CXmlHelper::GetAttributeValueOptional(element, "m_woundedFailBounties").ToInt();
    m_noAmmoFailBounties = CXmlHelper::GetAttributeValueOptional(element, "m_noAmmoFailBounties").ToInt();
    m_bossEscapedFailBounties = CXmlHelper::GetAttributeValueOptional(element, "m_bossEscapedFailBounties").ToInt();
    m_informantsUnlocked = CXmlHelper::GetAttributeValueOptional(element, "m_informantsUnlocked").ToInt();
    m_headshotsInRunning = CXmlHelper::GetAttributeValueOptional(element, "m_headshotsInRunning").ToInt();
    m_bulletsEarned = CXmlHelper::GetAttributeValueOptional(element, "m_bulletsEarned").ToInt();
    m_killsWithoutMisses = CXmlHelper::GetAttributeValueOptional(element, "m_killsWithoutMisses").ToInt();
    m_killsWithoutMissesBest = CXmlHelper::GetAttributeValueOptional(element, "m_killsWithoutMissesBest").ToInt();
    m_pistolHeadshots = CXmlHelper::GetAttributeValueOptional(element, "m_pistolHeadshots").ToInt();
    m_pacifistMissions = CXmlHelper::GetAttributeValueOptional(element, "m_pacifistMissions").ToInt();
    m_outOfEnergyTimes = CXmlHelper::GetAttributeValueOptional(element, "m_outOfEnergyTimes").ToInt();
    m_pistolHitsOnePersonBest = CXmlHelper::GetAttributeValueOptional(element, "m_pistolHitsOnePersonBest").ToInt();
    m_killsIn15SecBest = CXmlHelper::GetAttributeValueOptional(element, "m_killsIn15SecBest").ToInt();
    m_killsIn25SecBest = CXmlHelper::GetAttributeValueOptional(element, "m_killsIn25SecBest").ToInt();
    m_killsIn40SecBest = CXmlHelper::GetAttributeValueOptional(element, "m_killsIn40SecBest").ToInt();
    m_killsWithOneShotBest = CXmlHelper::GetAttributeValueOptional(element, "m_killsWithOneShotBest").ToInt();
    m_allAmmoTypesOpen = CXmlHelper::GetAttributeValueOptional(element, "m_allAmmoTypesOpen").ToBool();
    m_totalKillsCountByExplosion = CXmlHelper::GetAttributeValueOptional(element, "m_totalKillsCountByExplosion").ToInt();
    m_maxKillsCountByOneExplosion = CXmlHelper::GetAttributeValueOptional(element, "m_maxKillsCountByOneExplosion").ToInt();
    m_outOfRangeKillsCount = CXmlHelper::GetAttributeValueOptional(element, "m_outOfRangeKillsCount").ToInt();
    m_escapingKillsCount = CXmlHelper::GetAttributeValueOptional(element, "m_escapingKillsCount").ToInt();
    m_pillsUsed = CXmlHelper::GetAttributeValueOptional(element, "m_pillsUsed").ToInt();
    m_armorConsumeDamage = CXmlHelper::GetAttributeValueOptional(element, "m_armorConsumeDamage").ToInt();
    m_onBoatHeadshotsCount = CXmlHelper::GetAttributeValueOptional(element, "m_onBoatHeadshotsCount").ToInt();
    m_reviveUsed = CXmlHelper::GetAttributeValueOptional(element, "m_reviveUsed").ToInt();
    m_incendiaryAmmoKills = CXmlHelper::GetAttributeValueOptional(element, "m_incendiaryAmmoKills").ToInt();
    m_explosiveAmmoKills = CXmlHelper::GetAttributeValueOptional(element, "m_explosiveAmmoKills").ToInt();
    m_movingMissionsCompleted = CXmlHelper::GetAttributeValueOptional(element, "m_movingMissionsCompleted").ToInt();
    m_completedMissionsAtLocations = CXmlHelper::GetAttributeValueOptional(element, "m_completedMissionsAtLocations").Split(L",");
    if (m_completedMissionsAtLocations.size() == 1 && m_completedMissionsAtLocations[0] == XString("")) {
        m_completedMissionsAtLocations.setSize(0);
    }
    m_coopsCompleted = CXmlHelper::GetAttributeValueOptional(element, "m_coopsCompleted").ToInt();
    m_minigunKills = CXmlHelper::GetAttributeValueOptional(element, "m_minigunKills").ToInt();
    m_purchasedCyberScopes = CXmlHelper::GetAttributeValueOptional(element, "m_purchasedCyberScopes").ToInt();
    m_resurrects = CXmlHelper::GetAttributeValueOptional(element, "m_resurrects").ToInt();
    m_pillUsedTimeMS = CXmlHelper::GetAttributeValueOptional(element, "m_pillUsedTimeMS").ToInt();
    if (m_pillUsedTimeMS == 0) {
        m_pillUsedTimeMS = m_pillsUsed * 10000;
    }
    // Original: old saves drop their "trophies" node and the in-memory list.
    TiXmlElement* trophies = element->FirstChildElement("trophies");
    if (trophies != NULL) {
        trophies->Clear();
        m_trophies.removeAllElements();
    }
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
    m_killTimes.removeAllElements();
    m_killsInCurrentHunt = 0;
    m_pistolHitsOnTarget = 0;
}

void CPlayerGameStats::RegisterEnemyKill()
{
    const unsigned int now = CStdUtil_Cocoa::GetUpTimeMS();
    m_killTimes.addElement(now);
    ++m_killsInCurrentHunt;
    ++m_killsWithoutMisses;
    if (m_killsWithoutMisses > m_killsWithoutMissesBest) {
        m_killsWithoutMissesBest = m_killsWithoutMisses;
    }
    UpdateKillLogsStats();
}

void CPlayerGameStats::UpdateKillLogsStats()
{
    const int in15 = GetKillsInIntervalSec(15);
    if (in15 > m_killsIn15SecBest) {
        m_killsIn15SecBest = in15;
    }
    const int in25 = GetKillsInIntervalSec(25);
    if (in25 > m_killsIn25SecBest) {
        m_killsIn25SecBest = in25;
    }
    const int in40 = GetKillsInIntervalSec(40);
    if (in40 > m_killsIn40SecBest) {
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
    // the original asks for the ammo twice and compares a copy of the type
    if (weapon->GetActiveAmmo() != NULL && XString(weapon->GetActiveAmmo()->m_ammoType) == XString("pistol")) {
        ++m_pistolHeadshots;
    }
}

void CPlayerGameStats::RegisterHit(const CUnit*, const CDH_Weapon* weapon)
{
    ++m_hitsNumber;
    if (weapon == NULL) {
        return;
    }
    if (weapon->GetActiveAmmo() != NULL && XString(weapon->GetActiveAmmo()->m_ammoType) == XString("pistol")) {
        ++m_pistolHitsOnTarget;
        if (m_pistolHitsOnTarget > m_pistolHitsOnePersonBest) {
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
        while (m_killTimes[last] - m_killTimes[first] > limit && first < last) {
            ++first;
        }
        const int count = last - first + 1;
        if (count > best) {
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
    for (int index = 0; index < m_completedMissionsAtLocations.size(); ++index) {
        if (m_completedMissionsAtLocations[index] == location) {
            return;
        }
    }
    m_completedMissionsAtLocations.addElement(location);
}

bool CPlayerGameStats::CompletedMissionsAtEveryLocations() const
{
    // the original does not check the location manager or the location count
    return m_completedMissionsAtLocations.size() >= static_cast<App*>(WindowApp::m_instance)->m_locationManager->m_locations.m_count;
}

void CPlayerGameStats::RegisterMiscPurchase(int id)
{
    if (id == 4) {
        ++m_purchasedCyberScopes;
    }
}

bool CPlayerGameStats::HasPurchasedVisorAndCyberScope() const
{
    // the original does not check for a missing visor item
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (app->InitMiscItemManager()->getItemById(2)->isAvailable() != true) {
        return false;
    }
    return m_purchasedCyberScopes > 0;
}
