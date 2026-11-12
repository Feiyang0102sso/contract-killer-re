// CDialogWindow.h - CK message windows built from image, text and menu buttons.
#ifndef CDIALOGWINDOW_H
#define CDIALOGWINDOW_H

#include "WindowBase.h"
#include "CFontMgr.h"

class CBH_MenuButton;
class ImageWindow;
class TextWindow;

class CDialogWindow : public Window {
public:
    enum EDialogButton {
        BUTTON_NONE = 0,
        BUTTON_OK = 1,
        BUTTON_IGNORE = 2,
        BUTTON_YES = 3,
        BUTTON_NO = 4,
        BUTTON_BUY = 5,
        BUTTON_ACCEPT = 6,
        BUTTON_GET = 7,
        BUTTON_LOCAL_SAVE = 8,
        BUTTON_SERVER_SAVE = 9,
        BUTTON_RETRY = 10,
        BUTTON_ACKNOWLEDGE = 11,
        BUTTON_HIRE = 12,
        BUTTON_LATER = 13,
        BUTTON_AMMO = 14,
        BUTTON_WEAPON = 15,
        BUTTON_HELP = 16,
        BUTTON_EQUIP = 17,
        BUTTON_FACEBOOK_SHARE = 18,
        BUTTON_GAME_CENTER_LOGOUT = 19,
        BUTTON_FACEBOOK_LOGOUT = 20
    };

    explicit CDialogWindow(bool telephone);
    virtual ~CDialogWindow();
    virtual void OnStartupEasy();
    virtual void OnEasyFinished();
    virtual void OnCommand(Event& event);
    virtual void Paint(ICGraphics2d& graphics);

    // CK: not virtual (callers use a direct call; the vtable ends with SetButtons at +0xb8).
    void SetMessageText(const XString& text, CFont* font = NULL);
    virtual void SetButtons(EDialogButton first, EDialogButton second = BUTTON_NONE, EDialogButton third = BUTTON_NONE);
    void SetTitleText(const char* id);
    void SetTitleText(XString text);
    void SetSound(const char* name);
    void SetExtraImage(ICRenderSurface* surface);
    void SetExtraImageFromImageID(const char* name);
    ImageWindow* GetBackgroundImage();
    void SetButtonsCommands(int first, int second, int third);
    void SetButtonsCommandFirstParam(int first, int second, int third);
    void SetEasyStyle(bool start, bool close);
    void EasyClose();
    void AddDownTextWithIcon(const XString& text, const char* image, bool right, bool spacing, eGameFont font);
    void SetMissionRewards(int money, int experience, int energy, bool targeting);

protected:
    void CreateButton(CBH_MenuButton* button, EDialogButton type);

    ICRenderSurface* m_backgroundSurface; // +0xa4
    ImageRes m_unusedImage;               // +0xa8
    ImageWindow* m_extraImage;            // +0xb0
    ImageWindow* m_backgroundImage;       // +0xb4
    const char* m_extraImageName;         // +0xb8
    TextWindow* m_title;                  // +0xbc
    Window* m_content;                    // +0xc0
    Window* m_downText;                   // +0xc4
    int m_downTextCount;                  // +0xc8
    TextWindow* m_message;                // +0xcc
    int m_fieldd0;                        // +0xd0
    int m_fieldd4;                        // +0xd4
    int m_fieldd8;                        // +0xd8
    CBH_MenuButton* m_buttons[3];         // +0xdc
    int m_commands[3];                    // +0xe8
    int m_commandParams[3];               // +0xf4
    bool m_arrangeButtons;                // +0x100
    bool m_telephone;                     // +0x101
    bool m_closing;                       // +0x102
    bool m_animateOpen;                   // +0x103
    bool m_animateClose;                  // +0x104
    const char* m_sound;                  // +0x108
    bool m_soundPlayed;                   // +0x10c
    EDialogButton m_buttonTypes[3];       // +0x110
};

class CTextDialog : public CDialogWindow {
public:
    CTextDialog();
    virtual void SetButtons(EDialogButton first, EDialogButton second = BUTTON_NONE, EDialogButton third = BUTTON_NONE);

protected:
    int m_buttonOffsetY;    // +0x11c
};

/** Dialog shown before a hunt: telephone image, narrower content. */
class CBeforeHuntDialog : public CDialogWindow {
public:
    CBeforeHuntDialog();
};

/** Text dialog whose buttons appear after a delay (milliseconds). */
class CDialogWithDelayedButtons : public CTextDialog {
public:
    explicit CDialogWithDelayedButtons(int delay);
    virtual void Update();

protected:
    int m_delay;            // +0x120 milliseconds left
};

class CFriendAvatarWindow;
class CNGSUser;

/** Dialog shown before targetting a friend: shows the friend's avatar. */
class CBeforeTargettingDialog : public CDialogWindow {
public:
    explicit CBeforeTargettingDialog(CStrWChar friendId);
    explicit CBeforeTargettingDialog(int avatarId);
    virtual void Update();
    void SetMessageText(const XString& text, CFont* font = NULL);

    const void* m_pictureData;          // +0x11c Facebook picture of m_user
    unsigned int m_pictureSize;         // +0x120
    bool m_loadPicture;                 // +0x124
    CNGSUser* m_user;                   // +0x128
    CFriendAvatarWindow* m_avatar;      // +0x12c
};

struct SServerAwardMessage;

/** Dialog for a gift (money, credits, xp, item) sent by the server. */
class CServerGiftDialog : public CDialogWindow {
public:
    explicit CServerGiftDialog(const SServerAwardMessage* award);
    virtual void OnCommand(Event& event);

protected:
    const SServerAwardMessage* m_award; // +0x11c
};

/** Text dialog of a completed challenge. */
class CChallengeCompletedDialog : public CTextDialog {
public:
    CChallengeCompletedDialog();
    virtual void OnCommand(Event& event);
};

/** Telephone dialog announcing a server gift; pauses the game while shown. */
class CServerGiftNotifierDialog : public CDialogWindow {
public:
    CServerGiftNotifierDialog();
    virtual void OnCommand(Event& event);
};

// added by gserve: Facebook login reminder (defined later in CDialogWindow.cpp, sizeof 0x120)
class CCheckBox;  // added by gserve
class CFacebookLoginDialog : public CDialogWindow {  // added by gserve
public:  // added by gserve
    CFacebookLoginDialog();  // added by gserve
    virtual void OnCommand(Event& event);  // added by gserve
protected:  // added by gserve
    CCheckBox* m_reminderCheckBox;  // added by gserve, +0x11c
};  // added by gserve

#endif
