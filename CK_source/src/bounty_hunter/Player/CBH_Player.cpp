// CBH_Player.cpp - the CK wallet, energy, player profile, and ammo inventory.
#include "CBH_Player.h"

#include "App.h"
#include "CAmmo.h"
#include "CAmmoManager.h"
#include "CAvatar.h"
#include "CAchievementManager.h"
#include "CAvatarManager.h"
#include "CBH_Math.h"
#include "CBH_PersonType.h"
#include "CBH_SurfaceManager.h"
#include "CChallengeStats.h"
#include "CDH_SoundManager.h"
#include "CDH_Weapon.h"
#include "CDH_WeaponsManager.h"
#include "CFiveDaysBonus.h"
#include "CFiveDaysIncentiveBonus.h"
#include "CFactionType.h"
#include "CFactionTypeManager.h"
#include "CFriendGatedMissionManager.h"
#include "CGameAnalytics.h"
#include "CNGS.h"
#include "CNGSLocalUser.h"
#include "CNGSRemoteUser.h"
#include "CNGSRemoteUserList.h"
#include "CNGSSession.h"
#include "CHuntingInfo.h"
#include "CMissionManager.h"
#include "COverrideManager.h"
#include "CProgressSystem.h"
#include "CSaveManager.h"
#include "CStdUtil_Cocoa.h"
#include "CSwerveGame.h"
#include "CTutorialManager.h"
#include "CXmlHelper.h"
#include "DGHelper.h"
#include "GameCenter_Facade.h"
#include "GamePlayHUD.h"

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char CBH_Player_ARMv7_Size[(sizeof(CBH_Player) == 0x28c) ? 1 : -1];
#endif

namespace {
    int Nonnegative(int value)
    {
        if (value < 0) {
            return 0;
        }
        return value;
    }

}

CBH_Player* CBH_Player::GetInstance()
{
    void* found = NULL;
    CApplet::m_pApp->m_singletons->Find(ClassId, &found);
    CBH_Player* instance = static_cast<CBH_Player*>(found);
    if (instance == NULL) {
        instance = new CBH_Player;
    }
    return instance;
}

CBH_Player::CBH_Player()
    : CSingleton(ClassId)
    , m_energy(100)
    , m_cash(0)
    , m_credits(0)
    , m_experience(0)
    , m_level(1)
    , m_hp(100)
    , m_medkits(3)
    , m_reviveCount(0)
    , m_energyEnabled(true)
    , m_field39(false)
    , m_field3c(0)
    , m_tutorialState(0)
    , m_lastEnergyUpdateTime(App::GetTime())
    , m_lastHuntItem(-1)
    , m_lastHuntResult(0)
    , m_lastDailyBonusTime(0)
    , m_dailyBonusesInRow(0)
    , m_lastFiveDaysBonusTime(0)
    , m_activeFiveDaysBonusDay(0)
    , m_lastFiveDaysBonusTeaserDay(0)
    , m_lastFiveDaysIncentiveBonusTimestamp(0)
    , m_activeFiveDaysIncentiveBonusDay(0)
    , m_fiveDayIncentiveBonusForbidden(false)
    , m_receivedFiveDaysIncentiveBonus(false)
    , m_incentiveBonusExpirationShown(false)
    , m_appRated(false)
    , m_silentShotsCount(0)
    , m_cyberScopesCount(0)
    , m_lastGameVersionPlayed(100)
    , m_lastMissionVersionPlayed(100)
    , m_newPlayer(true)
    , m_newMissionVersion(true)
    , m_completedMissionCount(0)
    , m_tapjoyPoints(8)
    , m_friendGiftDays(8)
    , m_currentFriendGatedMissionId(0)
    , m_fielda4(-1)
    , m_avatar(NULL)
    , m_fieldb4(0)
    , m_hitechVisorEquipped(false)
    , m_reputation(8)
    , m_challengeStats(NULL)
    , m_targetFriendId(-1)
    , m_field23c(0)
    , m_field240(1)
    , m_speedTimeMS(0)
    , m_speedModeAvailableTimeMS(0)
    , m_speedModeWasActive(false)
    , m_ultrasoundWasActive(false)
    , m_consumingReactixControl(false)
    , m_reactixControlTime(0)
    , m_ultrasoundAttemptCount(0)
    , m_armorId(0)
    , m_armorDamageProof(0)
    , m_armorDamageProofMax(0)
    , m_ammoCounts(8)
    , m_estimatedWeapon(NULL)
    , m_estimatedAmmo(-1)
{
    m_lastDailyBonusTime = CStdUtil_Cocoa::GetTimeSeconds();
    m_lastFiveDaysBonusTime = m_lastDailyBonusTime;
    CStrWChar name("player");
    m_playerNameReady = false;
    m_authenticationStarted = false;
    m_fieldc0 = false;
    if (GameCenter_Facade::GetInstance()->isGameCenterSupported() == true
        && GameCenter_Facade::GetInstance()->isAuthenticated() == true) {
        GameCenter_Facade::GetInstance()->GetLocalPLayerName(name);
        m_playerNameReady = true;
        m_authenticationStarted = true;
    }
    m_playerName = name.GetWCharPtr();
    m_playerName = m_playerName.Upper();
    m_challengeStats = new CChallengeStats();
    Init();
}

void CBH_Player::Init()
{
    m_field208 = 0;
    m_missionBriefImage = NULL;
    m_energy = static_cast<App*>(WindowApp::m_instance)->InitProgressSystem()->GetMaxEnergyForLevel(m_level);
    m_hp = GetMaxHpForLevel(m_level);
    m_energyRestoreSeconds = static_cast<App*>(WindowApp::m_instance)->m_helper->m_config.GetIntegerValueByToken("ONE_ENERGY_RESTORE_SEC", true);
    InitFactionAttitude();
    for (const CAmmo* ammo = static_cast<const CAmmo*>(static_cast<App*>(WindowApp::m_instance)->InitAmmoManager()->firstItem());
         ammo != NULL;
         ammo = static_cast<const CAmmo*>(static_cast<App*>(WindowApp::m_instance)->InitAmmoManager()->nextItem())) {
        m_ammoCounts.Set(HashTableKey<const CAmmo*>(ammo), 0);
    }
    m_avatar = static_cast<App*>(WindowApp::m_instance)->InitAvatarManager()->GetAvatar(0);
    Load();
}

int CBH_Player::GetMaxEnergy()
{
    return static_cast<App*>(WindowApp::m_instance)->InitProgressSystem()->GetMaxEnergyForLevel(m_level);
}

int CBH_Player::GetMaxHp()
{
    return GetMaxHpForLevel(m_level);
}

void CBH_Player::InitFactionAttitude()
{
    for (int index = 0; index < static_cast<App*>(WindowApp::m_instance)->InitFactionTypeManager()->m_factions.m_count; ++index) {
        CFactionType* faction = static_cast<App*>(WindowApp::m_instance)->InitFactionTypeManager()->GetFactionAtIndex(index);
        if (faction != NULL) {
            m_reputation.Add(faction->m_name, 0);
        }
    }
}

void CBH_Player::Load()
{
    TiXmlElement* root = static_cast<App*>(WindowApp::m_instance)->m_saveManager->GetRootElement();
    TiXmlElement* player = root->FirstChildElement("player");
    if (player != NULL) {
        m_newPlayer = false;
        m_newMissionVersion = false;
        LoadStats(player);
        LoadMissionInfo(player);
        LoadAmmo(player);
        LoadAvatar(player);
        LoadTjPoints(player);
        LoadFriendGiftLasDay(player);
        m_gameStats.Load(player);
        m_challengeStats->Load(player);
        static_cast<App*>(WindowApp::m_instance)->InitAchievementManager()->Load(player);
    } else {
        SetPlayerStartParams();
    }
}

