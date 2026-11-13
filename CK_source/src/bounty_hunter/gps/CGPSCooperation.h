// CGPSCooperation.h - GPS cooperation marker and challenge badge.
#ifndef CGPSCOOPERATION_H
#define CGPSCOOPERATION_H

#include "CGPSItem.h"
#include "CCountNotificationWindow.h"

class CGPSCooperation;

class CGPSCGPSCooperationListener {
public:
    virtual void OnCooperationItemSelected(CGPSCooperation* cooperation) = 0;
    virtual void OnCoopVisible() = 0;
};

class CGPSCooperation : public CGPSItem {
public:
    CGPSCooperation();
    virtual ~CGPSCooperation() {}
    virtual void OnClick();
    virtual void Update(long elapsed);
    virtual void Paint(ICGraphics2d& graphics, int mapX, int mapY);
    virtual void Save(TiXmlElement*) {}
    virtual void Load(TiXmlElement*) {}
    void SetListener(CGPSCGPSCooperationListener* listener);

    XString m_title;                             // 0x48
    CGPSCGPSCooperationListener* m_listener;     // 0x4c
    CFont* m_font;                              // 0x50
    int m_titleWidth;                           // 0x54
    CCountNotificationWindow m_unread;          // 0x58
    int m_elapsed;                             // 0x140
};

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char GPSCooperationSizeCheck[sizeof(CGPSCooperation) == 0x144 ? 1 : -1];
#endif

#endif
