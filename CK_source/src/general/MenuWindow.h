// MenuWindow.h - the common list dialog for menu screens.
#ifndef MENUWINDOW_H
#define MENUWINDOW_H

#include "SimpleDialog.h"

class MenuWindow {
public:
    class MenuDialog : public SimpleDialog {
    public:
        explicit MenuDialog(MenuWindow* owner);
        virtual ~MenuDialog() {}

        virtual void Init();
        virtual void Shutdown();
        virtual void CancelDialog();
        virtual void OnCommand(Event& event);
        virtual void OnLayout();
        virtual void AdjustByDefRect();
        virtual void InitCenter();
        virtual void PopulateContent();

    private:
        MenuWindow* m_owner; // 0x170
        bool m_field174;    // 0x174
        int m_selectedKey; // 0x178
    };
};

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char MenuDialogSizeCheck[sizeof(MenuWindow::MenuDialog) == 0x17c ? 1 : -1];
#endif

#endif