void CBH_Player::Update()
{
    const unsigned int now = CStdUtil_Cocoa::GetTimeSeconds();
    unsigned int elapsed;
    unsigned int restoreSeconds;
    if (now <= m_lastEnergyUpdateTime) {
        m_lastEnergyUpdateTime = now;
        elapsed = 0;
        restoreSeconds = m_energyRestoreSeconds;
    } else {
        restoreSeconds = m_energyRestoreSeconds;
        elapsed = now - m_lastEnergyUpdateTime;
        if (elapsed > restoreSeconds) {
            AddEnergy(elapsed / restoreSeconds, false);
            m_lastEnergyUpdateTime = now;
            Save();
            goto updatePlayerName;
        }
    }
    m_energyRemainingSeconds = restoreSeconds - elapsed;

updatePlayerName:
    if (!m_playerNameReady && GameCenter_Facade::GetInstance()->isGameCenterSupported()) {
        const int state = GameCenter_Facade::GetInstance()->GetAuthenticationState();
        switch (state) {
        case 0:
        case GameCenter_Facade::RequestState_Failed:
            if (m_authenticationStarted) {
                m_playerName = L"PLAYER";
                m_playerNameReady = true;
            }
            break;
        case GameCenter_Facade::RequestState_Done: {
            CStrWChar name("player");
            GameCenter_Facade::GetInstance()->GetLocalPLayerName(name);
            m_playerName = name.GetWCharPtr();
            m_playerName = m_playerName.Upper();
            m_playerNameReady = true;
            break;
        }
        }
    }
}

void CBH_Player::AddEnergy(int amount, bool allowOverflow)
{
    // the original asks the progress system for the maximum each time it needs it
    if (!allowOverflow) {
        if (m_energy <= GetMaxEnergy()) {
            if (m_energy + amount > GetMaxEnergy()) {
                m_energy = GetMaxEnergy();
            } else {
                m_energy += amount;
            }
        }
    } else {
        m_energy += amount;
    }
    Save();
}

bool CBH_Player::TryMakeFiveDaysBonus(bool* newDay)
{
    if (!m_fiveDayIncentiveBonusForbidden || static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial()) {
        return false;
    }
    unsigned int now = CStdUtil_Cocoa::GetTimeSeconds();
    if (now == 0xffffffffu) {
        return false;
    }
    *newDay = false;
    CNGSSession* session = CNGS::GetInstance()->GetLocalUser()->m_session;
    bool online;
    if (App::IsNetTimeAvailable() == 1) {
        online = true;
        now = session->getNetworkCurrentTimeSeconds();
    } else {
        online = false;
    }
    int currentDay = DGHelper::GetDaysFromTimestamp(now);
    int previousDay = DGHelper::GetDaysFromTimestamp(m_lastFiveDaysBonusTime);
    if (currentDay == previousDay) {
        return false;
    }
    *newDay = true;
    if (!online) {
        return false;
    }
    int bonusDay;
    if (currentDay - previousDay >= 2) {
        bonusDay = 0;
        m_activeFiveDaysBonusDay = 0;
        m_dailyBonusesInRow = 0;
    } else if (currentDay >= previousDay) {
        bonusDay = m_activeFiveDaysBonusDay;
    } else {
        m_lastFiveDaysBonusTime = now;
        Save();
        return false;
    }
    static_cast<App*>(WindowApp::m_instance)->m_fiveDaysBonus->GiveBonus(bonusDay);
    m_lastFiveDaysBonusTime = now;
    ++m_activeFiveDaysBonusDay;
    ++m_dailyBonusesInRow;
    if (m_activeFiveDaysBonusDay >= CFiveDaysBonus::DAY_COUNT) {
        m_activeFiveDaysBonusDay = 0;
    }
    Save();
    return true;
}

bool CBH_Player::TryMakeFiveDaysIncentiveBonus()
{
    if (m_fiveDayIncentiveBonusForbidden) {
        return false;
    }
    unsigned int now = CStdUtil_Cocoa::GetTimeSeconds();
    if (now == 0xffffffffu) {
        return false;
    }
    CNGSSession* session = CNGS::GetInstance()->GetLocalUser()->m_session;
    bool offline;
    if (App::IsNetTimeAvailable() == 1) {
        offline = false;
        now = session->getNetworkCurrentTimeSeconds();
    } else {
        offline = true;
    }
    if (m_lastFiveDaysIncentiveBonusTimestamp == 0) {
        m_lastFiveDaysIncentiveBonusTimestamp = now - 24 * 60 * 60;
    }
    int currentDay = DGHelper::GetDaysFromTimestamp(now);
    int previousDay = DGHelper::GetDaysFromTimestamp(m_lastFiveDaysIncentiveBonusTimestamp);
    if (currentDay == previousDay || offline) {
        return false;
    }
    if (currentDay - previousDay >= 2) {
        m_fiveDayIncentiveBonusForbidden = true;
        return false;
    }
    if (currentDay < previousDay) {
        m_fiveDayIncentiveBonusForbidden = true;
        Save();
        return false;
    }
    static_cast<App*>(WindowApp::m_instance)->m_fiveDaysIncentiveBonus->GiveBonus(m_activeFiveDaysIncentiveBonusDay);
    m_receivedFiveDaysIncentiveBonus = true;
    m_lastFiveDaysIncentiveBonusTimestamp = now;
    if (++m_activeFiveDaysIncentiveBonusDay >= CFiveDaysBonus::DAY_COUNT) {
        m_fiveDayIncentiveBonusForbidden = true;
    }
    Save();
    return true;
}

bool CBH_Player::CanShowFiveDaysIncentiveBonusExpirationMessage()
{
    return m_receivedFiveDaysIncentiveBonus && m_fiveDayIncentiveBonusForbidden
        && !m_incentiveBonusExpirationShown;
}

void CBH_Player::RegisterFiveDaysIncentiveBonusExpirationMessageShown()
{
    m_incentiveBonusExpirationShown = true;
    Save();
}

bool CBH_Player::IsGameVersionNew()
{
    return m_lastGameVersionPlayed < 150;
}

bool CBH_Player::IsMissionVersionNew()
{
    return m_lastMissionVersionPlayed < 150;
}

void CBH_Player::ChangeGameVersion()
{
    m_newPlayer = false;
    m_lastGameVersionPlayed = 150;
    Save();
}

void CBH_Player::ChangeMissionVersion()
{
    m_newMissionVersion = false;
    m_lastMissionVersionPlayed = 150;
    Save();
}

void CBH_Player::Save()
{
    CSaveManager* save = static_cast<App*>(WindowApp::m_instance)->m_saveManager;
    if (save->m_pauseSaving || save->m_pauseGServe == 1) {
        return;
    }
    TiXmlElement* root = save->GetRootElement();
    TiXmlElement* player = CXmlHelper::ObtainElement(root, XString("player"));
    SaveStats(player);
    SaveReputation(player);
    SaveMissionInfo(player);
    SaveAmmo(player);
    player->SetAttribute("avatar_id", m_avatar->m_id);
    SaveTjPoints(player);
    SaveFriendGiftLasDay(player);
    m_gameStats.Save(player);
    static_cast<App*>(WindowApp::m_instance)->InitAchievementManager()->Save(player);
    m_challengeStats->Save(player);
    static_cast<App*>(WindowApp::m_instance)->m_saveManager->SaveOnNextUpdate(false);
}

