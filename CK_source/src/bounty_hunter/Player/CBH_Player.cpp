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
#include "CHuntingInfo.h"
#include "COverrideManager.h"
#include "CProgressSystem.h"
#include "CSaveManager.h"
#include "CStdUtil_Cocoa.h"
#include "CSwerveGame.h"
#include "CTutorialManager.h"
#include "CXmlHelper.h"
#include "DGHelper.h"
#include "GameCenter_Facade.h"

typedef char CBH_Player_ARMv7_Size[(sizeof(CBH_Player) == 0x28c) ? 1 : -1];

namespace {
    int ReadInt(const TiXmlElement* element, const char* name, int defaultValue = 0)
    {
        const char* value = element->Attribute(name);
        if (value != NULL) {
            return XString(value).ToInt();
        }
        return defaultValue;
    }

    void WriteNamedCounters(TiXmlElement* parent, const char* section, const char* child,
                            const char* keyAttribute, const char* valueAttribute,
                            const HashTable<XString, int>& values)
    {
        TiXmlElement* container = CXmlHelper::ObtainElement(parent, XString(section));
        container->Clear();
        for (int index = 0; index < values.m_count; ++index) {
            XString* name;
            const int count = *values.GetAt(index, &name);
            TiXmlElement* element = new TiXmlElement(child);
            element->SetStringAttribute(keyAttribute, XString::AnsiString(name->c_str()));
            element->SetAttribute(valueAttribute, count);
            container->LinkEndChild(element);
        }
    }

    int Nonnegative(int value)
    {
        if (value < 0) {
            return 0;
        }
        return value;
    }

