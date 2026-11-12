// ICGraphics.h - rendering device interface and the base of every device resource.
// (first drafted by b000x as PROVISIONAL; completed by b037x, the owner)
// The ARMv7 device vtable is CGraphics_Cocoa at 0x33a530.
#ifndef ICGRAPHICS_H
#define ICGRAPHICS_H

#include "CClass.h"
#include "CColor.h"
#include "ICBuffer.h"
#include "ICDisplayProgram.h"
#include "ICGraphicsAbstractionManager.h"
#include "ICGraphicsResource.h"

                                                    

                      
class CStrChar;
class CStrWChar;
struct Color_RGBA;
 

                    

class ICRasterizerState;
class ICRenderSurface;
class ICShaderProgram;

class ICGraphics {
public:
    /** Runtime class id of the graphics devices. */
    enum { ClassId = 0x0466e194 };

    /** One (key, value) initialization setting; a list ends with key 0. */
    struct Param {
        unsigned int m_key;     // 0x00
        unsigned int m_value;   // 0x04
    };
    typedef Param InitParam;

    enum Abstraction {
        Abstraction_None = 0,
        Abstraction_Hardware = 1,
        Abstraction_Software = 2
    };

    struct RenderBeginSettings {
        unsigned char m_preserveContents;
    };

    /** Primitive kinds of Draw(). */
    enum Primitive {
        Primitive_Lines = 0,
        Primitive_LineStrip = 1,
        Primitive_Triangles = 2,
        Primitive_TriangleStrip = 3
    };

    /** Texture filtering hint per sampler. */
    enum TextureSampleHint {
        TextureSampleHint_Default = 0
    };

    /** ClearBuffers() flags (bits of the "Flags" clear table entry). */
    enum ClearFlag {
        ClearFlag_Color = 0x010000,
        ClearFlag_Depth = 0x020000,
        ClearFlag_Stencil = 0x040000
    };

    /** Capabilities of the device (filled in by the platform layer). */
    struct Description {
        unsigned int m_abstraction;                 // 0x00
        const CStrChar* m_renderer;     // 0x04 device name
        unsigned int m_maxTextureSize;              // 0x08
        unsigned int m_maxTextureWidth;             // 0x0c
        unsigned int m_maxTextureHeight;            // 0x10
        unsigned int m_maxRenderTargetSize;         // 0x14
        unsigned int m_maxRenderTargetWidth;        // 0x18
        unsigned int m_maxRenderTargetHeight;       // 0x1c
        unsigned int m_maxViewportWidth;            // 0x20
        unsigned int m_maxViewportHeight;           // 0x24
        unsigned int m_maxTextureUnits;             // 0x28
        unsigned int m_maxVertexAttributes;         // 0x2c
        unsigned int m_maxShaderParameters;         // 0x30
    };

    /** The graphics device cached by the current application. */
    static inline ICGraphics* GetInstance()
    {
        CApp* app = CApplet::m_pApp;
        ICGraphics* instance = NULL;
        if (app != NULL) {
            instance = app->m_graphics;
            if (instance == NULL) {
                void* found = NULL;
                app->m_singletons->Find(ClassId, &found);
                instance = static_cast<ICGraphics*>(found);
                if (instance == NULL) {
                    instance = CreateInstance();
                }
                app->m_graphics = instance;
            }
        }
        return instance;
    }
    static ICGraphics* CreateInstance();

    virtual ~ICGraphics() {}

    virtual unsigned char Initialize(const Param* params) = 0;
    virtual void Destroy() = 0;
    virtual ICRenderSurface* GetTargetSurface() const = 0;
    virtual void RenderBegin(const RenderBeginSettings* settings) = 0;
    virtual void RenderEnd() = 0;
    virtual void ClearBuffers(unsigned int flags) = 0;
    virtual void SwapBuffers() = 0;
    virtual void SetDefaultActiveAbstractionLayer(Abstraction abstraction) = 0;
    virtual Abstraction GetDefaultActiveAbstractionLayer() const = 0;
    virtual void SetDefaultColorFormat(ColorFormat format) = 0;
    virtual ColorFormat GetDefaultColorFormat() const = 0;
    virtual void SetClearColor(int alpha, int red, int green, int blue) = 0;
    virtual void SetClearColor(const Color_ARGB_fixed& color) = 0;
    virtual void SetClearColor(unsigned int color) = 0;
    virtual void SetClearDepth(int depth) = 0;
    virtual void SetClearStencil(int stencil) = 0;
    virtual const Description* GetDescription(Abstraction abstraction) const = 0;
    virtual const CStrChar* GetVersion(Abstraction abstraction) const = 0;

};

       

#endif