void CBH_Player::SetAmmoForWeaponId(int id, int packs)
{
    // the original looks the ammo up again by its type name and does not check the weapon
    CDH_Weapon* weapon = static_cast<CDH_Weapon*>(static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->getItemById(id));
    const CAmmo* active = weapon->GetActiveAmmo();
    if (active == NULL) {
        return;
    }
    XString type = active->m_ammoType;
    const CAmmo* ammo = static_cast<App*>(WindowApp::m_instance)->InitAmmoManager()->GetAmmoByType(type);
    if (ammo != NULL && packs > 0) {
        SetAmmoCount(ammo, ammo->m_countInPack * packs);
    }
}

void CBH_Player::IncAmmo(const CAmmo* ammo, int packs)
{
    // the original has IncAmmoByBulletsCount expanded here (without its second count test)
    if (ammo != NULL && packs > 0) {
        int count = ammo->m_countInPack * packs;
        int total = *m_ammoCounts.Get(HashTableKey<const CAmmo*>(ammo)) + count;
        m_ammoCounts.Set(HashTableKey<const CAmmo*>(ammo), total);
        m_gameStats.AddBulletsEarned(count);
        Save();
    }
}

void CBH_Player::IncAmmoByBulletsCount(const CAmmo* ammo, int count)
{
    if (ammo != NULL && count > 0) {
        // the original reads the current count without a missing-entry check and stores the sum with Set
        int total = *m_ammoCounts.Get(HashTableKey<const CAmmo*>(ammo)) + count;
        m_ammoCounts.Set(HashTableKey<const CAmmo*>(ammo), total);
        m_gameStats.AddBulletsEarned(count);
        Save();
    }
}

void CBH_Player::SetAmmoCount(const CAmmo* ammo, int count)
{
    if (ammo != NULL) {
        m_ammoCounts.Set(HashTableKey<const CAmmo*>(ammo), count);
        Save();
    }
}

int CBH_Player::GetAvailableAmmo(const CAmmo* ammo, bool estimate)
{
    // the original compares a copy of the type name and checks neither the ammo nor the table entry
    if (XString(ammo->m_ammoType) == XString("infinite")) {
        return 0xfffffff;
    }
    if (estimate && m_estimatedWeapon != NULL && m_estimatedAmmo != -1) {
        if (m_estimatedWeapon->GetDefaultAmmo()->m_id == ammo->m_id) {
            return m_estimatedAmmo;
        }
    }
    return *m_ammoCounts.Get(HashTableKey<const CAmmo*>(ammo));
}

void CBH_Player::UseAmmo(const CAmmo* ammo)
{
    if (ammo != NULL) {
        if (m_estimatedWeapon != NULL && m_estimatedAmmo != -1) {
            m_estimatedAmmo--;
            if (m_estimatedAmmo < 0) {
                m_estimatedAmmo = 0;
            }
        } else {
            // the original reads the count without a missing-entry check and stores it back with Set
            int total = *m_ammoCounts.Get(HashTableKey<const CAmmo*>(ammo)) - 1;
            if (total < 0) {
                total = 0;
            }
            m_ammoCounts.Set(HashTableKey<const CAmmo*>(ammo), total);
            Save();
        }
    }
}

bool CBH_Player::HasAnyAmmo()
{
    // the original evaluates both weapons (no short circuit, no NULL checks)
    bool first = GetAvailableAmmo(static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon01(true)->GetActiveAmmo(), true) > 0;
    bool second = GetAvailableAmmo(static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon02(true)->GetActiveAmmo(), true) > 0;
    return second || first;
}

bool CBH_Player::HasAmmoInBothWeapons()
{
    bool first = GetAvailableAmmo(static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon01(true)->GetActiveAmmo(), true) > 0;
    bool second = GetAvailableAmmo(static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon02(true)->GetActiveAmmo(), true) > 0;
    return first && second;
}

bool CBH_Player::HasTranqWeapon()
{
    // the original walks the weapon manager with firstItem / nextItem
    CItem* item = static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->firstItem();
    while (item != NULL) {
        CDH_Weapon* weapon = static_cast<CDH_Weapon*>(item);
        if (weapon->isAvailable() == true && weapon->m_tranquillizer) {
            return true;
        }
        item = static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->nextItem();
    }
    return false;
}

bool CBH_Player::HasTranqWeaponWithin()
{
    // the original does not check the slots for NULL
    if (static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon01(true)->m_tranquillizer) {
        return true;
    }
    return static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon02(true)->m_tranquillizer;
}

bool CBH_Player::HasAmmoForTranqWeaponWithin()
{
    // the original fetches each weapon again for every access and does not check for NULL
    if (static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon01(true)->m_tranquillizer
        && GetAvailableAmmo(static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon01(true)->GetActiveAmmo(), true) > 0) {
        return true;
    }
    if (static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon02(true)->m_tranquillizer
        && GetAvailableAmmo(static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon02(true)->GetActiveAmmo(), true) > 0) {
        return true;
    }
    return false;
}

bool CBH_Player::HasWeaponWithRange(float range)
{
    // the original walks the weapon manager with firstItem / nextItem
    CItem* item = static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->firstItem();
    while (item != NULL) {
        CDH_Weapon* weapon = static_cast<CDH_Weapon*>(item);
        if (weapon->isAvailable() == true && weapon->effectiveRange() >= range) {
            return true;
        }
        item = static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->nextItem();
    }
    return false;
}

bool CBH_Player::HasWeaponWithRangeInHands(float range)
{
    CDH_Weapon* first = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon01(true);
    CDH_Weapon* second = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon02(true);
    if (first != NULL && first->effectiveRange() >= range) {
        return true;
    }
    if (second != NULL && second->effectiveRange() >= range) {
        return true;
    }
    return false;
}

int CBH_Player::WeaponsWithCategoryCount(CDH_Weapon::EWeaponCategory category)
{
    // the original reads WindowApp::m_instance for each manager call
    int count = 0;
    for (CDH_Weapon* weapon = static_cast<CDH_Weapon*>(static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->firstItem());
         weapon != NULL;
         weapon = static_cast<CDH_Weapon*>(static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->nextItem())) {
        if (weapon->isAvailable() == true) {
            count += weapon->m_category == category;
        }
    }
    return count;
}

void CBH_Player::AddMoney(int amount, EGainSource source)
{
    if (source != GAIN_SOURCE_IAP) {
        // the original does not check the override manager for NULL
        COverrideManager* manager = static_cast<App*>(WindowApp::m_instance)->m_overrideManager;
        int multiplier;
        {
            XString group(L"");
            if (source == GAIN_SOURCE_CHALLENGE) {
                group = L"challenge_mult";
            } else {
                group = L"missions_mult";
            }
            XString name(L"soft");
            multiplier = GetOverride(*manager, group, name);
        }
        if (multiplier > 100) {
            amount = (multiplier * amount) / 100;
        }
    }
    m_cash += amount;
    m_gameStats.AddMoneyEarned(amount);
    WindowApp::HandleTunnelCommand(0x83bb38a2, amount, 0, 0);
    Save();
}

int CBH_Player::GetOverride(COverrideManager& manager, const XString& group, const XString& name)
{
    unsigned long long value;
    unsigned char found;
    {
        const CStrWChar groupName(group.c_str());
        const CStrWChar attributeName(name.c_str());
        found = manager.FindIntAttribute(groupName, attributeName, &value);
    }
    if (found != 0) {
        return static_cast<int>(value);
    }
    return 0;
}

void CBH_Player::ReduceMoney(int amount)
{
    m_cash -= amount;
    m_gameStats.AddMoneySpent(amount);
    WindowApp::HandleTunnelCommand(0x83bb38a2, -amount, 0, 0);
    Save();
}

