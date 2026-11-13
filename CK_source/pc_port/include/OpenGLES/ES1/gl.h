#pragma once
#include <GLES/gl.h>

#ifdef __cplusplus
extern "C" {
#endif
const GLubyte* GL_APIENTRY ckGlGetString(GLenum name);
#ifdef __cplusplus
}
#endif
#define glGetString ckGlGetString
