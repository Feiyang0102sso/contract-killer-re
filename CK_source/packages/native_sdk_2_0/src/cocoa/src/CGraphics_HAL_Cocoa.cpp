#include "CGraphics_HAL.h"
#include "CGraphics_OGLES_Cocoa.h"

typedef char GraphicsOGLESCocoaSizeCheck[sizeof(CGraphics_OGLES_Cocoa) == 0x134 ? 1 : -1];

unsigned char CGraphics_HAL::InitializeHardware(const Param* params)
{
    m_hardware = new CGraphics_OGLES_Cocoa();
    if (m_hardware != NULL) {
        m_hardwareReady = m_hardware->Initialize(params) != 0;
    }
    return m_hardwareReady;
}

void CGraphics_HAL::DestroyHardware()
{
    if (m_hardware != NULL) {
        m_hardware->Destroy();
        delete m_hardware;
        m_hardware = NULL;
    }
    m_hardwareReady = false;
}