void CBH_Player::ResetBeforeHunting()
{
    m_field23c = 0;
    m_field240 = 1;
    m_speedTimeMS = 0;
    m_speedModeWasActive = false;
    m_ultrasoundWasActive = false;
    m_field280 = -1;
}

void CBH_Player::StartCover()
{
    if (m_field240 == 0) {
        m_field23c = 2;
    } else {
        m_field23c = 1;
        m_challengeStats->RegisterPlayerTakeCover();
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_soundManager->PlaySound("IDM_SPECIAL_DUCK", false, false);
    Save();
}

void CBH_Player::EndCover()
{
    if (m_field23c == 1) {
        m_field23c = 0;
        m_field240 = 0;
        return;
    }
    m_field39 = false;
    m_field23c = 0;
    m_field240 = 1;
    // the original does not check the active weapon
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon()->ResetInScropeRoutine();
}

bool CBH_Player::IsNeedHide() const
{
    return m_field240 == 1 || m_field23c == 2;
}

bool CBH_Player::IsInHide() const
{
    return m_field240 == 0 || m_field23c == 1;
}

bool CBH_Player::IsInProgress() const
{
    return m_field23c != 0;
}

void CBH_Player::TryShotPlayer(CUnit* unit)
{
    if (unit == NULL) {
        return;
    }
    CSwerveGame* game = static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame;
    if (game->m_slowMode || game->IsIntroTime() || m_field39) {
        return;
    }
    float distance = static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->GetDistanceToUnit(unit);
    const CBH_PersonType* person = unit->m_body.m_type;
    const CEnemyWeaponType* weapon = person->m_weapon;
    if (weapon == NULL) {
        return;
    }
    int distanceInUnits = static_cast<int>(distance);
    int probability = static_cast<int>(person->m_baseAccuracy * 100.0f);
    int baseDamage = weapon->m_baseDamage;
    float coefficient = static_cast<App*>(WindowApp::m_instance)->InitProgressSystem()->GetDmgCoeff();
    int level = GetInstance()->m_level;
    if (distanceInUnits > weapon->m_effectiveRange) {
        App* app = static_cast<App*>(WindowApp::m_instance);
        float accuracyScale = app->m_helper->m_config.GetFloatValueByToken("ACCURACY_OUT_OF_EFFECTIVE_RANGE_DECREASE", true);
        probability = static_cast<int>(probability * accuracyScale);
    }
    int hitProbability = 0;
    if (probability >= 0) {
        if (probability > 100) {
            probability = 100;
        }
        hitProbability = probability;
    }
    if (!CBH_Math::GetProbabiliyOutcome(hitProbability)) {
        App* app = static_cast<App*>(WindowApp::m_instance);
        app->m_soundManager->PlaySound("IDM_CHARACTER_BULLET_CLOSE", false, false);
        return;
    }
    if (!GetInstance()->IsNeedHide()) {
        return;
    }
    m_fieldc0 = true;
    m_field280 = unit->m_guid;
    int damage = static_cast<int>(static_cast<float>(baseDamage) / level / coefficient);
    if (m_armorDamageProof > 0) {
        static_cast<App*>(WindowApp::m_instance)->m_soundManager->PlaySound("IDM_ARMOR_HIT", false, false);
        int previousArmor = m_armorDamageProof;
        m_armorDamageProof -= damage;
        if (m_armorDamageProof >= 0) {
            static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->PlayInjureEffect(unit, true);
            m_gameStats.RegisterArmorConsumeDamage(damage);
            return;
        }
        m_gameStats.RegisterArmorConsumeDamage(previousArmor);
        damage = -m_armorDamageProof;
        m_armorDamageProof = 0;
    }
    m_hp -= damage;
    if (m_hp < 0) {
        m_hp = 0;
    }
    Save();
    static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->PlayInjureEffect(unit, false);
    static_cast<App*>(WindowApp::m_instance)->m_soundManager->PlaySound("IDM_CHARACTER_ENEMY_WOUND_MAN_01", false, false);
}

bool CBH_Player::IsArmorAvailable() const
{
    return m_armorDamageProof > 0;
}

void CBH_Player::AddMedkits(int amount)
{
    m_medkits += amount;
    Save();
}

bool CBH_Player::CanRestoreHealth()
{
    return m_medkits > 0;
}

void CBH_Player::RestoreHealth()
{
    if (m_medkits <= 0) {
        return;
    }
    int amount = static_cast<App*>(WindowApp::m_instance)->m_helper->m_config.GetIntegerValueByToken("MEDKIT_HP_RESTORE", true);
    AddHp(amount);
    --m_medkits;
    CGameAnalytics::logItemUsed(XString("00_medkit"));
    m_gameStats.AddMedkitsUsed(1);
    static_cast<App*>(WindowApp::m_instance)->m_soundManager->PlaySound("IDM_SPECIAL_AP_MEDKIT", false, false);
    WindowApp::HandleTunnelCommand(0x48a1ff19, amount, 0, 0);
    Save();
}

void CBH_Player::AddHp(int amount)
{
    // the original compares the new value and asks GetMaxHp() again for the clamp
    int hp = m_hp + amount;
    m_hp = hp;
    if (hp > GetMaxHp()) {
        m_hp = GetMaxHp();
    }
    Save();
}

int CBH_Player::GetPillsCount() const
{
    return m_speedModeAvailableTimeMS / 10000;
}

void CBH_Player::AddGoPills(int amount)
{
    if (amount > 0) {
        m_speedModeAvailableTimeMS += amount * 10000;
        Save();
    }
}

bool CBH_Player::IsSpeedModeAvailable() const
{
    return m_speedModeAvailableTimeMS > 0 || m_reactixControlTime > 0;
}

int CBH_Player::GetRemainingPillCount() const
{
    return m_speedModeAvailableTimeMS / 10000;
}

bool CBH_Player::IsSpeedMode() const
{
    return m_speedTimeMS > 0 || m_consumingReactixControl;
}

int CBH_Player::GetSpeedTimeMS() const
{
    return m_speedTimeMS;
}

void CBH_Player::ConsumeGoPill()
{
    if (m_speedTimeMS < 1) {
        App* app = static_cast<App*>(WindowApp::m_instance);
        app->m_soundManager->PlaySound("IDM_SLOW_START", false, false);
    }
    if (m_speedModeAvailableTimeMS >= 10000) {
        m_speedTimeMS += 10000;
        m_speedModeAvailableTimeMS -= 10000;
    } else {
        m_speedTimeMS += m_speedModeAvailableTimeMS;
        m_speedModeAvailableTimeMS = 0;
    }
    m_gameStats.RegisterPillAddTime(10000);
    Save();
    CGameAnalytics::logReactixUsed(static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_missionId, m_level);
}

void CBH_Player::SetArmor(int id, int damageProof)
{
    m_armorId = id;
    m_armorDamageProof = damageProof * 1000;
    m_armorDamageProofMax = damageProof * 1000;
}

void CBH_Player::AddRevive(int amount)
{
    m_reviveCount += amount;
    Save();
}

bool CBH_Player::CanRevive()
{
    return m_reviveCount > 0;
}

void CBH_Player::Revive()
{
    // the original does not check the count and does not save here
    AddHp(GetMaxHp());
    m_reviveCount--;
    if (m_reviveCount < 0) {
        m_reviveCount = 0;
    }
    CGameAnalytics::logItemUsed(XString("phoenix_serum"));
    ++m_challengeStats->m_revives;
    ++CBH_Player::GetInstance()->m_gameStats.m_resurrects;
    m_field39 = true;
    if (CSwerveGame::m_pInstance != NULL) {
        CSwerveGame::m_pInstance->m_hud->SetFireHeld(false);
    }
}

void CBH_Player::ReleaseResources()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_surfaceManager->ReleaseSurface("IDB_USER_PIC");
    m_avatar = NULL;
}

