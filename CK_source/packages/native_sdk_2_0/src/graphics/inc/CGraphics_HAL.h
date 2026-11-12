// CGraphics_HAL.h - the application graphics singleton and its hardware device.
#ifndef CGRAPHICS_HAL_H
#define CGRAPHICS_HAL_H

#include "CGraphicsAbstract.h"
#include "CSingleton.h"

class CGraphics_OGLES_Cocoa;

class CGraphicsSingleton : public CGraphicsAbstract, public CSingleton {
public:
    CGraphicsSingleton() : CSingleton(ICGraphics::ClassId) {}
    virtual ~CGraphicsSingleton();
};

class CGraphics_HAL : public CGraphicsSingleton {
public:
    CGraphics_HAL();
    virtual ~CGraphics_HAL();

    virtual ICRenderSurface* GetTargetSurface() const;
    virtual void RenderBegin(const RenderBeginSettings* settings);
    virtual void RenderEnd();
    virtual void ClearBuffers(unsigned int flags);
    virtual void SetClearColor(int alpha, int red, int green, int blue);
    virtual void SetClearColor(const Color_ARGB_fixed& color);
    virtual void SetClearColor(unsigned int color);
    virtual void SetClearDepth(int depth);
    virtual void SetClearStencil(int stencil);
    virtual const Description* GetDescription(Abstraction abstraction) const;
    virtual const CStrChar* GetVersion(Abstraction abstraction) const;

    unsigned char InitializeHardware(const Param* params);
    void DestroyHardware();

    CGraphics_OGLES_Cocoa* m_hardware;  // +0x3c
};

#endif
