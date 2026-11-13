// CGraphicsSingleton.h - application ownership of the graphics interface.
#ifndef CGRAPHICSSINGLETON_H
#define CGRAPHICSSINGLETON_H

#include "CApp.h"
#include "CApplet.h"
#include "CGraphicsAbstract.h"
#include "CSingleton.h"

class CGraphicsSingleton : public CGraphicsAbstract, public CSingleton {
public:
    CGraphicsSingleton() : CSingleton(ICGraphics::ClassId) {}

    virtual __attribute__((always_inline)) ~CGraphicsSingleton()
    {
        if (CApplet::m_pApp != NULL) {
            CApplet::m_pApp->m_graphics = NULL;
        }
    }
};

#endif