    int& AmmoCount(HashTable<HashTableKey<const CAmmo*>, int>& counts, const CAmmo* ammo)
    {
        const HashTableKey<const CAmmo*> key(ammo);
        int* value = counts.Get(key);
        if (value == NULL) {
            counts.Add(key, 0);
            value = counts.Get(key);
        }
        return *value;
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
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (app->m_saveManager == NULL) {
        return;
    }
    TiXmlElement* root = app->m_saveManager->GetRootElement();
    TiXmlElement* player = root->FirstChildElement("player");
    if (player == NULL) {
        SetPlayerStartParams();
        return;
    }
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
    app->InitAchievementManager()->Load(player);

    const TiXmlElement* reputation = player->FirstChildElement("reputation");
    if (reputation != NULL) {
        for (const TiXmlElement* entry = reputation->FirstChildElement("fraction"); entry != NULL;
             entry = entry->NextSiblingElement("fraction")) {
            const XString name = CXmlHelper::GetAttributeValueOptional(*entry, "name");
            m_reputation.Set(name, ReadInt(entry, "reputation"));
        }
    }
}

void CBH_Player::Update()
{
    const unsigned int now = CStdUtil_Cocoa::GetTimeSeconds();
    unsigned int elapsed = 0;
    if (m_lastEnergyUpdateTime < now) {
        elapsed = now - m_lastEnergyUpdateTime;
        if (m_energyRestoreSeconds != 0 && m_energyRestoreSeconds < elapsed) {
            AddEnergy(elapsed / m_energyRestoreSeconds, false);
            m_lastEnergyUpdateTime = now;
            Save();
        }
    } else {
        m_lastEnergyUpdateTime = now;
    }
    if (elapsed <= m_energyRestoreSeconds) {
        m_energyRemainingSeconds = m_energyRestoreSeconds - elapsed;
    }

    if (!m_playerNameReady) {
        GameCenter_Facade* gameCenter = GameCenter_Facade::GetInstance();
        if (gameCenter->isGameCenterSupported()) {
            const int state = gameCenter->GetAuthenticationState();
            if (state == GameCenter_Facade::RequestState_Done) {
                CStrWChar name("player");
                gameCenter->GetLocalPLayerName(name);
                m_playerName = XString(name.GetWCharPtr()).Upper();
                m_playerNameReady = true;
            } else if ((state == 0 || state == GameCenter_Facade::RequestState_Failed) && m_authenticationStarted) {
                m_playerName = XString(L"PLAYER");
                m_playerNameReady = true;
            }
        }
    }
}

void CBH_Player::AddEnergy(int amount, bool allowOverflow)
{
    if (allowOverflow) {
        m_energy += amount;
    } else {
        const int maximum = GetMaxEnergy();
        if (m_energy <= maximum) {
            m_energy += amount;
            if (m_energy > maximum) {
                m_energy = maximum;
            }
        }
    }
    Save();
}

bool CBH_Player::TryMakeFiveDaysBonus(bool* newDay)
{
    *newDay = false;
    if (!m_fiveDayIncentiveBonusForbidden || CTutorialManager::IsPlayerInTutorial()
        || CStdUtil_Cocoa::GetTimeSeconds() == 0xffffffffu) {
        return false;
    }

    const bool online = App::IsNetTimeAvailable() == 1;
    const unsigned int now = App::GetTime();
    const int elapsedDays = DGHelper::GetDaysFromTimestamp(now)
                          - DGHelper::GetDaysFromTimestamp(m_lastFiveDaysBonusTime);
    *newDay = elapsedDays != 0;
    if (!online || elapsedDays == 0) {
        return false;
    }
    if (elapsedDays < 0) {
        m_lastFiveDaysBonusTime = now;
        Save();
        return false;
    }
    if (elapsedDays >= 2) {
        m_activeFiveDaysBonusDay = 0;
        m_dailyBonusesInRow = 0;
    }

    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_fiveDaysBonus->GiveBonus(m_activeFiveDaysBonusDay);
    m_lastFiveDaysBonusTime = now;
    ++m_dailyBonusesInRow;
    if (++m_activeFiveDaysBonusDay >= CFiveDaysBonus::DAY_COUNT) {
        m_activeFiveDaysBonusDay = 0;
    }
    Save();
    return true;
}

bool CBH_Player::TryMakeFiveDaysIncentiveBonus()
{
    if (m_fiveDayIncentiveBonusForbidden || CStdUtil_Cocoa::GetTimeSeconds() == 0xffffffffu) {
        return false;
    }

    const bool online = App::IsNetTimeAvailable() == 1;
    const unsigned int now = App::GetTime();
    if (m_lastFiveDaysIncentiveBonusTimestamp == 0) {
        m_lastFiveDaysIncentiveBonusTimestamp = now - 24 * 60 * 60;
    }
    const int elapsedDays = DGHelper::GetDaysFromTimestamp(now)
                          - DGHelper::GetDaysFromTimestamp(m_lastFiveDaysIncentiveBonusTimestamp);
    if (!online || elapsedDays == 0) {
        return false;
    }
    if (elapsedDays < 0 || elapsedDays >= 2) {
        m_fiveDayIncentiveBonusForbidden = true;
        Save();
        return false;
    }

    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_fiveDaysIncentiveBonus->GiveBonus(m_activeFiveDaysIncentiveBonusDay);
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
    App* app = static_cast<App*>(WindowApp::m_instance);
    CSaveManager* save = app->m_saveManager;
    if (save == NULL || save->m_pauseSaving || save->m_pauseGServe == 1) {
        return;
    }
    TiXmlElement* player = CXmlHelper::ObtainElement(save->GetRootElement(), XString("player"));
    SaveStats(player);
    SaveReputation(player);
    SaveMissionInfo(player);
    SaveAmmo(player);
    if (m_avatar != NULL) {
        player->SetAttribute("avatar_id", m_avatar->m_id);
    }
    SaveTjPoints(player);
    SaveFriendGiftLasDay(player);
    m_gameStats.Save(player);
    m_challengeStats->Save(player);
    if (app->m_achievementManager != NULL) {
        app->m_achievementManager->Save(player);
    }
    save->SaveOnNextUpdate(false);
}

void CBH_Player::SetAmmoForWeaponId(int id, int packs)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_Weapon* weapon = app->InitWeaponManager()->GetWeaponById(id);
    if (weapon != NULL && packs > 0) {
        const CAmmo* ammo = weapon->GetActiveAmmo();
        if (ammo != NULL) {
            SetAmmoCount(ammo, ammo->m_countInPack * packs);
        }
    }
}

