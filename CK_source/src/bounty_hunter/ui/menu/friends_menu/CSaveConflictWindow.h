// CSaveConflictWindow.h - profile data conflict: device save against server save.
#ifndef CSAVECONFLICTWINDOW_H
#define CSAVECONFLICTWINDOW_H

#include "CDialogWindow.h"

class CSaveConflictWindow : public CTextDialog {
public:
    /** Both saves (filled by CGServeHandler::ShowSocialNetworkProfileDataConflict). */
    struct SConflictInfo {
        int m_localLevel;           // +0x00
        int m_localSoftCurrency;    // +0x04
        int m_localHardCurrency;    // +0x08
        int m_serverLevel;          // +0x0c
        int m_serverSoftCurrency;   // +0x10
        int m_serverHardCurrency;   // +0x14
    };

    CSaveConflictWindow(SConflictInfo& info);
    virtual void OnStartupEasy();
    virtual void OnCommand(Event& event);

    Window* CreateSaveInfoWindow(const XString& title, const XString& buttonText, int level, int softCurrency,
                                 int hardCurrency, unsigned int command);
    void EasyClose();
    Window* CreateLabelWithIcon(const char* icon, const XString& text);
};

#endif