bool CBH_Player::IsPlayerLevelMax()
{
    return m_level == static_cast<App*>(WindowApp::m_instance)->InitProgressSystem()->GetMaxLevel();
}

int CBH_Player::GetXpForNextLvl()
{
    // the original asks for the current level's XP at the maximum level, else for the next one
    if (m_level == static_cast<App*>(WindowApp::m_instance)->InitProgressSystem()->GetMaxLevel()) {
        return static_cast<App*>(WindowApp::m_instance)->InitProgressSystem()->GetMaxXPForLevel(m_level);
    }
    return static_cast<App*>(WindowApp::m_instance)->InitProgressSystem()->GetMaxXPForLevel(m_level + 1);
}

int CBH_Player::GetXpForCurrentLvl()
{
    return static_cast<App*>(WindowApp::m_instance)->InitProgressSystem()->GetMaxXPForLevel(m_level);
}

int CBH_Player::AddXp(int amount, EGainSource source)
{
    if (amount < 0) {
        return 0;
    }
    if (source != GAIN_SOURCE_IAP) {
        // the original does not check the override manager for NULL
        COverrideManager* manager = static_cast<App*>(WindowApp::m_instance)->m_overrideManager;
        int multiplier;
        {
            // the original releases both strings before testing the multiplier
            XString group(L"");
            if (source == GAIN_SOURCE_CHALLENGE) {
                group = L"challenge_mult";
            } else {
                group = L"missions_mult";
            }
            XString name(L"xp");
            multiplier = GetOverride(*manager, group, name);
        }
        if (multiplier > 100) {
            amount = multiplier * amount / 100;
        }
    }

    m_experience += amount;
    const int level = static_cast<App*>(WindowApp::m_instance)->InitProgressSystem()->GetLevel(m_experience);
    const int maximum = static_cast<App*>(WindowApp::m_instance)->InitProgressSystem()->GetMaxXP();
    if (level > m_level) {
        m_level = level;
        m_energy = static_cast<App*>(WindowApp::m_instance)->InitProgressSystem()->GetMaxEnergyForLevel(m_level);
        m_hp = GetMaxHpForLevel(m_level);
        CGameAnalytics::logLevelGained(level, m_cash, m_credits);
    }
    if (amount > 0) {
        CGameAnalytics::logXPGain(amount, static_cast<App*>(WindowApp::m_instance)->InitProgressSystem()->GetMaxXPForLevel(level) - m_experience);
    }
    if (m_experience > maximum) {
        m_experience = maximum;
    }
    if (amount > 0) {
        Save();
    }
    return amount;
}

void CBH_Player::OnEnemyKilled()
{
    // the original reads WindowApp::m_instance for each access
    static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->OnEnemyKilled();
    if (static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->m_multiplier == 3) {
        m_gameStats.AddX3MultiplierKills(1);
    }
    if (static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->m_multiplier == 2) {
        m_gameStats.AddX2MultiplierKills(1);
    }
}

void CBH_Player::OnEnemyTranquillized()
{
    // the original reads WindowApp::m_instance for each access
    static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->OnEnemyTranqullized();
    if (static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->m_multiplier == 3) {
        m_gameStats.AddX3MultiplierKills(1);
    }
    if (static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->m_multiplier == 2) {
        m_gameStats.AddX2MultiplierKills(1);
    }
}

void CBH_Player::OnEnemyEscaped()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_huntingInfo->OnEnemyEscaped();
}

void CBH_Player::OnMissionFinished(int id)
{
    // the original looks the mission up and discards the result
    static_cast<App*>(WindowApp::m_instance)->m_missionManager->GetMissionById(id);
    if (!IsMissionCompleted(id)) {
        const SCompletedMissionInfo completion = { id, false };
        m_completedMissions.addElement(completion);
        Save();
    }
}

void CBH_Player::SetPlayerStartParams()
{
    m_cash = static_cast<App*>(WindowApp::m_instance)->m_helper->m_config.GetIntegerValueByToken("PLAYER_START_MONEY", true);
    m_credits = static_cast<App*>(WindowApp::m_instance)->m_helper->m_config.GetIntegerValueByToken("PLAYER_START_CREDITS", true);
    for (CDH_Weapon* weapon = static_cast<CDH_Weapon*>(static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->firstItem());
         weapon != NULL;
         weapon = static_cast<CDH_Weapon*>(static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->nextItem())) {
        weapon->FlushAllUpgrades();
        weapon->setAvailable(false);
    }
    for (const CAmmo* ammo = static_cast<const CAmmo*>(static_cast<App*>(WindowApp::m_instance)->InitAmmoManager()->firstItem());
         ammo != NULL;
         ammo = static_cast<const CAmmo*>(static_cast<App*>(WindowApp::m_instance)->InitAmmoManager()->nextItem())) {
        m_ammoCounts.Set(HashTableKey<const CAmmo*>(ammo), 0);
    }
    const int id = static_cast<App*>(WindowApp::m_instance)->m_helper->m_config.GetIntegerValueByToken("PLAYER_START_WEAPON_1_WID", true);
    const int packs = static_cast<App*>(WindowApp::m_instance)->m_helper->m_config.GetIntegerValueByToken("PLAYER_START_WEAPON_1_AMMO_PACKS", true);
    static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->GetWeaponById(id)->setAvailable(true);
    App* app = static_cast<App*>(WindowApp::m_instance);
    GamePlayManager& firstSlot = app->m_helper->m_gamePlayManager;
    firstSlot.SetWeapon01(app->InitWeaponManager()->GetWeaponById(id)->m_id);
    app = static_cast<App*>(WindowApp::m_instance);
    GamePlayManager& secondSlot = app->m_helper->m_gamePlayManager;
    secondSlot.SetWeapon02(app->InitWeaponManager()->GetWeaponById(id)->m_id);
    SetAmmoForWeaponId(id, packs);
}

bool CBH_Player::IsMissionCompleted(int id)
{
    for (int index = 0; index < m_completedMissions.size(); ++index) {
        if (m_completedMissions[index].m_id == id) {
            return true;
        }
    }
    return false;
}

int CBH_Player::GetEnergyCooldownTimeLeft()
{
    int seconds = (GetMaxEnergy() - m_energy) * static_cast<int>(m_energyRestoreSeconds);
    if (seconds <= 0) {
        return 0;
    }
    return seconds;
}

int CBH_Player::GetMaxEnergyForLevel(int level)
{
    // the original reads WindowApp::m_instance for each call
    if (static_cast<App*>(WindowApp::m_instance)->InitProgressSystem() == NULL) {
        return 0;
    }
    return static_cast<App*>(WindowApp::m_instance)->InitProgressSystem()->GetMaxEnergyForLevel(level);
}

int CBH_Player::GetMaxHpForLevel(int level)
{
    // the original reads WindowApp::m_instance for each value
    const int base = static_cast<App*>(WindowApp::m_instance)->m_helper->m_config.GetIntegerValueByToken("BASE_PLAYER_HP", true);
    const int perLevel = static_cast<App*>(WindowApp::m_instance)->m_helper->m_config.GetIntegerValueByToken("PLAYER_HP_PER_LEVEL_GROWTH", true);
    return base + perLevel * Nonnegative(level - 1);
}

