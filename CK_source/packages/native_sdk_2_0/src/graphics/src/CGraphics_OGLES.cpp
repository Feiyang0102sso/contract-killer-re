#include "CGraphics_OGLES.h"

#include <OpenGLES/ES1/gl.h>
#include <string.h>
#include "CBlitUtil.h"
#include "CGraphics_HAL.h"
#include "CGraphics_OGLES_Cocoa.h"
#include "CRenderSurface.h"
#include "ICCore.h"

CGraphics_OGLES::CGraphics_OGLES()
{
    m_field9c = 0;
    m_eaglMajor = 0;
    m_eaglMinor = 0;
    m_fielda8 = 0;
    m_fieldac = 0;
    memset(&m_description, 0, sizeof(m_description));
}

CGraphics_OGLES::~CGraphics_OGLES()
{
}

CGraphics_OGLES* CGraphics_OGLES::GetHardware()
{
    CApp* app = CApplet::m_pApp;
    if (app == NULL) {
        return NULL;
    }
    CGraphics_HAL* device = static_cast<CGraphics_HAL*>(app->m_graphics);
    if (device == NULL) {
        return NULL;
    }
    return device->m_hardware;
}

ICRenderSurface* CGraphics_OGLES::GetTargetSurface() const
{
    return m_target;
}

void CGraphics_OGLES::RenderBegin(const RenderBeginSettings*)
{
    if (m_target != NULL) {
        m_target->m_flags &= ~ICRenderSurface::Flag_ContentsDiscarded;
    }
}

void CGraphics_OGLES::RenderEnd()
{
}

static unsigned int ClearColorChannel(int fixed)
{
    int channel = fixed >> 8;
    if (channel < 0) {
        return 0;
    }
    if (channel > 255) {
        return 255;
    }
    return channel;
}

void CGraphics_OGLES::SetClearColor(int alpha, int red, int green, int blue)
{
    m_clearColor.a = alpha;
    m_clearColor.r = red;
    m_clearColor.g = green;
    m_clearColor.b = blue;
    m_packedColor = ClearColorChannel(alpha) << 24 | ClearColorChannel(red) << 16 |
                    ClearColorChannel(green) << 8 | ClearColorChannel(blue);
}

void CGraphics_OGLES::SetClearColor(const Color_ARGB_fixed& color)
{
    m_clearColor = color;

    unsigned int alpha = 0;
    int alphaChannel = m_clearColor.a >> 8;
    if (alphaChannel >= 0) {
        alpha = 0xff000000u;
        if (alphaChannel <= 255) {
            alpha = (m_clearColor.a & 0xff00u) << 16;
        }
    }
    unsigned int red = 0;
    int redChannel = m_clearColor.r >> 8;
    if (redChannel >= 0) {
        red = 0xff0000u;
        if (redChannel <= 255) {
            red = (m_clearColor.r & 0xff00u) << 8;
        }
    }
    unsigned int green = 0;
    int greenChannel = m_clearColor.g >> 8;
    if (greenChannel >= 0) {
        green = m_clearColor.g & 0xff00u;
        if (greenChannel > 255) {
            green = 0xff00u;
        }
    }
    unsigned int blue = 0;
    int blueChannel = m_clearColor.b >> 8;
    if (blueChannel >= 0) {
        blue = blueChannel & 0xffu;
        if (blueChannel > 255) {
            blue = 0xffu;
        }
    }
    m_packedColor = blue | red | alpha | green;
}

void CGraphics_OGLES::SetClearColor(unsigned int color)
{
    m_packedColor = color;
    m_clearColor.Make(ColorFormat_A8R8G8B8, &color);
}

void CGraphics_OGLES::SetClearDepth(int depth)
{
    m_clearDepth = depth;
}

void CGraphics_OGLES::SetClearStencil(int stencil)
{
    m_clearStencil = stencil;
}

const ICGraphics::Description* CGraphics_OGLES::GetDescription(Abstraction abstraction) const
{
    if (abstraction == Abstraction_Hardware) {
        return &m_description;
    }
    return NULL;
}

const CStrChar* CGraphics_OGLES::GetVersion(Abstraction abstraction) const
{
    if (abstraction == Abstraction_Hardware) {
        return &m_version;
    }
    return NULL;
}

void CGraphics_OGLES::ClearBuffers(unsigned int flags)
{
    ICRenderSurface* target = m_target;
    if (target == NULL) {
        return;
    }
    GLbitfield mask = 0;
    if ((flags & ClearFlag_Color) && (target->m_flags & ICRenderSurface::Flag_HasColorFormat)) {
        glClearColorx(m_clearColor.r, m_clearColor.g, m_clearColor.b, m_clearColor.a);
        mask |= GL_COLOR_BUFFER_BIT;
    }
    if ((flags & ClearFlag_Depth) && (target->m_flags & ICRenderSurface::Flag_HasDepthBuffer)) {
        glClearDepthx(m_clearDepth);
        mask |= GL_DEPTH_BUFFER_BIT;
    }
    if ((flags & ClearFlag_Stencil) && (target->m_flags & ICRenderSurface::Flag_HasStencilBuffer)) {
        glClearStencil(m_clearStencil);
        mask |= GL_STENCIL_BUFFER_BIT;
    }
    glDisable(GL_SCISSOR_TEST);
    glDepthMask(GL_TRUE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClear(mask);
    target->m_flags &= ~ICRenderSurface::Flag_ContentsDiscarded;
}

void CGraphics_OGLES::Destroy()
{
    ReleaseDevice();
    m_contextReady = false;
}

void CGraphics_OGLES::InitializeIntrinsicHardwareShaderManagement()
{
    CBlitUtil_AddBlitOps_OGLES();
}

void CGraphics_OGLES::SetWidthAndHeightMappedViewportAndOrthoProjection(CRenderSurface& surface)
{
    unsigned int width;
    unsigned int height;
    surface.GetWidthAndHeight(width, height);
    if ((surface.m_flags & ICRenderSurface::Flag_Targetable) && CApplet::m_pCore->m_deviceOrientationSet) {
        const int farPlane = 0x7fff0000;
        switch (CApplet::m_pCore->m_deviceOrientation) {
        case ICCore::DeviceOrientation_Portrait:
            break;
        case ICCore::DeviceOrientation_LandscapeRight:
            glViewport(0, 0, height, width);
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glOrthox(0, 0u - (height << 16), 0, width << 16, 0, farPlane);
            glRotatex(90 << 16, 0, 0, 0x10000);
            return;
        case ICCore::DeviceOrientation_PortraitUpsideDown:
            glViewport(0, 0, width, height);
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glOrthox(width << 16, 0, 0, height << 16, 0, farPlane);
            return;
        case ICCore::DeviceOrientation_LandscapeLeft:
            glViewport(0, 0, height, width);
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glOrthox(height << 16, 0, 0u - (width << 16), 0, 0, farPlane);
            glRotatex(-90 * 65536, 0, 0, 0x10000);
            return;
        default:
            return;
        }
    }
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrthox(0, width << 16, height << 16, 0, 0, 0x7fff0000);
}
