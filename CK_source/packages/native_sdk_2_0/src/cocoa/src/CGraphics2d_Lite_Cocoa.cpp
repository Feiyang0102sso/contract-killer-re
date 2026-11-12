// CGraphics2d_Lite_Cocoa.cpp - the Cocoa factories for CK's GLES 2D renderer.
#include "CGraphics2d_Lite_HAL.h"
#include "CGraphics2d_Lite_OGLES.h"

ICGraphics2d* ICGraphics2d::CreateInstance()
{
    return new CGraphics2d_Lite_HAL();
}

ICGraphics2d::IDisplayProgram* ICGraphics2d::IDisplayProgram::CreateInstance(ICGraphics::Abstraction abstraction)
{
    if (abstraction == ICGraphics::Abstraction_Software) {
        return NULL;
    }
    return new CGraphics2d_Lite_OGLES::DisplayProgram();
}