void CBH_Player::SaveStats(TiXmlElement* element)
{
    element->SetAttribute("hp", m_hp);
    element->SetAttribute("energy", m_energy);
    element->SetAttribute("medkits", m_medkits);
    element->SetAttribute("revive", m_reviveCount);
    element->SetAttribute("hunt_credits", m_credits);
    element->SetAttribute("xp", m_experience);
    element->SetAttribute("points", m_cash);
    element->SetAttribute("level", m_level);
    element->SetAttribute("tutorial_state", m_tutorialState);
    element->SetAttribute("last_daily_bonus_time", m_lastDailyBonusTime);
    element->SetAttribute("daily_bonus_row", m_dailyBonusesInRow);
    element->SetAttribute("last_five_days_bonus_time", m_lastFiveDaysBonusTime);
    element->SetAttribute("active_five_days_bonus_day", m_activeFiveDaysBonusDay);
    element->SetAttribute("current_friend_gated_mission_id", m_currentFriendGatedMissionId);
    element->SetAttribute("speedModeAvailableTimeMS", m_speedModeAvailableTimeMS);
    element->SetAttribute("armorID", m_armorId);
    element->SetAttribute("armorDamageProof", m_armorDamageProof);
    element->SetAttribute("armorDamageProofMax", m_armorDamageProofMax);
    element->SetAttribute("last_hunt_item", m_lastHuntItem);
    element->SetAttribute("last_hunt_result", m_lastHuntResult);
    element->SetAttribute("app_rated", m_appRated);
    element->SetAttribute("last_energy_update_time", m_lastEnergyUpdateTime);
    element->SetAttribute("last_game_version_played", m_lastGameVersionPlayed);
    element->SetAttribute("silentShotsCount", m_silentShotsCount);
    element->SetAttribute("cyberScopesCount", m_cyberScopesCount);
    element->SetAttribute("reactixControlTime", m_reactixControlTime);
    element->SetAttribute("isHitechVisorEquiped", m_hitechVisorEquipped);
    element->SetAttribute("lastFiveDaysIncentiveBonusTimestamp", m_lastFiveDaysIncentiveBonusTimestamp);
    element->SetAttribute("activeFiveDaysIncentiveBonusDay", m_activeFiveDaysIncentiveBonusDay);
    element->SetAttribute("fiveDayIncentiveBonusForbidden", m_fiveDayIncentiveBonusForbidden);
    element->SetAttribute("ReceivedFiveDaysIncentiveBonus", m_receivedFiveDaysIncentiveBonus);
    element->SetAttribute("FiveDaysIncentiveBonusExpirationMessageShown", m_incentiveBonusExpirationShown);
    element->SetAttribute("last_mission_version_played", m_lastMissionVersionPlayed);
    element->SetAttribute("ultrasoundAttemptCount", m_ultrasoundAttemptCount);
    CDH_Weapon* first = static_cast<App*>(WindowApp::m_instance)->m_helper->m_gamePlayManager.GetGameData()->GetWeapon01(false);
    if (first != NULL) {
        element->SetAttribute("weapon_slot1", first->m_id);
    }
    CDH_Weapon* second = static_cast<App*>(WindowApp::m_instance)->m_helper->m_gamePlayManager.GetGameData()->GetWeapon02(false);
    if (second != NULL) {
        element->SetAttribute("weapon_slot2", second->m_id);
    }
}

void CBH_Player::SaveReputation(TiXmlElement* element)
{
    // the original writes the loop out here (an inlined helper keeps cmp+beq instead of cbz)
    TiXmlElement* section = CXmlHelper::ObtainElement(element, XString("reputation"));
    section->Clear();
    for (int index = 0; index < m_reputation.m_count; ++index) {
        TiXmlElement* child = new TiXmlElement("fraction");
        XString* name;
        const int value = *m_reputation.GetAt(index, &name);
        child->SetStringAttribute("name", XString::AnsiString(name->c_str()));
        child->SetAttribute("reputation", value);
        // the original inserts a copy and deletes the temporary element
        section->InsertEndChild(*child);
        delete child;
    }
}

void CBH_Player::SaveMissionInfo(TiXmlElement* element)
{
    TiXmlElement* section = CXmlHelper::ObtainElement(element, XString("completed_missions"));
    section->Clear();
    for (int index = 0; index < m_completedMissions.size(); ++index) {
        SCompletedMissionInfo completion = m_completedMissions[index];
        TiXmlElement* child = new TiXmlElement("completed_mission");
        child->SetAttribute("id", completion.m_id);
        child->SetAttribute("tranq", completion.m_tranquillized);
        // the original inserts a copy and deletes the temporary element
        section->InsertEndChild(*child);
        delete child;
    }
}

void CBH_Player::SaveAmmo(TiXmlElement* element)
{
    TiXmlElement* section = CXmlHelper::ObtainElement(element, XString("ammo_info"));
    section->Clear();
    for (int index = 0; index < m_ammoCounts.m_count; ++index) {
        TiXmlElement* child = new TiXmlElement("ammo");
        HashTableKey<const CAmmo*>* key;
        m_ammoCounts.GetAt(index, &key);
        const CAmmo* ammo = key->m_value;
        XString type = ammo->m_ammoType;
        child->SetStringAttribute("type", XString::AnsiString(type.c_str()));
        child->SetAttribute("count", GetAvailableAmmo(ammo, false));
        section->InsertEndChild(*child);
        delete child;
    }
}

void CBH_Player::SaveTjPoints(TiXmlElement* element)
{
    TiXmlElement* section = CXmlHelper::ObtainElement(element, XString("tapjoy_info"));
    section->Clear();
    for (int index = 0; index < m_tapjoyPoints.m_count; ++index) {
        TiXmlElement* child = new TiXmlElement("tapjoy_points");
        XString* name;
        const int points = *m_tapjoyPoints.GetAt(index, &name);
        child->SetStringAttribute("devId", XString::AnsiString(name->c_str()));
        child->SetAttribute("points", points);
        // the original inserts a copy and deletes the temporary element
        section->InsertEndChild(*child);
        delete child;
    }
}

void CBH_Player::SaveFriendGiftLasDay(TiXmlElement* element)
{
    if (element == NULL) {
        return;
    }
    TiXmlElement* section = CXmlHelper::ObtainElement(element, XString("friendGiftsInfo"));
    section->Clear();
    for (int index = 0; index < m_friendGiftDays.m_count; ++index) {
        TiXmlElement* child = new TiXmlElement("friendGift");
        XString* name;
        const int day = *m_friendGiftDays.GetAt(index, &name);
        child->SetAttribute("socialID", name->ToInt());
        child->SetAttribute("lastDay", day);
        // the original inserts a copy and deletes the temporary element
        section->InsertEndChild(*child);
        delete child;
    }
}

