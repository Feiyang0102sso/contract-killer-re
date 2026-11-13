// CGraphics2d_Lite_OGLES.h - the OpenGL ES 1.x renderer and its recorded draw lists.
#ifndef CGRAPHICS2D_LITE_OGLES_H
#define CGRAPHICS2D_LITE_OGLES_H

#include <OpenGLES/ES1/gl.h>
#include "CColor.h"
#include "CMatrix2d.h"
#include "CRectangle.h"
#include "CVector2d.h"
#include "ICGraphics2d.h"
#include "TCVector.h"
#include "CGraphicsAbstract.h"
#include "CRenderSurfaceBuffer.h"
#include "CVertexBuffer.h"
#include "CVertexAttribute.h"

class CGraphics2dAbstract : public ICGraphics2d {
public:
    CGraphics2dAbstract()
        : m_initialized(0)
        , m_softwareReady(0)
        , m_hardwareReady(0)
        , m_rendering(0)
    {
    }
    virtual ~CGraphics2dAbstract() {}

    virtual void SetActiveAbstractionLayer(ICGraphics::Abstraction abstraction) {}
    virtual ICGraphics::Abstraction GetActiveAbstractionLayer() { return ICGraphics::Abstraction_None; }

    /** 16.16 fixed point 2D affine matrix: rows 0..1 the linear part, row 2 the translation. */
    class Matrix {
    public:
        Matrix()
        {
            m[0][0] = 0x10000;
            m[1][0] = 0;
            m[0][1] = 0;
            m[1][1] = 0x10000;
            m[2][0] = 0;
            m[2][1] = 0;
        }

        /** this = this * other, linear part only (the translation is kept). */
        void precat_R(Matrix* other);
        void multiply(int a, int b, int c, int d, int tx, int ty);

        int m[3][2];
    };

    /** Saved render states: one stack per ICGraphics2d::State (State_Blend has two). */
    class AbstractState {
    public:
        /** 20 byte entry of the State_2 stack, with packed and fixed-point channels. */
        struct Entry20 {
            unsigned int m_argb;
            Color_ARGB_fixed m_color;
        };

        void Initialize(unsigned int depth);
        void Destroy();
        void Push(ICGraphics2d::State state);
        void Pop(ICGraphics2d::State state);

        TCStack<unsigned char> m_alphaTest;     // 0x00
        TCStack<unsigned char> m_blend;         // 0x0c
        TCStack<unsigned int> m_blendArg;       // 0x18
        TCStack<Entry20> m_state2;              // 0x24
        TCStack<unsigned char> m_colorKey;      // 0x30
        TCStack<unsigned char> m_state4;        // 0x3c
    };

    /** 2D display program: the IDisplayProgram interface over the shared core (0x50 bytes). */
    class DisplayProgram : public ICGraphics2d::IDisplayProgram, public CGraphicsAbstract::DisplayProgram {
    public:
        DisplayProgram(ICGraphics::Abstraction abstraction);
        virtual ~DisplayProgram();

        virtual unsigned char Initialize(const CreationInfo& info);
        virtual ICGraphics::Abstraction GetAbstraction();
        virtual void Destroy();
        virtual void ExecuteText(Component* component);
        virtual void ExecuteSpecial(Component* component) = 0;

        ICGraphics::Abstraction m_abstraction;  // 0x48
        ICGraphics2d* m_graphics;               // 0x4c set by Initialize
    };

    using ICGraphics2d::Draw;
    virtual void Draw(ICRenderText2d& text);

    unsigned char m_initialized;            // 0x04
    unsigned char m_softwareReady;          // 0x05
    unsigned char m_hardwareReady;          // 0x06
    unsigned char m_rendering;              // 0x07
    ICGraphics::Abstraction m_abstraction;  // 0x08 not set by the constructor
};

class CGraphics2d_Lite_OGLES : public CGraphics2dAbstract {
public:
    CGraphics2d_Lite_OGLES();
    virtual ~CGraphics2d_Lite_OGLES();
    virtual unsigned char Initialize();
    virtual void Destroy();
    virtual void RenderBegin(unsigned char writeAlpha);
    virtual void RenderEnd();
    virtual void DisplayListBegin(IDisplayProgram& program, IDisplayProgram::Mode mode);
    virtual void DisplayListEnd();
    virtual void Run(IDisplayProgram& program);
    virtual void SetOrigin(Origin origin);
    virtual void SetShaderMode(ShaderMode mode);
    virtual void SetShader(ICShader* shader);
    virtual void SetColor(int alpha, int red, int green, int blue);
    virtual void SetColor(const Color_ARGB_fixed& color);
    virtual void SetColor(unsigned int argb);
    virtual void Enable(State state);
    virtual void Disable(State state);
    virtual void PushState(State state);
    virtual void PopState(State state);
    virtual void SetBlendArg(BlendArg arg);
    virtual void LoadIdentity();
    virtual void LoadTransform(const CMatrix2d* matrix, const CVector2d* translation);
    virtual void MultiplyTransform(const CMatrix2d* matrix, const CVector2d* translation);
    virtual void Translate(int x, int y);
    virtual void Rotate(int angle);
    virtual void Scale(int x, int y);
    virtual void PushTransform();
    virtual void PopTransform();
    virtual void GetTransform(const CMatrix2d** matrix, const CVector2d** translation);
    virtual void SetVertexPositionCoords(int componentCount, unsigned int type, int stride,
                                 const void* coords);
    virtual void SetClip(unsigned short x, unsigned short y, unsigned short width, unsigned short height);
    virtual void SetClip(const CRectangle& clip);
    virtual const CRectangle& GetClip() const;
    virtual void Draw(ICRenderSurface& surface, Flip flip, const CRectangle* source);
    virtual void Draw(Primitive primitive, int first, int count);
    virtual void HandleConfigStateBasedOnSrcFormat(RSBFormat format);

