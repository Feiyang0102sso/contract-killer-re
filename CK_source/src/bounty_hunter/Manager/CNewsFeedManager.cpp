// CNewsFeedManager.cpp - CK message records and news list.
#include "CNewsFeedManager.h"

CNewsFeedManager::CNewsFeedManager()
{
    // The vector's default constructor is the complete shipped initialization.
}

CNewsFeedManager::~CNewsFeedManager()
{
    SAFE_DELETE_VECTOR(m_messages);
}

void CNewsFeedManager::Clear()
{
    for (int index = 0; index < m_messages.size(); index++) {
        const SMessage* message = m_messages[index];
        switch (message->m_type) {
        case NEWS_FRIEND_JOINED:
            delete static_cast<const SFriendJoinedGameMessage*>(message);
            break;
        case NEWS_TARGETTED:
        case NEWS_FRIEND_INVITED:
        case NEWS_GUILD_INVITE:
            delete static_cast<const STargettedMessage*>(message);
            break;
        case NEWS_AWARD:
            delete static_cast<const SServerAwardMessage*>(message);
            break;
        case NEWS_MISSION_INVITE:
        case NEWS_CHALLENGE_INVITE:
        case NEWS_CHALLENGE_ACCEPTED:
        case NEWS_CHALLENGE_DECLINED:
        case NEWS_CHALLENGE_COMPLETED:
            delete static_cast<const SMissionInviteMessage*>(message);
            break;
        case NEWS_GIFT:
            delete static_cast<const SGiftReceivedMessage*>(message);
            break;
        default:
            delete message;
            break;
        }
    }
    m_messages.removeAllElements();
    m_emptyMessage = NULL;
}

void CNewsFeedManager::AppendMessage(const SMessage* message)
{
    // AddEmptyNewsMessage appends the placeholder. The constructor only initializes the vector.
    int last = m_messages.size() - 1;
    if (last >= 0 && m_messages[last]->m_type == NEWS_EMPTY) {
        delete m_messages[last];
        m_messages.removeElementAt(last);
        m_emptyMessage = NULL;
    }
    m_messages.addElement(message);
}

void CNewsFeedManager::AddTargettedMessage(int senderId, const CStrWChar& text, bool highlighted)
{
    for (int index = 0; index < m_messages.size(); ++index) {
        const SMessage* message = m_messages[index];
        if (message->m_type == NEWS_TARGETTED && message->m_senderId == senderId
            && message->m_highlighted == highlighted && static_cast<const STargettedMessage*>(message)->m_text == text) {
            return;
        }
    }
    STargettedMessage* message = new STargettedMessage();
    message->m_type = NEWS_TARGETTED;
    message->m_senderId = senderId;
    message->m_highlighted = highlighted;
    message->m_text = text;
    AppendMessage(message);
}

void CNewsFeedManager::RemoveMessage(const SMessage* message)
{
    // The original first checks membership, then repeats the search; it does not delete the message.
    if (m_messages.indexOf(message, 0) != -1) {
        m_messages.removeElementAt(m_messages.indexOf(message, 0));
    }
}

void CNewsFeedManager::RemoveMessage(const SMessage* message, int ignored)
{
    RemoveMessage(message);
}

const SMessage* CNewsFeedManager::GetNewsAt(int index)
{
    return m_messages[index];
}

void CNewsFeedManager::AddMissionInviteMessage(int senderId, int missionId)
{
    for (int index = 0; index < m_messages.size(); ++index) {
        const SMessage* message = m_messages[index];
        if (message->m_type == NEWS_MISSION_INVITE && message->m_senderId == senderId
            && static_cast<const SMissionInviteMessage*>(message)->m_missionId == missionId) {
            return;
        }
    }
    SMissionInviteMessage* message = new SMissionInviteMessage();
    message->m_type = NEWS_MISSION_INVITE;
    message->m_senderId = senderId;
    message->m_missionId = missionId;
    AppendMessage(message);
}

void CNewsFeedManager::AddFriendJoinedGame(CStrWChar text)
{
    SFriendJoinedGameMessage* message = new SFriendJoinedGameMessage();
    message->m_type = NEWS_FRIEND_JOINED;
    message->m_text = text;
    AppendMessage(message);
}

void CNewsFeedManager::AddFriendInvitedToGame(CStrWChar text)
{
    STargettedMessage* message = new STargettedMessage();
    message->m_type = NEWS_FRIEND_INVITED;
    message->m_text = text;
    AppendMessage(message);
}

void CNewsFeedManager::AddGiftReceivedMessage(int senderId, int itemIndex, int messageId)
{
    SGiftReceivedMessage* message = new SGiftReceivedMessage();
    message->m_type = NEWS_GIFT;
    message->m_senderId = senderId;
    message->m_messageId = messageId;
    message->m_itemIndex = itemIndex;
    AppendMessage(message);
}

