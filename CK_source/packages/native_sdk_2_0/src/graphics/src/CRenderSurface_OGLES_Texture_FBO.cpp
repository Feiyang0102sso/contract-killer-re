// CRenderSurface_OGLES_Texture_FBO.cpp - targetable texture rendered into through a framebuffer object.
#include "CRenderSurface_OGLES_Texture_FBO.h"
#include "CGraphics_HAL.h"
#include "CGraphics_OGLES_Cocoa.h"
#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>

                                                                         

typedef char TextureFBOSizeCheck[sizeof(CRenderSurface_OGLES_Texture_FBO) == 0x40 ? 1 : -1];

CRenderSurface_OGLES_Texture_FBO::CRenderSurface_OGLES_Texture_FBO()
{
    m_flags = (m_flags & ~(Flag_Texture | Flag_Software | Flag_Writable)) | Flag_Texture | Flag_Hardware | Flag_Writable;
    m_framebuffer = 0;
    m_depthRenderbuffer = 0;
}

CRenderSurface_OGLES_Texture_FBO::~CRenderSurface_OGLES_Texture_FBO()
{
    Destroy();
}

void CRenderSurface_OGLES_Texture_FBO::Destroy()
{
    if (m_depthRenderbuffer != 0) {
        glDeleteRenderbuffersOES(1, &m_depthRenderbuffer);
        m_depthRenderbuffer = 0;
    }
    if (m_framebuffer != 0) {
        glDeleteFramebuffersOES(1, &m_framebuffer);
        m_framebuffer = 0;
    }
    CRenderSurface_OGLES_Targetable::Destroy();
}

unsigned char CRenderSurface_OGLES_Texture_FBO::Initialize(const InitParam* params)
{
    unsigned char result = CRenderSurface_OGLES_Texture::Initialize(params);
    GLenum depthFormat = 0;
    for (const InitParam* param = params; param->m_key != InitParam_End; ++param) {
        if (param->m_key == InitParam_DepthFormat && param->m_value == RSBFormat_Depth16) {
            depthFormat = GL_DEPTH_COMPONENT16_OES;
        }
    }
    if (result) {
        // The original queries the renderbuffer binding for both saved names.
        GLint previousFramebuffer = 0;
        GLint previousRenderbuffer = 0;
        glGetIntegerv(GL_RENDERBUFFER_BINDING_OES, &previousFramebuffer);
        glGetIntegerv(GL_RENDERBUFFER_BINDING_OES, &previousRenderbuffer);
        glGenFramebuffersOES(1, &m_framebuffer);
        glBindFramebufferOES(GL_FRAMEBUFFER_OES, m_framebuffer);
        glFramebufferTexture2DOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_TEXTURE_2D, m_texture, 0);
        if (depthFormat != 0) {
            CRenderSurfaceBuffer* buffer = m_buffers[Buffer_Color];
            glGenRenderbuffersOES(1, &m_depthRenderbuffer);
            glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_depthRenderbuffer);
            glRenderbufferStorageOES(GL_RENDERBUFFER_OES, depthFormat, buffer->m_width, buffer->m_height);
            glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_DEPTH_ATTACHMENT_OES, GL_RENDERBUFFER_OES,
                                         m_depthRenderbuffer);
        }
        glBindRenderbufferOES(GL_RENDERBUFFER_OES, previousRenderbuffer);
        glBindFramebufferOES(GL_FRAMEBUFFER_OES, previousFramebuffer);
    } else {
        Destroy();
    }
    return result;
}

void CRenderSurface_OGLES_Texture_FBO::Bind()
{
    CGraphics_HAL* graphics = static_cast<CGraphics_HAL*>(ICGraphics::GetInstance());
    if (this == graphics->m_hardware->m_target) {
        return;
    }
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, m_framebuffer);
    graphics = static_cast<CGraphics_HAL*>(ICGraphics::GetInstance());
    graphics->m_target = this;
    graphics->m_hardware->m_target = this;
}

void CRenderSurface_OGLES_Texture_FBO::Lock(unsigned char** data, int& pitch, LockType lockType,
                                            const CRectangle* area)
{
    if (data != NULL) {
        *data = NULL;
    }
    pitch = 0;
}

void CRenderSurface_OGLES_Texture_FBO::Unlock()
{
}
