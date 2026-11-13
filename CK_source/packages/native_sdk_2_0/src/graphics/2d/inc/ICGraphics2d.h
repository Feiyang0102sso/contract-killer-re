// ICGraphics2d.h - 2d drawing interface of a graphics abstraction (created by b037x, owner of
// ICGraphics2d::GetInstance in graphics/src/CGraphicsAbstractionManager.cpp).
// (gfx 2026-11-10: rebuilt as CK's old native_sdk 2d interface. Slot order from the CK vtables
// of CGraphics2d_Lite_OGLES @0x33b300 and CGraphics2d_Lite_HAL @0x33b250; the CKZ-era float /
// Color_RGBA interface is gone. Coordinates of Translate / Scale / Rotate are 16.16 fixed point.)
#ifndef ICGRAPHICS2D_H
#define ICGRAPHICS2D_H

#include "CClass.h"
#include "CApplet.h"
#include "CApp.h"
#include "CHash.h"
#include "ICGraphics.h"




class CMatrix2d;
class CVector2d;
class CLineSegment2d;



class CRectangle;
struct Color_RGBA;
struct Color_ARGB_fixed;




class ICRenderSurface;
class ICRenderText2d;
class ICShader;


class ICGraphics2d {
public:
    /** Key in the application singleton table (CStringToKey("ICGraphics2d")). */
    enum { ClassId = 0x66e79740 };

    /** Mirroring of a drawn surface. */
    enum Flip {
        Flip_None = 0,
        Flip_Horizontal = 1,
        Flip_Vertical = 2,
        Flip_Both = 3
    };

    /** Primitive kinds of Draw(Primitive, first, count) (CGraphics2d_Lite_OGLES::Draw). */
    enum Primitive {
        Primitive_Lines = 1,
        Primitive_Polygon = 2,
        Primitive_Rectangles = 3
    };

    /** Render states of Enable / Disable / PushState / PopState (names inferred from the OGLES
        implementation: 0 toggles GL_ALPHA_TEST, 1 GL_BLEND, 3 the magenta color key). */
    enum State {
        State_AlphaTest = 0,
        State_Blend = 1,
        State_2 = 2,
        State_ColorKey = 3,
        State_4 = 4
    };

    /** Blend functions of SetBlendArg (0/2: src alpha, 1/3: additive, 4: one / one). */
    enum BlendArg {
        BlendArg_0 = 0,
        BlendArg_1 = 1,
        BlendArg_2 = 2,
        BlendArg_3 = 3,
        BlendArg_4 = 4
    };

    /** Coordinate origin used for hardware scissor coordinates. */
    enum Origin {
        Origin_0 = 0,
        Origin_TopLeft = 1
    };

    /** Shader mode (SetShaderMode; ignored by the OGLES renderer). */
    enum ShaderMode {
        ShaderMode_0 = 0
    };

    /** Recorded sequence of 2d drawing commands. */
    class IDisplayProgram {
    public:
        /** Recording mode (CreationInfo::m_mode / DisplayListBegin). */
        enum Mode {
            Mode_0 = 0,
            Mode_1 = 1,
            Mode_2 = 2,
            Mode_3 = 3,     // added by gfx (GetComponentMinNumOfEntries)
            Mode_4 = 4,     // added by gfx
            Mode_5 = 5      // added by gfx
        };

        /** Creation settings (CGraphics2dAbstract::DisplayProgram::Initialize). */
        struct CreationInfo {
            unsigned short m_capacity0;     // 0x00
            unsigned short m_capacity1;     // 0x02
            int m_mode;                     // 0x04 Mode: 1 and 2 select the core flags
            int m_layers;                   // 0x08
            int m_flag0c;                   // 0x0c 0: sized from the device description
            unsigned short m_capacity10;    // 0x10
            unsigned short m_capacity12;    // 0x12
            unsigned short m_capacity14;    // 0x14
        };

        /** Platform factory. */
        static IDisplayProgram* CreateInstance(ICGraphics::Abstraction abstraction);

        virtual ~IDisplayProgram() {}

