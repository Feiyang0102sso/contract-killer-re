// CDH_GameSoftkeyWidget.h - pause button shown in the gameplay HUD.
#ifndef CDH_GAMESOFTKEYWIDGET_H
#define CDH_GAMESOFTKEYWIDGET_H

#include "ButtonWindow.h"

// The original symbol spells the class name without the second 't'.
class CDH_GameSofkeyWidget : public ButtonWindow {
public:
    CDH_GameSofkeyWidget();
    virtual ~CDH_GameSofkeyWidget();
    static int GetPauseButtonHeight();
    virtual void OnPointerEvent(Event& event);
};

#endif
