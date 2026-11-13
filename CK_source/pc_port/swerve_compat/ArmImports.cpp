// Swerve's GLES 1.1 and C imports use the ARM base procedure-call standard.
#include "ArmImports.h"
#include "ArmCall.h"
#include <SwerveApi.h>
#include "SwvPort.h"
#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utility>

extern "C" {
// Fixed point and engine-private entry points (GlesDesktop.cpp, swvglruntime.cpp, swvpngload.cpp).
void GL_APIENTRY glDepthRangex(GLfixed nearDepth, GLfixed farDepth);
void GL_APIENTRY glFogx(GLenum name, GLfixed value);
void GL_APIENTRY glLightModelx(GLenum name, GLfixed value);
void GL_APIENTRY glTexEnvx(GLenum target, GLenum name, GLfixed value);
void GL_APIENTRY glTexParameterx(GLenum target, GLenum name, GLfixed value);
void glBindBufferSUPERSCAPE(unsigned int target, unsigned int buffer);
void glBufferDataSUPERSCAPE(unsigned int target, int size, const void* data, unsigned int usage);
void glBufferSubDataSUPERSCAPE(unsigned int target, int offset, int size, const void* data);
void glGenBuffersSUPERSCAPE(int count, unsigned int* buffers);
void glDeleteBuffersSUPERSCAPE(int count, const unsigned int* buffers);
int swvPNGLoadBuffer(swvEngine* engine, unsigned int size, const unsigned char* data, swvImage2D** image);
}

