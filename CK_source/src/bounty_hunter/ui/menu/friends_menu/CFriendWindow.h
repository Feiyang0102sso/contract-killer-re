#ifndef CFRIENDWINDOW_H
#define CFRIENDWINDOW_H

#include "WindowBase.h"

class CBHFriend;
class TextWindow;

class CFriendWindow : public Window {
public:
    explicit CFriendWindow(int friendId);
    virtual void Update();
    virtual void OnCommand(Event& event);

private:
    Window* CreateFriendIcon(int friendId, bool facebook, bool gameCenter);
    Window* CreateFriendInfo(const XString& name, int level, int contracts);

    int m_friendId;
    CBHFriend* m_friend;
    unsigned char m_populated;  // 0xac: any nonzero value skips refresh
    TextWindow* m_level;
    TextWindow* m_contracts;
};

#endif
