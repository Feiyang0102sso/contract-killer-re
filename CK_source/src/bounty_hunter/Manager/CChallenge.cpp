#include "CChallenge.h"
#include "CSaveManager.h"
#define AD_BANNER_SIZES_NO_MARGINS
#include "AdBannerSizes.h"
#undef AD_BANNER_SIZES_NO_MARGINS

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

// the original resets the whole reward (store order as in the binary)
void ResetReward(SChallengeReward& reward)
{
    reward.m_experience = 0;
    reward.m_credits = 0;
    reward.m_cash = 0;
    reward.m_itemAmount = 0;
    reward.m_itemId = 0;
    reward.m_itemManager = NULL;
    reward.m_awarded = false;
    reward.m_friendId = -1;
}
}

SChallengeReward::SChallengeReward()
    : m_credits(0), m_cash(0), m_experience(0), m_itemAmount(0), m_itemId(0),
      m_itemManager(NULL), m_image(NULL), m_friendId(-1), m_awarded(false)
{
}

CChallenge::CChallenge(TiXmlElement* element)
    : m_overrideRevision(0), m_day(0), m_softRewardCap(0),
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
    if (m_rewards != NULL) {
        if (m_rewards[0] != NULL) {
            delete m_rewards[0];
            m_rewards[0] = NULL;
        }
        if (m_rewards[1] != NULL) {
            delete m_rewards[1];
            m_rewards[1] = NULL;
        }
        if (m_rewards[2] != NULL) {
            delete m_rewards[2];
            m_rewards[2] = NULL;
        }
        if (m_rewards != NULL) {
            delete[] m_rewards;
            m_rewards = NULL;
        }
    }
    lua_close(m_script);
}

void CChallenge::ParseRoot(TiXmlElement* element)
{
    m_id = CXmlHelper::GetAttributeValue(element, "id").ToInt();
    m_name = CXmlHelper::GetResString(element, "name");
    m_goal = CXmlHelper::GetAttributeValue(element, "goal_value").ToInt();
    m_day = CXmlHelper::GetAttributeValue(element, "day").ToInt();
    m_difficulty = CXmlHelper::GetAttributeValue(element, "difficulty_coef").ToFloat();
    m_softRewardCap = CXmlHelper::GetAttributeValue(element, "soft_reward_cap").ToInt();
}

void CChallenge::ParseRewards(TiXmlElement* element)
{
    // Original: a reward only exists when its node is present; solo gives the computed XP,
    // x1 gives four times that as cash, capped by m_softRewardCap.
    m_rewards = new SChallengeReward*[RewardCount];
    m_rewards[0] = NULL;
    m_rewards[1] = NULL;
    m_rewards[2] = NULL;
    if (element->FirstChildElement("solo_reward") != NULL) {
        m_rewards[0] = new SChallengeReward();
        m_rewards[0]->m_image = "IDB_COOP_EXP_ICON";
        m_rewards[0]->m_experience = CalculateSoloReward();
    }
    if (element->FirstChildElement("x1_reward") != NULL) {
        m_rewards[1] = new SChallengeReward();
        m_rewards[1]->m_image = "IDB_COOP_SOFT_CURRENCY_ICON";
        int cash = m_softRewardCap;
        if (CalculateSoloReward() * 4 < cash) {
            cash = CalculateSoloReward() * 4;
        }
        m_rewards[1]->m_cash = cash;
    }
    TiXmlElement* second = element->FirstChildElement("x2_reward");
    if (second != NULL) {
        m_rewards[2] = ParseReward(second);
    }
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
    XString body = CXmlHelper::GetAttributeValue(source, "script");
    luaL_loadstring(script, XString::AnsiString(body.c_str()));
    lua_pcall(script, 0, LUA_MULTRET, 0);
}

int CChallenge::GetCurrProgress() const
{
    lua_State* script = m_script;
    lua_getglobal(script, "GetCurrProgress");
    if (lua_isfunction(script, -1)) {
        luabridge::TypeTable::tdstack<CChallengeStats>::push(script, *CBH_Player::GetInstance()->m_challengeStats, 0);
        int progress = 0;
        if (lua_pcall(script, 1, 1, 0) == 0) {
            progress = lua_tointeger(script, -1);
            lua_pop(script, 1);
        }
        return progress;
    }
    lua_pop(script, 1);
    return 0;
}

