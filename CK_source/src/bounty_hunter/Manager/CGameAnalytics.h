// CGameAnalytics.h - analytics events of the game (Flurry custom events), the push token and the
// kill / skillshot events that are collected during a mission and sent afterwards.
#ifndef CGAMEANALYTICS_H
#define CGAMEANALYTICS_H

#include "xString.h"
#include "Vector_ss.h"
#include "CStrChar.h"
#include "CMission.h"

class CGameAnalytics {
public:
    /** Currencies reported by logTotalCurrency. */
    enum ECurrencyType {
        CURRENCY_SOFT = 0,
        CURRENCY_HARD = 1,
        CURRENCY_IAP = 2
    };

    /** Result of a one-time offer / daily deal dialog (values not known yet). */
    enum EOTOResult {
        OTO_RESULT_NONE = 0
    };

    /** How a zombie was killed (values not known yet). */
    enum EKillType {
        KILL_TYPE_NONE = 0
    };

    enum EChallengeCompleteState {
        CHALLENGE_COMPLETE_SOLO = 1,
        CHALLENGE_COMPLETE_ONE_FRIEND = 2,
        CHALLENGE_COMPLETE_TWO_FRIENDS = 3
    };

    enum EShopEnterFrom {
        SHOP_ENTER_GPS = 0,
        SHOP_ENTER_GPS_ITEM = 1,
        SHOP_ENTER_NETWORK = 2,
        SHOP_ENTER_MISSION_PROMPT = 3
    };

    enum ETutorialEvent {
        TUTORIAL_EVENT_FIRST_MISSION_RESULT = 0,
        TUTORIAL_EVENT_STORY_INFORMANT_SELECTED = 2,
        TUTORIAL_EVENT_ENTERED_SAFEHOUSE = 5,
        TUTORIAL_EVENT_LEFT_SAFEHOUSE = 6,
        TUTORIAL_EVENT_INFORMANT_PURCHASED = 7,
        TUTORIAL_EVENT_FINISHED = 9
    };

    /** A deferred "zombie killed" event. */
    struct SZombieKilled {
        XString m_name;         // 0x00
        int m_value;            // 0x04
        EKillType m_killType;   // 0x08
    };

    /** A deferred "skillshot" event. */
    struct SSkillshot {
        XString m_name;         // 0x00
        int m_value;            // 0x04
    };

