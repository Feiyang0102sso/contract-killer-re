// CGraphics2dSingleton.h - a 2D renderer registered as the application's ICGraphics2d singleton.
#ifndef CGRAPHICS2DSINGLETON_H
#define CGRAPHICS2DSINGLETON_H

#include "CGraphics2d_Lite_OGLES.h"
#include "CSingleton.h"
#include "CApplet.h"
#include "CApp.h"

class CGraphics2dSingleton : public CGraphics2dAbstract, public CSingleton {
public:
    CGraphics2dSingleton() : CSingleton(ICGraphics2d::ClassId) {}
    // forget the cached instance
    inline virtual ~CGraphics2dSingleton()
    {
        if (CApplet::m_pApp != NULL) {
            CApplet::m_pApp->m_graphics2d = NULL;
        }
    }
};

#endif
