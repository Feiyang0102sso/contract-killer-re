// PROVISIONAL (created by ui): owner src/bounty_hunter/ui/menu/friends_menu/CFriendAvatarWindow.cpp not written yet
// CFriendAvatarWindow.h - avatar picture of a friend; only what CBeforeTargettingDialog (CDialogWindow.cpp) uses.
// Size 0xd0 from the np_malloc in CBeforeTargettingDialog::CBeforeTargettingDialog(int) (0xf9738).
// The avatar window implementation and its field layout have since been recovered.
#ifndef CFRIENDAVATARWINDOW_H
#define CFRIENDAVATARWINDOW_H

#include "WindowBase.h"
#include "CStrWChar.h"

class CWaitWindow;
class ImageWindow;

class CFriendAvatarWindow : public Window {
public:
    explicit CFriendAvatarWindow(int avatarId);
    explicit CFriendAvatarWindow(CStrWChar friendId);
    virtual ~CFriendAvatarWindow();
    virtual void Update();

    void SetDefaultImage(const char* name);
    void ShowBorder();

private:
    void Init(CStrWChar friendId);
    void StopWaiting();

    unsigned int m_pictureSize;           // 0xa4
    bool m_loading;                       // 0xa8
    bool m_loaded;                        // 0xa9
    bool m_showBorder;                    // 0xaa
    ICRenderSurface* m_downloadedPicture;  // 0xac owned by this window
    ImageWindow* m_picture;               // 0xb0 borrows the surface
    ImageWindow* m_border;                // 0xb4
    CWaitWindow* m_spinner;                // 0xb8
    CStrWChar m_facebookId;                // 0xbc
    const char* m_defaultImage;           // 0xcc key in the surface manager
};

typedef char FriendAvatarWindowSizeCheck[sizeof(CFriendAvatarWindow) == 0xd0 ? 1 : -1];

#endif
