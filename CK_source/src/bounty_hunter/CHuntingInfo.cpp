// Mission counters, rewards and score reporting from CK ARMv7.
#undef memset
#include <string.h>
#include "CHuntingInfo.h"
#include "App.h"
#include "CAmmoManager.h"
#include "CBH_Player.h"
#include "CDH_BasicGameData.h"
#include "CDH_Weapon.h"
#include "CGameAnalytics.h"
#include "DGHelper.h"
#include "GameCenter_Facade.h"

namespace {

const XString& InitAssignedLocation(CHuntingInfo::SAssignedData& data)
{
    memset(&data.m_field18, 0, 0x20);
    return *XString::pEmpty;
}

const XString& InitAssignedMissionName(CHuntingInfo::SAssignedData& data)
{
    data.m_weaponCategory = 0;
    memset(&data.m_informantUnlockOnLevelUp, 0, 5);
    data.m_requiredWeaponId = -1;
    data.m_randomWeaponId = -1;
    data.m_fielda1 = false;
    memset(&data.m_tripleMultiplierSeconds, 0, 0xf);
    return *XString::pEmpty;
}

} // namespace

const char* CHuntingInfo::BOARD_XP = "BOUNTY_BOARD_EXP";
const char* CHuntingInfo::BOARD_HEADSHOTS = "BOUNTY_BOARD_HEADSHOTS";
const char* CHuntingInfo::BOARD_TOTAL_KILLS = "BOUNTY_BOARD_TOTAL_KILLS";
const char* CHuntingInfo::BOARD_CONTRACTS_COMPLETED = "BOUNTY_BOARD_TOTAL_CONTRACTS";
const char* CHuntingInfo::BOUNTY_TOTAL_CAPTURED = "BOUNTY_BOARD_TOTAL_CAPTURED";

CHuntingInfo::SAssignedData::SAssignedData()
    : m_locationName(InitAssignedLocation(*this)), m_cameraHorizontalLimit(90.0f),
      m_cameraVerticalLimit(45.0f), m_introTimeMs(-1), m_cameraDurationMs(-1),
      m_cameraHUDModel(0), m_additionalMinionCount(0),
      m_missionName(InitAssignedMissionName(*this))
{
    // Assignment-specific fields are left for the mission setup to populate.
}

CHuntingInfo::CHuntingInfo()
    : m_result(BountyResultPending), m_fieldac(false), m_bossTranquillized(false), m_fieldc0(false)
{
}

void CHuntingInfo::SetHuntingData(SAssignedData data)
{
    m_assignedData = data;
    CleanHuntingStatistic();
}

void CHuntingInfo::CleanHuntingStatistic()
{
    m_result = BountyResultPending;
    m_fieldac = false;
    m_bossTranquillized = false;
    m_fieldc0 = false;
    m_experienceEarned = 0;
    m_enemiesKilled = 0;
    m_enemiesTranquillized = 0;
    m_enemiesEscaped = 0;
    m_primaryHits = 0;
    m_secondaryHits = 0;
    m_primaryKills = 0;
    m_secondaryKills = 0;
    m_headshots = 0;
}

void CHuntingInfo::SetBountyResult(EBountyResult result)
{
    m_result = result;
}

bool CHuntingInfo::IsPlayerWin() const
{
    return (unsigned int)m_result - BountyResultWin < 2;
}

bool CHuntingInfo::IsAmbush() const
{
    return m_assignedData.m_cameraDurationMs > 0 || m_assignedData.m_ambush;
}

bool CHuntingInfo::SilentKill() const
{
    return m_assignedData.m_cameraDurationMs < 1 && m_assignedData.m_silentKill;
}

void CHuntingInfo::ReduceRewardMoney(int amount)
{
    int money = m_assignedData.m_moneyReward - amount;
    if (money < 0) {
        money = 0;
    }
    m_assignedData.m_moneyReward = money;
}

void CHuntingInfo::OnReceivedXP(int amount)
{
    m_experienceEarned += amount;
}

void CHuntingInfo::OnEnemyKilled()
{
    m_enemiesKilled++;
}

void CHuntingInfo::OnEnemyTranqullized()
{
    m_enemiesTranquillized++;
}

void CHuntingInfo::OnEnemyEscaped()
{
    m_enemiesEscaped++;
}

void CHuntingInfo::OnBossTranquillized()
{
    m_bossTranquillized = true;
}

