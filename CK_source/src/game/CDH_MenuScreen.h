// CDH_MenuScreen.h - a localized menu with a selected item index.
#ifndef CDH_MENUSCREEN_H
#define CDH_MENUSCREEN_H

#include "MenuWindow.h"

class CDH_MenuScreen : public MenuWindow::MenuDialog {
public:
    CDH_MenuScreen();
    virtual ~CDH_MenuScreen();
    virtual Window* PopulateSofkeys();
    void setMenuItems(const char* const* items, int count);
    void setTitleDelayed(const char* title);
    virtual void InitCenter();
    virtual void PopulateContent();
    virtual void ClickItem(int id, bool byPointer);
    virtual void OnCommand(Event& event);
    virtual void OnGetFocus();
    virtual void OnLostFocus();

    Vector<const char*> m_items;  // 0x17c
    Vector<const char*> m_icons;  // 0x18c
    Vector<int> m_itemKeys;       // 0x19c
    int m_selectedIndex;          // 0x1ac
    bool m_showBack;              // 0x1b0
};

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char MenuScreenSizeCheck[sizeof(CDH_MenuScreen) == 0x1b4 ? 1 : -1];
#endif

#endif
