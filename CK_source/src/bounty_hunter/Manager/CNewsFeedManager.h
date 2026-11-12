// CNewsFeedManager.h - CK's transient news entries and read state.
#ifndef CNEWSFEEDMANAGER_H
#define CNEWSFEEDMANAGER_H

#include "CStrWChar.h"
#include "Vector_ss.h"

enum ENewsType {
    NEWS_TARGETTED = 0,
    NEWS_MISSION_INVITE,
    NEWS_FRIEND_JOINED,
    NEWS_FRIEND_INVITED,
    NEWS_GIFT,
    NEWS_AWARD,
    NEWS_GUILD_INVITE,
    NEWS_GUILD_ACCEPTED,
    NEWS_GUILD_DECLINED,
    NEWS_CHALLENGE_INVITE,
    NEWS_CHALLENGE_ACCEPTED,
    NEWS_CHALLENGE_DECLINED,
    NEWS_CHALLENGE_COMPLETED,
    NEWS_EMPTY
};

struct SMessage {
    ENewsType m_type;         // 0x00
    int m_senderId;           // 0x04
    mutable bool m_read;      // 0x08
    bool m_flag09;            // 0x09
    bool m_highlighted;       // 0x0a
};

struct STargettedMessage : SMessage {
    CStrWChar m_text;          // 0x0c
};

struct SFriendJoinedGameMessage : STargettedMessage {};

struct SMissionInviteMessage : SMessage {
    int m_missionId;          // 0x0c
    mutable bool m_accepted;  // 0x10
    mutable bool m_acknowledged;
    int m_messageId;          // 0x14
};

struct SGiftReceivedMessage : SMessage {
    int m_messageId;          // 0x0c
    mutable bool m_claimed;   // 0x10
    mutable bool m_acknowledged; // 0x11
    int m_itemIndex;          // 0x14
};

struct SServerAwardMessage : SMessage {
    int m_commonCurrency;     // 0x0c
    int m_rareCurrency;       // 0x10
    int m_experience;         // 0x14
    int m_itemIndex;          // 0x18
    int m_messageId;          // 0x1c
    CStrWChar m_text;          // 0x20
};

class CNewsFeedManager {
public:
    CNewsFeedManager();
    ~CNewsFeedManager();
    void Clear();
    void AddTargettedMessage(int senderId, const CStrWChar& text, bool highlighted);
    void AddMissionInviteMessage(int senderId, int missionId);
    void AddFriendJoinedGame(CStrWChar text);
    void AddFriendInvitedToGame(CStrWChar text);
    void AddGiftReceivedMessage(int senderId, int itemIndex, int messageId);
    void AddServerAwardMessage(const CStrWChar& text, int common, int rare, int experience,
                               int itemIndex, int messageId);
    void AddGuildInviteMessage(int senderId, const CStrWChar& text);
    void AddGuildInviteResponseMessage(int senderId, bool accepted);
    void AddChallengeInviteMessage(int senderId, int challengeId, int messageId);
    void AddChallengeResponseMessage(int senderId, int challengeId, bool accepted);
    void AddFriendCompletedChallengeMessage(int senderId, int challengeId, const CStrWChar& text, int messageId);
    void AddEmptyNewsMessage();
    void MarkAllMessagesAsRead();
    int GetUnreadNonEmptyMessageCount();
    int GetUnreadNonEmptyMessageCount(ENewsType type);
    int GetNewsCount();
    int GetNewsCount(ENewsType type);
    int GetNewsByType(ENewsType type, Vector<const SMessage*>& news);
    void RemoveMessage(const SMessage* message);
    void RemoveMessage(const SMessage* message, int ignored);
    const SMessage* GetNewsAt(int index);

    Vector<const SMessage*> m_messages;
    const SMessage* m_emptyMessage; // 0x10

private:
    void AppendMessage(const SMessage* message);
    bool HasChallengeMessage(int messageId) const;
};

#endif
