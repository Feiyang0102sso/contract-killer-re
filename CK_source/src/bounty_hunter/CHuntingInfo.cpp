// Mission counters, rewards and score reporting from CK ARMv7.
#include "CHuntingInfo.h"
#include "App.h"
#include "CAmmoManager.h"
#include "CBH_Player.h"
#include "CDH_BasicGameData.h"
#include "CDH_Weapon.h"
#include "CGameAnalytics.h"
#include "DGHelper.h"
#include "GameCenter_Facade.h"

CHuntingInfo::SAssignedData::SAssignedData()
    : m_missionId(-1), m_cameraHorizontalLimit(90.0f), m_cameraVerticalLimit(45.0f), m_introTimeMs(-1),
      m_cameraDurationMs(-1),
      m_cameraHUDModel(0), m_additionalMinionCount(0), m_weaponCategory(0),
      m_requiredWeaponId(-1), m_randomWeaponId(-1)
{
    memset(m_field18, 0, sizeof(m_field18));
    m_bossType = NULL;
    m_squadType = NULL;
    m_scriptType = NULL;
    m_civilianCount = 0;
    m_experienceReward = 0;
    m_moneyReward = 0;
    m_experiencePenalty = 0;
    m_moneyPenalty = 0;
    m_ambientSound = NULL;
    m_field4c = 0;
    m_cameraIndex = 0;
    m_informantUnlockOnLevelUp = false;
    m_hasTarget = false;
    m_revenge = false;
    m_friendGated = false;
    m_weaponGated = false;
    m_field84 = 0;
    m_alertSeconds = 0;
    m_escapeSeconds = 0;
    m_tripleMultiplierSeconds = 0;
    m_doubleMultiplierSeconds = 0;
    m_ambush = false;
    m_tranquillize = false;
    m_multiplierCountdown = false;
    m_field9b = false;
    m_field9c = false;
    m_bossFirst = false;
    m_bossWins = false;
    m_bossSpawnsLast = false;
    m_silentKill = false;
    m_fielda1 = false;
    m_hasFakeBoss = false;
}

CHuntingInfo::CHuntingInfo()
{
    CleanHuntingStatistic();
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
    CPlayerGameStats& statistics = player->m_gameStats;
    statistics.AddTotalBounties(1);
    if (IsPlayerWin()) {
        statistics.RegisterCompletedMissionAt(m_assignedData.m_locationName);
        if (m_assignedData.m_cameraDurationMs > 0) {
            player->FieldAt(0x1b4)++;
        }
        if (m_assignedData.m_friendGated) {
            CGameAnalytics::logFriendsGatedCompleted(player->GetCurrentFriendGatedMissionID());
            player->SetNextFriendGatedMissionID();
        }
        player->m_lastHuntResult = 1;
        if (m_bossTranquillized) {
            player->m_lastHuntResult = 2;
        }
        statistics.AddSuccessBounties(1);
        statistics.AddEnemyEscaped(m_enemiesEscaped);
        statistics.AddEnemyTranquillized(m_enemiesTranquillized);
        player->AddMoney(m_assignedData.m_moneyReward, GAIN_SOURCE_MISSION);
        CGameAnalytics::logCurrencyChange(XString("00_mission reward"), m_assignedData.m_moneyReward,
                                          CGameAnalytics::CURRENCY_SOFT);
        App* app = static_cast<App*>(WindowApp::m_instance);
        int ammoPacks = app->m_helper->m_config.GetIntegerValueByToken("BASE_PISTOL_AMMO_PACKS_AWARD", true);
        if (ammoPacks > 0) {
            CAmmo* ammo = app->InitAmmoManager()->GetAmmoByType(XString("pistol"));
            if (ammo != NULL) {
                player->IncAmmo(ammo, ammoPacks);
            }
        }
        if (m_assignedData.m_missionId >= 0) {
            WindowApp::HandleTunnelCommand(0x45f9bf40, m_assignedData.m_missionId, 0, 0);
        }
        CDH_BasicGameData* game = app->m_helper->GetGameData();
        const XString& mission = m_assignedData.m_missionName;
        CDH_Weapon* primary = game->GetWeapon01(true);
        CDH_Weapon* secondary = game->GetWeapon02(true);
        if (primary != NULL) {
            CGameAnalytics::logPrimaryWeaponHits(m_primaryHits, mission, primary->m_id);
            CGameAnalytics::logPrimaryWeaponKills(m_primaryKills, mission, primary->m_id);
        }
        if (secondary != NULL) {
            CGameAnalytics::logSecondaryWeaponHits(m_secondaryHits, mission, secondary->m_id);
            CGameAnalytics::logSecondaryWeaponKills(m_secondaryKills, mission, secondary->m_id);
        }
        CGameAnalytics::logHeadshots(m_headshots, mission);
        CGameAnalytics::logMissionComplete(mission);
        if (m_assignedData.m_weaponGated) {
            CGameAnalytics::logWeaponGatedCompleted(XString(m_assignedData.m_weaponCategory));
        }
    } else {
        player->m_lastHuntResult = m_assignedData.m_ambush;
    }
    player->Save();
    UpdateLeaderboards();
}

bool CHuntingInfo::UpdateLeaderboards()
{
    CBH_Player* player = CBH_Player::GetInstance();
    CPlayerGameStats& statistics = player->m_gameStats;
    GameCenter_Facade* gameCenter = GameCenter_Facade::GetInstance();
    if (!gameCenter->isGameCenterSupported() || !gameCenter->isAuthenticated()) {
        return false;
    }
    gameCenter->StartPostingScore("BOUNTY_BOARD_EXP", player->m_experience);
    gameCenter->StartPostingScore("BOUNTY_BOARD_HEADSHOTS", statistics.m_headshotsNumber);
    gameCenter->StartPostingScore("BOUNTY_BOARD_TOTAL_KILLS", statistics.m_targetsKilled + statistics.m_enemyKilled);
    gameCenter->StartPostingScore("BOUNTY_BOARD_TOTAL_CONTRACTS", statistics.m_successBounties);
    gameCenter->StartPostingScore("BOUNTY_BOARD_TOTAL_CAPTURED", statistics.m_enemyTranquillized);
    return true;
}

typedef char AssertAssignedDataSize[sizeof(CHuntingInfo::SAssignedData) == 0xa8 ? 1 : -1];
typedef char AssertHuntingInfoSize[sizeof(CHuntingInfo) == 0xd8 ? 1 : -1];
