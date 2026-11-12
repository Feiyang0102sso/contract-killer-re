#include "CGraphics_OGLES_Cocoa.h"

#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#include <string.h>
#include "CApplet.h"
#include "CRenderSurface_OGLES_Window_Cocoa.h"
#include "ICCore.h"

unsigned char CGraphics_OGLES_Cocoa::Initialize(const Param* params)
{
    m_layer = CApplet::m_eaglLayer;
    if (m_layer == NULL) {
        return 0;
    }
    for (const Param* param = params; param->m_key != 0; param++) {
        switch (param->m_key) {
        case 1:
            m_colorBits = param->m_value;
            break;
        case 2:
            if ((unsigned char)param->m_value != 0) {
                m_softwareOnly = true;
                Destroy();
                return 0;
            }
            break;
        case 3:
            m_background = param->m_value;
            break;
        case 4:
            m_scaleX = param->m_value;
            break;
        case 5:
            m_scaleY = param->m_value;
            break;
        case 6:
            m_samples = param->m_value;
            break;
        }
    }

    CGraphics_OGLES_Cocoa_GetEAGLVersion(&m_eaglMajor, &m_eaglMinor);
    RSBFormat format = RSBFormat_A8B8G8R8;
    if (m_colorBits < 17) {
        format = RSBFormat_R5G6B5;
    }
    void* context = CreateContext((void*)format, NULL);
    if (context == NULL) {
        return 0;
    }
    SetCurrentContext(context);
    m_version = (const char*)glGetString(GL_VERSION);
    m_fielda8 = 1;
    m_fieldac = 1;
    GLint maxTexture = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexture);
    for (int index = 0; index < 4; index++) {
        m_textureLimits[index] = maxTexture;
    }
    InitializeIntrinsicHardwareShaderManagement();
    m_description.m_abstraction = Abstraction_Hardware;
    m_description.m_renderer = &m_version;
    m_description.m_maxTextureSize = maxTexture;
    m_description.m_maxTextureWidth = maxTexture;
    m_description.m_maxTextureHeight = maxTexture;
    m_description.m_maxRenderTargetSize = maxTexture;
    m_description.m_maxRenderTargetWidth = maxTexture;
    m_description.m_maxRenderTargetHeight = maxTexture;
    m_description.m_maxViewportWidth = maxTexture;
    m_description.m_maxViewportHeight = maxTexture;
    GLint textureUnits = 0;
    glGetIntegerv(GL_MAX_TEXTURE_UNITS, &textureUnits);
    m_description.m_maxTextureUnits = textureUnits;
    m_description.m_maxVertexAttributes = 96;
    m_description.m_maxShaderParameters = 192;

    ICCore* core = CApplet::m_pCore;
    for (int index = 1; index < 5; index++) {
        core->m_canSetDeviceOrientation[index] = 1;
    }
    core->m_applicationCanSetOrientation = 1;
    const char* extensions = (const char*)glGetString(GL_EXTENSIONS);
    m_canDiscardFramebuffer = extensions != NULL && strstr(extensions, "GL_EXT_discard_framebuffer") != NULL;
    m_npotTextures = extensions != NULL && strstr(extensions, "GL_OES_texture_npot") != NULL;
    m_npotRenderTargets = extensions != NULL && strstr(extensions, "GL_APPLE_texture_2D_limited_npot") != NULL;
    SetCurrentContext(NULL);
    ReleaseContext(context);
    m_hardwareReady = m_resources.Init(100, 50, 1) != 0;
    if (!m_hardwareReady) {
        Destroy();
    }
    return m_hardwareReady;
}

void CGraphics_OGLES_Cocoa::RenderBegin(const RenderBeginSettings* settings)
{
    if (m_target != NULL && (m_target->m_flags & ICRenderSurface::Flag_ContentsDiscarded)) {
        if (settings == NULL || !settings->m_preserveContents) {
            ClearBuffers(ClearFlag_Color | ClearFlag_Depth | ClearFlag_Stencil);
        }
        m_target->m_flags &= ~ICRenderSurface::Flag_ContentsDiscarded;
    }
    m_rendering = true;
}

void CGraphics_OGLES_Cocoa::RenderEnd()
{
    m_rendering = false;
}

void CGraphics_OGLES_Cocoa::SwapBuffers()
{
    if (m_target == NULL || (m_target->m_flags & ICRenderSurface::Flag_Window) == 0) {
        return;
    }
    CRenderSurface_OGLES_Window_Cocoa* window = static_cast<CRenderSurface_OGLES_Window_Cocoa*>(m_target);
    if ((window->m_flags & ICRenderSurface::Flag_MultisampleMask) == 0) {
        if (m_canDiscardFramebuffer) {
            const GLenum attachments[] = { GL_DEPTH_ATTACHMENT_OES, GL_STENCIL_ATTACHMENT_OES };
            glDiscardFramebufferEXT(GL_FRAMEBUFFER_OES, 2, attachments);
        }
    } else {
        glDisable(GL_SCISSOR_TEST);
        glBindFramebufferOES(GL_READ_FRAMEBUFFER_APPLE, window->m_multisampleFramebuffer);
        glBindFramebufferOES(GL_DRAW_FRAMEBUFFER_APPLE, window->m_framebuffer);
        glResolveMultisampleFramebufferAPPLE();
        if (m_canDiscardFramebuffer) {
            const GLenum attachments[] = { GL_COLOR_ATTACHMENT0_OES, GL_DEPTH_ATTACHMENT_OES, GL_STENCIL_ATTACHMENT_OES };
            glDiscardFramebufferEXT(GL_READ_FRAMEBUFFER_APPLE, 3, attachments);
        }
        glBindRenderbufferOES(GL_RENDERBUFFER_OES, window->m_colorRenderbuffer);
        glBindFramebufferOES(GL_FRAMEBUFFER_OES, window->m_multisampleFramebuffer);
    }
    CGraphics_OGLES_Cocoa_PresentRenderBuffer(window->m_context);
    window->DiscardContents();
}

void CGraphics_OGLES_Cocoa::ReleaseDevice()
{
    // EAGL context ownership belongs to the window render surface.
}

void* CGraphics_OGLES_Cocoa::CreateContext(void* format, const int*)
{
    if (format == (void*)RSBFormat_A8B8G8R8) {
        return CGraphics_OGLES_Cocoa_CreateContext_A8B8G8R8(m_layer);
    }
    if (format == (void*)RSBFormat_R5G6B5) {
        return CGraphics_OGLES_Cocoa_CreateContext_R5G6B5(m_layer);
    }
    return NULL;
}

void CGraphics_OGLES_Cocoa::ReleaseContext(void* context)
{
    CGraphics_OGLES_Cocoa_ReleaseContext(context);
}

void CGraphics_OGLES_Cocoa::SetCurrentContext(void* context)
{
    CGraphics_OGLES_Cocoa_SetCurrentContext(context);
}