void CChallenge::CheckCompletion()
{
    if (!m_active) {
        return;
    }
    if (m_hasCustomCheck) {
        CallExternalCheckFunc();
        return;
    }
    if (GetCurrProgress() >= m_goal) {
        OnComplete();
    }
}

void CChallenge::CallExternalCheckFunc()
{
    lua_State* script = m_script;
    lua_getglobal(script, "CheckFunc");
    if (lua_isfunction(script, -1)) {
        luabridge::TypeTable::tdstack<CChallengeStats>::push(script, *CBH_Player::GetInstance()->m_challengeStats, 0);
        if (lua_pcall(script, 1, 1, 0) == 0 && lua_toboolean(script, -1) > 0) {
            OnComplete();
        }
    } else {
        lua_pop(script, 1);
    }
}

int CChallenge::CalculateSoloReward() const
{
    // the original keeps the level as a float and subtracts the two XP limits as floats
    float level = static_cast<float>(CBH_Player::GetInstance()->GetLevel());
    if (level > 1.0f) {
        float next = static_cast<float>(static_cast<App*>(WindowApp::m_instance)->InitProgressSystem()->GetMaxXPForLevel(static_cast<int>(level + 1.0f)));
        float current = static_cast<float>(static_cast<App*>(WindowApp::m_instance)->InitProgressSystem()->GetMaxXPForLevel(static_cast<int>(level)));
        return MathLib::Round(m_difficulty * ((next - current) / (2.0f * level + 63.0f)));
    }
    return 0;
}

void CChallenge::UpdateRewards() const
{
    if (!m_rewards[0]->m_awarded) {
        m_rewards[0]->m_experience = CalculateSoloReward();
    }
    if (!m_rewards[1]->m_awarded) {
        int scaledReward = CalculateSoloReward() * 4;
        int amount = m_softRewardCap;
        if (scaledReward < amount) {
            amount = scaledReward;
        }
        m_rewards[1]->m_cash = amount;
    }
}

void CChallenge::OnComplete()
{
    UpdateRewards();
    if (!m_rewards[0]->m_awarded) {
        m_rewards[0]->m_awarded = true;
        GiveReward(m_rewards[0]);
        m_completedAt = CStdUtil_Cocoa::GetTimeSeconds();
        SendCompletionMessageToFriends();
        CheckDelayedReward(m_rewards[1]->m_friendId);
        CheckDelayedReward(m_rewards[2]->m_friendId);
        CGameAnalytics::logChalengeCompleted(CGameAnalytics::CHALLENGE_COMPLETE_SOLO);
        static_cast<App*>(WindowApp::m_instance)->m_challengeManager->Save();
    }
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
    for (int index = 0; index < friends->m_users.Size(); ++index) {
        CNGSRemoteUser* receiver = friends->m_users[index];
        if (receiver->GetClientID() == friendId) {
            XString id = XString::Format(L"%i", m_id);
            CObjectMapString payload(CStrWChar(id.c_str()));
            XString message = XString::Format(Window::ResString("IDS_CHALLENGE_COMPLETE_PUSH_TEXT").c_str(),
                                               local->GetNickName().GetWCharPtr(), m_name.c_str());
            XString button = Window::ResString("IDS_CHALLENGE_PUSH_BUTTON");
            // The original keeps the heap-allocated push details after sending.
            CNGSPushNotificationDetails* details = new CNGSPushNotificationDetails;
            XString::AnsiString alertText(message.c_str());
            XString::AnsiString buttonText(button.c_str());
            details->m_buttonText.Concatenate(buttonText);
            details->m_alertText.Concatenate(alertText);
            receiver->SendMessage(&payload, ChallengeMessageDurationSeconds, ChallengeCompletedMessage, true, details);
        }
    }
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
    return m_invitedFriends.indexOf(friendId) != -1;
}

SChallengeReward* CChallenge::GetRewardForFriend(int friendId)
{
    // the original takes the first of the two friend tiers that is free (-1) or already this friend's
    if (m_rewards[1]->m_friendId == -1 || m_rewards[1]->m_friendId == friendId) {
        UpdateRewards();
        return m_rewards[1];
    }
    if (m_rewards[2]->m_friendId == -1 || m_rewards[2]->m_friendId == friendId) {
        UpdateRewards();
        return m_rewards[2];
    }
    return NULL;
}

