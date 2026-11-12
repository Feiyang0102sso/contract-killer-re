// CGraphics_OGLES_Cocoa.h - EAGL context, rendering and buffer presentation for CK.
#ifndef CGRAPHICS_OGLES_COCOA_H
#define CGRAPHICS_OGLES_COCOA_H

#include "CGraphics_OGLES.h"

void CGraphics_OGLES_Cocoa_GetEAGLVersion(unsigned int* major, unsigned int* minor);
void* CGraphics_OGLES_Cocoa_CreateContext_R5G6B5(void* layer);
void* CGraphics_OGLES_Cocoa_CreateContext_A8B8G8R8(void* layer);
void CGraphics_OGLES_Cocoa_ReleaseContext(void* context);
void CGraphics_OGLES_Cocoa_SetCurrentContext(void* context);
void CGraphics_OGLES_Cocoa_PresentRenderBuffer(void* context);
unsigned char CGraphics_OGLES_Cocoa_RenderBufferStorage(void* layer, void* context);

class CGraphics_OGLES_Cocoa : public CGraphics_OGLES {
public:
    virtual unsigned char Initialize(const Param* params);
    virtual void RenderBegin(const RenderBeginSettings* settings);
    virtual void RenderEnd();
    virtual void SwapBuffers();
    virtual void ReleaseDevice();
    virtual void* CreateContext(void* format, const int* attributes);
    virtual void ReleaseContext(void* context);
    virtual void SetCurrentContext(void* context);

    void* m_layer;                  // +0x12c
    bool m_canDiscardFramebuffer;  // +0x130
};

#endif