    static void SendPushTokenImpl(const char* token);
    /** Send the push token stored by SendPushStatistics (once). */
    static void SendDeferredPushToken();
    /** Keep `token` for SendDeferredPushToken. */
    static void SendPushStatistics(const char* token);
    static void SendTapJoyID(const XString& id);
    static void SendAdMarvelID(int id);
    static void logAppStart();
    static void logAppStop();
    static void logAppBackground();
    static void logAppForeground();
    static void logPushNotification(bool inForeground);
    static void logGMGEntered();
    static void logZombiesPromoPressed();
    static void logCurrencyChange(XString source, int amount, int total, ECurrencyType type);
    static void logCurrencyChange(const XString& source, int amount, ECurrencyType type);
    static void logGiftAccepted(int source, const XString& item);  // added by ui (CK, used by CServerGiftDialog)
    static void logGiftSentIngame(int friendId, const XString& item);
    static void logCurrencyRecieved(int money, int credits);  // added by ui (CK, used by CServerGiftDialog)
    static void logTotalCurrency(int playerId, int amount, ECurrencyType type);
    static void logIAPBuyClicked(XString item, int price, ECurrencyType type);
    static void logIAPBuyClicked(const XString& item);
    static void logIAPPurchaseSuccessful(XString item, int price, ECurrencyType type);
    static void logIAPAfterFail(XString item);
    static void logRecommendedWeaponPurchase(XString item, int weaponId);
    static void logIAPPurchaseCanceled(XString item, int price, ECurrencyType type);
    static void logIAPPurchaseFailed(XString item, int price, ECurrencyType type);
    static void logIAPPurchaseCanceled(const XString& item);
    static void logIAPPurchaseFailed(const XString& item);
    static void logIAPPurchased(const XString& item, int level);
    static void logCurrentCash(int cash);
    static void logCurrentContractCredits(int credits);
    static void logEnergyValue(int energy);
    static void logLevelGained(int level);
    static void logLevelGained(int level, int cash, int credits);
    static void logXPGain(int amount, int remaining);
    static void logDailyBonus(const XString& bonus, int day);
    static void logNewDailyBonus(const XString& bonus, int day);
    static void logOneTimeOfferResult(const XString& item, int price, EOTOResult result);
    static void logDailyDealResult(const XString& item, int price, EOTOResult result);
    static void logTutorialStepCompleted(int step);
    static void logMissionStarted(XString mission, CMission::EMissionDifficulty difficulty);
    static void logMissionCompleted(XString mission, int value1, int value2);
    static void logMissionCompleteScore(XString mission, int score, int value);
    static void logHelicopterMissionCompleted(int mission, int value);
    static void logMissionQuit(XString mission, int value);
    static void logMissionFailed(XString mission, int value1, int value2);
    static void logGameplayEnter(const XString& mission, int money, int experience);
    static void logCoverUsed(const XString& mission, int weaponId);
    static void logMissionStarted(const XString& mission);
    static void logMissionFailed(const XString& mission, int reason);
    static void logMissionFailed(int reason, const XString& mission);
    static void logMissionComplete(const XString& mission);
    static void logNotEnoughEnergy(int remaining);
    static void logPrimaryWeaponHits(int hits, const XString& mission, int weaponId);
    static void logSecondaryWeaponHits(int hits, const XString& mission, int weaponId);
    static void logPrimaryWeaponKills(int kills, const XString& mission, int weaponId);
    static void logSecondaryWeaponKills(int kills, const XString& mission, int weaponId);
    static void logHeadshots(int count, const XString& mission);
    static void logFriendsGatedCompleted(int missionId);
    static void logWeaponGatedCompleted(const XString& category);
    static void logSoundSwitch(bool enabled);
    static void logMusicSwitch(bool enabled);
    static void logPushSwitch(bool enabled);
    static void logFireButtonSwitch(bool enabled);
    static void logItemViewed(const XString& item, int price, ECurrencyType type);
    static void logBuyItemClicked(const XString& item, int price, ECurrencyType type);
    static void logItemPurchased(const XString& item, int price, ECurrencyType type);
    static void logIngameItemPurchased(const XString& item, int price, ECurrencyType type);
    static void logItemUsed(const XString& item);
    static void logRandomWeaponMissionPurchased(const XString& item, bool purchased);
    static void logChalengeShared(unsigned int friendCount);
    static void logChalengeCompleted(EChallengeCompleteState state);
    static void logAssassinsInviteAccepted(int friendId);
    static void logAssassinsInviteDeclined(int friendId);
    static void logFriendTargetMission(int friendId, const XString& bounty);
    static void logFriendTargetRevenge(int friendId, const XString& bounty);
    static void logItemPurchaseCancelled(XString item, int price, ECurrencyType type);
    static void logItemEquipped(XString item);
    static void logItemEquipped(const XString& item, int price, ECurrencyType currency);
    static void logSwitchWeapon(const XString& mission, int id);
    /** Send the collected kill / skillshot events. */
    static void flushDeferredEvents();
    /** `deferred`: only collect the event (sent by flushDeferredEvents). */
    static void logZombieKilled(XString name, int value, EKillType killType, bool deferred);
    static void logSkillshot(XString name, int value, bool deferred);
    static void logMenuAccessed(XString menu);
    static void logCycleScoreAchieved(int cycle, int score, int value);
    static void logAdShown(int value);
    static void logAdBonus(int value1, int value2);
    static void logSurvivalModeFinishedWave(int wave, int value);
    static void logSurvivalModeZombiesKilled(int wave, int value);
    static void logSurvivalModeXpEarned(int wave, int value);
    static void logTrickShotEarned(const XString& name);
    static void logSurvivalModeSoftEarned(int wave, int value);
    static void logSurvivalModeFinalScore(int wave, int value);
    static void logSurvivalModeShopEntered(int wave, bool entered);
    static void logSurvivalModeWeaponPurchased(int wave, int weaponId);
    static void logCivilianRevivePurchased();
    static void logScopeActivated(const XString& missionName, int weaponId);  // added by weapon
    static void logSteadyActivated(const XString& mission, int weaponId);
    /** 1 Facebook, 2 Game Center, 3 both networks (CNotificationHandler). */  // added by gserve
    static XString GetTimeString();
    static void logGservLogin(int networks);  // added by gserve
    static void logGservLogout(int networks);  // added by gserve
    static void logFriendAccepted(const XString& friendName, int amount);  // added by app
    static void logCameraUnlocked(int cameraId);
    static void logMissionReceived(const XString& mission, int source);
    static void logInformantUnlocked(int informantId);
    static void logSafehouseEnter();
    static void logStoreEnter(EShopEnterFrom source);
    static void logLeaderboards();
    static void logGameRated(bool rated);
    static void logGservFbImported();
    static void logGservGcImported();
    static void logFriendInvited(const XString& name);
    static void logFriendInvited(int sent, int remaining);
    static void logPostFbWall(const XString& message);
    static void logRandomFriendTargetSet(const XString& name, int source);
    static void logTotalFriendsCount(int facebook, int gameCenter);
    static void logReactixUsed(int missionId, int time);
    static void logSubContractAppear(unsigned int id);
    static void logSubContractDisappear(unsigned int id);
    static void logSubContractCollected(unsigned int id);
    static void logTutorialEvents(ETutorialEvent event);
    static void logVehicleMissionsMinigun(bool purchased);

    static CStrChar m_deferredToken;
    static CStrChar m_lastDeferredToken;
    static Vector<SZombieKilled> m_zombieKilled;
    static Vector<SSkillshot> m_skillshot;
    /** Set while a purchase follows a failed mission (logIAPPurchaseSuccessful also sends logIAPAfterFail). */
    static bool g_logMissionFailure;
};

#endif
