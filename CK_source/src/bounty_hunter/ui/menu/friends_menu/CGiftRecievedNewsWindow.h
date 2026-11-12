#ifndef CGIFTRECIEVEDNEWSWINDOW_H
#define CGIFTRECIEVEDNEWSWINDOW_H

#include "WindowBase.h"

struct SGiftReceivedMessage;

class CGiftRecievedNewsWindow : public Window {
public:
    explicit CGiftRecievedNewsWindow(const SGiftReceivedMessage* message);
    virtual void OnCommand(Event& event);

private:
    void CreateUserInterface(const XString& sender, const XString& item, bool fromFriend);

    int m_senderId;
};

#endif
