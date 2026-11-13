#ifndef CFRIENDSWINDOW_H
#define CFRIENDSWINDOW_H

#include "CBaseScreen.h"
#include "CStrWChar.h"
#include "TCVector.h"
#include "Vector_ss.h"

class CIncentivizedWindow;
class CTabWindow;
class CFriendListWindow;
class CNewsFeedWindow;
class CImageButton;
class CFont;

// One cached entry per friend (0x20 bytes, layout from UpdateFriendsTab).
struct SFriendInfo {
    XString m_name;      // 0x00 released by the TCVector destructor
    void* m_avatar;      // 0x04 avatar surface
    int m_unknown8;      // 0x08
    int m_avatarWidth;   // 0x0c
    int m_avatarData;    // 0x10
    bool m_hasAvatar;    // 0x14
    int m_userIndex;     // 0x18 index in the friend list
    bool m_done;         // 0x1c
};

// Original 1.5.2 layout (operator new 0x150).
class CFriendsWindow : public CBaseScreen {
public:
    CFriendsWindow();
    void SetTab(int tab);
    virtual void Update();
    virtual void OnCommand(Event& event);
    virtual void RefreshFriendTab();

private:
    void CreateUserInteface();
    void CreateLoginButtons();
    Window* CreateNewsFeedTab();
    Window* CreateFriendsTab();
    void CreateInviteWindow();
    void FillNewsFeedTab(bool firstFill);
    void UpdateFriendsTab();
    void UpdateLoginButtons();
    void UpdateNewsFeedTab();
    void UpdateCountNotification();
    void ShowFacebookIncentiveWindow(unsigned int offer);
    void ShowFacebookInviteFriendsWindow();
    void HandleFacebookInvitation(CIncentivizedWindow* picker);
    void ShowSelectGiftRecipient();
    void HandleGiftSending(CIncentivizedWindow* picker);
    int GetNonPlayingFriendsCount();

    TCVector<SFriendInfo> m_friendInfos;          // 0xd8
    CTabWindow* m_tabs;                           // 0xf0
    Window* m_inviteWindow;                       // 0xf4
    TCVector<CStrWChar> m_facebookInviteIds;      // 0xf8
    Vector<int> m_giftRecipients;                 // 0x110
    int m_giftIndex;                              // 0x120
    CFriendListWindow* m_friendList;              // 0x124
    CNewsFeedWindow* m_newsFeed;                  // 0x128
    CImageButton* m_loginButton12c;               // 0x12c
    CImageButton* m_loginButton130;               // 0x130
    Window* m_loginButton134;                     // 0x134
    Window* m_loginButton138;                     // 0x138
    Window* m_waitWindow;                         // 0x13c CWaitWindow, only valid for a pending batch
    int m_sentInviteCount;                        // 0x140 zero when no batch is pending
    bool m_incentiveShown;                        // 0x144
    bool m_inviteFailShown;                       // 0x145
    bool m_selectingGiftRecipient;                // 0x146
    int m_giftRecipientId;                        // 0x148
    CFont* m_loginFont;                           // 0x14c
};

#endif
