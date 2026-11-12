#ifndef CDH_LOADINGSCREEN_H
#define CDH_LOADINGSCREEN_H

#include "WindowBase.h"

class CDH_LoadingBarWidget;
class CResBank;

/** CK title screen coordinating login and game resource preload. */
class CDH_LoadingScreen : public Window {
public:
    explicit CDH_LoadingScreen(bool tapToContinue);
    virtual ~CDH_LoadingScreen();
    virtual void Init();
    virtual void OnPointerPressed(Event& event);
    virtual void Update();
    virtual void Paint(ICGraphics2d& graphics);
    virtual void OnCommand(Event& event);
    void SetResInfo(CResBank* bank, const char* keyset, int delay);

private:
    enum State { Delay, Preload, Login, WaitForTap };
    bool m_tapToContinue;        // 0xa4
    ICRenderSurface* m_title;    // 0xa8
    bool m_loginPending;         // 0xac
    CDH_LoadingBarWidget* m_bar; // 0xb0
    int m_initialDelay;          // 0xb4
    int m_elapsed;               // 0xb8
    bool m_fieldbc;              // 0xbc
    bool m_preloadFailed;        // 0xbd
    CResBank* m_resBank;         // 0xc0
    State m_state;               // 0xc4
    bool m_restartRequested;     // 0xc8
    bool m_profileRequested;     // 0xc9
};

#endif
