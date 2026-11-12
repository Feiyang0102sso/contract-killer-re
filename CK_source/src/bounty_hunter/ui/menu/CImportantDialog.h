// CImportantDialog.h - CBloodWindow with a scrolling message and one OK button sending a command.
#ifndef CIMPORTANTDIALOG_H
#define CIMPORTANTDIALOG_H

#include "CBloodWindow.h"

class CImageButton;

class CImportantDialog : public CBloodWindow {
public:
    CImportantDialog();
    virtual void OnCommand(Event& event);

    void CreateInteface();
    Window* CreateButton(const XString& text);
    void SetMessageText(const XString& text);
    void SetCommand(unsigned int command);

protected:
    TextWindow* m_message;      // 0x120
    CImageButton* m_okButton;   // 0x124
    unsigned int m_command;     // 0x128 tunnel command sent by OK
};

#endif
