#ifndef CTARGETTEDNEWSWINDOW_H
#define CTARGETTEDNEWSWINDOW_H

#include "WindowBase.h"

class CNGSRemoteUser;
struct STargettedMessage;

class CTargettedNewsWindow : public Window {
public:
    explicit CTargettedNewsWindow(const STargettedMessage* message);
    virtual void OnCommand(Event& event);

private:
    void CreateUserInterface();
    void HandleRevengePressed();

    int m_senderId;
    bool m_revengeAvailable;  // 0xa8 reserved; the message supplies the highlight flag
    CNGSRemoteUser* m_sender;
    const STargettedMessage* m_message;
    int m_messageId;
};

#endif
