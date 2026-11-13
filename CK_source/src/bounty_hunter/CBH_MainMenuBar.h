// CBH_MainMenuBar.h - button row and separator in the Network window.
#ifndef CBH_MAINMENUBAR_H
#define CBH_MAINMENUBAR_H

#include "CRectangle.h"
#include "Vector_ss.h"
#include "WindowBase.h"

class CBH_MenuButton;

struct MenuButtonDesc {
    const char* m_image;
    int m_command;
    const char* m_caption;
};

class CBH_MainMenuBar : public Window {
public:
    CBH_MainMenuBar(int x, int y, int width, int height);
    virtual ~CBH_MainMenuBar() { ReleaseResources(); }
    virtual void Paint(ICGraphics2d& graphics);
    virtual void OnPointerEvent(Event& event);
    virtual void OnCommand(Event& event);
    virtual void ReleaseResources();

    void SetButtons(Vector<MenuButtonDesc*> descriptions);

private:
    Vector<CBH_MenuButton*> m_buttons;
    CRectangle m_backgroundRect;
    ImageRes m_background;
    ImageRes m_border;
};

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char MainMenuBarSizeCheck[sizeof(CBH_MainMenuBar) == 0xcc ? 1 : -1];
#endif

#endif
