#ifndef CCHALLENGESWINDOW_H
#define CCHALLENGESWINDOW_H

#include "WindowBase.h"
#include "Vector_ss.h"

class CChallengeWindow;

// Original 1.5.2 layout: derives from Window directly (0xa4 bytes), total 0xbc.
class CChallengesWindow : public Window {
public:
    CChallengesWindow();
    virtual void OnCommand(Event& event);

private:
    void CreateUI();
    static Window* CreateTimer();
    Window* CreateChallengesList();
    void HandleChallengeWindowPressed(CChallengeWindow* selected);

    int m_unknownA4;                    // 0xa4 set to -1 by the constructor
    Window* m_page;                     // 0xa8
    Vector<CChallengeWindow*> m_cards;  // 0xac
};

#endif
