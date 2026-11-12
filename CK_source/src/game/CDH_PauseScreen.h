#ifndef CDH_PAUSESCREEN_H
#define CDH_PAUSESCREEN_H

#include "CDH_SoftkeysScreen.h"

class CSettingsWindow;

/** Pause dialog and exit confirmation. */
class CDH_PauseScreen : public CDH_SoftkeysScreen {
public:
    enum { ConfirmMissionExit = 3 };
    CDH_PauseScreen(bool soundOff, bool allowQuit);
    virtual ~CDH_PauseScreen();
    virtual int ShouldBeModal() { return 1; }
    virtual void OnCreateState(Window** window, int state);
    virtual void OnStateInterrupt(int state, Window* window);
    virtual void OnCommand(Event& event);
    virtual Window* PopulateSofkeys();

    const char* const* m_items;      // 0xa4
    int m_itemCount;                 // 0xa8
    CSettingsWindow* m_pauseDialog;  // 0xac
    bool m_quitMission;      // 0xb0
};

#endif