void CChallenge::CheckDelayedReward(int friendId)
{
    // the original: look the reward up, refresh the rewards, then pay once the first one is awarded
    if (friendId == -1) {
        return;
    }
    SChallengeReward* reward = GetRewardForFriend(friendId);
    if (reward == NULL) {
        return;
    }
    UpdateRewards();
    if (!m_rewards[0]->m_awarded) {
        return;
    }
    GiveReward(reward);
    reward->m_awarded = true;
}

bool CChallenge::FriendCompleteChallenge(int friendId)
{
    bool gaveReward = false;
    SChallengeReward* reward = GetRewardForFriend(friendId);
    if (reward != NULL) {
        XString nickname = CGServeHelper::GetFriendNickname(friendId);
        int firstFriend = m_rewards[1]->m_friendId;
        if (firstFriend == -1 && firstFriend != friendId) {
            m_rewards[1]->m_friendId = friendId;
            m_rewards[1]->m_friendName = nickname;
            ++CBH_Player::GetInstance()->m_gameStats.m_coopsCompleted;
            CGameAnalytics::logChalengeCompleted(CGameAnalytics::CHALLENGE_COMPLETE_ONE_FRIEND);
        } else {
            int secondFriend = m_rewards[2]->m_friendId;
            if (secondFriend == friendId && secondFriend == -1 && firstFriend != friendId) {
                m_rewards[2]->m_friendId = friendId;
                m_rewards[2]->m_friendName = nickname;
                CGameAnalytics::logChalengeCompleted(CGameAnalytics::CHALLENGE_COMPLETE_TWO_FRIENDS);
            }
        }
        UpdateRewards();
        gaveReward = m_rewards[0]->m_awarded;
        if (gaveReward) {
            GiveReward(reward);
            reward->m_awarded = true;
        }
    }
    static_cast<App*>(WindowApp::m_instance)->m_challengeManager->Save();
    return gaveReward;
}

void CChallenge::GiveReward(const SChallengeReward* reward) const
{
    // the original asks for the player instance in every branch and does not check the item
    if (reward->m_experience > 0) {
        CBH_Player::GetInstance()->AddXp(reward->m_experience, GAIN_SOURCE_CHALLENGE);
    }
    if (reward->m_cash > 0) {
        CBH_Player::GetInstance()->AddMoney(reward->m_cash, GAIN_SOURCE_CHALLENGE);
    }
    if (reward->m_credits > 0) {
        CBH_Player::GetInstance()->AddCredits(reward->m_credits);
    }
    if (reward->m_itemManager != NULL) {
        reward->m_itemManager->getItemById(reward->m_itemId)->setAvailable(true);
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
    UpdateRewards();
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
        return reward->m_itemManager->getItemById(reward->m_itemId)->GetName();
    }
    return XString("");
}

void CChallenge::Start(int finishTime)
{
    // the original neither resets the rewards nor the completion time here; it saves the manager
    m_finishTime = finishTime;
    m_active = true;
    m_invitedFriends.removeAllElements();
    static_cast<App*>(WindowApp::m_instance)->m_challengeManager->Save();
}

void CChallenge::Load(TiXmlElement* parent)
{
    TiXmlElement* section = parent->FirstChildElement(XString::AnsiString(XString::Format(L"challenge_%u", m_id).c_str()));
    if (section == NULL) {
        return;
    }
    // Original: the reward nodes are not checked; each "reward" defaults to the current value.
    m_active = CXmlHelper::GetAttributeValueOrUseDefault(section, "isAcive", XString("0")).ToBool();
    TiXmlElement* solo = section->FirstChildElement("solo_reward");
    m_rewards[0]->m_awarded = CXmlHelper::GetAttributeValueOrUseDefault(solo, "isAwarded", XString("0")).ToBool();
    m_rewards[0]->m_experience = CXmlHelper::GetAttributeValueOrUseDefault(solo, "reward", XString(m_rewards[0]->m_experience)).ToInt();
    m_finishTime = CXmlHelper::GetAttributeValueOrUseDefault(section, "finishTime", XString("0")).ToInt();
    TiXmlElement* first = section->FirstChildElement("x1_reward");
    m_rewards[1]->m_awarded = CXmlHelper::GetAttributeValueOrUseDefault(first, "isAwarded", XString("0")).ToBool();
    m_rewards[1]->m_friendId = CXmlHelper::GetAttributeValueOrUseDefault(first, "friendId", XString("-1")).ToInt();
    m_rewards[1]->m_cash = CXmlHelper::GetAttributeValueOrUseDefault(first, "reward", XString(m_rewards[1]->m_cash)).ToInt();
    m_rewards[1]->m_friendName = CXmlHelper::GetAttributeValueOrUseDefault(first, "friendName", XString(""));
    TiXmlElement* second = section->FirstChildElement("x2_reward");
    m_rewards[2]->m_awarded = CXmlHelper::GetAttributeValueOrUseDefault(second, "isAwarded", XString("0")).ToBool();
    m_rewards[2]->m_friendId = CXmlHelper::GetAttributeValueOrUseDefault(second, "friendId", XString("-1")).ToInt();
    m_rewards[2]->m_friendName = CXmlHelper::GetAttributeValueOrUseDefault(second, "friendName", XString(""));
    LoadInvitedFirends(section);
}