namespace ck::arm {
namespace {

template<class T>
T Unpack(uint32_t word)
{
    if constexpr (std::is_pointer_v<T>) {
        if constexpr (std::is_function_v<std::remove_pointer_t<T>>) {
            return reinterpret_cast<T>(word);
        } else {
            return static_cast<T>(const_cast<void*>(ResolvePointer(word)));
        }
    } else if constexpr (std::is_same_v<T, float>) {
        float value;
        memcpy(&value, &word, sizeof(value));
        return value;
    } else {
        return static_cast<T>(word);
    }
}

template<class Result, class... Args, class Function, size_t... indices>
uint64_t CallNative(Function function, const uint32_t* words, std::index_sequence<indices...>)
{
    if constexpr (std::is_void_v<Result>) {
        function(Unpack<Args>(words[indices])...);
        return 0;
    } else {
        return Pack(function(Unpack<Args>(words[indices])...));
    }
}

template<class Result, class... Args>
uint64_t CallNative(Result (*function)(Args...), const uint32_t* words)
{
    return CallNative<Result, Args...>(function, words, std::index_sequence_for<Args...>{});
}

template<class Result, class... Args>
uint64_t CallNative(Result (__stdcall *function)(Args...), const uint32_t* words)
{
    return CallNative<Result, Args...>(function, words, std::index_sequence_for<Args...>{});
}

uint64_t WideArg(const uint32_t* words)
{
    return static_cast<uint64_t>(words[0]) | (static_cast<uint64_t>(words[1]) << 32);
}

uint64_t Divide64(const uint32_t* words)
{
    return static_cast<int64_t>(WideArg(words)) / static_cast<int64_t>(WideArg(words + 2));
}

uint64_t PureVirtual(const uint32_t*)
{
    fprintf(stderr, "Swerve called a pure virtual function\n");
    abort();
}

/** The VBO populator the engine got from swvGetGLPopulateVBO (ArmMachine.cpp hands it this slot). */
uint64_t PopulateVbo(const uint32_t* words)
{
    const swvPopulateVBOFunction populate = swvGetGLPopulateVBO();
    const int size = populate(Unpack<const swvVertexData*>(words[0]), Unpack<void*>(words[1]),
                              Unpack<swvVBOFormat*>(words[2]));
    return static_cast<uint32_t>(size);
}

struct Import {
    const char* name;
    NativeImport call;
};

#define IMPORT(Name) { #Name, +[](const uint32_t* args) { return CallNative(&Name, args); } }

const Import imports[] = {
    { "__cxa_pure_virtual", PureVirtual },
    { "__divdi3", Divide64 },
    { "__divsi3", +[](const uint32_t* args) -> uint64_t { return Unpack<int>(args[0]) / Unpack<int>(args[1]); } },
    { "__modsi3", +[](const uint32_t* args) -> uint64_t { return Unpack<int>(args[0]) % Unpack<int>(args[1]); } },
    { "__udivsi3", +[](const uint32_t* args) -> uint64_t { return args[0] / args[1]; } },
    { "__umodsi3", +[](const uint32_t* args) -> uint64_t { return args[0] % args[1]; } },
    IMPORT(cosf),
    IMPORT(glActiveTexture),
    IMPORT(glAlphaFunc),
    IMPORT(glBindBufferSUPERSCAPE),
    IMPORT(glBindTexture),
    IMPORT(glBlendFunc),
    IMPORT(glBufferDataSUPERSCAPE),
    IMPORT(glBufferSubDataSUPERSCAPE),
    IMPORT(glClear),
    IMPORT(glClearColor),
    IMPORT(glClearDepthf),
    IMPORT(glClientActiveTexture),
    IMPORT(glColor4f),
    IMPORT(glColorMask),
    IMPORT(glColorPointer),
    IMPORT(glCompressedTexImage2D),
    IMPORT(glCullFace),
    IMPORT(glDeleteBuffersSUPERSCAPE),
    IMPORT(glDeleteTextures),
    IMPORT(glDepthFunc),
    IMPORT(glDepthMask),
    IMPORT(glDepthRangex),
    IMPORT(glDisable),
    IMPORT(glDisableClientState),
    IMPORT(glDrawArrays),
    IMPORT(glDrawElements),
    IMPORT(glEnable),
    IMPORT(glEnableClientState),
    IMPORT(glFinish),
    IMPORT(glFogf),
    IMPORT(glFogfv),
    IMPORT(glFogx),
    IMPORT(glFrontFace),
    IMPORT(glGenBuffersSUPERSCAPE),
    IMPORT(glGenTextures),
    IMPORT(glGetError),
    IMPORT(glHint),
    IMPORT(glLightModelfv),
    IMPORT(glLightModelx),
    IMPORT(glLightf),
    IMPORT(glLightfv),
    IMPORT(glLoadIdentity),
    IMPORT(glLoadMatrixf),
    IMPORT(glMaterialf),
    IMPORT(glMaterialfv),
    IMPORT(glMatrixMode),
    IMPORT(glNormalPointer),
    IMPORT(glPixelStorei),
    IMPORT(glPolygonOffset),
    IMPORT(glPopMatrix),
    IMPORT(glPushMatrix),
    IMPORT(glRotatef),
    IMPORT(glScissor),
    IMPORT(glShadeModel),
    IMPORT(glTexCoordPointer),
    IMPORT(glTexEnvfv),
    IMPORT(glTexEnvx),
    IMPORT(glTexImage2D),
    IMPORT(glTexParameterx),
    IMPORT(glTranslatef),
    IMPORT(glVertexPointer),
    IMPORT(glViewport),
    IMPORT(memcpy),
    IMPORT(sinf),
    IMPORT(swvAdler32),
    IMPORT(swvDecompressError),
    IMPORT(swvDecompressOnData),
    IMPORT(swvDecompressOnDataEnd),
    IMPORT(swvDecompressOnDataStart),
    IMPORT(swvExit),
    IMPORT(swvFileManagerGetInstance),
    IMPORT(swvFileManagerOpenFileForReading),
    IMPORT(swvFileManagerRelease),
    IMPORT(swvFileReadBytes),
    IMPORT(swvFileRelease),
    IMPORT(swvFileSeek),
    IMPORT(swvFileSize),
    IMPORT(swvGLBindImage),
    IMPORT(swvGLCanDeleteImage2DData),
    IMPORT(swvGLRebindImage),
    IMPORT(swvGLReleaseImage),
    IMPORT(swvGLRuntimeCleanup),
    IMPORT(swvGLRuntimePrepare),
    IMPORT(swvGLShareContext),
    IMPORT(swvGLSupportVBO),
    IMPORT(swvGLUnpalettizeImage2D),
    IMPORT(swvGetGLMaxTextureSize),
    IMPORT(swvGetGLMaxViewportDims),
    IMPORT(swvGetGLPopulateVBO),
    IMPORT(swvGetGLStretchBlit),
    IMPORT(swvGetGLSubmeshPacking),
    IMPORT(swvMemCpy),
    IMPORT(swvMemSet),
    IMPORT(swvPNGLoadBuffer),
    IMPORT(swvStrChr),
    IMPORT(swvStrDup),
    IMPORT(swvStrLen),
    IMPORT(swvStrLwr),
    IMPORT(swvStrNCmp),
    IMPORT(swvSystemAllocate),
    IMPORT(swvSystemFree),
    IMPORT(tanf),
    { "swvPopulateVBO", PopulateVbo },
};
#undef IMPORT

} // namespace

NativeImport FindImport(const char* name)
{
    for (const Import& entry : imports) {
        if (strcmp(name, entry.name) == 0) {
            return entry.call;
        }
    }
    return nullptr;
}

} // namespace ck::arm
