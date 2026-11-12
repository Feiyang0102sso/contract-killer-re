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

class CGraphics2dAbstract : public ICGraphics2d {
public:
    CGraphics2dAbstract()
        : m_initialized(0)
        , m_softwareReady(0)
        , m_hardwareReady(0)
        , m_field7(0)
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
        /** 20 byte entry of the State_2 stack (content not recovered). */
        struct Entry20 {
            int m_values[5];
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

        ICGraphics::Abstraction m_abstraction;  // 0x48
        ICGraphics2d* m_graphics;               // 0x4c set by Initialize
    };

    using ICGraphics2d::Draw;
    virtual void Draw(ICRenderText2d& text);

    unsigned char m_initialized;            // 0x04
    unsigned char m_softwareReady;          // 0x05
    unsigned char m_hardwareReady;          // 0x06
    unsigned char m_field7;                 // 0x07
    ICGraphics::Abstraction m_abstraction;  // 0x08 not set by the constructor
};

class CGraphics2d_Lite_OGLES : public CGraphics2dAbstract {
public:
    CGraphics2d_Lite_OGLES();
    virtual ~CGraphics2d_Lite_OGLES();
    virtual unsigned char Initialize();
    virtual void Destroy();
    virtual void SetActiveAbstractionLayer(ICGraphics::Abstraction abstraction);
    virtual ICGraphics::Abstraction GetActiveAbstractionLayer();
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

    struct Vertex {
        GLfloat x, y;
        GLfloat u, v;
    };

    struct DrawCommand {
        int count;
        GLenum primitive;
        GLuint texture;
        Color_ARGB_fixed color;
        CRectangle clip;
        BlendArg blendArg;
        bool blend;
        bool alphaTest;
    };

    class DisplayProgram : public CGraphics2dAbstract::DisplayProgram {
    public:
        /** Component level (index into the per mode level tables). */
        enum LevelId {
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

private:
    typedef CGraphicsAbstract::DisplayProgram::Component Component;
    typedef CGraphicsAbstract::DisplayProgram::PriorityStacks PriorityStacks;

    bool IsRecording() const;
    Component& ComponentAtLevel(int level);
    void SetTextComponent(int level, unsigned int command, const int* arguments, int count);
    void ConsiderAddingToDisplayList(unsigned int command, int count, ...);
    void ConsiderDisplayProgramAutoAdvanceLayerAndUpdateInfo();
    void InitializeDisplayProgramStates();

    Vertex TransformVertex(float x, float y, float u, float v) const;
    void SetCommandState(DrawCommand& command, GLenum primitive, GLuint texture) const;
    void ApplyClip(const CRectangle& clip) const;
    void UpdateActiveClipRect_HW();
    void ApplyCommandState(const DrawCommand& command) const;
    void Execute(const DrawCommand& command, const Vertex* vertices) const;
    void Submit(DrawCommand& command);
    Vertex ReadVertex(int index) const;
    void DrawHardwareLines(int first, int count);
    void DrawHardwarePolygon(int first, int count);
    void DrawHardwareRectangles(int first, int count);
    void DrawUntexturedVertices(GLenum primitive, int first, int count);

    DisplayProgram* m_recording;
    PriorityStacks m_priorityStacks;
    Component m_previousComponents[DisplayProgram::LevelId_Count - 1];
    unsigned int m_primitiveIndex;
    bool m_depthEnabled;
    bool m_depthWriting;
    bool m_textureEnabled;
    GLuint m_boundTexture;
    GLenum m_textureEnvironment;
    Origin m_origin;
    Matrix m_transform;
    TCVector<Vertex> m_vertices;
    TCStack<Matrix> m_transformStack;
    TCVector<int> m_stateStack[5];
    TCVector<BlendArg> m_blendStack;
    TCVector<Color_ARGB_fixed> m_colorStack;
    TCVector<unsigned int> m_colorWordStack;
    bool m_states[5];
    BlendArg m_blendArg;
    Color_ARGB_fixed m_color;
    unsigned int m_colorWord;
    CMatrix2d m_returnMatrix;
    CVector2d m_returnTranslation;
    CRectangle m_clip;
    CRectangle m_target;
    CRectangle m_activeClip;
    const unsigned char* m_coords;
    unsigned int m_coordType;
    int m_coordStride;
    int m_coordCount;
    bool m_rendering;
};

#endif