void CBH_Player::IncAmmo(const CAmmo* ammo, int packs)
{
    if (ammo != NULL && packs > 0) {
        IncAmmoByBulletsCount(ammo, ammo->m_countInPack * packs);
    }
}

void CBH_Player::IncAmmoByBulletsCount(const CAmmo* ammo, int count)
{
    if (ammo != NULL && count > 0) {
        AmmoCount(m_ammoCounts, ammo) += count;
        m_gameStats.AddBulletsEarned(count);
        Save();
    }
}

void CBH_Player::SetAmmoCount(const CAmmo* ammo, int count)
{
    if (ammo != NULL) {
        AmmoCount(m_ammoCounts, ammo) = count;
        Save();
    }
}

int CBH_Player::GetAvailableAmmo(const CAmmo* ammo, bool estimate)
{
    if (ammo == NULL) {
        return 0;
    }
    if (ammo->m_ammoType == XString("infinite")) {
        return 0xfffffff;
    }
    if (estimate && m_estimatedWeapon != NULL && m_estimatedAmmo != -1) {
        const CAmmo* base = m_estimatedWeapon->GetDefaultAmmo();
        if (base != NULL && base->m_id == ammo->m_id) {
            return m_estimatedAmmo;
        }
    }
    const int* count = m_ammoCounts.Get(HashTableKey<const CAmmo*>(ammo));
    if (count != NULL) {
        return *count;
    }
    return 0;
}

void CBH_Player::UseAmmo(const CAmmo* ammo)
{
    if (ammo != NULL) {
        if (m_estimatedWeapon != NULL && m_estimatedAmmo != -1) {
            m_estimatedAmmo = Nonnegative(m_estimatedAmmo - 1);
        } else {
            int& count = AmmoCount(m_ammoCounts, ammo);
            count = Nonnegative(count - 1);
            Save();
        }
    }
}

bool CBH_Player::HasAnyAmmo()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_BasicGameData* data = app->m_helper->GetGameData();
    CDH_Weapon* first = data->GetWeapon01(true);
    CDH_Weapon* second = data->GetWeapon02(true);
    return (first != NULL && GetAvailableAmmo(first->GetActiveAmmo(), true) > 0)
        || (second != NULL && GetAvailableAmmo(second->GetActiveAmmo(), true) > 0);
}

bool CBH_Player::HasAmmoInBothWeapons()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_BasicGameData* data = app->m_helper->GetGameData();
    CDH_Weapon* first = data->GetWeapon01(true);
    CDH_Weapon* second = data->GetWeapon02(true);
    return first != NULL && second != NULL && GetAvailableAmmo(first->GetActiveAmmo(), true) > 0
        && GetAvailableAmmo(second->GetActiveAmmo(), true) > 0;
}

bool CBH_Player::HasTranqWeapon()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    Vector<CDH_Weapon*>& weapons = app->InitWeaponManager()->GetWeapons();
    for (int index = 0; index < weapons.size(); ++index) {
        CDH_Weapon* weapon = weapons[index];
        if (weapon->isAvailable() && weapon->m_tranquillizer) {
            return true;
        }
    }
    return false;
}

bool CBH_Player::HasTranqWeaponWithin()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_BasicGameData* data = app->m_helper->GetGameData();
    CDH_Weapon* first = data->GetWeapon01(true);
    CDH_Weapon* second = data->GetWeapon02(true);
    return (first != NULL && first->m_tranquillizer) || (second != NULL && second->m_tranquillizer);
}

bool CBH_Player::HasAmmoForTranqWeaponWithin()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_BasicGameData* data = app->m_helper->GetGameData();
    CDH_Weapon* weapons[] = {data->GetWeapon01(true), data->GetWeapon02(true)};
    for (int index = 0; index < 2; ++index) {
        CDH_Weapon* weapon = weapons[index];
        if (weapon != NULL && weapon->m_tranquillizer && GetAvailableAmmo(weapon->GetActiveAmmo(), true) > 0) {
            return true;
        }
    }
    return false;
}

