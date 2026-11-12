// CGraphics2d_Lite_HAL_Cocoa.cpp - select the OpenGL ES backend on iOS.
#include "CGraphics2d_Lite_HAL.h"
#include "CGraphics2d_Lite_OGLES.h"

void CGraphics2d_Lite_HAL::InitializeHardware()
{
    ICGraphics::GetInstance()->GetVersion(ICGraphics::Abstraction_Hardware);
    m_device = new CGraphics2d_Lite_OGLES();
    m_hardwareReady = m_device->Initialize();
}