void CBH_Player::LoadStats(TiXmlElement* element)
{
    m_hp = CXmlHelper::GetAttributeValue(element, "hp").ToInt();
    m_energy = CXmlHelper::GetAttributeValue(element, "energy").ToInt();
    m_medkits = CXmlHelper::GetAttributeValue(element, "medkits").ToInt();
    m_credits = CXmlHelper::GetAttributeValue(element, "hunt_credits").ToInt();
    m_experience = CXmlHelper::GetAttributeValue(element, "xp").ToInt();
    m_cash = CXmlHelper::GetAttributeValue(element, "points").ToInt();
    m_level = CXmlHelper::GetAttributeValue(element, "level").ToInt();
    m_tutorialState = CXmlHelper::GetAttributeValue(element, "tutorial_state").ToInt();
    m_lastDailyBonusTime = CXmlHelper::GetAttributeValue(element, "last_daily_bonus_time").ToInt();
    m_dailyBonusesInRow = CXmlHelper::GetAttributeValue(element, "daily_bonus_row").ToInt();
    m_lastHuntItem = CXmlHelper::GetAttributeValue(element, "last_hunt_item").ToInt();
    m_lastHuntResult = CXmlHelper::GetAttributeValue(element, "last_hunt_result").ToInt();
    m_appRated = CXmlHelper::GetAttributeValue(element, "app_rated").ToBool();
    m_lastEnergyUpdateTime = CXmlHelper::GetAttributeValue(element, "last_energy_update_time").ToInt();
    m_silentShotsCount = CXmlHelper::GetAttributeValueOrUseDefault(element, "silentShotsCount", XString("0")).ToInt();
    m_cyberScopesCount = CXmlHelper::GetAttributeValueOrUseDefault(element, "cyberScopesCount", XString("0")).ToInt();
    m_reactixControlTime = CXmlHelper::GetAttributeValueOrUseDefault(element, "reactixControlTime", XString("0")).ToInt();
    m_hitechVisorEquipped = CXmlHelper::GetAttributeValueOrUseDefault(element, "isHitechVisorEquiped", XString("0")).ToInt() != 0;
    m_lastGameVersionPlayed = CXmlHelper::GetAttributeValueOrUseDefault(element, "last_game_version_played", XString(L"100")).ToInt();
    m_lastFiveDaysBonusTime = CXmlHelper::GetAttributeValueOrUseDefault(element, "last_five_days_bonus_time", XString(L"0")).ToInt();
    m_activeFiveDaysBonusDay = CXmlHelper::GetAttributeValueOrUseDefault(element, "active_five_days_bonus_day", XString(L"0")).ToInt();
    m_speedModeAvailableTimeMS = CXmlHelper::GetAttributeValueOrUseDefault(element, "speedModeAvailableTimeMS", XString(L"0")).ToInt();
    m_ultrasoundAttemptCount = CXmlHelper::GetAttributeValueOrUseDefault(element, "ultrasoundAttemptCount", XString(L"0")).ToInt();
    m_armorId = CXmlHelper::GetAttributeValueOrUseDefault(element, "armorID", XString(L"0")).ToInt();
    m_armorDamageProof = CXmlHelper::GetAttributeValueOrUseDefault(element, "armorDamageProof", XString(L"0")).ToInt();
    m_armorDamageProofMax = CXmlHelper::GetAttributeValueOrUseDefault(element, "armorDamageProofMax", XString(L"0")).ToInt();
    int firstId = CXmlHelper::GetAttributeValue(element, "weapon_slot1").ToInt();
    CItem* first = static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->getItemById(firstId);
    if (first != NULL) {
        static_cast<App*>(WindowApp::m_instance)->m_helper->m_gamePlayManager.SetWeapon01(first->m_id);
    }
    int secondId = CXmlHelper::GetAttributeValue(element, "weapon_slot2").ToInt();
    CItem* second = static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->getItemById(secondId);
    if (second != NULL) {
        static_cast<App*>(WindowApp::m_instance)->m_helper->m_gamePlayManager.SetWeapon02(second->m_id);
    }
    if (m_credits < 0) {
        m_credits = 0;
    }
    m_currentFriendGatedMissionId = CXmlHelper::GetAttributeValueOrUseDefault(element, "current_friend_gated_mission_id", XString(L"0")).ToInt();
    m_lastFiveDaysIncentiveBonusTimestamp = CXmlHelper::GetAttributeValueOrUseDefault(element, "lastFiveDaysIncentiveBonusTimestamp", XString(L"0")).ToInt();
    m_activeFiveDaysIncentiveBonusDay = CXmlHelper::GetAttributeValueOrUseDefault(element, "activeFiveDaysIncentiveBonusDay", XString(L"0")).ToInt();
    m_fiveDayIncentiveBonusForbidden = CXmlHelper::GetAttributeValueOrUseDefault(element, "fiveDayIncentiveBonusForbidden", XString(L"0")).ToBool();
    m_reviveCount = CXmlHelper::GetAttributeValueOrUseDefault(element, "revive", XString(L"0")).ToInt();
    m_receivedFiveDaysIncentiveBonus = CXmlHelper::GetAttributeValueOrUseDefault(element, "ReceivedFiveDaysIncentiveBonus", XString(L"0")).ToBool();
    m_incentiveBonusExpirationShown = CXmlHelper::GetAttributeValueOrUseDefault(element, "FiveDaysIncentiveBonusExpirationMessageShown", XString(L"0")).ToBool();
    if (m_lastGameVersionPlayed >= 101 && m_lastGameVersionPlayed < 140) {
        m_fiveDayIncentiveBonusForbidden = true;
    }
    m_lastMissionVersionPlayed = CXmlHelper::GetAttributeValueOrUseDefault(element, "last_mission_version_played", XString(L"100")).ToInt();
}

void CBH_Player::LoadMissionInfo(TiXmlElement* element)
{
    const TiXmlElement* section = element->FirstChildElement("completed_missions");
    if (section == NULL) {
        return;
    }
    // the original looks up the first child before clearing the list
    const TiXmlElement* child = section->FirstChildElement("completed_mission");
    m_completedMissions.removeAllElements();
    for (; child != NULL; child = child->NextSiblingElement("completed_mission")) {
        int id = CXmlHelper::GetAttributeValue(child, "id").ToInt();
        bool tranquillized = CXmlHelper::GetAttributeValue(child, "tranq").ToBool();
        SCompletedMissionInfo completion;
        completion.m_id = id;
        completion.m_tranquillized = tranquillized;
        m_completedMissions.addElement(completion);
    }
}

void CBH_Player::LoadAmmo(TiXmlElement* element)
{
    const TiXmlElement* section = element->FirstChildElement("ammo_info");
    if (section != NULL) {
        const TiXmlElement* child = section->FirstChildElement("ammo");
        while (child != NULL) {
            XString type = CXmlHelper::GetAttributeValue(child, "type");
            int count = CXmlHelper::GetAttributeValue(child, "count").ToInt();
            const CAmmo* ammo = static_cast<App*>(WindowApp::m_instance)->InitAmmoManager()->GetAmmoByType(type);
            m_ammoCounts.Set(HashTableKey<const CAmmo*>(ammo), count);
            child = child->NextSiblingElement("ammo");
        }
    }
}

void CBH_Player::LoadAvatar(TiXmlElement* element)
{
    int id = CXmlHelper::GetAttributeValue(element, "avatar_id").ToInt();
    m_avatar = static_cast<App*>(WindowApp::m_instance)->InitAvatarManager()->GetAvatar(id);
}

void CBH_Player::LoadTjPoints(TiXmlElement* element)
{
    const TiXmlElement* section = element->FirstChildElement("tapjoy_info");
    if (section != NULL) {
        const TiXmlElement* child = section->FirstChildElement("tapjoy_points");
        while (child != NULL) {
            // the original releases the name only after stepping to the next sibling
            XString name = CXmlHelper::GetAttributeValue(child, "devId");
            int points = CXmlHelper::GetAttributeValue(child, "points").ToInt();
            m_tapjoyPoints.Set(name, points);
            child = child->NextSiblingElement("tapjoy_points");
        }
    }
}