bool CBH_Player::HasWeaponWithRange(float range)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    Vector<CDH_Weapon*>& weapons = app->InitWeaponManager()->GetWeapons();
    for (int index = 0; index < weapons.size(); ++index) {
        CDH_Weapon* weapon = weapons[index];
        if (weapon->isAvailable() && weapon->effectiveRange() >= range) {
            return true;
        }
    }
    return false;
}

bool CBH_Player::HasWeaponWithRangeInHands(float range)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_BasicGameData* data = app->m_helper->GetGameData();
    CDH_Weapon* first = data->GetWeapon01(true);
    CDH_Weapon* second = data->GetWeapon02(true);
    return (first != NULL && first->effectiveRange() >= range)
        || (second != NULL && second->effectiveRange() >= range);
}

int CBH_Player::WeaponsWithCategoryCount(CDH_Weapon::EWeaponCategory category)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    int count = 0;
    for (CDH_Weapon* weapon = static_cast<CDH_Weapon*>(app->InitWeaponManager()->firstItem()); weapon != NULL;
         weapon = static_cast<CDH_Weapon*>(app->InitWeaponManager()->nextItem())) {
        if (weapon->isAvailable() == true) {
            count += weapon->m_category == category;
        }
    }
    return count;
}

void CBH_Player::AddMoney(int amount, EGainSource source)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (source != GAIN_SOURCE_IAP && app->m_overrideManager != NULL) {
        XString group(L"missions_mult");
        if (source == GAIN_SOURCE_CHALLENGE) {
            group = XString(L"challenge_mult");
        }
        int multiplier = GetOverride(*app->m_overrideManager, group, XString(L"soft"));
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
    const CStrWChar groupName(group.c_str());
    const CStrWChar attributeName(name.c_str());
    unsigned long long value;
    if (manager.FindIntAttribute(groupName, attributeName, &value)) {
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
    m_field23c = 1;
    if (m_field240 == 0) {
        m_field23c = 2;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    m_challengeStats->RegisterPlayerTakeCover();
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
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_Weapon* weapon = app->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
    if (weapon != NULL) {
        weapon->ResetInScropeRoutine();
    }
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
    App* app = static_cast<App*>(WindowApp::m_instance);
    CSwerveGame* game = app->m_helper->m_swerveGame;
    if (unit == NULL || game->m_finishing || game->IsGamePaused() || game->m_slowMode || game->IsIntroTime() || m_field39) {
        return;
    }
    const CBH_PersonType* person = unit->m_body.m_type;
    const CEnemyWeaponType* weapon = person->m_weapon;
    if (weapon == NULL) {
        return;
    }
    int probability = static_cast<int>(person->m_baseAccuracy * 100.0f);
    if (static_cast<int>(game->GetDistanceToUnit(unit)) > weapon->m_effectiveRange) {
        float coefficient = app->m_helper->m_config.GetFloatValueByToken("ACCURACY_OUT_OF_EFFECTIVE_RANGE_DECREASE", true);
        probability = static_cast<int>(probability * coefficient);
    }
    if (probability > 100) {
        probability = 100;
    } else if (probability < 0) {
        probability = 0;
    }
    if (!CBH_Math::GetProbabiliyOutcome(probability)) {
        app->m_soundManager->PlaySound("IDM_CHARACTER_BULLET_CLOSE", false, false);
        return;
    }
    if (!IsNeedHide()) {
        return;
    }
    float coefficient = app->InitProgressSystem()->GetDmgCoeff();
    if (m_level < 1 || coefficient <= 0.0f) {
        return;
    }
    int damage = static_cast<int>(static_cast<float>(weapon->m_baseDamage) / m_level / coefficient);
    m_fieldc0 = true;
    m_field280 = unit->m_guid;
    if (m_armorDamageProof > 0) {
        app->m_soundManager->PlaySound("IDM_ARMOR_HIT", false, false);
        if (m_armorDamageProof >= damage) {
            m_armorDamageProof -= damage;
            game->PlayInjureEffect(unit, true);
            m_gameStats.RegisterArmorConsumeDamage(damage);
            return;
        }
        m_gameStats.RegisterArmorConsumeDamage(m_armorDamageProof);
        damage -= m_armorDamageProof;
        m_armorDamageProof = 0;
    }
    m_hp -= damage;
    if (m_hp < 0) {
        m_hp = 0;
    }
    Save();
    game->PlayInjureEffect(unit, false);
    app->m_soundManager->PlaySound("IDM_CHARACTER_ENEMY_WOUND_MAN_01", false, false);
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
    App* app = static_cast<App*>(WindowApp::m_instance);
    int amount = app->m_helper->m_config.GetIntegerValueByToken("MEDKIT_HP_RESTORE", true);
    AddHp(amount);
    --m_medkits;
    m_gameStats.AddMedkitsUsed(1);
    app->m_soundManager->PlaySound("IDM_SPECIAL_AP_MEDKIT", false, false);
    WindowApp::HandleTunnelCommand(0x48a1ff19, amount, 0, 0);
    Save();
}

void CBH_Player::AddHp(int amount)
{
    m_hp += amount;
    const int maximum = GetMaxHp();
    if (m_hp > maximum) {
        m_hp = maximum;
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
    if (m_speedModeAvailableTimeMS < 10000) {
        m_speedTimeMS += m_speedModeAvailableTimeMS;
        m_speedModeAvailableTimeMS = 0;
    } else {
        m_speedTimeMS += 10000;
        m_speedModeAvailableTimeMS -= 10000;
    }
    m_gameStats.RegisterPillAddTime(10000);
    Save();
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
    if (m_reviveCount > 0) {
        AddHp(GetMaxHp());
        --m_reviveCount;
        ++m_gameStats.m_resurrects;
        ++m_challengeStats->m_revives;
        m_field39 = true;
        Save();
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
    CProgressSystem* progress = static_cast<App*>(WindowApp::m_instance)->InitProgressSystem();
    if (m_level >= progress->GetMaxLevel()) {
        return progress->GetMaxXP();
    }
    return progress->GetMaxXPForLevel(m_level + 1);
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
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (source != GAIN_SOURCE_IAP && app->m_overrideManager != NULL) {
        XString group(L"missions_mult");
        if (source == GAIN_SOURCE_CHALLENGE) {
            group = XString(L"challenge_mult");
        }
        const int multiplier = GetOverride(*app->m_overrideManager, group, XString(L"xp"));
        if (multiplier > 100) {
            amount = multiplier * amount / 100;
        }
    }

    m_experience += amount;
    CProgressSystem* progress = app->InitProgressSystem();
    const int level = progress->GetLevel(m_experience);
    const int maximum = progress->GetMaxXP();
    if (level > m_level) {
        m_level = level;
        m_energy = progress->GetMaxEnergyForLevel(m_level);
        m_hp = GetMaxHpForLevel(m_level);
        CGameAnalytics::logLevelGained(m_level, m_cash, m_credits);
    }
    if (amount > 0) {
        CGameAnalytics::logXPGain(amount, progress->GetMaxXPForLevel(level) - m_experience);
    }
    if (maximum < m_experience) {
        m_experience = maximum;
    }
    if (amount > 0) {
        Save();
    }
    return amount;
}

void CBH_Player::OnEnemyKilled()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_huntingInfo->OnEnemyKilled();
    if (app->m_helper->m_swerveGame->m_multiplier == 3) {
        m_gameStats.AddX3MultiplierKills(1);
    }
    if (app->m_helper->m_swerveGame->m_multiplier == 2) {
        m_gameStats.AddX2MultiplierKills(1);
    }
}

void CBH_Player::OnEnemyTranquillized()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_huntingInfo->OnEnemyTranqullized();
    if (app->m_helper->m_swerveGame->m_multiplier == 3) {
        m_gameStats.AddX3MultiplierKills(1);
    }
    if (app->m_helper->m_swerveGame->m_multiplier == 2) {
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
    if (!IsMissionCompleted(id)) {
        const SCompletedMissionInfo completion = { id, false };
        m_completedMissions.addElement(completion);
        Save();
    }
}

void CBH_Player::SetPlayerStartParams()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (app->m_helper != NULL) {
        ConfigReader& config = app->m_helper->m_config;
        m_cash = config.GetIntegerValueByToken("PLAYER_START_MONEY", true);
        m_credits = config.GetIntegerValueByToken("PLAYER_START_CREDITS", true);
        CDH_WeaponsManager* weapons = app->InitWeaponManager();
        for (int index = 0; index < weapons->m_weapons.size(); ++index) {
            CDH_Weapon* weapon = weapons->m_weapons[index];
            weapon->FlushAllUpgrades();
            weapon->setAvailable(false);
        }
        for (int index = 0; index < m_ammoCounts.m_count; ++index) {
            *m_ammoCounts.GetAt(index, NULL) = 0;
        }
        const int id = config.GetIntegerValueByToken("PLAYER_START_WEAPON_1_WID", true);
        const int packs = config.GetIntegerValueByToken("PLAYER_START_WEAPON_1_AMMO_PACKS", true);
        CDH_Weapon* weapon = weapons->GetWeaponById(id);
        if (weapon != NULL) {
            weapon->setAvailable(true);
            CDH_BasicGameData* data = app->m_helper->m_gamePlayManager.GetGameData();
            data->SetWeapon(0, id);
            data->SetWeapon(1, id);
            SetAmmoForWeaponId(id, packs);
        }
    }
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
    return Nonnegative(static_cast<int>(m_energyRestoreSeconds) * (GetMaxEnergy() - m_energy));
}

int CBH_Player::GetMaxEnergyForLevel(int level)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (app->InitProgressSystem() == NULL) {
        return 0;
    }
    return app->InitProgressSystem()->GetMaxEnergyForLevel(level);
}

int CBH_Player::GetMaxHpForLevel(int level)
{
    ConfigReader& config = static_cast<App*>(WindowApp::m_instance)->m_helper->m_config;
    const int base = config.GetIntegerValueByToken("BASE_PLAYER_HP", true);
    const int perLevel = config.GetIntegerValueByToken("PLAYER_HP_PER_LEVEL_GROWTH", true);
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
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (app->m_helper != NULL) {
        const CDH_BasicGameData* data = app->m_helper->m_gamePlayManager.GetGameData();
        if (data->m_weaponIds[0] >= 0) {
            element->SetAttribute("weapon_slot1", data->m_weaponIds[0]);
        }
        if (data->m_weaponIds[1] >= 0) {
            element->SetAttribute("weapon_slot2", data->m_weaponIds[1]);
        }
    }
}

void CBH_Player::SaveReputation(TiXmlElement* element)
{
    WriteNamedCounters(element, "reputation", "fraction", "name", "reputation", m_reputation);
}

void CBH_Player::SaveMissionInfo(TiXmlElement* element)
{
    TiXmlElement* section = CXmlHelper::ObtainElement(element, XString("completed_missions"));
    section->Clear();
    for (int index = 0; index < m_completedMissions.size(); ++index) {
        const SCompletedMissionInfo& completion = m_completedMissions[index];
        TiXmlElement* child = new TiXmlElement("completed_mission");
        child->SetAttribute("id", completion.m_id);
        child->SetAttribute("tranq", completion.m_tranquillized);
        section->LinkEndChild(child);
    }
}

void CBH_Player::SaveAmmo(TiXmlElement* element)
{
    TiXmlElement* section = CXmlHelper::ObtainElement(element, XString("ammo_info"));
    section->Clear();
    for (int index = 0; index < m_ammoCounts.m_count; ++index) {
        HashTableKey<const CAmmo*>* key;
        m_ammoCounts.GetAt(index, &key);
        const CAmmo* ammo = key->m_value;
        TiXmlElement* child = new TiXmlElement("ammo");
        child->SetStringAttribute("type", XString::AnsiString(ammo->m_ammoType.c_str()));
        child->SetAttribute("count", GetAvailableAmmo(ammo, false));
        section->LinkEndChild(child);
    }
}

void CBH_Player::SaveTjPoints(TiXmlElement* element)
{
    WriteNamedCounters(element, "tapjoy_info", "tapjoy_points", "devId", "points", m_tapjoyPoints);
}

void CBH_Player::SaveFriendGiftLasDay(TiXmlElement* element)
{
    if (element == NULL) {
        return;
    }
    TiXmlElement* section = CXmlHelper::ObtainElement(element, XString("friendGiftsInfo"));
    section->Clear();
    for (int index = 0; index < m_friendGiftDays.m_count; ++index) {
        XString* name;
        const int day = *m_friendGiftDays.GetAt(index, &name);
        TiXmlElement* child = new TiXmlElement("friendGift");
        child->SetAttribute("socialID", name->ToInt());
        child->SetAttribute("lastDay", day);
        section->LinkEndChild(child);
    }
}

void CBH_Player::LoadStats(TiXmlElement* element)
{
    m_hp = ReadInt(element, "hp");
    m_energy = ReadInt(element, "energy");
    m_medkits = ReadInt(element, "medkits");
    m_reviveCount = ReadInt(element, "revive");
    m_credits = ReadInt(element, "hunt_credits");
    m_experience = ReadInt(element, "xp");
    m_cash = ReadInt(element, "points");
    m_level = ReadInt(element, "level");
    m_tutorialState = ReadInt(element, "tutorial_state");
    m_lastDailyBonusTime = ReadInt(element, "last_daily_bonus_time");
    m_dailyBonusesInRow = ReadInt(element, "daily_bonus_row");
    m_lastFiveDaysBonusTime = ReadInt(element, "last_five_days_bonus_time");
    m_activeFiveDaysBonusDay = ReadInt(element, "active_five_days_bonus_day");
    m_currentFriendGatedMissionId = ReadInt(element, "current_friend_gated_mission_id");
    m_speedModeAvailableTimeMS = ReadInt(element, "speedModeAvailableTimeMS");
    m_armorId = ReadInt(element, "armorID");
    m_armorDamageProof = ReadInt(element, "armorDamageProof");
    m_armorDamageProofMax = ReadInt(element, "armorDamageProofMax");
    m_lastHuntItem = ReadInt(element, "last_hunt_item");
    m_lastHuntResult = ReadInt(element, "last_hunt_result");
    m_appRated = CXmlHelper::GetAttributeValueOptional(*element, "app_rated").ToBool();
    m_lastEnergyUpdateTime = ReadInt(element, "last_energy_update_time");
    m_lastGameVersionPlayed = ReadInt(element, "last_game_version_played", 100);
    m_silentShotsCount = ReadInt(element, "silentShotsCount");
    m_cyberScopesCount = ReadInt(element, "cyberScopesCount");
    m_reactixControlTime = ReadInt(element, "reactixControlTime");
    m_hitechVisorEquipped = CXmlHelper::GetAttributeValueOptional(*element, "isHitechVisorEquiped").ToBool();
    m_lastFiveDaysIncentiveBonusTimestamp = ReadInt(element, "lastFiveDaysIncentiveBonusTimestamp");
    m_activeFiveDaysIncentiveBonusDay = ReadInt(element, "activeFiveDaysIncentiveBonusDay");
    m_fiveDayIncentiveBonusForbidden = CXmlHelper::GetAttributeValueOptional(*element, "fiveDayIncentiveBonusForbidden").ToBool();
    m_receivedFiveDaysIncentiveBonus = CXmlHelper::GetAttributeValueOptional(*element, "ReceivedFiveDaysIncentiveBonus").ToBool();
    m_incentiveBonusExpirationShown = CXmlHelper::GetAttributeValueOptional(*element, "FiveDaysIncentiveBonusExpirationMessageShown").ToBool();
    m_lastMissionVersionPlayed = ReadInt(element, "last_mission_version_played", 100);
    m_ultrasoundAttemptCount = ReadInt(element, "ultrasoundAttemptCount");
    m_credits = Nonnegative(m_credits);
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (app->m_helper != NULL) {
        CDH_WeaponsManager* weapons = app->InitWeaponManager();
        CDH_BasicGameData* data = app->m_helper->m_gamePlayManager.GetGameData();
        const int first = ReadInt(element, "weapon_slot1");
        const int second = ReadInt(element, "weapon_slot2");
        if (weapons->GetWeaponById(first) != NULL) {
            data->SetWeapon(0, first);
        }
        if (weapons->GetWeaponById(second) != NULL) {
            data->SetWeapon(1, second);
        }
    }
    if (m_lastGameVersionPlayed >= 101 && m_lastGameVersionPlayed < 140) {
        m_fiveDayIncentiveBonusForbidden = true;
    }
}

void CBH_Player::LoadMissionInfo(TiXmlElement* element)
{
    const TiXmlElement* section = element->FirstChildElement("completed_missions");
    if (section == NULL) {
        return;
    }
    m_completedMissions.removeAllElements();
    for (const TiXmlElement* child = section->FirstChildElement("completed_mission"); child != NULL;
         child = child->NextSiblingElement("completed_mission")) {
        SCompletedMissionInfo completion;
        completion.m_id = ReadInt(child, "id");
        completion.m_tranquillized = CXmlHelper::GetAttributeValueOptional(*child, "tranq").ToBool();
        m_completedMissions.addElement(completion);
    }
}

void CBH_Player::LoadAmmo(TiXmlElement* element)
{
    const TiXmlElement* section = element->FirstChildElement("ammo_info");
    if (section == NULL) {
        return;
    }
    CAmmoManager* ammunition = static_cast<App*>(WindowApp::m_instance)->InitAmmoManager();
    for (const TiXmlElement* child = section->FirstChildElement("ammo"); child != NULL;
         child = child->NextSiblingElement("ammo")) {
        const XString type = CXmlHelper::GetAttributeValueOptional(*child, "type");
        const CAmmo* ammo = ammunition->GetAmmoByType(type);
        if (ammo != NULL) {
            AmmoCount(m_ammoCounts, ammo) = ReadInt(child, "count");
        }
    }
}

void CBH_Player::LoadAvatar(TiXmlElement* element)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CAvatar* avatar = app->InitAvatarManager()->GetAvatar(ReadInt(element, "avatar_id"));
    if (avatar != NULL) {
        m_avatar = avatar;
    }
}

