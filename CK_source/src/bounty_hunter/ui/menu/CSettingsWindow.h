#ifndef CSETTINGSWINDOW_H
#define CSETTINGSWINDOW_H

#include "WindowBase.h"

class CImageButton;
class TextWindow;
class CDH_RadioGroupWidget;

/** CK settings pane shared by the pause menu and the GPS screen. */
class CSettingsWindow : public Window {
public:
    CSettingsWindow(bool pauseMenu, bool returnToGPS);
    virtual ~CSettingsWindow();
    virtual void OnStartupEasy();
    virtual void OnEasyFinished();
    virtual void OnClosed();
    virtual void OnCommand(Event& event);

    Window* CreatePauseMenu();
    Window* CreateSettingMenu();
    void ShowContent(Window* content);

private:
    CImageButton* AddItem(Window* content, const char* label, int command, int row);
    Window* CreateRadioGroup(CDH_RadioGroupWidget** group, const char* label, const char* onLabel,
                             const char* offLabel, bool value);
    CDH_RadioGroupWidget* m_sound;        // 0xa4
    CDH_RadioGroupWidget* m_music;        // 0xa8
    CDH_RadioGroupWidget* m_fireButton;   // 0xac
    CDH_RadioGroupWidget* m_push;         // 0xb0
    CImageButton* m_fieldb4;      // 0xb4
    CDH_RadioGroupWidget* m_facebook;     // 0xb8
    Window* m_content;            // 0xbc
    Window* m_contentHost;        // 0xc0
    TextWindow* m_title;          // 0xc4
    bool m_pauseMenu;             // 0xc8
    bool m_returnToGPS;           // 0xc9
};

#endif
