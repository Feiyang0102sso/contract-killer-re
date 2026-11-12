// CK uses the OpenGL ES fixed-function renderer on iOS. This original factory returns NULL.
#include "ICShader.h"

ICShader* ICShader::CreateInstance(ICGraphics::Abstraction)
{
    return 0;
}