        virtual unsigned char Initialize(const CreationInfo& info) = 0;
        virtual void SetLayer(int layer) = 0;
        virtual int GetLayer() = 0;
        virtual ICGraphics::Abstraction GetAbstraction() = 0;
    };

    /** Platform factory. */
    static ICGraphics2d* CreateInstance();

    /** The application's 2d renderer: cached in CApp, created on first use. */
    static inline ICGraphics2d* GetInstance()
    {
        CApp* app = CApplet::m_pApp;
        ICGraphics2d* instance = NULL;
        if (app != NULL) {
            instance = app->m_graphics2d;
            if (instance == NULL) {
                void* found = NULL;
                app->m_singletons->Find(ClassId, &found);
                // test the found pointer itself (cbnz in the original callers)
                if (found == NULL) {
                    instance = CreateInstance();
                } else {
                    instance = (ICGraphics2d*)found;
                }
                app->m_graphics2d = instance;
            }
        }
        return instance;
    }

    virtual ~ICGraphics2d() {}

    virtual unsigned char Initialize() = 0;                                             // 0x08
    virtual void Destroy() = 0;                                                         // 0x0c
    virtual void SetActiveAbstractionLayer(ICGraphics::Abstraction abstraction) = 0;   // 0x10
    virtual ICGraphics::Abstraction GetActiveAbstractionLayer() = 0;                   // 0x14
    virtual void RenderBegin(unsigned char clear) = 0;                                  // 0x18
    virtual void RenderEnd() = 0;                                                       // 0x1c
    virtual void DisplayListBegin(IDisplayProgram& program, IDisplayProgram::Mode mode) = 0;  // 0x20
    virtual void DisplayListEnd() = 0;                                                  // 0x24
    virtual void Run(IDisplayProgram& program) = 0;                                     // 0x28
    virtual void SetOrigin(Origin origin) = 0;                                          // 0x2c
    virtual void SetShaderMode(ShaderMode mode) = 0;                                    // 0x30
    virtual void SetShader(ICShader* shader) = 0;                                       // 0x34
    virtual void SetColor(int alpha, int red, int green, int blue) = 0;                 // 0x38
    virtual void SetColor(const Color_ARGB_fixed& color) = 0;                           // 0x3c
    virtual void SetColor(unsigned int argb) = 0;                                       // 0x40
    virtual void Enable(State state) = 0;                                               // 0x44
    virtual void Disable(State state) = 0;                                              // 0x48
    virtual void PushState(State state) = 0;                                            // 0x4c
    virtual void PopState(State state) = 0;                                             // 0x50
    virtual void SetBlendArg(BlendArg arg) = 0;                                         // 0x54
    virtual void LoadIdentity() = 0;                                                    // 0x58
    virtual void LoadTransform(const CMatrix2d* matrix, const CVector2d* translation) = 0;      // 0x5c
    virtual void MultiplyTransform(const CMatrix2d* matrix, const CVector2d* translation) = 0;  // 0x60
    /** Offsets are 16.16 fixed point. */
    virtual void Translate(int x, int y) = 0;                                           // 0x64
    /** Angle in 16.16 fixed point degrees. */
    virtual void Rotate(int angle) = 0;                                                 // 0x68
    virtual void Scale(int x, int y) = 0;                                               // 0x6c
    virtual void PushTransform() = 0;                                                   // 0x70
    virtual void PopTransform() = 0;                                                    // 0x74
    virtual void GetTransform(const CMatrix2d** matrix, const CVector2d** translation) = 0;  // 0x78
    virtual void SetVertexPositionCoords(int componentCount, unsigned int type, int stride,
                                         const void* coords) = 0;                       // 0x7c
    virtual void SetClip(unsigned short x, unsigned short y, unsigned short width,
                         unsigned short height) = 0;                                    // 0x80
    virtual void SetClip(const CRectangle& clip) = 0;                        // 0x84
    virtual const CRectangle& GetClip() const = 0;                           // 0x88
    virtual void Draw(ICRenderSurface& surface, Flip flip, const CRectangle* source) = 0;  // 0x8c
    virtual void Draw(ICRenderText2d& text) = 0;                                        // 0x90
    virtual void Draw(Primitive primitive, int first, int count) = 0;                   // 0x94
};



#endif
