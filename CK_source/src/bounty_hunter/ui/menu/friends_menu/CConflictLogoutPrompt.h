// CConflictLogoutPrompt.h - account conflict prompt: log out of Facebook or Game Center.
#ifndef CCONFLICTLOGOUTPROMPT_H
#define CCONFLICTLOGOUTPROMPT_H

#include "CBloodWindow.h"

class CConflictLogoutPrompt : public CBloodWindow {
public:
    CConflictLogoutPrompt();
    virtual void OnStartupEasy();
    virtual void EasyClose();

    Window* CreateTexts();
    static Window* CreateButtons();
    static Window* CreateButton(const char* icon, const XString& text, unsigned int command);
};

#endif
