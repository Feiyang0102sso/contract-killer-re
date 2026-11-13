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
    int m_fieldB0;  // original allocation includes an unused final word
};

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char ChallengeInviteNewsWindowSizeCheck[sizeof(CChallengeInviteNewsWindow) == 0xb4 ? 1 : -1];
#endif

#endif
