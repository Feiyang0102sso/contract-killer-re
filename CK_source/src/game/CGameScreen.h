#ifndef CGAMESCREEN_H
#define CGAMESCREEN_H

#include "WindowBase.h"
#include "CGPSMapScreen.h"

/** State owner for the GPS map, mission, and results screens. */
class CGameScreen : public Window {
public:
    CGameScreen();
    virtual ~CGameScreen();
    virtual void Init();
    virtual void Shutdown();
    virtual void OnCommand(Event& event);
    virtual void OnCreateState(Window** window, int state);
    virtual void OnStateInterrupt(int state, Window* window);
    virtual void Activate();
    virtual void Deactivate();

    int m_previousState;       // 0xa4
    ICRenderSurface* m_missionSurface; // 0xa8
    CGPSMapScreen::CachedData* m_mapCache; // 0xac
};

#endif
