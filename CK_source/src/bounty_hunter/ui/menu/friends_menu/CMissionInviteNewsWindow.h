#ifndef CMISSIONINVITENEWSWINDOW_H
#define CMISSIONINVITENEWSWINDOW_H

#include "WindowBase.h"

class CNGSRemoteUser;
struct SMissionInviteMessage;

class CMissionInviteNewsWindow : public Window {
public:
    explicit CMissionInviteNewsWindow(const SMissionInviteMessage* message);
    virtual void OnCommand(Event& event);

private:
    void CreateUserInterface();

    int m_senderId;
    int m_missionId;
    CNGSRemoteUser* m_sender;
    const SMissionInviteMessage* m_message;
};

#endif
