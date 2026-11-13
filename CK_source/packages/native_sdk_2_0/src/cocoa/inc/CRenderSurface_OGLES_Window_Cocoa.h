// CRenderSurface_OGLES_Window_Cocoa.h - the window (CAEAGLLayer) render surface: an EAGL context with
// a color renderbuffer backed by the layer, a depth renderbuffer and an optional multisample framebuffer
// that is resolved into the window framebuffer before presenting.
// (first drafted by b002 as PROVISIONAL; completed by b039x, the owner of
// cocoa/src/CRenderSurface_OGLES_Window_Cocoa.cpp)
#ifndef CRENDERSURFACE_OGLES_WINDOW_COCOA_H
#define CRENDERSURFACE_OGLES_WINDOW_COCOA_H

#include "CRenderSurface_OGLES_Texture.h"

class CRenderSurface_OGLES_Window_Cocoa : public CRenderSurface_OGLES_Texture {
public:
    CRenderSurface_OGLES_Window_Cocoa();
    virtual ~CRenderSurface_OGLES_Window_Cocoa();

    virtual unsigned char Initialize(const InitParam* params);
    virtual void Destroy();
    /** Only a locked surface can be locked again (its color buffer holds the read back pixels). */
    virtual void Lock(unsigned char** data, int& pitch, LockType lockType,
                      const CRectangle* area);
    virtual void Unlock();
    /** Size of the color buffer, swapped when the device is in landscape orientation. */
    virtual unsigned char GetWidthAndHeight(unsigned int& width, unsigned int& height);
    /** Make the window the render target of the device (both the HAL and its GLES device). */
    virtual void Bind();

    unsigned char InitializeFrameBuffer();
    void DestroyFrameBuffer();

    void* m_context;                                // 0x30 EAGLContext the surface presents with
    unsigned int m_framebuffer;                     // 0x34
    unsigned int m_colorRenderbuffer;               // 0x38 backed by the layer
    unsigned int m_depthRenderbuffer;               // 0x3c
    unsigned int m_multisampleFramebuffer;          // 0x40 0: no multisampling
    unsigned int m_multisampleColorRenderbuffer;    // 0x44
    unsigned int m_multisampleDepthRenderbuffer;    // 0x48
    int m_lockCount;                                // 0x4c
};

#endif
