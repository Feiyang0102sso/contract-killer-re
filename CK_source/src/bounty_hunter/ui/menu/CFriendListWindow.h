#ifndef CFRIENDLISTWINDOW_H
#define CFRIENDLISTWINDOW_H

#include "WindowBase.h"
#include "Vector_ss.h"

class CFriendListWindow : public Window {
public:
    enum { INVITE_FRIENDS = -0x2e5f1780, SHOW_COOP = -0x5dba8252 };

    CFriendListWindow();
    virtual ~CFriendListWindow() {}
    virtual void OnCommand(Event& event);
    void AddFriend(int friendId);

private:
    Window* CreateMissionButton(const char* image, const XString& caption, unsigned int command);
    Window* CreateFriendsWindow();

    int m_row;
    Window* m_friends;
    int m_reserved[2];
    Vector<int> m_listState;  // Reserved list storage released by the original destructor.
};

typedef char FriendListWindowSizeCheck[sizeof(CFriendListWindow) == 0xc4 ? 1 : -1];

#endif
