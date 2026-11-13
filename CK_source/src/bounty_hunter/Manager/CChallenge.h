#ifndef CCHALLENGE_H
#define CCHALLENGE_H

#include "Vector_ss.h"
#include "xString.h"

class CItemManager;
class COverrideManager;
class TiXmlElement;
struct lua_State;

struct SChallengeReward {
    SChallengeReward();

    int m_credits;
    int m_cash;
    int m_experience;
    int m_itemAmount;
    int m_itemId;
    CItemManager* m_itemManager;
    const char* m_image;
    int m_friendId;
    XString m_friendName;
    bool m_awarded;
};

class CChallenge {
    Vector<int> m_invitedFriends;
    SChallengeReward** m_rewards;
    int m_overrideRevision;

public:
    explicit CChallenge(TiXmlElement* element);
    ~CChallenge();
    void CheckCompletion();
    int GetCurrProgress() const;
    void UpdateRewards() const;
    void Start(int finishTime);
    void Load(TiXmlElement* parent);
    void Save(TiXmlElement* parent);
    const SChallengeReward* GetSoloReward() const;
    const SChallengeReward* GetX1Reward() const;
    const SChallengeReward* GetX2Reward() const;
    XString GetRewardString(const SChallengeReward* reward);
    void FriendInvited(int friendId);
    bool IsFriendInvited(int friendId);
    bool FriendCompleteChallenge(int friendId);
    SChallengeReward* GetRewardForFriend(int friendId);
    bool CheckOverride();
    const XString& GetFriendName(int index);
    int GetFriendId(int index);

    int m_day;
    int m_softRewardCap;
    lua_State* m_script;
    int m_id;
    int m_goal;
    XString m_name;
    int m_finishTime;
    int m_completedAt;
    bool m_active;
    bool m_hasCustomCheck;
    float m_difficulty;

private:
    void ParseRoot(TiXmlElement* element);
    void ParseRewards(TiXmlElement* element);
    static void RegisterNativeClasses(lua_State* script);
    void ParseScript(TiXmlElement* element, lua_State* script);
    void SaveInvitedFriends(TiXmlElement* section);
    void LoadInvitedFirends(TiXmlElement* section);
    SChallengeReward* ParseReward(TiXmlElement* element);
    int CalculateSoloReward() const;
    void CallExternalCheckFunc();
    void OnComplete();
    void SendCompletionMessageToFriends();
    void SendCompletionMessage(int friendId);
    void CheckDelayedReward(int friendId);
    void GiveReward(const SChallengeReward* reward) const;
    bool DoOverride(COverrideManager& overrides, const XString& name);
    bool DoOverrideItemReward(SChallengeReward* reward, COverrideManager& overrides, const XString& name);
    bool DoOverrideSoftReward(SChallengeReward* reward, COverrideManager& overrides, const XString& name);
    bool DoOverrideCashReward(SChallengeReward* reward, COverrideManager& overrides, const XString& name);

};

#endif