void CBH_Player::LoadFriendGiftLasDay(TiXmlElement* element)
{
    if (element == NULL) {
        return;
    }
    const TiXmlElement* section = element->FirstChildElement("friendGiftsInfo");
    if (section == NULL) {
        return;
    }
    const TiXmlElement* child = section->FirstChildElement("friendGift");
    while (child != NULL) {
        XString guid = CXmlHelper::GetAttributeValueOrUseDefault(child, "socialUserGUID", XString(""));
        int day = CXmlHelper::GetAttributeValue(child, "lastDay").ToInt();
        int id = -1;
        if (static_cast<unsigned int>(guid.ByteSize()) >= sizeof(wchar_t)) {
            CNGSRemoteUserList* friends = CNGS::GetInstance()->GetLocalUser()->m_remoteUserLists[0];
            for (int index = 0; index < friends->m_users.Size(); ++index) {
                CNGSRemoteUser* user = friends->m_users[index];
                if (user != NULL) {
                    XString friendGuid(user->GetIDForSocialNetwork(SOCIAL_NETWORK_FACEBOOK).GetWCharPtr());
                    if (friendGuid == guid) {
                        id = user->GetClientID();
                    }
                }
            }
        } else {
            id = CXmlHelper::GetAttributeValueOrUseDefault(child, "socialID", XString("-1")).ToInt();
        }
        if (m_friendGiftDays.Get(XString(id)) == NULL && id != -1) {
            m_friendGiftDays.Set(XString(id), day);
        }
        child = child->NextSiblingElement("friendGift");
    }
}

CBH_Player::~CBH_Player()
{
    m_ammoCounts.Clear();
    if (m_challengeStats != NULL) {
        delete m_challengeStats;
        m_challengeStats = NULL;
    }
}

int CBH_Player::GetReputation(const XString& name)
{
    const int* reputation = m_reputation.Get(name);
    if (reputation != NULL) {
        return *reputation;
    }
    return 0;
}

int CBH_Player::GetReputationLimit()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    return app->m_helper->m_config.GetIntegerValueByToken("REPUTATION_LIMIT", true);
}

void CBH_Player::SetAvatar(const CAvatar* avatar)
{
    m_avatar = avatar;
    Save();
}

void CBH_Player::GamePlayTick(int elapsed)
{
    m_speedTimeMS = Nonnegative(m_speedTimeMS - elapsed);
    bool speedActive = IsSpeedMode();
    CSwerveGame* game = CSwerveGame::m_pInstance;
    bool ultrasoundActive = false;
    if (game != NULL && game->m_stunMs >= 1) {
        ultrasoundActive = true;
    }
    if (m_speedModeWasActive && !speedActive) {
        static_cast<App*>(WindowApp::m_instance)->m_soundManager->PlaySound("IDM_SLOW_END", false, false);
    }
    if (m_ultrasoundWasActive && !ultrasoundActive) {
        static_cast<App*>(WindowApp::m_instance)->m_soundManager->PlaySound("IDM_ULTRASOUND_END", false, false);
    }
    m_speedModeWasActive = speedActive;
    m_ultrasoundWasActive = ultrasoundActive;
    if (m_consumingReactixControl) {
        m_reactixControlTime -= elapsed;
        m_gameStats.RegisterPillAddTime(elapsed);
        Save();
        if (m_reactixControlTime <= 0) {
            m_reactixControlTime = 0;
            m_consumingReactixControl = false;
            m_speedTimeMS = 0;
            Save();
        }
    }
}

void CBH_Player::StopConsumeReactixControl()
{
    m_consumingReactixControl = false;
    m_speedTimeMS = 0;
    Save();
}

int CBH_Player::GetFriendGiftLastDay(int socialId)
{
    const int* day = m_friendGiftDays.Get(XString(socialId));
    if (day != NULL) {
        return *day;
    }
    return -1;
}

void CBH_Player::SetFriendGiftLastDay(int socialId, int day)
{
    m_friendGiftDays.Set(XString(socialId), day);
    Save();
}

void CBH_Player::CancelFriendInvitation(int socialId)
{
    int index = m_friendInvitations.indexOf(socialId);
    if (index >= 0) {
        m_friendInvitations.removeElementAt(index);
        Save();
    }
}

int CBH_Player::GetCurrentFriendGatedMissionID() const
{
    return m_currentFriendGatedMissionId;
}

void CBH_Player::SetNextFriendGatedMissionID()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    int missionCount = app->m_friendMissionManager->MissionCount();
    if (missionCount > 0) {
        m_currentFriendGatedMissionId = (m_currentFriendGatedMissionId + 1) % missionCount;
    }
}

void CBH_Player::SetEstimatedWeapon(CDH_Weapon* weapon, int count)
{
    m_estimatedWeapon = weapon;
    m_estimatedAmmo = count;
}

bool CBH_Player::IsEnegyEnabled()
{
    COverrideManager* manager = static_cast<App*>(WindowApp::m_instance)->m_overrideManager;
    if (manager == NULL) {
        return m_energyEnabled;
    }
    CStrWChar group("energy_disable");
    CStrWChar key("disable");
    unsigned long long disabled = 0;
    if (manager->FindIntAttribute(group, key, &disabled)) {
        m_energyEnabled = disabled != 1;
    }
    return m_energyEnabled;
}

void CBH_Player::AddCredits(int amount)
{
    m_credits = Nonnegative(m_credits + amount);
    Save();
}

void CBH_Player::ReduceCredits(int amount)
{
    m_credits = Nonnegative(m_credits - amount);
    Save();
}

void CBH_Player::ReduceEnergy(int amount)
{
    if (IsEnegyEnabled()) {
        m_energy = Nonnegative(m_energy - amount);
        Save();
    }
}

void CBH_Player::ReduceSilentShotsCount(int count)
{
    m_silentShotsCount = Nonnegative(m_silentShotsCount - count);
    Save();
}

void CBH_Player::AddReactixControlTimeSec(int seconds)
{
    m_reactixControlTime += seconds * 1000;
}

int CBH_Player::GetReactixControlTimeSec()
{
    return m_reactixControlTime / 1000;
}

void CBH_Player::StartConsumeReactixControl()
{
    if (m_reactixControlTime == 0 || m_consumingReactixControl) {
        return;
    }
    if (m_speedTimeMS < 1) {
        App* app = static_cast<App*>(WindowApp::m_instance);
        app->m_soundManager->PlaySound("IDM_SLOW_START", false, false);
    }
    m_speedTimeMS += m_reactixControlTime;
    m_consumingReactixControl = true;
    Save();
}

void CBH_Player::ConsumeUltrasoundEmitter()
{
    if (m_ultrasoundAttemptCount > 0) {
        m_ultrasoundAttemptCount--;
    } else {
        m_ultrasoundAttemptCount = 0;
    }
    CGameAnalytics::logItemUsed(XString("untrasound_emitter"));
    CSwerveGame* game = CSwerveGame::m_pInstance;
    if (game != NULL && game->m_stunMs < 1) {
        App* app = static_cast<App*>(WindowApp::m_instance);
        app->m_soundManager->PlaySound("IDM_ULTRASOUND_START", false, false);
    }
    Save();
}

void CBH_Player::AddUltrasoundEmitterAttempt(int count)
{
    m_ultrasoundAttemptCount += count;
    Save();
}

void CBH_Player::AddCyberScopeCount(int count)
{
    m_cyberScopesCount += count;
    Save();
}

void CBH_Player::ReduceCyberScopeCount(int count)
{
    m_cyberScopesCount = Nonnegative(m_cyberScopesCount - count);
    Save();
}

void CBH_Player::AddSilentShotsCount(int count)
{
    m_silentShotsCount += count;
    Save();
}

void CBH_Player::MissionCompleted()
{
    ++m_completedMissionCount;
}

void CBH_Player::Save(bool force)
{
    Save();
    CSaveManager* save = static_cast<App*>(WindowApp::m_instance)->m_saveManager;
    if (force && save != NULL && save->m_pending) {
        save->SaveOnNextUpdate(true);
    }
}

void CBH_Player::SetTutorialState(int state)
{
    m_tutorialState = state;
    Save();
}