void CChallenge::LoadInvitedFirends(TiXmlElement* section)
{
    m_invitedFriends.removeAllElements();
    TiXmlElement* invited = section->FirstChildElement("invited_friends");
    if (invited == NULL) {
        return;
    }
    // Original: only -1 is skipped; duplicates are not filtered.
    for (TiXmlElement* entry = invited->FirstChildElement("friend"); entry != NULL;
         entry = entry->NextSiblingElement("friend")) {
        int id = CXmlHelper::GetAttributeValueOrUseDefault(entry, "id", XString("-1")).ToInt();
        if (id != -1) {
            m_invitedFriends.addElement(id);
        }
    }
}

void CChallenge::Save(TiXmlElement* parent)
{
    // Original: builds a fresh node, appends a copy of it to the parent and schedules a save.
    TiXmlElement* section = new TiXmlElement(XString::AnsiString(XString::Format(L"challenge_%u", m_id).c_str()));
    section->SetAttribute("isAcive", m_active);
    section->SetAttribute("finishTime", m_finishTime);
    TiXmlElement* solo = new TiXmlElement("solo_reward");
    solo->SetAttribute("isAwarded", m_rewards[0]->m_awarded);
    solo->SetAttribute("reward", m_rewards[0]->m_experience);
    section->InsertEndChild(*solo);
    delete solo;
    TiXmlElement* first = new TiXmlElement("x1_reward");
    first->SetAttribute("isAwarded", m_rewards[1]->m_awarded);
    first->SetAttribute("friendId", m_rewards[1]->m_friendId);
    first->SetAttribute("friendName", XString::AnsiString(m_rewards[1]->m_friendName.c_str()));
    first->SetAttribute("reward", m_rewards[1]->m_cash);
    section->InsertEndChild(*first);
    delete first;
    TiXmlElement* second = new TiXmlElement("x2_reward");
    second->SetAttribute("isAwarded", m_rewards[2]->m_awarded);
    second->SetAttribute("friendId", m_rewards[2]->m_friendId);
    second->SetAttribute("friendName", XString::AnsiString(m_rewards[2]->m_friendName.c_str()));
    section->InsertEndChild(*second);
    delete second;
    SaveInvitedFriends(section);
    parent->InsertEndChild(*section);
    delete section;
    static_cast<App*>(WindowApp::m_instance)->m_saveManager->SaveOnNextUpdate(false);
}

void CChallenge::SaveInvitedFriends(TiXmlElement* section)
{
    TiXmlElement* invited = CXmlHelper::ObtainElement(section, XString("invited_friends"));
    invited->Clear();
    for (int index = 0; index < m_invitedFriends.size(); ++index) {
        TiXmlElement* entry = new TiXmlElement("friend");
        entry->SetAttribute("id", m_invitedFriends[index]);
        invited->InsertEndChild(*entry);
        delete entry;
    }
}

bool CChallenge::DoOverrideSoftReward(SChallengeReward* reward, COverrideManager& overrides, const XString& name)
{
    // the original resets a reward of another kind first and does not range-check the values
    if (reward->m_cash == 0) {
        ResetReward(*reward);
    }
    bool changed = false;
    unsigned long long amount;
    if (overrides.FindIntAttribute(CStrWChar(name.c_str()), CStrWChar(L"amount"), &amount)) {
        reward->m_cash = static_cast<int>(amount);
        changed = true;
    }
    CStrWChar image;
    if (overrides.FindStringAttribute(CStrWChar(name.c_str()), CStrWChar(L"image"), &image)) {
        reward->m_image = static_cast<App*>(WindowApp::m_instance)->InitResMap()->getResourceID(XString(image.GetWCharPtr()));
        changed = true;
    }
    return changed;
}

