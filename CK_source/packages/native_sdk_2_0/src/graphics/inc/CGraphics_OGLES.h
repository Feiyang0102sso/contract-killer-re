// CGraphics_OGLES.h - CK's OpenGL ES 1.x backend. See recovery/legacy-graphics for the earlier CKZ import.
#ifndef CGRAPHICS_OGLES_H
#define CGRAPHICS_OGLES_H

#include "CGraphicsAbstract.h"
#include "CHash.h"
#include "CRegistry.h"
#include "CStrChar.h"

class CRenderSurface;

class CGraphics_OGLES : public CGraphicsAbstract {
public:
    CGraphics_OGLES();
    virtual ~CGraphics_OGLES();

    static CGraphics_OGLES* GetHardware();

    virtual void Destroy();
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

    virtual void ReleaseDevice() = 0;
    virtual void* CreateContext(void* format, const int* attributes) = 0;
    virtual void ReleaseContext(void* context) = 0;
    virtual void SetCurrentContext(void* context) = 0;

    void InitializeIntrinsicHardwareShaderManagement();
    void SetWidthAndHeightMappedViewportAndOrthoProjection(CRenderSurface& surface);

    int m_field34;                // +0x34
    unsigned int m_packedColor;  // +0x38
    Color_ARGB_fixed m_clearColor; // +0x3c
    int m_clearDepth;            // +0x4c
    int m_clearStencil;          // +0x50
    bool m_contextReady;         // +0x54
    Description m_description;   // +0x58
    CStrChar m_version;           // +0x8c
    int m_field9c;                // +0x9c
    unsigned int m_eaglMajor;     // +0xa0
    unsigned int m_eaglMinor;     // +0xa4
    int m_fielda8;                // +0xa8
    int m_fieldac;                // +0xac
    int m_npotTextures;          // +0xb0 full NPOT extension
    int m_npotRenderTargets;     // +0xb4 limited NPOT extension
    int m_textureLimits[4];       // +0xb8
    CHash m_resources;            // +0xc8
    CRegistry m_resourceRegistry; // +0xdc
};

#endif
