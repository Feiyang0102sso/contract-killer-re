// GamePlayManager.cpp - the active player data and its weapon slots.
#include "GamePlayManager.h"

#include "App.h"
#include "CBH_Player.h"
#include "CDH_Weapon.h"
#include "CDH_WeaponsManager.h"
#include "CGameAnalytics.h"
#include "CHuntingInfo.h"
#include "CSwerveGame.h"
#include "DGHelper.h"

typedef char GamePlayManager_ARMv7_Size[(sizeof(GamePlayManager) == 0xc0) ? 1 : -1];

namespace {
    void LogEquipped(const CDH_Weapon& weapon)
    {
        XString id = XString::Format(L"%02d_%s", weapon.m_id, weapon.m_name.c_str());
        int price = weapon.m_pricePoints;
        CGameAnalytics::ECurrencyType currency = CGameAnalytics::CURRENCY_SOFT;
        if (price < 1) {
            price = weapon.getPriceCredits();
            currency = CGameAnalytics::CURRENCY_HARD;
        }
        CGameAnalytics::logItemEquipped(id, price, currency);
    }
}

GamePlayManager::GamePlayManager()
    : m_gameType(GAME_TYPE_QUICK)
    , m_fieldbc(false)
    , m_fieldbd(true)
{
}

GamePlayManager::~GamePlayManager()
{
}

void GamePlayManager::init()
{
}

void GamePlayManager::Serialize(DataOutputStream&)
{
}

void GamePlayManager::DeSerialize(JDataInputStream&)
{
}

void GamePlayManager::RestoreAmmo()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    Vector<CDH_Weapon*>& weapons = app->InitWeaponManager()->GetWeapons();
    for (int index = 0; index < weapons.size(); ++index) {
        weapons[index]->restoreAmmo();
    }
}

void GamePlayManager::Reset()
{
    m_playerData.GetGameData(static_cast<game_type_t>(1))->Reset();
    m_playerData.reset();
    static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->resetData();
}

void GamePlayManager::FinishGame(bool won)
{
    GetGameData()->FinishGame(won);
}

void GamePlayManager::BreakGame()
{
}

int GamePlayManager::GetXPRemaining() const
{
    return m_playerData.GetXPPerLevel(m_playerData.m_field88) - m_playerData.m_field84;
}

int GamePlayManager::GetLevel() const
{
    return m_playerData.m_field88;
}

bool GamePlayManager::GetLevelUp() const
{
    return (m_playerData.m_field90 & 1) != 0;
}

void GamePlayManager::ResetLevelUp()
{
    m_playerData.m_field90 = 0;
}

void GamePlayManager::SwitchWeapons()
{
    // the original checks neither the game, the weapon nor the hunting info
    GetGameData()->SwitchWeapons();
    static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->onWeaponSwitch();
    App* app = static_cast<App*>(WindowApp::m_instance);
    CGameAnalytics::logSwitchWeapon(app->m_huntingInfo->m_assignedData.m_missionName,
                                    app->m_helper->GetGameData()->GetActiveWeapon()->m_id);
}

void GamePlayManager::SetWeapon01(int id)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_Weapon* weapon = app->InitWeaponManager()->GetWeaponById(id);
    if (weapon != NULL) {
        LogEquipped(*weapon);
    }
    CDH_BasicGameData* data = GetGameData();
    data->m_activeWeaponId = id;
    data->m_weaponIds[0] = id;
}

void GamePlayManager::SetWeapon02(int id)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_Weapon* weapon = app->InitWeaponManager()->GetWeaponById(id);
    if (weapon != NULL) {
        LogEquipped(*weapon);
    }
    GetGameData()->m_weaponIds[1] = id;
}

void GamePlayManager::SetWeapon(int slot, CDH_Weapon* weapon)
{
    int id = -1;
    if (weapon != NULL) {
        id = weapon->GetId();
    }
    SetWeapon(slot, id);
    CBH_Player::GetInstance()->Save();
}

void GamePlayManager::SetWeapon(int slot, int id)
{
    if (slot == 0) {
        SetWeapon01(id);
    } else {
        SetWeapon02(id);
    }
}