bool CChallenge::DoOverrideCashReward(SChallengeReward* reward, COverrideManager& overrides, const XString& name)
{
    if (reward->m_credits == 0) {
        ResetReward(*reward);
    }
    bool changed = false;
    unsigned long long amount;
    if (overrides.FindIntAttribute(CStrWChar(name.c_str()), CStrWChar(L"amount"), &amount)) {
        reward->m_credits = static_cast<int>(amount);
        changed = true;
    }
    CStrWChar image;
    if (overrides.FindStringAttribute(CStrWChar(name.c_str()), CStrWChar(L"image"), &image)) {
        reward->m_image = static_cast<App*>(WindowApp::m_instance)->InitResMap()->getResourceID(XString(image.GetWCharPtr()));
        changed = true;
    }
    return changed;
}

bool CChallenge::DoOverrideItemReward(SChallengeReward* reward, COverrideManager& overrides, const XString& name)
{
    // the original does not validate the item manager or the item id
    if (reward->m_itemManager == NULL) {
        ResetReward(*reward);
    }
    CStrWChar itemType;
    bool changed = false;
    if (overrides.FindStringAttribute(CStrWChar(name.c_str()), CStrWChar(L"item_type"), &itemType)) {
        reward->m_itemManager = App::GetItemManagerByName(XString(itemType.GetWCharPtr()));
        changed = true;
    }
    unsigned long long amount;
    if (overrides.FindIntAttribute(CStrWChar(name.c_str()), CStrWChar(L"amount"), &amount)) {
        reward->m_itemAmount = static_cast<int>(amount);
        changed = true;
    }
    unsigned long long itemId;
    if (overrides.FindIntAttribute(CStrWChar(name.c_str()), CStrWChar(L"item_id"), &itemId)) {
        reward->m_itemId = static_cast<int>(itemId);
        changed = true;
    }
    CStrWChar image;
    if (overrides.FindStringAttribute(CStrWChar(name.c_str()), CStrWChar(L"image"), &image)) {
        reward->m_image = static_cast<App*>(WindowApp::m_instance)->InitResMap()->getResourceID(XString(image.GetWCharPtr()));
        changed = true;
    }
    return changed;
}

bool CChallenge::DoOverride(COverrideManager& overrides, const XString& name)
{
    // the original reports a change whenever the attribute exists and does not validate the coefficient
    CStrWChar rewardType;
    bool changed = false;
    if (overrides.FindStringAttribute(CStrWChar(name.c_str()), CStrWChar(L"reward"), &rewardType)) {
        if (rewardType == L"cash") {
            DoOverrideCashReward(m_rewards[2], overrides, name);
        } else if (rewardType == L"soft") {
            DoOverrideSoftReward(m_rewards[2], overrides, name);
        } else if (rewardType == L"item") {
            DoOverrideItemReward(m_rewards[2], overrides, name);
        }
        changed = true;
    }
    CStrWChar value;
    if (overrides.FindStringAttribute(CStrWChar(name.c_str()), CStrWChar(L"difficulty_coef"), &value)) {
        m_difficulty = XString(value.GetWCharPtr()).ToFloat();
        changed = true;
    }
    return changed;
}

bool CChallenge::CheckOverride()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    COverrideManager* overrides = app->m_overrideManager;
    // the original does not check the override manager for NULL
    if (overrides->changesMark() == m_overrideRevision) {
        return false;
    }
    m_overrideRevision = overrides->changesMark();
    return DoOverride(*overrides, XString::Format(L"challenge_%d", m_id));
}

const XString& CChallenge::GetFriendName(int index)
{
    switch (index) {
    case 0:
        return m_rewards[1]->m_friendName;
    case 1:
        return m_rewards[2]->m_friendName;
    }
    // the original returns a reference to a released temporary here (kept as in the binary)
    return XString();
}

int CChallenge::GetFriendId(int index)
{
    // the original is a switch with one load per case
    switch (index) {
    case 0:
        return m_rewards[1]->m_friendId;
    case 1:
        return m_rewards[2]->m_friendId;
    }
    return -1;
}

typedef char CKChallengeSize[sizeof(CChallenge) == 0x40 ? 1 : -1];
typedef char CKChallengeRewardSize[sizeof(SChallengeReward) == 0x28 ? 1 : -1];
