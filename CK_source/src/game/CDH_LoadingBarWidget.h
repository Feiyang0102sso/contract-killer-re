#ifndef CDH_LOADINGBARWIDGET_H
#define CDH_LOADINGBARWIDGET_H

#include "WindowBase.h"
#include "CStrWChar.h"

/** Loading progress and connecting prompt at the bottom of the CK title. */
class CDH_LoadingBarWidget : public Window {
public:
    enum eState { STATE_LOADING = 0, STATE_CONNECTING = 1, STATE_TAP = 2 };

    CDH_LoadingBarWidget();
    virtual ~CDH_LoadingBarWidget() {}
    virtual void Update();
    virtual void Paint(ICGraphics2d& graphics);
    void setState(eState state);

    CStrWChar m_text;       // 0xa4
    CFont* m_font;          // 0xb4
    int m_textLength;       // 0xb8
    int m_blinkTime;        // 0xbc
    int m_progress;         // 0xc0
    eState m_state;         // 0xc4
    bool m_blinking;        // 0xc8
};

#endif
