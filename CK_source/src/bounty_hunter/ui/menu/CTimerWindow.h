// CTimerWindow.h - a countdown to an application timestamp.
#ifndef CTIMERWINDOW_H
#define CTIMERWINDOW_H

#include "BaseDialog.h"

class CTimerWindow : public TextWindow {
public:
    CTimerWindow(unsigned int endTime, CFont* font);
    virtual ~CTimerWindow() {}
    virtual void Update();

private:
    unsigned int m_endTime;  // 0xd0
    int m_displayedSeconds;  // 0xd4
};

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char TimerWindowSizeCheck[sizeof(CTimerWindow) == 0xd8 ? 1 : -1];
#endif

#endif
