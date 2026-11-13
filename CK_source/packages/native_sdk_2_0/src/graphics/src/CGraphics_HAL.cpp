#include "CGraphics_HAL.h"

#include "CGraphics_OGLES_Cocoa.h"

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char GraphicsAbstractSizeCheck[sizeof(CGraphicsAbstract) == 0x34 ? 1 : -1];
#endif
#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char GraphicsHALSizeCheck[sizeof(CGraphics_HAL) == 0x40 ? 1 : -1];
#endif

CGraphicsAbstract::CGraphicsAbstract()
{
    m_colorFormat = ColorFormat_Invalid;
    m_abstraction = Abstraction_None;
    m_listener = NULL;
    m_colorBits = 0;
    m_samples = 0;
    m_window = NULL;
    m_target = NULL;
    m_softwareReady = false;
    m_hardwareReady = false;
    m_rendering = false;
    ResetConfiguration();
}

void CGraphicsAbstract::ResetConfiguration()
{
    m_scaleX = 0x10000;
    m_scaleY = 0x10000;
    m_background = Color_A8R8G8B8_t::Black.value;
    m_softwareOnly = false;
    m_windowFlag = false;
    m_initialized = false;
}

CGraphics_HAL::CGraphics_HAL()
{
    m_initialized = false;
    m_softwareReady = false;
    m_hardwareReady = false;
    m_hardware = NULL;
}

CGraphics_HAL::~CGraphics_HAL()
{
}

ICRenderSurface* CGraphics_HAL::GetTargetSurface() const
{
    // the original forwards without a NULL check
    return m_hardware->GetTargetSurface();
}

void CGraphics_HAL::RenderBegin(const RenderBeginSettings* settings)
{
    if (m_hardware->GetTargetSurface() != NULL) {  // the original asks the device directly
        m_hardware->RenderBegin(settings);
    }
}

void CGraphics_HAL::RenderEnd()
{
    if (m_hardware->GetTargetSurface() != NULL) {  // the original asks the device directly
        m_hardware->RenderEnd();
    }
}

void CGraphics_HAL::ClearBuffers(unsigned int flags)
{
    if (m_hardware->GetTargetSurface() != NULL) {  // the original asks the device directly
        m_hardware->ClearBuffers(flags);
    }
}

void CGraphics_HAL::SetClearColor(int alpha, int red, int green, int blue)
{
    m_hardware->SetClearColor(alpha, red, green, blue);
}

void CGraphics_HAL::SetClearColor(const Color_ARGB_fixed& color)
{
    m_hardware->SetClearColor(color);
}

void CGraphics_HAL::SetClearColor(unsigned int color)
{
    m_hardware->SetClearColor(color);
}

void CGraphics_HAL::SetClearDepth(int depth)
{
    m_hardware->SetClearDepth(depth);
}

void CGraphics_HAL::SetClearStencil(int stencil)
{
    m_hardware->SetClearStencil(stencil);
}

const ICGraphics::Description* CGraphics_HAL::GetDescription(Abstraction abstraction) const
{
    if (abstraction == Abstraction_Hardware && m_hardware != NULL) {
        return m_hardware->GetDescription(abstraction);
    }
    return NULL;
}

const CStrChar* CGraphics_HAL::GetVersion(Abstraction abstraction) const
{
    if (abstraction == Abstraction_Hardware && m_hardware != NULL) {
        return m_hardware->GetVersion(abstraction);
    }
    return NULL;
}
