// CGameAnalytics.cpp - CK event names and arguments passed to the Flurry adapter.
#include "CGameAnalytics.h"
#include "App.h"
#include "CFlurry.h"
#include "CFriendGatedMission.h"
#include "CFriendGatedMissionManager.h"
#include "CStrWChar.h"
#include "CStdUtil_Cocoa.h"

CStrChar CGameAnalytics::m_deferredToken;

void CGameAnalytics::logIAPBuyClicked(const XString& item)
{
    CStrWChar details(item.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_CURRENCY_IAP_BUY_CLICKED", details.GetWCharPtr(), 0, 0);
}

void CGameAnalytics::logCameraUnlocked(int cameraId)
{
    XString id(cameraId);
    CStrWChar details(id.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_ACTIVITY_CAMERA_UNLOCKED", details.GetWCharPtr(), 1, 0);
}

void CGameAnalytics::logMissionReceived(const XString& mission, int source)
{
    CStrWChar details(mission.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_ACTIVITY_MISSION_RECEIVED", details.GetWCharPtr(), 0, source);
}

void CGameAnalytics::logInformantUnlocked(int informantId)
{
    CStrWChar details;
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_ACTIVITY_INFORMANT_UNLOCKED", details.GetWCharPtr(), informantId, 0);
}

void CGameAnalytics::logSafehouseEnter()
{
    CStrWChar details;
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_ACTIVITY_SAFEHOUSE_ENTER", details.GetWCharPtr(), 0, 0);
}

void CGameAnalytics::logStoreEnter(EShopEnterFrom source)
{
    CStrWChar details;
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_ACTIVITY_STORE_ENTER", details.GetWCharPtr(), source, 0);
}

void CGameAnalytics::logLeaderboards()
{
    CStrWChar details;
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_LEADERBOARDS", details.GetWCharPtr(), 0, 0);
}

void CGameAnalytics::logGameRated(bool rated)
{
    const char* response = "no";
    if (rated) {
        response = "yes";
    }
    CStrWChar details(response);
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_GAME_RATED", details.GetWCharPtr(), 0, 0);
}

void CGameAnalytics::logSoundSwitch(bool enabled)
{
    const char* value = "off";
    if (enabled) {
        value = "on";
    }
    XString text(value);
    CStrWChar details(text.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_SOUND_SETTING_SWITCHED", details.GetWCharPtr(), 0, 0);
}

void CGameAnalytics::logMusicSwitch(bool enabled)
{
    const char* value = "off";
    if (enabled) {
        value = "on";
    }
    XString text(value);
    CStrWChar details(text.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_MUSIC_SETTING_SWITCHED", details.GetWCharPtr(), 0, 0);
}

void CGameAnalytics::logGservFbImported()
{
    XString time = GetTimeString();
    CStrWChar details(time.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_GSERV_FB_IMPORTED", details.GetWCharPtr(), 1, 0);
}

void CGameAnalytics::logGservGcImported()
{
    XString time = GetTimeString();
    CStrWChar details(time.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_GSERV_GC_IMPORTED", details.GetWCharPtr(), 1, 0);
}

void CGameAnalytics::logFriendInvited(const XString& name)
{
    CStrWChar details(name.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_FRIEND_INVITED", details.GetWCharPtr(), 1, 0);
}

void CGameAnalytics::logPostFbWall(const XString& message)
{
    CStrWChar details(message.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_POST_FB_WALL", details.GetWCharPtr(), 1, 0);
}

void CGameAnalytics::logRandomFriendTargetSet(const XString& name, int source)
{
    CStrWChar details(name.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_FRIEND_TARGET_RANDOM", details.GetWCharPtr(), source, 0);
}

void CGameAnalytics::logTotalFriendsCount(int facebook, int gameCenter)
{
    XString counts = XString::Format(L"FB: %u, GC %u", facebook, gameCenter);
    CStrWChar details(counts.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_GSERV_FRIENDS_TOTAL_NEW", details.GetWCharPtr(), 0, 0);
}

void CGameAnalytics::logFriendInvited(int sent, int remaining)
{
    XString counts = XString::Format(L"At once: %u, Remains: %u", sent, remaining);
    CStrWChar details(counts.c_str());
    CFlurry::LogCustomEvent("GSERV_INVITE", details.GetWCharPtr(), 0, 0);
}

void CGameAnalytics::logReactixUsed(int missionId, int time)
{
    CStrWChar details("");
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_GAMEPLAY_REACTIX_USED", details.GetWCharPtr(), missionId, time);
}

void CGameAnalytics::logSubContractAppear(unsigned int id)
{
    XString time = GetTimeString();
    CStrWChar details(time.c_str());
    CFlurry::LogCustomEvent("SUBCONTRACT_APPEAR", details.GetWCharPtr(), id, 0);
}

void CGameAnalytics::logSubContractDisappear(unsigned int id)
{
    XString time = GetTimeString();
    CStrWChar details(time.c_str());
    CFlurry::LogCustomEvent("SUBCONTRACT_DISAPPEAR", details.GetWCharPtr(), id, 0);
}

void CGameAnalytics::logSubContractCollected(unsigned int id)
{
    XString time = GetTimeString();
    CStrWChar details(time.c_str());
    CFlurry::LogCustomEvent("SUBCONTRACT_COLLECTED", details.GetWCharPtr(), id, 0);
}

void CGameAnalytics::logTutorialEvents(ETutorialEvent event)
{
    XString time = GetTimeString();
    CStrWChar details(time.c_str());
    CFlurry::LogCustomEvent("TUTORIAL_EVENTS", details.GetWCharPtr(), event, 0);
}

void CGameAnalytics::logVehicleMissionsMinigun(bool purchased)
{
    XString time = GetTimeString();
    CStrWChar details(time.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_VEHICLE_MISSIONS_MINIGUN", details.GetWCharPtr(), purchased, 0);
}

void CGameAnalytics::logGMGEntered()
{
    CStrWChar details;
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_GMG_ENTERED", details.GetWCharPtr(), 0, 0);
}

void CGameAnalytics::logZombiesPromoPressed()
{
    CStrWChar details;
    CFlurry::LogCustomEvent("CK_ZOMBIES_PROMO", details.GetWCharPtr(), 0, 0);
}

void CGameAnalytics::logTrickShotEarned(const XString& name)
{
    CStrWChar details(name.c_str());
    CFlurry::LogCustomEvent("TRICK_SHOT_EARNED", details.GetWCharPtr(), CStdUtil_Cocoa::GetTimeSeconds(), 0);
}

void CGameAnalytics::logGameplayEnter(const XString& mission, int money, int experience)
{
    CStrWChar details(mission.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_ACTIVITY_GAMEPLAY_ENTER", details.GetWCharPtr(), money, experience);
}

void CGameAnalytics::logCoverUsed(const XString& mission, int weaponId)
{
    CStrWChar details(mission.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_GAMEPLAY_CROUCH", details.GetWCharPtr(), weaponId, 0);
}

void CGameAnalytics::logMissionStarted(const XString& mission)
{
    CStrWChar details(mission.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_ACTIVITY_MISSION_STARTED", details.GetWCharPtr(), 0, 0);
}

void CGameAnalytics::logMissionFailed(const XString& mission, int reason)
{
    CStrWChar details(mission.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_ACTIVITY_MISSION_FAILED", details.GetWCharPtr(), 0, reason);
}

// Overload with the reason first: logs the "MISSION_FAILURE" event, reason goes into value1.
void CGameAnalytics::logMissionFailed(int reason, const XString& mission)
{
    CStrWChar details(mission.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_MISSION_FAILURE", details.GetWCharPtr(), reason, 0);
}

void CGameAnalytics::logMissionComplete(const XString& mission)
{
    CStrWChar details(mission.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_ACTIVITY_MISSION_COMPLETE", details.GetWCharPtr(), 0, 0);
}

void CGameAnalytics::logNotEnoughEnergy(int remaining)
{
    CStrWChar details;
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_NOT_ENOUGH_ENERGY", details.GetWCharPtr(), remaining, 0);
}

void CGameAnalytics::logPrimaryWeaponHits(int hits, const XString& mission, int weaponId)
{
    CStrWChar details(mission.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_ACTIVITY_HIT_TARGET_PRIMARY", details.GetWCharPtr(), hits, weaponId);
}

void CGameAnalytics::logSecondaryWeaponHits(int hits, const XString& mission, int weaponId)
{
    CStrWChar details(mission.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_ACTIVITY_HIT_TARGET_SECONDARY", details.GetWCharPtr(), hits, weaponId);
}

void CGameAnalytics::logPrimaryWeaponKills(int kills, const XString& mission, int weaponId)
{
    CStrWChar details(mission.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_ACTIVITY_KILL_TARGET_PRIMARY", details.GetWCharPtr(), kills, weaponId);
}

void CGameAnalytics::logSecondaryWeaponKills(int kills, const XString& mission, int weaponId)
{
    CStrWChar details(mission.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_ACTIVITY_KILL_TARGET_SECONDARY", details.GetWCharPtr(), kills, weaponId);
}

void CGameAnalytics::logHeadshots(int count, const XString& mission)
{
    CStrWChar details(mission.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_ACTIVITY_HEADSHOT", details.GetWCharPtr(), count, 0);
}

void CGameAnalytics::logFriendsGatedCompleted(int missionId)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CFriendGatedMission* mission = app->m_friendMissionManager->GetMissionById(missionId);
    if (mission != NULL) {
        XString time = GetTimeString();
        CStrWChar details(time.c_str());
        CFlurry::LogCustomEvent("FRIENDS_GATED_COMPLETED", details.GetWCharPtr(), mission->m_friendCount, 0);
    }
}

void CGameAnalytics::logWeaponGatedCompleted(const XString& category)
{
    CStrWChar details(category.c_str());
    CFlurry::LogCustomEvent("WEAPON_GATED_COMPLETED", details.GetWCharPtr(), CStdUtil_Cocoa::GetTimeSeconds(), 0);
}

void CGameAnalytics::logItemEquipped(const XString& item, int, ECurrencyType)
{
    CStrWChar info(item.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_ITEM_EQUIPPED", info.GetWCharPtr(), 0, 0);
}

void CGameAnalytics::logSwitchWeapon(const XString& mission, int id)
{
    CStrWChar info(mission.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_GAMEPLAY_SWITCH_WEAPON", info.GetWCharPtr(), id, 0);
}

void CGameAnalytics::logLevelGained(int level, int cash, int credits)
{
    XString text(cash);
    CStrWChar info(text.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_PROGRESSION_LEVEL_ACHIEVED", info.GetWCharPtr(), level, credits);
}

void CGameAnalytics::logXPGain(int amount, int remaining)
{
    XString text(remaining);
    CStrWChar info(text.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_PROGRESSION_EXP_GAINED", info.GetWCharPtr(), amount, 0);
}

void CGameAnalytics::SendTapJoyID(const XString& id)
{
    CStrWChar info(id.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_TAPJOY_ID", info.GetWCharPtr(), 0, 0);
}

void CGameAnalytics::SendAdMarvelID(int id)
{
    XString text = XString::Format(L"%d", id);
    CStrWChar info(text.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_ADMARVEL_ID", info.GetWCharPtr(), 0, 0);
}

void CGameAnalytics::SendPushTokenImpl(const char* token)
{
    CStrWChar info(token);
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_PUSH_TOKEN", info.GetWCharPtr(), 0, 0);
}

void CGameAnalytics::SendDeferredPushToken()
{
    if (m_deferredToken.GetLength() > 0) {
        SendPushTokenImpl(m_deferredToken.GetCharPtr());
        m_deferredToken = "";
    }
}

void CGameAnalytics::SendPushStatistics(const char* token)
{
    m_deferredToken = token;
}

void CGameAnalytics::logCurrencyChange(const XString& source, int amount, ECurrencyType type)
{
    if (amount != 0) {
        CStrWChar info(source.c_str());
        CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_CURRENCY_IAP_BOUGHT", info.GetWCharPtr(), amount, type);
    }
}

void CGameAnalytics::logCurrentCash(int cash)
{
    XString text = XString::Format(L"%d", cash);
    CStrWChar info(text.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_CURRENCY_TOTAL_SOFT", info.GetWCharPtr(), 0, 0);
}

void CGameAnalytics::logGiftAccepted(int source, const XString& item)
{
    CStrWChar details(item.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_GIFT_ACCEPTED", details.GetWCharPtr(), source, 0);
}

void CGameAnalytics::logGiftSentIngame(int friendId, const XString& item)
{
    CStrWChar details(item.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_GIFT_SENT_INGAME", details.GetWCharPtr(), friendId, 0);
}

void CGameAnalytics::logFriendTargetMission(int friendId, const XString& bounty)
{
    CStrWChar details(bounty.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_FRIEND_TARGET_SET", details.GetWCharPtr(), friendId, 0);
}

void CGameAnalytics::logFriendTargetRevenge(int friendId, const XString& bounty)
{
    CStrWChar details(bounty.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_FRIEND_TARGET_REVENGE", details.GetWCharPtr(), friendId, 0);
}

void CGameAnalytics::logCurrencyRecieved(int money, int credits)
{
    XString text = XString::Format(L"Cash: %u, Credits: %u", money, credits);
    CStrWChar details(text.c_str());
    CFlurry::LogCustomEvent("GSERV_CURRENCY_RECEIVED", details.GetWCharPtr(), 0, 0);
}

void CGameAnalytics::logCurrentContractCredits(int credits)
{
    XString text = XString::Format(L"%d", credits);
    CStrWChar info(text.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_CURRENCY_TOTAL_HARD", info.GetWCharPtr(), 0, 0);
}

void CGameAnalytics::logEnergyValue(int energy)
{
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_ENERGY_VALUE", static_cast<const wchar_t*>(NULL), energy, 0);
}

void CGameAnalytics::logItemViewed(const XString& item, int price, ECurrencyType type)
{
    CStrWChar info(item.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_ITEM_VIEWED", info.GetWCharPtr(), price, type);
}

void CGameAnalytics::logBuyItemClicked(const XString& item, int, ECurrencyType)
{
    CStrWChar info(item.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_ITEM_BUY_CLICKED", info.GetWCharPtr(), 0, 0);
}

void CGameAnalytics::logIngameItemPurchased(const XString& item, int, ECurrencyType)
{
    CStrWChar info(item.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_INGAME_ITEM_PURCHASED", info.GetWCharPtr(), 0, 0);
}

void CGameAnalytics::logItemUsed(const XString& item)
{
    CStrWChar info(item.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_ITEM_USED", info.GetWCharPtr(), 0, 0);
}

void CGameAnalytics::logItemPurchased(const XString& item, int, ECurrencyType)
{
    CStrWChar info(item.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_ITEM_PURCHASED", info.GetWCharPtr(), 0, 0);
}

void CGameAnalytics::logRandomWeaponMissionPurchased(const XString& item, bool purchased)
{
    CStrWChar info(item.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_RANDOM_WEAPON_MISSION", info.GetWCharPtr(), purchased, 0);
}

void CGameAnalytics::logChalengeShared(unsigned int friendCount)
{
    XString time = GetTimeString();
    CStrWChar info(time.c_str());
    CFlurry::LogCustomEvent("CHALLENGE_SHARED", info.GetWCharPtr(), friendCount, 0);
}

void CGameAnalytics::logChalengeCompleted(EChallengeCompleteState state)
{
    XString text = XString::Format(L"%d", state);
    CStrWChar details(text.c_str());
    CFlurry::LogCustomEvent("CHALLENGE_COMPLETED", details.GetWCharPtr(), CStdUtil_Cocoa::GetTimeSeconds(), 0);
}

void CGameAnalytics::logAssassinsInviteAccepted(int friendId)
{
    XString time = GetTimeString();
    CStrWChar details(time.c_str());
    CFlurry::LogCustomEvent("ASSASSINS_INVITE_ACCEPTED", details.GetWCharPtr(), friendId, 0);
}

void CGameAnalytics::logAssassinsInviteDeclined(int friendId)
{
    XString time = GetTimeString();
    CStrWChar details(time.c_str());
    CFlurry::LogCustomEvent("ASSASSINS_INVITE_DECLINED", details.GetWCharPtr(), friendId, 0);
}

void CGameAnalytics::logIAPPurchased(const XString& item, int level)
{
    CStrWChar info(item.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_IAP_PURCHASED", info.GetWCharPtr(), level, 0);
}

void CGameAnalytics::logIAPPurchaseCanceled(const XString& item)
{
    CStrWChar info(item.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_CURRENCY_IAP_PURCHASE_CANCELLED", info.GetWCharPtr(), 0, 0);
}

void CGameAnalytics::logIAPPurchaseFailed(const XString& item)
{
    CStrWChar info(item.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_CURRENCY_IAP_PURCHASE_FAILED", info.GetWCharPtr(), 0, 0);
}

void CGameAnalytics::logDailyBonus(const XString& bonus, int day)
{
    CStrWChar info(bonus.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_DAILY_BONUS_ITEM", info.GetWCharPtr(), day, 0);
}

void CGameAnalytics::logNewDailyBonus(const XString& bonus, int day)
{
    CStrWChar info(bonus.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_NEW_DAILY_BONUS_ITEM", info.GetWCharPtr(), day, 0);
}

void CGameAnalytics::logFireButtonSwitch(bool enabled)
{
    const char* state = "off";
    if (enabled) {
        state = "on";
    }
    XString text(state);
    CStrWChar info(text.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_FIRE_BUTTON_SETTING_CHANGED", info.GetWCharPtr(), 0, 0);
}

void CGameAnalytics::logScopeActivated(const XString& mission, int weaponId)
{
    CStrWChar details(mission.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_GAMEPLAY_SCOPE_ACTIVATED", details.GetWCharPtr(), weaponId, 0);
}

void CGameAnalytics::logSteadyActivated(const XString& mission, int weaponId)
{
    CStrWChar info(mission.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_GAMEPLAY_STEADY_ACTIVATED", info.GetWCharPtr(), weaponId, 0);
}

XString CGameAnalytics::GetTimeString()
{
    CStdTime* time = CStdUtil_Cocoa::LocalTime(CStdUtil_Cocoa::GetTimeSeconds());
    return XString::Format(L"%02d:%02d:%02d %02d.%02d.%02d", time->m_hour, time->m_minute, time->m_second,
                           time->m_dayOfMonth, time->m_month + 1, time->m_year - 100);
}

void CGameAnalytics::logGservLogin(int networks)
{
    XString time = GetTimeString();
    CStrWChar details(time.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_GSERV_LOGIN", details.GetWCharPtr(), networks, 0);
}

void CGameAnalytics::logGservLogout(int networks)
{
    XString time = GetTimeString();
    CStrWChar details(time.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_GSERV_LOGOUT", details.GetWCharPtr(), networks, 0);
}

void CGameAnalytics::logFriendAccepted(const XString& friendName, int amount)
{
    CStrWChar details(friendName.c_str());
    CFlurry::LogCustomEvent("BOUNTY_EVT_TYPE_GSERV_SUCCESSFUL_INSTALL", details.GetWCharPtr(), amount, 0);
}
