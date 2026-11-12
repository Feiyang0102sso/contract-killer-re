#ifndef CGUILDNEWSWINDOW_H
#define CGUILDNEWSWINDOW_H

#include "WindowBase.h"

class CNGSRemoteUser;
struct STargettedMessage;

class CGuildInviteNewsWindow : public Window {
public:
    explicit CGuildInviteNewsWindow(const STargettedMessage* message);
    virtual void OnCommand(Event& event);

private:
    void CreateUserInterface();
    int m_senderId;
    CNGSRemoteUser* m_sender;
    const STargettedMessage* m_message;
    int m_messageId;
};

#endif
