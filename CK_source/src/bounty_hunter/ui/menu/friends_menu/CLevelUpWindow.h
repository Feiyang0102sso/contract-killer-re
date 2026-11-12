#ifndef CLEVELUPWINDOW_H
#define CLEVELUPWINDOW_H

#include "CDialogWindow.h"

class SG_Instance;
class SG_Presenter;

class CLevelUpAnimWindow : public Window {
public:
    CLevelUpAnimWindow();
    virtual ~CLevelUpAnimWindow();
    virtual void Paint(ICGraphics2d& graphics);
    virtual void Update();

private:
    SG_Instance* m_sprites;
    SG_Presenter* m_animation;
};

class CLevelUpWindow : public CTextDialog {
public:
    CLevelUpWindow(XString message, bool allowShare);
    virtual void OnCommand(Event& event);
    void SetCloseCommand(int command) { m_closeCommand = command; }
    virtual void OnEasyFinished();
    virtual void OnStartupEasy();
    // CK: a new virtual slot (+0xbc) hiding the non-virtual CDialogWindow::EasyClose.
    virtual void EasyClose();
    void AddMessageText();
    Window* CreateButtons(bool allowShare);
    Window* CreateButton(const char* icon, const XString& text, unsigned int command);

private:
    int m_closeCommand;
    XString m_levelMessage;
};

#endif