void CBH_Player::LoadTjPoints(TiXmlElement* element)
{
    const TiXmlElement* section = element->FirstChildElement("tapjoy_info");
    if (section != NULL) {
        for (const TiXmlElement* child = section->FirstChildElement("tapjoy_points"); child != NULL;
             child = child->NextSiblingElement("tapjoy_points")) {
            const XString name = CXmlHelper::GetAttributeValueOptional(*child, "devId");
            m_tapjoyPoints.Set(name, ReadInt(child, "points"));
        }
    }
}

void CBH_Player::LoadFriendGiftLasDay(TiXmlElement* element)
{
    const TiXmlElement* section = element->FirstChildElement("friendGiftsInfo");
    if (section != NULL) {
        for (const TiXmlElement* child = section->FirstChildElement("friendGift"); child != NULL;
             child = child->NextSiblingElement("friendGift")) {
            const int id = ReadInt(child, "socialID", -1);
            if (id != -1) {
                m_friendGiftDays.Set(XString(id), ReadInt(child, "lastDay"));
            }
        }
    }
}

CBH_Player::~CBH_Player()
{
    delete m_challengeStats;
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
    bool ultrasoundActive = game != NULL && game->m_stunMs > 0;
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (m_speedModeWasActive && !speedActive) {
        app->m_soundManager->PlaySound("IDM_SLOW_END", false, false);
    }
    if (m_ultrasoundWasActive && !ultrasoundActive) {
        app->m_soundManager->PlaySound("IDM_ULTRASOUND_END", false, false);
    }
    m_speedModeWasActive = speedActive;
    m_ultrasoundWasActive = ultrasoundActive;
    if (m_consumingReactixControl) {
        m_reactixControlTime -= elapsed;
        m_gameStats.RegisterPillAddTime(elapsed);
        if (m_reactixControlTime < 1) {
            m_reactixControlTime = 0;
            m_consumingReactixControl = false;
            m_speedTimeMS = 0;
        }
        Save();
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
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (app->m_overrideManager != NULL) {
        unsigned long long disabled;
        if (app->m_overrideManager->FindIntAttribute(CStrWChar("energy_disable"), CStrWChar("disable"), &disabled)) {
            m_energyEnabled = disabled != 1;
        }
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
    m_ultrasoundAttemptCount = Nonnegative(m_ultrasoundAttemptCount - 1);
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