    class DisplayProgram : public CGraphics2dAbstract::DisplayProgram {
    public:
        /** Component level (index into the per mode level tables). */
        enum LevelId {
            LevelId_Blend = 0,
            LevelId_Layer = 1,
            LevelId_Texture = 2,
            LevelId_Count = 8
        };

        /** Minimum number of entries of a component of `level` in `mode` (0 for the other modes). */
        static int GetComponentMinNumOfEntries(IDisplayProgram::Mode mode, LevelId level);

        DisplayProgram()
            : CGraphics2dAbstract::DisplayProgram(ICGraphics::Abstraction_Hardware), m_recordingMode(Mode_0)
        {
        }

        virtual void SetLayer(int layer);
        virtual int GetLayer();
        virtual void ExecuteSpecial(Component* component);

        Mode m_recordingMode;  // 0x50, retained while the program is replayed
    };

    typedef CGraphicsAbstract::DisplayProgram::Component Component;
    typedef CGraphicsAbstract::DisplayProgram::PriorityStacks PriorityStacks;

    void ConsiderAddingToDisplayList(unsigned int command, int count, ...);
    /** Advance the layer when the state stack tops changed since the last draw (modes 4 and 5). */
    inline void ConsiderDisplayProgramAutoAdvanceLayerAndUpdateInfo()
    {
        if (m_previousComponents[0].m_flag4 == 0) {
            for (int level = 0; level < m_priorityStacks.m_stackCount - 1; level++) {
                m_previousComponents[level] = m_priorityStacks.m_stacks[level].Top();
            }
        } else if (m_recording->m_currPos != IDisplayProgram::Mode_5 ||
                   m_priorityStacks.m_stacks[0].Top().m_data[0] != (int)0xc6fb25f8) {
            int level = 0;
            for (;;) {
                if (level >= m_priorityStacks.m_stackCount - 1) {
                    return;
                }
                if (!(m_previousComponents[level] == m_priorityStacks.m_stacks[level].Top())) {
                    break;
                }
                level++;
            }
            IDisplayProgram* program = m_recording;
            program->SetLayer(program->GetLayer() + 1);
            for (level = 0; level < m_priorityStacks.m_stackCount - 1; level++) {
                m_previousComponents[level] = m_priorityStacks.m_stacks[level].Top();
            }
        }
    }

    void UpdateActiveClipRect_HW();
    void DrawHardwareLines(int first, int count);
    void DrawHardwarePolygon(int first, int count);
    void DrawHardwareRectangles(int first, int count);

    // Layout from the constructor (0x728f0) and Initialize (0x72a64); size 0x198 (HAL InitializeHardware).
    unsigned char m_textureEnabled;             // 0x0c GL_TEXTURE_2D enabled by Draw
    GLenum m_textureEnvironment;                // 0x10 GL_TEXTURE_ENV_MODE last set
    Origin m_origin;                            // 0x14
    int m_field18;                              // 0x18 cleared by Initialize / RenderBegin
    AbstractState m_state;                      // 0x1c
    CRectangle m_clip;                          // 0x64
    CRectangle m_activeClip;                    // 0x6c target clipped by m_clip
    CIndexVertexBuffer m_vertexBuffer;          // 0x74 shorts for the primitive draws
    CIndexVertexBuffer m_quadBuffer;            // 0xb0 textured quads of Draw(surface)
    int m_coordCount;                           // 0xec SetVertexPositionCoords
    unsigned int m_coordType;                   // 0xf0
    int m_coordStride;                          // 0xf4
    const void* m_coords;                       // 0xf8
    TCStack<unsigned int> m_transformFlags;     // 0xfc TransformFlag bits per transform level
    unsigned char m_trackTransform;             // 0x108 keep m_transformStack (for recording)
    TCStack<Matrix> m_transformStack;           // 0x10c
    CMatrix2d m_returnMatrix;                   // 0x118 GetTransform (16.16 words)
    CVector2d m_returnTranslation;              // 0x130
    unsigned int m_primitiveIndex;              // 0x138 depth of recorded primitives
    GLuint m_boundTexture;                      // 0x13c last texture bound by Draw
    DisplayProgram* m_recording;                // 0x140
    const CVertexAttribute::Decl* m_texturedDecl;   // 0x144 VtxDecl_t of the recording mode
    int m_texturedStride;                       // 0x148 bytes per textured vertex
    int m_texturedShorts;                       // 0x14c shorts per textured vertex
    const CVertexAttribute::Decl* m_colorDecl;  // 0x150 VtxDecl_cf of the recording mode
    int m_colorStride;                          // 0x154
    int m_colorShorts;                          // 0x158
    unsigned char m_depthEnabled;               // 0x15c Run of a depth mode
    unsigned char m_depthWriting;               // 0x15d
    unsigned char m_textureBound;               // 0x15e ExecuteSpecial texture state
    GLuint m_lastTexture;                       // 0x160 ExecuteSpecial
    PriorityStacks m_priorityStacks;            // 0x164
    int* m_savedPoolPos;                        // 0x188 pool position of the recorded chains
    int m_lastMode;                             // 0x18c mode of the last DisplayListBegin
    Component* m_previousComponents;            // 0x190 state tops at the last draw (modes 4/5)
    int m_field194;                             // 0x194 never touched by this class (new allocates 0x198)
};

#endif
