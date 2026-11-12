#ifndef CCHALLENGEINVITENEWSWINDOW_H
#define CCHALLENGEINVITENEWSWINDOW_H

#include "WindowBase.h"

class CNGSRemoteUser;
struct SMissionInviteMessage;

class CChallengeInviteNewsWindow : public Window {
public:
    explicit CChallengeInviteNewsWindow(const SMissionInviteMessage* message);

private:
    void CreateUserInterface();

    int m_senderId;
    CNGSRemoteUser* m_sender;
    const SMissionInviteMessage* m_message;
};

#endif
