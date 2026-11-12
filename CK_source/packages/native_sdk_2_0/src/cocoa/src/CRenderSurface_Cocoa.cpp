#include "ICGraphics.h"
#include "CRenderSurface_OGLES_Texture.h"
#include "CRenderSurface_OGLES_Texture_FBO.h"
#include "CRenderSurface_OGLES_Window_Cocoa.h"

ICRenderSurface* ICRenderSurface::CreateInstance(ICGraphics::Abstraction abstraction, Type type, Targetability targetability)
{
    if (abstraction == ICGraphics::Abstraction_Software) {
        return NULL;
    }
    ICGraphics::GetInstance()->GetVersion(ICGraphics::Abstraction_Hardware);
    if (type == Type_Window) {
        return new CRenderSurface_OGLES_Window_Cocoa();
    }
    if (type == Type_Default) {
        if (targetability == Targetability_2 || targetability == Targetability_3) {
            return new CRenderSurface_OGLES_Texture_FBO();
        }
        return new CRenderSurface_OGLES_Texture();
    }
    return NULL;
}