void CNewsFeedManager::AddServerAwardMessage(const CStrWChar& text, int common, int rare,
                                             int experience, int itemIndex, int messageId)
{
    SServerAwardMessage* message = new SServerAwardMessage();
    message->m_type = NEWS_AWARD;
    message->m_commonCurrency = common;
    message->m_rareCurrency = rare;
    message->m_experience = experience;
    message->m_itemIndex = itemIndex;
    message->m_messageId = messageId;
    message->m_text = text;
    AppendMessage(message);
}

void CNewsFeedManager::AddGuildInviteMessage(int senderId, const CStrWChar& text)
{
    for (int index = 0; index < m_messages.size(); ++index) {
        const SMessage* message = m_messages[index];
        if (message->m_type == NEWS_GUILD_INVITE && message->m_senderId == senderId
            && static_cast<const STargettedMessage*>(message)->m_text == text) {
            return;
        }
    }
    STargettedMessage* message = new STargettedMessage();
    message->m_type = NEWS_GUILD_INVITE;
    message->m_senderId = senderId;
    message->m_text = text;
    AppendMessage(message);
}

void CNewsFeedManager::AddGuildInviteResponseMessage(int senderId, bool accepted)
{
    SMessage* message = new SMessage();
    message->m_type = NEWS_GUILD_DECLINED;
    if (accepted) {
        message->m_type = NEWS_GUILD_ACCEPTED;
    }
    message->m_senderId = senderId;
    AppendMessage(message);
}

void CNewsFeedManager::AddChallengeInviteMessage(int senderId, int challengeId, int messageId)
{
    if (HasChallengeMessage(messageId)) {
        return;
    }
    SMissionInviteMessage* message = new SMissionInviteMessage();
    message->m_type = NEWS_CHALLENGE_INVITE;
    message->m_senderId = senderId;
    message->m_missionId = challengeId;
    message->m_messageId = messageId;
    AppendMessage(message);
}

void CNewsFeedManager::AddChallengeResponseMessage(int senderId, int challengeId, bool accepted)
{
    SMissionInviteMessage* message = new SMissionInviteMessage();
    message->m_type = NEWS_CHALLENGE_DECLINED;
    if (accepted) {
        message->m_type = NEWS_CHALLENGE_ACCEPTED;
    }
    message->m_senderId = senderId;
    message->m_missionId = challengeId;
    AppendMessage(message);
}

void CNewsFeedManager::AddFriendCompletedChallengeMessage(int senderId, int challengeId,
                                                          const CStrWChar& text, int messageId)
{
    if (HasChallengeMessage(messageId)) {
        return;
    }
    SMissionInviteMessage* message = new SMissionInviteMessage();
    message->m_type = NEWS_CHALLENGE_COMPLETED;
    message->m_senderId = senderId;
    message->m_missionId = challengeId;
    message->m_messageId = messageId;
    AppendMessage(message);
}

bool CNewsFeedManager::HasChallengeMessage(int messageId) const
{
    for (int index = 0; index < m_messages.size(); ++index) {
        const SMessage* message = m_messages[index];
        if (message->m_type == NEWS_CHALLENGE_INVITE || message->m_type == NEWS_CHALLENGE_COMPLETED) {
            if (static_cast<const SMissionInviteMessage*>(message)->m_messageId == messageId) {
                return true;
            }
        }
    }
    return false;
}

void CNewsFeedManager::AddEmptyNewsMessage()
{
    if (GetNewsCount(NEWS_EMPTY) == 0) {
        SMessage* message = new SMessage();
        message->m_type = NEWS_EMPTY;
        m_emptyMessage = message;
        m_messages.addElement(m_emptyMessage);
    }
}

void CNewsFeedManager::MarkAllMessagesAsRead()
{
    for (int index = 0; index < m_messages.size(); index++) {
        m_messages[index]->m_read = true;
    }
}

int CNewsFeedManager::GetUnreadNonEmptyMessageCount()
{
    int unread = 0;
    for (int index = 0; index < m_messages.size(); index++) {
        const SMessage* message = m_messages[index];
        if (!message->m_read && message->m_type != NEWS_EMPTY) {
            unread++;
        }
    }
    return unread;
}

int CNewsFeedManager::GetUnreadNonEmptyMessageCount(ENewsType type)
{
    int unread = 0;
    for (int index = 0; index < m_messages.size(); index++) {
        const SMessage* message = m_messages[index];
        if (!message->m_read && message->m_type != NEWS_EMPTY && message->m_type == type) {
            unread++;
        }
    }
    return unread;
}

int CNewsFeedManager::GetNewsCount()
{
    return m_messages.size();
}

int CNewsFeedManager::GetNewsCount(ENewsType type)
{
    int count = 0;
    for (int index = 0; index < m_messages.size(); index++) {
        if (m_messages[index]->m_type == type) {
            count++;
        }
    }
    return count;
}

int CNewsFeedManager::GetNewsByType(ENewsType type, Vector<const SMessage*>& news)
{
    for (int index = 0; index < m_messages.size(); index++) {
        const SMessage* message = m_messages[index];
        if (message->m_type == type) {
            news.addElement(message);
        }
    }
    return news.size();
}

typedef char AssertNewsFeedManagerSize[sizeof(CNewsFeedManager) == 0x14 ? 1 : -1];
