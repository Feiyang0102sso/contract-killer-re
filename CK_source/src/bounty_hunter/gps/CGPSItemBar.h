// CGPSItemBar.h - GPS friends marker and unread news badge.
#ifndef CGPSITEMBAR_H
#define CGPSITEMBAR_H

#include "CGPSItem.h"
#include "CCountNotificationWindow.h"

class CGPSBar;

class CGPSBarListener {
public:
    virtual void OnBarItemSelected(CGPSBar* bar) = 0;
    virtual void OnBarVisible() = 0;
};

class CGPSBar : public CGPSItem {
public:
    CGPSBar();
    virtual ~CGPSBar() {}
    virtual void Paint(ICGraphics2d& graphics, int mapX, int mapY);
    virtual void Update(long elapsed);
    virtual void OnClick();
    virtual void Save(TiXmlElement*) {}
    virtual void Load(TiXmlElement*) {}

    CGPSBarListener* m_listener;                // 0x48
    CFont* m_font;                              // 0x4c
    int m_titleWidth;                           // 0x50
    XString m_title;                            // 0x54
    CCountNotificationWindow m_unread;          // 0x58
    int m_elapsed;                             // 0x140
};

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char GPSBarSizeCheck[sizeof(CGPSBar) == 0x144 ? 1 : -1];
#endif

#endif
