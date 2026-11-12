#include "CChallenge.h"

#include "App.h"
#include "CBH_Player.h"
#include "CChallengeManager.h"
#include "CChallengeStats.h"
#include "CGameAnalytics.h"
#include "CGServeHelper.h"
#include "CNGS.h"
#include "CNGSLocalUser.h"
#include "CNGSPushNotificationDetails.h"
#include "CNGSRemoteUser.h"
#include "CNGSRemoteUserList.h"
#include "CObjectMapValue.h"
#include "COverrideManager.h"
#include "CProgressSystem.h"
#include "CShopItem.h"
#include "CStdUtil_Cocoa.h"
#include "CXmlHelper.h"
#include "ResMap.h"
#include "lualib.h"
#include "luabridge.h"
#include <limits.h>
#include <math.h>

namespace {
const char* const RewardSections[] = {"solo_reward", "x1_reward", "x2_reward"};
const int RewardCount = sizeof(RewardSections) / sizeof(RewardSections[0]);
const unsigned int ChallengeMessageDurationSeconds = 86400;
const long long ChallengeCompletedMessage = 1011;

bool ReadOverrideAmount(COverrideManager& overrides, const CStrWChar& path,
                        const wchar_t* key, int& amount)
{
    unsigned long long value;
    if (!overrides.FindIntAttribute(path, CStrWChar(key), &value) || value > INT_MAX) {
        return false;
    }
    amount = static_cast<int>(value);
    return true;
}

bool UpdateOverrideImage(SChallengeReward& reward, COverrideManager& overrides, const CStrWChar& path)
{
    CStrWChar image;
    if (!overrides.FindStringAttribute(path, CStrWChar(L"image"), &image)) {
        return false;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    const char* resource = app->InitResMap()->getResourceID(XString(image.GetWCharPtr()));
    if (resource == NULL) {
        return false;
    }
    reward.m_image = resource;
    return true;
}

void ClearRewardAmounts(SChallengeReward& reward)
{
    reward.m_credits = 0;
    reward.m_cash = 0;
    reward.m_experience = 0;
    reward.m_itemAmount = 0;
    reward.m_itemId = 0;
    reward.m_itemManager = NULL;
}
}

SChallengeReward::SChallengeReward()
    : m_credits(0), m_cash(0), m_experience(0), m_itemAmount(0), m_itemId(0),
      m_itemManager(NULL), m_image(NULL), m_friendId(-1), m_awarded(false)
{
}

CChallenge::CChallenge(TiXmlElement* element)
    : m_rewards(new SChallengeReward*[RewardCount]), m_overrideRevision(0), m_day(0), m_softRewardCap(0),
      m_script(NULL), m_id(0), m_goal(0), m_finishTime(0), m_completedAt(0),
      m_active(false), m_hasCustomCheck(false), m_difficulty(0.0f)
{
    ParseRoot(element);
    ParseRewards(element);
    m_script = luaL_newstate();
    if (m_script != NULL) {
        luaL_openlibs(m_script);
        RegisterNativeClasses(m_script);
        ParseScript(element, m_script);
        lua_getglobal(m_script, "CheckFunc");
        m_hasCustomCheck = lua_isfunction(m_script, -1);
        lua_pop(m_script, 1);
    }
}

CChallenge::~CChallenge()
{
    if (m_script != NULL) {
        lua_close(m_script);
    }
    for (int index = 0; index < RewardCount; ++index) {
        delete m_rewards[index];
    }
    delete[] m_rewards;
}

void CChallenge::ParseRoot(TiXmlElement* element)
{
    m_id = CXmlHelper::GetAttributeValue(*element, "id").ToInt();
    m_name = CXmlHelper::GetResString(*element, "name");
    m_goal = CXmlHelper::GetAttributeValue(*element, "goal_value").ToInt();
    m_day = CXmlHelper::GetAttributeValue(*element, "day").ToInt();
    m_difficulty = CXmlHelper::GetAttributeValue(*element, "difficulty_coef").ToFloat();
    m_softRewardCap = CXmlHelper::GetAttributeValue(*element, "soft_reward_cap").ToInt();
}

void CChallenge::ParseRewards(TiXmlElement* element)
{
    m_rewards[0] = new SChallengeReward();
    m_rewards[0]->m_image = "IDB_COOP_EXP_ICON";
    m_rewards[1] = new SChallengeReward();
    m_rewards[1]->m_image = "IDB_COOP_SOFT_CURRENCY_ICON";
    m_rewards[2] = ParseReward(element->FirstChildElement(RewardSections[2]));
}

SChallengeReward* CChallenge::ParseReward(TiXmlElement* element)
{
    SChallengeReward* reward = new SChallengeReward();
    if (element == NULL) {
        return reward;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    ResMap* resources = app->InitResMap();
    TiXmlElement* entry = element->FirstChildElement("xp_reward");
    if (entry != NULL) {
        reward->m_experience = CXmlHelper::GetAttributeValue(*entry, "amount").ToInt();
        reward->m_image = resources->getResourceID(CXmlHelper::GetAttributeValue(*entry, "image"));
    }
    entry = element->FirstChildElement("cash_reward");
    if (entry != NULL) {
        reward->m_credits = CXmlHelper::GetAttributeValue(*entry, "amount").ToInt();
        reward->m_image = resources->getResourceID(CXmlHelper::GetAttributeValue(*entry, "image"));
    }
    entry = element->FirstChildElement("soft_reward");
    if (entry != NULL) {
        reward->m_cash = CXmlHelper::GetAttributeValue(*entry, "amount").ToInt();
        reward->m_image = resources->getResourceID(CXmlHelper::GetAttributeValue(*entry, "image"));
    }
    entry = element->FirstChildElement("item_reward");
    if (entry != NULL) {
        reward->m_itemManager = App::GetItemManagerByName(CXmlHelper::GetAttributeValue(*entry, "item_type"));
        reward->m_itemId = CXmlHelper::GetAttributeValue(*entry, "item_id").ToInt();
        reward->m_itemAmount = CXmlHelper::GetAttributeValue(*entry, "amount").ToInt();
        reward->m_image = resources->getResourceID(CXmlHelper::GetAttributeValue(*entry, "image"));
    }
    return reward;
}

void CChallenge::RegisterNativeClasses(lua_State* script)
{
    luabridge::class__<CChallengeStats> stats(script, "CChallengeStats");
    stats.method("GetTargetsKilled", &CChallengeStats::GetTargetsKilled);
    stats.method("GetHeadshotsNumber", &CChallengeStats::GetHeadshotsNumber);
    stats.method("GetTargetsKilledWithoutScope", &CChallengeStats::GetTargetsKilledWithoutScope);
    stats.method("GetOneShots", &CChallengeStats::GetOneShots);
    stats.method("GetMissionsWithoutMedkitsUsed", &CChallengeStats::GetMissionsWithoutMedkitsUsed);
    stats.method("GetFastestMissionCompletionTime", &CChallengeStats::GetFastestMissionCompletionTime);
    stats.method("GetCompletedBounties", &CChallengeStats::GetCompletedBounties);
    stats.method("GetKillsByWeaponType", &CChallengeStats::GetKillsByWeaponType);
    stats.method("GetHeadshotsByWeaponType", &CChallengeStats::GetHeadshotsByWeaponType);
    stats.method("GetHeadshotsInRow", &CChallengeStats::GetHeadshotsInRow);
    stats.method("GetKillsBeforeTakeCoverInMission", &CChallengeStats::GetKillsBeforeTakeCoverInMission);
    stats.method("GetBossKillsWithHeadshotsWOScope", &CChallengeStats::GetBossKillsWithHeadshotsWOScope);
    stats.method("GetOneShotsInRow", &CChallengeStats::GetOneShotsInRow);
    stats.method("GetMissionsWithoutInjure", &CChallengeStats::GetMissionsWithoutInjure);
    stats.method("GetMissionsWithHeadshotsOnly", &CChallengeStats::GetMissionsWithHeadshotsOnly);
    stats.method("GetHeadshotsOnLocation", &CChallengeStats::GetHeadshotsOnLocation);
    stats.method("GetMissionsWithoutEnemyGettingCover", &CChallengeStats::GetMissionsWithoutEnemyGettingCover);
    stats.method("GetMissionsCompleteWithoutPlayerTakeCover", &CChallengeStats::GetMissionsCompleteWithoutPlayerTakeCover);
    stats.method("AddMissionsWihoutEnemyGettingCover", &CChallengeStats::AddMissionsWihoutEnemyGettingCover);
    stats.method("ResetKillsBeforeTakeCoverInMission", &CChallengeStats::ResetKillsBeforeTakeCoverInMission);
    stats.method("GetNonScopeBossHeadshotsWhileEscaping", &CChallengeStats::GetNonScopeBossHeadshotsWhileEscaping);
    stats.method("GetAmbushWithoutMiss", &CChallengeStats::GetAmbushWithoutMiss);
    stats.method("GetKillsOutOfRange", &CChallengeStats::GetKillsOutOfRange);
    stats.method("GetMissionsCompletedFor30Seconds", &CChallengeStats::GetMissionsCompletedFor30Seconds);
    stats.method("GetHeadshotsInRowWithWeaponCategory", &CChallengeStats::GetHeadshotsInRowWithWeaponCategory);
    stats.method("GetMissionsWithSameWeaponCategory", &CChallengeStats::GetMissionsWithSameWeaponCategory);
    stats.method("GetSilentHeadshots", &CChallengeStats::GetSilentHeadshots);
    stats.method("GetMovingCameraMissionWins", &CChallengeStats::GetMovingCameraMissionWins);
    stats.method("GetMissionsWithExplosiveAmmoOnly", &CChallengeStats::GetMissionsWithExplosiveAmmoOnly);
    stats.method("GetMissionsWithIncendiaryAmmoOnly", &CChallengeStats::GetMissionsWithIncendiaryAmmoOnly);
    stats.method("GetWeaponGatedMissionsWins", &CChallengeStats::GetWeaponGatedMissionsWins);
    stats.method("GetBossKillingByExplosiveObject", &CChallengeStats::GetBossKillingByExplosiveObject);
    stats.method("GetMissionsWithoutAlert", &CChallengeStats::GetMissionsWithoutAlert);
    stats.method("GetKillsByWeaponId", &CChallengeStats::GetKillsByWeaponId);
    stats.method("GetRevives", &CChallengeStats::GetRevives);
    stats.method("GetMissionsCompletedInRow", &CChallengeStats::GetMissionsCompletedInRow);
    stats.method("GetMultiKills", &CChallengeStats::GetMultiKills);
    stats.method("GetMissionsWithSameWeapon", &CChallengeStats::GetMissionsWithSameWeapon);
    stats.method("GetMissionsWithAimAssist", &CChallengeStats::GetMissionsWithAimAssist);
}

void CChallenge::ParseScript(TiXmlElement* element, lua_State* script)
{
    const TiXmlElement* source = element->FirstChildElement("script");
    if (source == NULL) {
        return;
    }
    XString body = CXmlHelper::GetAttributeValue(*source, "script");
    XString::AnsiString scriptText(body.c_str());
    if (luaL_loadstring(script, scriptText) == 0 && lua_pcall(script, 0, 0, 0) == 0) {
        return;
    }
    lua_pop(script, 1);
}

bool CChallenge::CallScript(const char* name) const
{
    if (m_script == NULL) {
        return false;
    }
    lua_getglobal(m_script, name);
    if (!lua_isfunction(m_script, -1)) {
        lua_pop(m_script, 1);
        return false;
    }
    luabridge::TypeTable::tdstack<CChallengeStats>::push(m_script, *CBH_Player::GetInstance()->m_challengeStats, 0);
    if (lua_pcall(m_script, 1, 1, 0) != 0) {
        lua_pop(m_script, 1);
        return false;
    }
    return true;
}

int CChallenge::GetCurrProgress() const
{
    if (!CallScript("GetCurrProgress")) {
        return 0;
    }
    int progress = lua_tointeger(m_script, -1);
    lua_pop(m_script, 1);
    return progress;
}

void CChallenge::CheckCompletion()
{
    if (!m_active || m_rewards[0]->m_awarded) {
        return;
    }
    if (m_hasCustomCheck) {
        CallExternalCheckFunc();
    } else if (m_goal > 0 && GetCurrProgress() >= m_goal) {
        OnComplete();
    }
}

void CChallenge::CallExternalCheckFunc()
{
    if (CallScript("CheckFunc")) {
        bool complete = lua_toboolean(m_script, -1) != 0;
        lua_pop(m_script, 1);
        if (complete) {
            OnComplete();
        }
    }
}

int CChallenge::CalculateSoloReward() const
{
    int level = CBH_Player::GetInstance()->GetLevel();
    if (level <= 1) {
        return 0;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    CProgressSystem* progress = app->InitProgressSystem();
    int difference = progress->GetMaxXPForLevel(level + 1) - progress->GetMaxXPForLevel(level);
    return MathLib::Round(m_difficulty * (static_cast<float>(difference) / (2.0f * level + 63.0f)));
}

void CChallenge::UpdateRewards() const
{
    if (!m_rewards[0]->m_awarded) {
        m_rewards[0]->m_experience = CalculateSoloReward();
    }
    if (!m_rewards[1]->m_awarded) {
        int amount = CalculateSoloReward() * 4;
        if (amount > m_softRewardCap) {
            amount = m_softRewardCap;
        }
        m_rewards[1]->m_cash = amount;
    }
}

void CChallenge::OnComplete()
{
    UpdateRewards();
    m_rewards[0]->m_awarded = true;
    m_completedAt = CStdUtil_Cocoa::GetTimeSeconds();
    App* app = static_cast<App*>(WindowApp::m_instance);
    GiveReward(m_rewards[0]);
    SendCompletionMessageToFriends();
    CheckDelayedReward(m_rewards[1]->m_friendId);
    CheckDelayedReward(m_rewards[2]->m_friendId);
    CGameAnalytics::logChalengeCompleted(CGameAnalytics::CHALLENGE_COMPLETE_SOLO);
    app->m_challengeManager->Save();
}

void CChallenge::SendCompletionMessageToFriends()
{
    for (int index = 0; index < m_invitedFriends.size(); ++index) {
        SendCompletionMessage(m_invitedFriends[index]);
    }
}

void CChallenge::SendCompletionMessage(int friendId)
{
    if (friendId < 0) {
        return;
    }
    CNGSLocalUser* local = CNGS::GetInstance()->GetLocalUser();
    CNGSRemoteUserList* friends = local->m_remoteUserLists[0];
    CNGSRemoteUser* receiver = friends->findRemoteUserByID(friendId);
    if (receiver == NULL) {
        return;
    }
    XString id(m_id);
    CObjectMapString payload(CStrWChar(id.c_str()));
    XString format = Window::ResString("IDS_CHALLENGE_COMPLETE_PUSH_TEXT");
    CStrWChar nickname = local->GetNickName();
    XString message = XString::Format(format.c_str(), nickname.GetWCharPtr(), m_name.c_str());
    CNGSPushNotificationDetails details;
    details.m_buttonText = Window::ResString("IDS_CHALLENGE_PUSH_BUTTON").c_str();
    details.m_alertText = message.c_str();
    receiver->SendMessage(&payload, ChallengeMessageDurationSeconds, ChallengeCompletedMessage, true, &details);
}

void CChallenge::FriendInvited(int friendId)
{
    if (IsFriendInvited(friendId)) {
        return;
    }
    if (m_rewards[0]->m_awarded) {
        SendCompletionMessage(friendId);
    }
    m_invitedFriends.addElement(friendId);
    static_cast<App*>(WindowApp::m_instance)->m_challengeManager->Save();
}

bool CChallenge::IsFriendInvited(int friendId)
{
    return m_invitedFriends.indexOf(friendId) >= 0;
}

SChallengeReward* CChallenge::GetRewardForFriend(int friendId)
{
    SChallengeReward* available = NULL;
    for (int tier = 1; tier < RewardCount; ++tier) {
        SChallengeReward* reward = m_rewards[tier];
        if (reward->m_friendId == friendId) {
            UpdateRewards();
            return reward;
        }
        if (available == NULL && reward->m_friendId == -1 && !reward->m_awarded) {
            available = reward;
        }
    }
    if (available != NULL) {
        UpdateRewards();
    }
    return available;
}

void CChallenge::CheckDelayedReward(int friendId)
{
    if (friendId < 0 || !m_rewards[0]->m_awarded) {
        return;
    }
    SChallengeReward* reward = GetRewardForFriend(friendId);
    if (reward != NULL && reward->m_friendId == friendId && !reward->m_awarded) {
        reward->m_awarded = true;
        GiveReward(reward);
    }
}

bool CChallenge::FriendCompleteChallenge(int friendId)
{
    if (friendId <= 0) {
        return false;
    }
    SChallengeReward* reward = GetRewardForFriend(friendId);
    if (reward == NULL || reward->m_awarded) {
        return false;
    }
    if (reward->m_friendId == -1) {
        reward->m_friendId = friendId;
        reward->m_friendName = CGServeHelper::GetFriendNickname(friendId);
        CGameAnalytics::EChallengeCompleteState state = CGameAnalytics::CHALLENGE_COMPLETE_TWO_FRIENDS;
        if (reward == m_rewards[1]) {
            CBH_Player* player = CBH_Player::GetInstance();
            ++player->m_gameStats.m_coopsCompleted;
            player->Save();
            state = CGameAnalytics::CHALLENGE_COMPLETE_ONE_FRIEND;
        }
        CGameAnalytics::logChalengeCompleted(state);
    }
    CheckDelayedReward(friendId);
    static_cast<App*>(WindowApp::m_instance)->m_challengeManager->Save();
    return reward->m_awarded;
}

void CChallenge::GiveReward(const SChallengeReward* reward) const
{
    CBH_Player* player = CBH_Player::GetInstance();
    if (reward->m_experience > 0) {
        player->AddXp(reward->m_experience, GAIN_SOURCE_CHALLENGE);
    }
    if (reward->m_cash > 0) {
        player->AddMoney(reward->m_cash, GAIN_SOURCE_CHALLENGE);
    }
    if (reward->m_credits > 0) {
        player->AddCredits(reward->m_credits);
    }
    if (reward->m_itemManager != NULL) {
        CItem* item = reward->m_itemManager->getItemById(reward->m_itemId);
        if (item != NULL) {
            item->setAvailable(true);
        }
    }
}

const SChallengeReward* CChallenge::GetSoloReward() const
{
    UpdateRewards();
    return m_rewards[0];
}

const SChallengeReward* CChallenge::GetX1Reward() const
{
    UpdateRewards();
    return m_rewards[1];
}

const SChallengeReward* CChallenge::GetX2Reward() const
{
    return m_rewards[2];
}

XString CChallenge::GetRewardString(const SChallengeReward* reward)
{
    if (reward->m_experience > 0) {
        return XString::Format(Window::ResString("IDS_CHALLENGE_REWARD_XP").c_str(), reward->m_experience);
    }
    if (reward->m_cash > 0) {
        return XString::Format(Window::ResString("IDS_CHALLENGE_REWARD_SOFT").c_str(), reward->m_cash);
    }
    if (reward->m_credits > 0) {
        return XString::Format(Window::ResString("IDS_CHALLENGE_REWARD_CREDITS").c_str(), reward->m_credits);
    }
    if (reward->m_itemManager != NULL) {
        CItem* item = reward->m_itemManager->getItemById(reward->m_itemId);
        if (item != NULL) {
            return item->GetName();
        }
    }
    return XString();
}

void CChallenge::Start(int finishTime)
{
    m_finishTime = finishTime;
    m_completedAt = 0;
    m_active = true;
    m_invitedFriends.removeAllElements();
    for (int index = 0; index < RewardCount; ++index) {
        m_rewards[index]->m_awarded = false;
        m_rewards[index]->m_friendId = -1;
        m_rewards[index]->m_friendName = XString();
    }
}

void CChallenge::Load(TiXmlElement* parent)
{
    XString tag = XString::Format(L"challenge_%u", m_id);
    TiXmlElement* section = CXmlHelper::GetElement(parent, tag);
    if (section == NULL) {
        return;
    }
    m_active = CXmlHelper::GetIntAttributeValueOrUseDefault(section, "isAcive", 0) != 0;
    m_finishTime = CXmlHelper::GetIntAttributeValueOrUseDefault(section, "finishTime", 0);
    for (int index = 0; index < RewardCount; ++index) {
        SChallengeReward* reward = m_rewards[index];
        const TiXmlElement* entry = section->FirstChildElement(RewardSections[index]);
        if (entry == NULL) {
            continue;
        }
        reward->m_awarded = CXmlHelper::GetIntAttributeValueOrUseDefault(entry, "isAwarded", 0) != 0;
        if (index == 0) {
            reward->m_experience = CXmlHelper::GetIntAttributeValueOrUseDefault(entry, "reward", 0);
        } else {
            reward->m_friendId = CXmlHelper::GetIntAttributeValueOrUseDefault(entry, "friendId", -1);
            reward->m_friendName = CXmlHelper::GetAttributeValueOptional(*entry, "friendName");
            if (index == 1) {
                reward->m_cash = CXmlHelper::GetIntAttributeValueOrUseDefault(entry, "reward", 0);
            }
        }
    }
    LoadInvitedFirends(section);
}

void CChallenge::LoadInvitedFirends(TiXmlElement* section)
{
    const TiXmlElement* invited = section->FirstChildElement("invited_friends");
    m_invitedFriends.removeAllElements();
    if (invited != NULL) {
        for (const TiXmlElement* entry = invited->FirstChildElement("friend"); entry != NULL;
             entry = entry->NextSiblingElement("friend")) {
            int id = CXmlHelper::GetIntAttributeValueOrUseDefault(entry, "id", -1);
            if (id >= 0 && !IsFriendInvited(id)) {
                m_invitedFriends.addElement(id);
            }
        }
    }
}

void CChallenge::Save(TiXmlElement* parent)
{
    XString tag = XString::Format(L"challenge_%u", m_id);
    TiXmlElement* section = CXmlHelper::ObtainElement(parent, tag);
    section->SetAttribute("isAcive", m_active);
    section->SetAttribute("finishTime", m_finishTime);
    for (int index = 0; index < RewardCount; ++index) {
        const SChallengeReward* reward = m_rewards[index];
        TiXmlElement* entry = CXmlHelper::ObtainElement(section, XString(RewardSections[index]));
        entry->SetAttribute("isAwarded", reward->m_awarded);
        if (index == 0) {
            entry->SetAttribute("reward", reward->m_experience);
        } else {
            entry->SetAttribute("friendId", reward->m_friendId);
            entry->SetAttribute("friendName", XString::AnsiString(reward->m_friendName.c_str()));
            if (index == 1) {
                entry->SetAttribute("reward", reward->m_cash);
            }
        }
    }
    SaveInvitedFriends(section);
}

void CChallenge::SaveInvitedFriends(TiXmlElement* section)
{
    TiXmlElement* invited = CXmlHelper::ObtainElement(section, XString("invited_friends"));
    invited->Clear();
    for (int index = 0; index < m_invitedFriends.size(); ++index) {
        TiXmlElement* entry = new TiXmlElement("friend");
        entry->SetAttribute("id", m_invitedFriends[index]);
        invited->LinkEndChild(entry);
    }
}

bool CChallenge::DoOverrideSoftReward(SChallengeReward* reward, COverrideManager& overrides, const XString& name)
{
    CStrWChar path(name.c_str());
    int amount = 0;
    bool hasAmount = ReadOverrideAmount(overrides, path, L"amount", amount);
    if (hasAmount) {
        ClearRewardAmounts(*reward);
        reward->m_cash = amount;
    }
    bool hasImage = UpdateOverrideImage(*reward, overrides, path);
    return hasAmount || hasImage;
}

bool CChallenge::DoOverrideCashReward(SChallengeReward* reward, COverrideManager& overrides, const XString& name)
{
    CStrWChar path(name.c_str());
    int amount = 0;
    bool hasAmount = ReadOverrideAmount(overrides, path, L"amount", amount);
    if (hasAmount) {
        ClearRewardAmounts(*reward);
        reward->m_credits = amount;
    }
    bool hasImage = UpdateOverrideImage(*reward, overrides, path);
    return hasAmount || hasImage;
}

bool CChallenge::DoOverrideItemReward(SChallengeReward* reward, COverrideManager& overrides, const XString& name)
{
    CStrWChar path(name.c_str());
    CItemManager* manager = reward->m_itemManager;
    int itemId = reward->m_itemId;
    int amount = reward->m_itemAmount;
    CStrWChar itemType;
    bool hasType = overrides.FindStringAttribute(path, CStrWChar(L"item_type"), &itemType) != 0;
    if (hasType) {
        manager = App::GetItemManagerByName(XString(itemType.GetWCharPtr()));
    }
    bool hasId = ReadOverrideAmount(overrides, path, L"item_id", itemId);
    bool hasAmount = ReadOverrideAmount(overrides, path, L"amount", amount);
    bool changed = false;
    if ((hasType || hasId || hasAmount) && manager != NULL && manager->getItemById(itemId) != NULL) {
        ClearRewardAmounts(*reward);
        reward->m_itemManager = manager;
        reward->m_itemId = itemId;
        reward->m_itemAmount = amount;
        changed = true;
    }
    bool hasImage = UpdateOverrideImage(*reward, overrides, path);
    return changed || hasImage;
}

bool CChallenge::DoOverride(COverrideManager& overrides, const XString& name)
{
    CStrWChar path(name.c_str());
    CStrWChar rewardType;
    bool changed = false;
    if (overrides.FindStringAttribute(path, CStrWChar(L"reward"), &rewardType)) {
        XString type(rewardType.GetWCharPtr());
        if (type == L"cash") {
            changed = DoOverrideCashReward(m_rewards[2], overrides, name);
        } else if (type == L"soft") {
            changed = DoOverrideSoftReward(m_rewards[2], overrides, name);
        } else if (type == L"item") {
            changed = DoOverrideItemReward(m_rewards[2], overrides, name);
        }
    }
    CStrWChar value;
    if (overrides.FindStringAttribute(path, CStrWChar(L"difficulty_coef"), &value)) {
        float difficulty = XString(value.GetWCharPtr()).ToFloat();
        if (isfinite(difficulty) && difficulty >= 0.0f) {
            m_difficulty = difficulty;
            changed = true;
        }
    }
    return changed;
}

bool CChallenge::CheckOverride()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    COverrideManager* overrides = app->m_overrideManager;
    if (overrides == NULL || overrides->changesMark() == m_overrideRevision) {
        return false;
    }
    m_overrideRevision = overrides->changesMark();
    return DoOverride(*overrides, XString::Format(L"challenge_%d", m_id));
}

const XString& CChallenge::GetFriendName(int index)
{
    if (index == 0 || index == 1) {
        return m_rewards[index + 1]->m_friendName;
    }
    static const XString empty;
    return empty;
}

int CChallenge::GetFriendId(int index)
{
    if (index == 0 || index == 1) {
        return m_rewards[index + 1]->m_friendId;
    }
    return -1;
}

typedef char CKChallengeSize[sizeof(CChallenge) == 0x40 ? 1 : -1];
typedef char CKChallengeRewardSize[sizeof(SChallengeReward) == 0x28 ? 1 : -1];
