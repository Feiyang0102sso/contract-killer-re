#ifndef CNEWSFEEDWINDOW_H
#define CNEWSFEEDWINDOW_H

#include "WindowBase.h"

struct SMessage;
struct SMissionInviteMessage;
struct SFriendJoinedGameMessage;
struct SGiftReceivedMessage;
struct SServerAwardMessage;
struct STargettedMessage;

/** Scrollable list of player news and gifts embedded in the friends screen. */
class CNewsFeedWindow : public Window {
public:
    CNewsFeedWindow();
    virtual ~CNewsFeedWindow();

    void ClearNews();
    void ShowEmptyNews();
    void ReceiveGift(const SGiftReceivedMessage* message);
    void ReceiveAward(const SServerAwardMessage* message);
    void FriendInstall(const SFriendJoinedGameMessage* message);
    void Targetted(const STargettedMessage* message);
    void ShowGuildInvite(const STargettedMessage* message);
    void ShowGuildInviteResponse(const SMessage* message, bool accepted);
    void ShowChallengeInvite(const SMissionInviteMessage* message);
    void ShowChallengeInviteResponse(const SMissionInviteMessage* message, bool accepted);
    void MissionInvite(const SMissionInviteMessage* message);

private:
    void CreateUserInterface();

    Window* m_rows;
    int m_nextRow;
    int m_column;
};

#endif
