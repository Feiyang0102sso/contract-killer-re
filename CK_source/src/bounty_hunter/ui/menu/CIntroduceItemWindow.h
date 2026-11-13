// CIntroduceItemWindow.h - introduces an item before a mission or purchase.
#ifndef CINTRODUCEITEMWINDOW_H
#define CINTRODUCEITEMWINDOW_H

#include "CDialogWindow.h"

class CItem;

class CIntroduceItemWindow : public CTextDialog {
public:
    CIntroduceItemWindow(CItem* item, XString message, bool showShop, bool pauseTime);
    virtual ~CIntroduceItemWindow() {}
    virtual void OnCommand(Event& event);
    virtual void OnClosed();

    void SetOnCloseCommand(unsigned int command);

private:
    void CreateUI(CItem* item);
    void HandleButtonPressed();

    CItem* m_item;
    unsigned int m_closeCommand;
    XString m_introMessage;
    bool m_showShop;
    bool m_pauseTime;
};

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char IntroduceItemWindowSizeCheck[sizeof(CIntroduceItemWindow) == 0x130 ? 1 : -1];
#endif

#endif