void CHuntingInfo::AfterSceneActivity()
{
    CBH_Player* player = CBH_Player::GetInstance();
    player->m_gameStats.AddTotalBounties(1);
    if (IsPlayerWin()) {
        CBH_Player::GetInstance()->m_gameStats.RegisterCompletedMissionAt(m_assignedData.m_locationName);
    }
    if (IsPlayerWin() && m_assignedData.m_cameraDurationMs > 0) {
        CBH_Player::GetInstance()->FieldAt(0x1b4)++;
    }
    if (!IsPlayerWin()) {
        player->m_lastHuntResult = m_assignedData.m_ambush;
        player->Save();
    } else {
        if (m_assignedData.m_friendGated) {
            CGameAnalytics::logFriendsGatedCompleted(player->GetCurrentFriendGatedMissionID());
            CBH_Player::GetInstance()->SetNextFriendGatedMissionID();
        }
        bool bossTranquillized = m_bossTranquillized;
        CBH_Player* currentPlayer = CBH_Player::GetInstance();
        int result = 1;
        if (bossTranquillized) {
            result = 2;
        }
        currentPlayer->m_lastHuntResult = result;
        currentPlayer->Save();
        CBH_Player::GetInstance()->m_gameStats.AddSuccessBounties(1);
        CBH_Player::GetInstance()->m_gameStats.AddEnemyEscaped(m_enemiesEscaped);
        CBH_Player::GetInstance()->m_gameStats.AddEnemyTranquillized(m_enemiesTranquillized);
        CBH_Player::GetInstance()->AddMoney(m_assignedData.m_moneyReward, GAIN_SOURCE_MISSION);
        CGameAnalytics::logCurrencyChange(XString("00_mission reward"), m_assignedData.m_moneyReward,
                                          CGameAnalytics::CURRENCY_SOFT);

        int ammoPacks = static_cast<App*>(WindowApp::m_instance)->m_helper->m_config.GetIntegerValueByToken(
            "BASE_PISTOL_AMMO_PACKS_AWARD", true);
        CAmmo* ammo = static_cast<App*>(WindowApp::m_instance)->InitAmmoManager()->GetAmmoByType(XString("pistol"));
        if (ammoPacks > 0) {
            CBH_Player::GetInstance()->IncAmmo(ammo, ammoPacks);
        }
        if (m_assignedData.m_missionId >= 0) {
            WindowApp::HandleTunnelCommand(0x45f9bf40, m_assignedData.m_missionId, 0, 0);
        }

        const XString& mission = m_assignedData.m_missionName;
        int primaryHits = m_primaryHits;
        CDH_Weapon* primary = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon01(true);
        CGameAnalytics::logPrimaryWeaponHits(primaryHits, mission, primary->m_id);
        int secondaryHits = m_secondaryHits;
        CDH_Weapon* secondary = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon02(true);
        CGameAnalytics::logSecondaryWeaponHits(secondaryHits, mission, secondary->m_id);
        int primaryKills = m_primaryKills;
        primary = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon01(true);
        CGameAnalytics::logPrimaryWeaponKills(primaryKills, mission, primary->m_id);
        int secondaryKills = m_secondaryKills;
        secondary = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon02(true);
        CGameAnalytics::logSecondaryWeaponKills(secondaryKills, mission, secondary->m_id);
        CGameAnalytics::logHeadshots(m_headshots, mission);
        CGameAnalytics::logMissionComplete(mission);
        if (m_assignedData.m_weaponGated) {
            CGameAnalytics::logWeaponGatedCompleted(XString(m_assignedData.m_weaponCategory));
        }
    }
    UpdateLeaderboards();
}

bool CHuntingInfo::UpdateLeaderboards()
{
    int experience = CBH_Player::GetInstance()->m_experience;
    int headshots = CBH_Player::GetInstance()->m_gameStats.m_headshotsNumber;
    int enemiesKilled = CBH_Player::GetInstance()->m_gameStats.m_enemyKilled;
    int targetsKilled = CBH_Player::GetInstance()->m_gameStats.m_targetsKilled;
    int completedContracts = CBH_Player::GetInstance()->m_gameStats.m_successBounties;
    int enemiesCaptured = CBH_Player::GetInstance()->m_gameStats.m_enemyTranquillized;

    if (!GameCenter_Facade::GetInstance()->isGameCenterSupported()) {
        return false;
    }
    if (!GameCenter_Facade::GetInstance()->isAuthenticated()) {
        return false;
    }
    GameCenter_Facade::GetInstance()->StartPostingScore(BOARD_XP, experience);
    GameCenter_Facade::GetInstance()->StartPostingScore(BOARD_HEADSHOTS, headshots);
    GameCenter_Facade::GetInstance()->StartPostingScore(BOARD_TOTAL_KILLS, targetsKilled + enemiesKilled);
    GameCenter_Facade::GetInstance()->StartPostingScore(BOARD_CONTRACTS_COMPLETED, completedContracts);
    GameCenter_Facade::GetInstance()->StartPostingScore(BOUNTY_TOTAL_CAPTURED, enemiesCaptured);
    return true;
}

typedef char AssertAssignedDataSize[sizeof(CHuntingInfo::SAssignedData) == 0xa8 ? 1 : -1];
typedef char AssertHuntingInfoSize[sizeof(CHuntingInfo) == 0xd8 ? 1 : -1];
