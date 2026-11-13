// CGraphics2d_Lite_OGLES.cpp - GLES1 drawing and display program playback.
// Rebuilt from the CK 1.5.2 functions (layout: constructor 0x728f0, Initialize 0x72a64).
// Immediate mode uses the GL matrix stack and 4.12 texture coordinates (texture matrix of
// RenderBegin); display list recording (modes 2..5) transforms the short vertices on the CPU.
#include "CGraphics2d_Lite_OGLES.h"

#include <OpenGLES/ES1/glext.h>
#include <stdarg.h>
#include "ARM_math.h"
#include "CBlitUtil.h"
#include "CGraphics2d_Lite_HAL.h"
#include "CGraphics_HAL.h"
#include "CGraphics_OGLES.h"
#include "CGraphics_OGLES_Cocoa.h"
#include "CMathFixed.h"
#include "CRenderSurface_OGLES_Texture.h"
#include "ICCore.h"

namespace {
    const int kFixedOne = 65536;
    const unsigned int kByteVertex = 0x2f978;
    const unsigned int kUnsignedByteVertex = 0x23f978;
    const unsigned int kFixedVertex = 0x30ee34;
    const unsigned int kShortVertex = 0x3f9726;
    const unsigned int kUnsignedShortVertex = 0x13f9726;
    const unsigned int kIntVertex = 0x3f9702;
    const unsigned int kUnsignedIntVertex = 0x13f9702;

    // display list command keys (CGraphics2dAbstract::DisplayProgram::ExecuteText)
    const unsigned int kEnable = 0xdc7991b1;
    const unsigned int kDisable = 0xc6fb25f8;
    const unsigned int kPushState = 0xbb855bf6;
    const unsigned int kPopState = 0xa9b130f8;
    const unsigned int kSetBlend = 0x9b25d779;
    const unsigned int kSetColor = 0x93e08c5f;
    const unsigned int kSetClip = 0xd13e0bbd;
    const unsigned int kEndCommands = 0xbca5ef3b;
    const unsigned int kLoadIdentity = 0xe0e5c6ac;
    const unsigned int kLoadTransform = 0xfa5b810f;
    const unsigned int kMultiplyTransform = 0xe2e99e40;
    const unsigned int kTranslate = 0xd9c4dbf2;
    const unsigned int kRotate = 0xdd09c331;
    const unsigned int kScale = 0xbdd149f4;
    const unsigned int kPushTransform = 0xfa4633cf;
    const unsigned int kPopTransform = 0xd54821fb;

    // vertex buffer keys of AquireVtxBuffer
    const unsigned int kPrimitiveBufferKey = 0xc33bd4e2;
    const unsigned int kQuadBufferKey = 0xbc33bd5f;

    // m_transformFlags bits
    const unsigned int kTransformTranslate = 1;
    const unsigned int kTransformScale = 2;
    const unsigned int kTransformRotate = 4;

    GLenum GLVertexType(unsigned int type)
    {
        switch (type) {
        case kByteVertex:
            return GL_BYTE;
        case kUnsignedByteVertex:
            return GL_UNSIGNED_BYTE;
        case kShortVertex:
            return GL_SHORT;
        case kUnsignedShortVertex:
            return GL_UNSIGNED_SHORT;
        case kFixedVertex:
            return GL_FIXED;
        }
        return 0;
    }

    /** Bytes of one component of a vertex type key (-1 for unknown types). */
    inline int SizeOfType(unsigned int type)
    {
        switch (type) {
        case kByteVertex:
        case kUnsignedByteVertex:
            return 1;
        case kShortVertex:
        case kUnsignedShortVertex:
            return 2;
        case kIntVertex:
        case kUnsignedIntVertex:
            return 4;
        }
        return -1;
    }

    /** 16.16 word `index` of a CK 2D math object (the 2D interface passes fixed point words). */
    inline int FixedWord(const void* object, int index)
    {
        return reinterpret_cast<const int*>(object)[index];
    }

    inline void SetFixedWord(void* object, int index, int value)
    {
        reinterpret_cast<int*>(object)[index] = value;
    }

    /** ceil(log2(extent)) of a texture tile extent (up to 1024). */
    inline int TextureShift(int extent)
    {
        int shift = 0;
        while ((1 << shift) < extent) {
            shift++;
        }
        return shift;
    }
}

// level -> table slot, and minimum component entries per slot (CK 1.5.2 data)
static const CVertexAttribute::Decl CGraphics2d_Lite_OGLES_DisplayProgram_Mode_BestApproximationToPaintersAlgorithm_VtxDecl_t[] = {
    { 0, 0, kShortVertex, 3, 12, 1, 0 },
    { 1, 8, kShortVertex, 2, 12, 1, 0 },
    { 0, 0, 0, 0, 0, 0, 0 }
};
static const CVertexAttribute::Decl CGraphics2d_Lite_OGLES_DisplayProgram_Mode_BestApproximationToPaintersAlgorithm_VtxDecl_cf[] = {
    { 0, 0, kShortVertex, 3, 8, 1, 0 },
    { 0, 0, 0, 0, 0, 0, 0 }
};
static const CVertexAttribute::Decl CGraphics2d_Lite_OGLES_DisplayProgram_Mode_FastAndCompact_VtxDecl_t[] = {
    { 0, 0, kShortVertex, 2, 8, 1, 0 },
    { 1, 4, kShortVertex, 2, 8, 1, 0 },
    { 0, 0, 0, 0, 0, 0, 0 }
};
static const CVertexAttribute::Decl CGraphics2d_Lite_OGLES_DisplayProgram_Mode_FastAndCompact_VtxDecl_cf[] = {
    { 0, 0, kShortVertex, 2, 4, 1, 0 },
    { 0, 0, 0, 0, 0, 0, 0 }
};
static const CVertexAttribute::Decl CGraphics2d_Lite_OGLES_DisplayProgram_Mode_PaintersAlgorithmAutoAdvanceLayers_VtxDecl_t[] = {
    { 0, 0, kShortVertex, 2, 8, 1, 0 },
    { 1, 4, kShortVertex, 2, 8, 1, 0 },
    { 0, 0, 0, 0, 0, 0, 0 }
};
static const CVertexAttribute::Decl CGraphics2d_Lite_OGLES_DisplayProgram_Mode_PaintersAlgorithmAutoAdvanceLayers_VtxDecl_cf[] = {
    { 0, 0, kShortVertex, 2, 4, 1, 0 },
    { 0, 0, 0, 0, 0, 0, 0 }
};
static const CVertexAttribute::Decl CGraphics2d_Lite_OGLES_DisplayProgram_Mode_PaintersAlgorithmAutoAdvanceLayersLikelyMostOptimal_VtxDecl_t[] = {
    { 0, 0, kShortVertex, 3, 12, 1, 0 },
    { 1, 8, kShortVertex, 2, 12, 1, 0 },
    { 0, 0, 0, 0, 0, 0, 0 }
};
static const CVertexAttribute::Decl CGraphics2d_Lite_OGLES_DisplayProgram_Mode_PaintersAlgorithmAutoAdvanceLayersLikelyMostOptimal_VtxDecl_cf[] = {
    { 0, 0, kShortVertex, 3, 8, 1, 0 },
    { 0, 0, 0, 0, 0, 0, 0 }
};
static const int CGraphics2d_Lite_OGLES_DisplayProgram_Mode_PaintersAlgorithmAutoAdvanceLayers[8] = {
    1, 0, 2, 3, 4, 5, 6, 7
};
static const int CGraphics2d_Lite_OGLES_DisplayProgram_Mode_PaintersAlgorithmAutoAdvanceLayersLikelyMostOptimal[8] = {
    0, 1, 2, 3, 4, 5, 6, 7
};
static const int CGraphics2d_Lite_OGLES_DisplayProgram_Mode_BestApproximationToPaintersAlgorithm_MinCmpLen[8] = {
    20, 12, 12, 32, 20, 20, 32, 16
};
static const int CGraphics2d_Lite_OGLES_DisplayProgram_Mode_FastAndCompact_MinCmpLen[8] = {
    12, 20, 12, 32, 20, 20, 32, 16
};
static const int CGraphics2d_Lite_OGLES_DisplayProgram_Mode_PaintersAlgorithmAutoAdvanceLayers_MinCmpLen[8] = {
    12, 20, 12, 32, 20, 20, 32, 16
};
static const int CGraphics2d_Lite_OGLES_DisplayProgram_Mode_PaintersAlgorithmAutoAdvanceLayersLikelyMostOptimal_MinCmpLen[8] = {
    20, 12, 12, 32, 20, 20, 32, 16
};
static const int CGraphics2d_Lite_OGLES_DisplayProgram_Mode_PaintersAlgorithmAutoAdvanceLayers_CmpMinNumOfEntries[8] = {
    1, 3, 1, 6, 3, 3, 6, 2
};
static const int CGraphics2d_Lite_OGLES_DisplayProgram_Mode_PaintersAlgorithmAutoAdvanceLayersLikelyMostOptimal_CmpMinNumOfEntries[8] = {
    3, 1, 1, 6, 3, 3, 6, 2
};

/** ICGraphics::GetInstance with the found pointer tested itself (cbnz, as in the original callers). */
static inline ICGraphics* GraphicsInstance()
{
    CApp* app = CApplet::m_pApp;
    ICGraphics* instance = NULL;
    if (app != NULL) {
        instance = app->m_graphics;
        if (instance == NULL) {
            void* found = NULL;
            app->m_singletons->Find(ICGraphics::ClassId, &found);
            if (found == NULL) {
                instance = ICGraphics::CreateInstance();
            } else {
                instance = (ICGraphics*)found;
            }
            app->m_graphics = instance;
        }
    }
    return instance;
}

/** The OpenGL ES device of the HAL graphics instance. */
static inline CGraphics_OGLES_Cocoa* GraphicsInstanceHardware()
{
    CGraphics_HAL* graphics = static_cast<CGraphics_HAL*>(GraphicsInstance());
    return graphics->m_hardware;
}

/** Priority stack of `level` in `mode` (-1 outside the priority modes). */
static inline int LevelIndex(ICGraphics2d::IDisplayProgram::Mode mode, int level)
{
    int index = -1;
    switch (mode) {
    case ICGraphics2d::IDisplayProgram::Mode_2:
        index = CGraphics2d_Lite_OGLES_DisplayProgram_Mode_PaintersAlgorithmAutoAdvanceLayersLikelyMostOptimal[level];
        break;
    case ICGraphics2d::IDisplayProgram::Mode_3:
        index = CGraphics2d_Lite_OGLES_DisplayProgram_Mode_PaintersAlgorithmAutoAdvanceLayers[level];
        break;
    case ICGraphics2d::IDisplayProgram::Mode_4:
        index = CGraphics2d_Lite_OGLES_DisplayProgram_Mode_PaintersAlgorithmAutoAdvanceLayers[level];
        break;
    case ICGraphics2d::IDisplayProgram::Mode_5:
        index = CGraphics2d_Lite_OGLES_DisplayProgram_Mode_PaintersAlgorithmAutoAdvanceLayersLikelyMostOptimal[level];
        break;
    default:
        break;
    }
    return index;
}

/** Component sizes (bytes) of the levels of `mode`. */
static inline const int* MinCmpLen(ICGraphics2d::IDisplayProgram::Mode mode)
{
    const int* lengths = NULL;
    switch (mode) {
    case ICGraphics2d::IDisplayProgram::Mode_2:
        lengths = CGraphics2d_Lite_OGLES_DisplayProgram_Mode_BestApproximationToPaintersAlgorithm_MinCmpLen;
        break;
    case ICGraphics2d::IDisplayProgram::Mode_3:
        lengths = CGraphics2d_Lite_OGLES_DisplayProgram_Mode_FastAndCompact_MinCmpLen;
        break;
    case ICGraphics2d::IDisplayProgram::Mode_4:
        lengths = CGraphics2d_Lite_OGLES_DisplayProgram_Mode_PaintersAlgorithmAutoAdvanceLayers_MinCmpLen;
        break;
    case ICGraphics2d::IDisplayProgram::Mode_5:
        lengths = CGraphics2d_Lite_OGLES_DisplayProgram_Mode_PaintersAlgorithmAutoAdvanceLayersLikelyMostOptimal_MinCmpLen;
        break;
    default:
        break;
    }
    return lengths;
}

/** GL state calls are made only outside display list recording. */
#define CK_GL_IMMEDIATE() (m_recording == NULL || m_recording->m_currPos == IDisplayProgram::Mode_0)

/** Make room for `words` pool entries in a plain (mode 1) display list. */
#define CK_DL_RESERVE(words)                                                                        \
    if (m_recording->m_poolCursor->m_entries + m_recording->m_poolCursor->m_count <                \
        m_recording->m_poolPos + (words)) {                                                         \
        m_recording->AdjustCurrPos(words);                                                          \
    }

/** Append one word at the display list write position. */
#define CK_DL_WRITE(value)                                                                          \
    *m_recording->m_poolPos = (value);                                                              \
    m_recording->m_poolPos++;

int CGraphics2d_Lite_OGLES::DisplayProgram::GetComponentMinNumOfEntries(IDisplayProgram::Mode mode, LevelId level)
{
    switch (mode) {
    case IDisplayProgram::Mode_2:
        return CGraphics2d_Lite_OGLES_DisplayProgram_Mode_PaintersAlgorithmAutoAdvanceLayersLikelyMostOptimal_CmpMinNumOfEntries
            [CGraphics2d_Lite_OGLES_DisplayProgram_Mode_PaintersAlgorithmAutoAdvanceLayersLikelyMostOptimal[level]];
    case IDisplayProgram::Mode_3:
        return CGraphics2d_Lite_OGLES_DisplayProgram_Mode_PaintersAlgorithmAutoAdvanceLayers_CmpMinNumOfEntries
            [CGraphics2d_Lite_OGLES_DisplayProgram_Mode_PaintersAlgorithmAutoAdvanceLayers[level]];
    case IDisplayProgram::Mode_4:
        return CGraphics2d_Lite_OGLES_DisplayProgram_Mode_PaintersAlgorithmAutoAdvanceLayers_CmpMinNumOfEntries
            [CGraphics2d_Lite_OGLES_DisplayProgram_Mode_PaintersAlgorithmAutoAdvanceLayers[level]];
    case IDisplayProgram::Mode_5:
        return CGraphics2d_Lite_OGLES_DisplayProgram_Mode_PaintersAlgorithmAutoAdvanceLayersLikelyMostOptimal_CmpMinNumOfEntries
            [CGraphics2d_Lite_OGLES_DisplayProgram_Mode_PaintersAlgorithmAutoAdvanceLayersLikelyMostOptimal[level]];
    default:
        return 0;
    }
}

void CGraphics2d_Lite_OGLES::DisplayProgram::SetLayer(int layer)
{
    m_layer = layer;
    CGraphics2d_Lite_OGLES* device =
        static_cast<CGraphics2d_Lite_OGLES*>(static_cast<CGraphics2d_Lite_HAL*>(m_graphics)->m_device);
    if (device->m_recording != this) {
        return;
    }
    TCStack<Component>& stack = device->m_priorityStacks.m_stacks[LevelIndex(m_recordingMode, LevelId_Layer)];
    Component& component = stack.m_data[stack.m_top - 1];
    int entries = GetComponentMinNumOfEntries(m_recordingMode, LevelId_Layer);
    component.m_flag4 = 1;
    component.m_flag6 = entries * 4 + 8;
    component.m_flag5 = 0x10;
    component.m_data[0] = m_layer;
}

int CGraphics2d_Lite_OGLES::DisplayProgram::GetLayer()
{
    return m_layer;
}

void CGraphics2d_Lite_OGLES::DisplayProgram::ExecuteSpecial(Component* component)
{
    CGraphics2d_Lite_OGLES* device =
        static_cast<CGraphics2d_Lite_OGLES*>(static_cast<CGraphics2d_Lite_HAL*>(m_graphics)->m_device);
    unsigned int kind = component->m_flag5 >> 4;
    if (kind - 3 > 1) {
        if (kind != 2) {
            return;
        }
        // texture component
        if (component->m_data[0] != 0) {
            if (!device->m_textureBound) {
                device->m_textureBound = 1;
                glEnable(GL_TEXTURE_2D);
                glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            }
            if (device->m_lastTexture == (GLuint)component->m_data[0]) {
                return;
            }
            device->m_lastTexture = component->m_data[0];
            glBindTexture(GL_TEXTURE_2D, component->m_data[0]);
            return;
        }
        if (!device->m_textureBound) {
            return;
        }
        device->m_textureBound = 0;
        glDisable(GL_TEXTURE_2D);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        return;
    }

    // vertex buffer component (3: lines, 4: triangles)
    GLenum environment;
    if (device->m_state.m_blend.Top()) {
        environment = GL_REPLACE;
        if (device->m_state.m_blendArg.Top() < BlendArg_2) {
            environment = GL_MODULATE;
        }
        if (device->m_depthEnabled && device->m_depthWriting) {
            glDepthMask(GL_FALSE);
            device->m_depthWriting = 0;
        }
    } else {
        environment = GL_REPLACE;
        if (device->m_depthEnabled && !device->m_depthWriting) {
            glDepthMask(GL_TRUE);
            device->m_depthWriting = 1;
        }
    }
    if (device->m_textureEnvironment != environment) {
        device->m_textureEnvironment = environment;
        glTexEnvx(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, environment);
    }
    // textures modulated by the alpha only
    bool restoreColor = false;
    if (environment != GL_REPLACE && (component->m_flag5 & 8) != 0) {
        const Color_ARGB_fixed& color = device->m_state.m_state2.Top().m_color;
        if (color.r != kFixedOne || color.g != kFixedOne || color.b != kFixedOne) {
            restoreColor = true;
            glColor4x(kFixedOne, kFixedOne, kFixedOne, color.a);
        }
    }

    CVertexBuffer* buffer = reinterpret_cast<CVertexBuffer*>(component->m_data[0]);
    const CVertexAttribute::Decl* attributes = reinterpret_cast<const CVertexAttribute::Decl*>(buffer->m_attributes);
    glVertexPointer((signed char)attributes[0].m_fieldC, GLVertexType(attributes[0].m_type), buffer->m_pool.m_reserve,
                    buffer->m_pool.m_data + attributes[0].m_field4);
    if ((component->m_flag5 & 2) != 0) {
        attributes = reinterpret_cast<const CVertexAttribute::Decl*>(buffer->m_attributes);
        glTexCoordPointer((signed char)attributes[1].m_fieldC, GLVertexType(attributes[1].m_type),
                          buffer->m_pool.m_reserve, buffer->m_pool.m_data + attributes[1].m_field4);
    }
    GLenum primitive = GL_LINES;
    if ((component->m_flag5 >> 4) != 3) {
        primitive = 0;
        if ((component->m_flag5 >> 4) == 4) {
            primitive = GL_TRIANGLES;
        }
    }
    if (buffer->m_field4 != CVertexBuffer::Type_IndexVertex) {
        glDrawArrays(primitive, 0, (buffer->m_pool.m_cursor - buffer->m_pool.m_data) / buffer->m_pool.m_reserve);
    } else {
        CIndexVertexBuffer* indexed = static_cast<CIndexVertexBuffer*>(buffer);
        int count = (indexed->m_indices.m_cursor - indexed->m_indices.m_data) / indexed->m_indices.m_reserve;
        glDrawElements(primitive, count, GLVertexType(indexed->m_indexType), indexed->m_indices.m_data);
    }
    if (restoreColor) {
        const Color_ARGB_fixed& color = device->m_state.m_state2.Top().m_color;
        glColor4x(color.r, color.g, color.b, color.a);
    }
}

typedef char DisplayProgramSize[sizeof(CGraphics2d_Lite_OGLES::DisplayProgram) == 0x54 ? 1 : -1];
typedef char GraphicsSize[sizeof(CGraphics2d_Lite_OGLES) == 0x198 ? 1 : -1];

/** Vertex attribute as written by this renderer (the 16 byte CVertexAttribute::Decl). */
struct VertexAttribute {
    int m_index;            // 0x00
    int m_offset;           // 0x04 bytes into the vertex
    unsigned int m_type;    // 0x08 component type key, 0 ends a list
    signed char m_count;    // 0x0c components
    signed char m_stride;   // 0x0d bytes per vertex
    short m_normalized;     // 0x0e
};

/** Attribute `index` of `buffer`. */
#define CK_ATTRIBUTE(buffer, index) (reinterpret_cast<VertexAttribute*>((buffer).m_attributes)[index])

/** Padding unit of a component type (size modulo 4, -1 for unknown types). */
static inline int PaddingOfType(unsigned int type)
{
    switch (type) {
    case kByteVertex:
    case kUnsignedByteVertex:
        return 1;
    case kShortVertex:
    case kUnsignedShortVertex:
        return 2;
    case kIntVertex:
    case kUnsignedIntVertex:
        return 0;
    }
    return -1;
}

/** Stride of the attribute list of `buffer`: the last attribute plus its components, 4 byte aligned. */
static inline void UpdateVertexStride(CVertexBuffer& buffer)
{
    int last = 0;
    for (int index = 0; index < buffer.m_attributeCount; index++) {
        if (buffer.m_pool.m_reserve < CK_ATTRIBUTE(buffer, index).m_offset) {
            buffer.m_pool.m_reserve = CK_ATTRIBUTE(buffer, index).m_offset;
            last = index;
        }
    }
    const VertexAttribute& attribute = CK_ATTRIBUTE(buffer, last);
    int offset = attribute.m_offset;
    unsigned int type = attribute.m_type;
    int components = attribute.m_count;
    buffer.m_pool.m_reserve = SizeOfType(type) * components + offset + ((PaddingOfType(type) * -components) & 3);
    for (int index = 0; index < buffer.m_attributeCount; index++) {
        CK_ATTRIBUTE(buffer, index).m_stride = buffer.m_pool.m_reserve;
    }
}

CGraphics2d_Lite_OGLES::CGraphics2d_Lite_OGLES()
    : m_priorityStacks()
{
    m_abstraction = ICGraphics::Abstraction_Hardware;
    m_field18 = 0;
    m_clip.m_width = 0;
    m_clip.m_height = 0;
    m_clip.m_x = 0;
    m_clip.m_y = 0;
    m_textureEnabled = 0;
    m_textureEnvironment = GL_REPLACE;
    m_trackTransform = 1;
    m_savedPoolPos = NULL;
    m_recording = NULL;
    m_texturedDecl = NULL;
    m_colorDecl = NULL;
    m_lastTexture = 0;
    m_textureBound = 0;
    m_depthEnabled = 0;
    m_depthWriting = 0;
    m_lastMode = 0;
    m_previousComponents = NULL;
    m_initialized = 1;
    m_hardwareReady = 1;
}

CGraphics2d_Lite_OGLES::~CGraphics2d_Lite_OGLES()
{
    m_priorityStacks.Destroy();
    m_priorityStacks.m_conditionals.Destroy();
}

unsigned char CGraphics2d_Lite_OGLES::Initialize()
{
    m_field18 = 0;
    m_state.Initialize(5);
    SetActiveAbstractionLayer(ICGraphics::Abstraction_Hardware);
    SetOrigin(Origin_TopLeft);
    Disable(State_ColorKey);
    Disable(State_AlphaTest);
    Disable(State_Blend);
    Disable(State_4);
    SetBlendArg(BlendArg_2);

    unsigned int width;
    unsigned int height;
    GraphicsInstance()->GetTargetSurface()->GetWidthAndHeight(width, height);
    SetClip(0, 0, width, height);

    // shorts of the primitive draws
    const ICGraphics::Description* description = GraphicsInstance()->GetDescription(ICGraphics::Abstraction_Hardware);
    m_vertexBuffer.Initialize(description->m_maxShaderParameters * 2, description->m_maxVertexAttributes * 4, 1);
    m_vertexBuffer.m_indices.m_reserve = 2;
    m_vertexBuffer.m_indexType = kUnsignedShortVertex;
    CK_ATTRIBUTE(m_vertexBuffer, 0).m_index = 0;
    CK_ATTRIBUTE(m_vertexBuffer, 0).m_offset = 0;
    CK_ATTRIBUTE(m_vertexBuffer, 0).m_type = kShortVertex;
    CK_ATTRIBUTE(m_vertexBuffer, 0).m_count = 2;
    CK_ATTRIBUTE(m_vertexBuffer, 0).m_stride = 0;
    CK_ATTRIBUTE(m_vertexBuffer, 0).m_normalized = 1;
    UpdateVertexStride(m_vertexBuffer);

    // textured quads of Draw(surface): position and 4.12 texture coordinate shorts
    m_quadBuffer.Initialize(0x18, 0x40, 2);
    m_quadBuffer.m_indices.m_reserve = 2;
    m_quadBuffer.m_indexType = kUnsignedShortVertex;
    CK_ATTRIBUTE(m_quadBuffer, 0).m_index = 0;
    CK_ATTRIBUTE(m_quadBuffer, 0).m_offset = 0;
    CK_ATTRIBUTE(m_quadBuffer, 0).m_type = kShortVertex;
    CK_ATTRIBUTE(m_quadBuffer, 0).m_count = 2;
    CK_ATTRIBUTE(m_quadBuffer, 0).m_stride = 0;
    CK_ATTRIBUTE(m_quadBuffer, 0).m_normalized = 1;
    CK_ATTRIBUTE(m_quadBuffer, 1).m_index = 0;
    CK_ATTRIBUTE(m_quadBuffer, 1).m_offset = 4;
    CK_ATTRIBUTE(m_quadBuffer, 1).m_type = kShortVertex;
    CK_ATTRIBUTE(m_quadBuffer, 1).m_count = 2;
    CK_ATTRIBUTE(m_quadBuffer, 1).m_stride = 0;
    CK_ATTRIBUTE(m_quadBuffer, 1).m_normalized = 1;
    UpdateVertexStride(m_quadBuffer);

    m_transformFlags.Initialize(32);
    m_transformStack.Initialize(32);
    m_transformStack.Top() = Matrix();

    m_trackTransform = 1;
    m_savedPoolPos = NULL;
    m_recording = NULL;
    m_texturedDecl = NULL;
    m_colorDecl = NULL;
    m_lastTexture = 0;
    m_textureBound = 0;
    m_depthEnabled = 0;
    m_depthWriting = 0;
    m_lastMode = 0;
    m_previousComponents = NULL;
    m_initialized = 1;
    m_hardwareReady = 1;
    return 1;
}

void CGraphics2d_Lite_OGLES::Destroy()
{
    m_lastMode = 0;
    if (m_previousComponents != NULL) {
        delete[] m_previousComponents;
        m_previousComponents = NULL;
    }
    m_savedPoolPos = NULL;
    m_recording = NULL;
    m_texturedDecl = NULL;
    m_colorDecl = NULL;
    m_lastTexture = 0;
    m_textureBound = 0;
    m_depthEnabled = 0;
    m_depthWriting = 0;
    m_transformStack.Destroy();
    m_transformFlags.Destroy();
    m_vertexBuffer.Destroy();
    m_quadBuffer.Destroy();
    m_state.Destroy();
    m_hardwareReady = 0;
    m_initialized = 0;
}

void CGraphics2d_Lite_OGLES::RenderBegin(unsigned char writeAlpha)
{
    glDisable(GL_DITHER);
    glDisable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glShadeModel(GL_FLAT);
    glAlphaFuncx(GL_GREATER, 0);
    glDepthRangex(0, kFixedOne);
    ICRenderSurface* target = GraphicsInstance()->GetTargetSurface();
    if (target != NULL) {
        glDisable(GL_SCISSOR_TEST);
        unsigned int width;
        unsigned int height;
        target->GetWidthAndHeight(width, height);
        CRectangle bounds(0, 0, width, height);
        m_clip = bounds;
        m_activeClip = bounds;
        GraphicsInstanceHardware()->SetWidthAndHeightMappedViewportAndOrthoProjection(*static_cast<CRenderSurface*>(target));
        m_field18 = 0;
    }
    glActiveTexture(GL_TEXTURE0);
    m_textureEnvironment = GL_REPLACE;
    glTexEnvx(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    // the texture coordinates are 4.12 fixed point
    glMatrixMode(GL_TEXTURE);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_TEXTURE_2D);
    glLoadIdentity();
    glScalex(0x10, 0x10, CMathFixed::One);
    glMatrixMode(GL_MODELVIEW);
    LoadIdentity();
    if (m_state.m_alphaTest.Top()) {
        glEnable(GL_ALPHA_TEST);
    } else {
        glDisable(GL_ALPHA_TEST);
    }
    if (m_state.m_blend.Top()) {
        glEnable(GL_BLEND);
    } else {
        glDisable(GL_BLEND);
    }
    GLenum source = GL_SRC_ALPHA;
    GLenum destination = GL_ONE_MINUS_SRC_ALPHA;
    switch (m_state.m_blendArg.Top()) {
    case BlendArg_0:
    case BlendArg_2:
        break;
    case BlendArg_1:
    case BlendArg_3:
        destination = GL_ONE;
        break;
    case BlendArg_4:
        source = GL_ONE;
        destination = GL_ONE;
        break;
    default:
        source = GL_ZERO;
        destination = GL_ZERO;
        break;
    }
    glBlendFunc(source, destination);
    glDisable(GL_NORMALIZE);
    glDisable(GL_DITHER);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDepthMask(GL_FALSE);
    glDisable(GL_FOG);
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_LIGHTING);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    ICRenderSurface::Description surfaceDescription;
    if (target != NULL && target->GetDescription(surfaceDescription)) {
        if (surfaceDescription.m_multisample == 0) {
            glDisable(GL_MULTISAMPLE);
        } else {
            glEnable(GL_MULTISAMPLE);
        }
    }
    const ICGraphics::Description* description = GraphicsInstance()->GetDescription(ICGraphics::Abstraction_Hardware);
    for (unsigned int unit = 1; unit < description->m_maxTextureUnits; unit++) {
        glActiveTexture(GL_TEXTURE0 + unit);
        glDisable(GL_TEXTURE_2D);
        glClientActiveTexture(GL_TEXTURE0 + unit);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }
    glDisableClientState(GL_NORMAL_ARRAY);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    const Color_ARGB_fixed& color = m_state.m_state2.Top().m_color;
    glColor4x(color.r, color.g, color.b, color.a);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, writeAlpha != 0);
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_TEXTURE_2D);
    m_textureEnabled = 0;
    glClientActiveTexture(GL_TEXTURE0);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);
    m_primitiveIndex = 0;
    m_boundTexture = 0;
    m_rendering = 1;
}

void CGraphics2d_Lite_OGLES::RenderEnd()
{
    m_rendering = 0;
}

void CGraphics2d_Lite_OGLES::DisplayListBegin(IDisplayProgram& program, IDisplayProgram::Mode mode)
{
    DisplayProgram& display = static_cast<DisplayProgram&>(program);
    display.ClearCore();
    display.m_currPos = mode;
    display.m_recordingMode = mode;
    bool priority;
    if (mode != IDisplayProgram::Mode_0) {
        const CVertexAttribute::Decl* colored;
        switch (display.m_recordingMode) {
        case IDisplayProgram::Mode_2:
            m_texturedStride = 12;
            m_texturedShorts = 6;
            m_texturedDecl = CGraphics2d_Lite_OGLES_DisplayProgram_Mode_BestApproximationToPaintersAlgorithm_VtxDecl_t;
            m_colorStride = 8;
            m_colorShorts = 4;
            colored = CGraphics2d_Lite_OGLES_DisplayProgram_Mode_BestApproximationToPaintersAlgorithm_VtxDecl_cf;
            break;
        case IDisplayProgram::Mode_3:
            m_texturedStride = 8;
            m_texturedShorts = 4;
            m_texturedDecl = CGraphics2d_Lite_OGLES_DisplayProgram_Mode_FastAndCompact_VtxDecl_t;
            m_colorStride = 4;
            m_colorShorts = 2;
            colored = CGraphics2d_Lite_OGLES_DisplayProgram_Mode_FastAndCompact_VtxDecl_cf;
            break;
        case IDisplayProgram::Mode_4:
            m_texturedStride = 8;
            m_texturedShorts = 4;
            m_texturedDecl = CGraphics2d_Lite_OGLES_DisplayProgram_Mode_PaintersAlgorithmAutoAdvanceLayers_VtxDecl_t;
            m_colorStride = 4;
            m_colorShorts = 2;
            colored = CGraphics2d_Lite_OGLES_DisplayProgram_Mode_PaintersAlgorithmAutoAdvanceLayers_VtxDecl_cf;
            break;
        case IDisplayProgram::Mode_5:
            m_texturedStride = 12;
            m_texturedShorts = 6;
            m_texturedDecl = CGraphics2d_Lite_OGLES_DisplayProgram_Mode_PaintersAlgorithmAutoAdvanceLayersLikelyMostOptimal_VtxDecl_t;
            m_colorStride = 8;
            m_colorShorts = 4;
            colored = CGraphics2d_Lite_OGLES_DisplayProgram_Mode_PaintersAlgorithmAutoAdvanceLayersLikelyMostOptimal_VtxDecl_cf;
            break;
        default:
            colored = NULL;
            m_texturedDecl = NULL;
            break;
        }
        m_colorDecl = colored;

        // the state stacks and the conditionals of the blend and layer levels
        int levelCount = 0;
        if (LevelIndex(display.m_recordingMode, 0) >= 0) {
            levelCount = DisplayProgram::LevelId_Count;
        }
        m_priorityStacks.Initialize(levelCount, 32, 50);
        PriorityStacks::ConditionalMgr::Condition condition;
        PriorityStacks::ConditionalMgr::Result result;
        condition.m_op = 1;
        condition.m_wordIndex = 0;
        condition.m_value = kDisable;
        m_priorityStacks.m_conditionals.m_values[LevelIndex(display.m_recordingMode, DisplayProgram::LevelId_Blend)] = condition;
        result.m_true = 0;
        result.m_false = 1;
        m_priorityStacks.m_conditionals.m_results[LevelIndex(display.m_recordingMode, DisplayProgram::LevelId_Blend)] = result;
        condition.m_op = 3;
        condition.m_wordIndex = 0;
        condition.m_value = -1;
        m_priorityStacks.m_conditionals.m_values[LevelIndex(display.m_recordingMode, DisplayProgram::LevelId_Layer)] = condition;
        result.m_true = 0;
        result.m_false = 1;
        m_priorityStacks.m_conditionals.m_results[LevelIndex(display.m_recordingMode, DisplayProgram::LevelId_Layer)] = result;
        result.m_true = 1;
        result.m_false = 1;
        m_priorityStacks.m_conditionals.m_results[LevelIndex(display.m_recordingMode, 7)] = result;
        priority = true;
    } else {
        priority = false;
        m_texturedDecl = NULL;
        m_colorDecl = NULL;
    }

    m_recording = &display;
    m_savedPoolPos = display.m_poolPos;
    display.m_field22 = 1;
    const int* lengths = MinCmpLen(display.m_recordingMode);
    if (priority) {
        // previous state tops for the automatic layers (modes 4 and 5)
        bool autoAdvance = mode == IDisplayProgram::Mode_4 || mode == IDisplayProgram::Mode_5;
        if (m_previousComponents == NULL || autoAdvance) {
            if (m_lastMode != mode && (autoAdvance || m_previousComponents != NULL)) {
                if (m_previousComponents != NULL) {
                    delete[] m_previousComponents;
                }
                m_previousComponents = new Component[m_priorityStacks.m_stackCount - 1];
            }
            if (m_previousComponents != NULL) {
                m_previousComponents[0].m_flag4 = 0;
            }
        } else {
            delete[] m_previousComponents;
            m_previousComponents = NULL;
        }
        for (int level = 0; level < m_priorityStacks.m_stackCount; level++) {
            Component& top = m_priorityStacks.m_stacks[level].Top();
            top.m_flag4 = 0;
            top.m_flag6 = lengths[level];
            top.m_next = NULL;
        }
        TCStack<Component>& stack = m_priorityStacks.m_stacks[LevelIndex(display.m_recordingMode, DisplayProgram::LevelId_Layer)];
        Component& layer = stack.m_data[stack.m_top - 1];
        layer.m_flag4 = 1;
        layer.m_flag6 = DisplayProgram::GetComponentMinNumOfEntries(display.m_recordingMode, DisplayProgram::LevelId_Layer) * 4 + 8;
        layer.m_flag5 = 0x10;
        layer.m_data[0] = display.m_layer;
    }
    m_lastMode = mode;
    Disable(State_Blend);
}

void CGraphics2d_Lite_OGLES::DisplayListEnd()
{
    m_savedPoolPos = NULL;
    if (m_recording != NULL) {
        m_recording->m_field22 = 0;
        m_recording = NULL;
    }
}

void CGraphics2d_Lite_OGLES::Run(IDisplayProgram& program)
{
    DisplayProgram& display = static_cast<DisplayProgram&>(program);
    m_recording = &display;
    int previousMode = display.m_currPos;
    display.m_currPos = IDisplayProgram::Mode_1;

    // the stack bottoms: the current state
    Component* component = &m_priorityStacks.m_stacks[LevelIndex(display.m_recordingMode, 0)].Top();
    int entries = DisplayProgram::GetComponentMinNumOfEntries(display.m_recordingMode, (DisplayProgram::LevelId)0);
    component->m_flag4 = 2;
    component->m_flag6 = entries * 4 + 8;
    if (m_state.m_blend.Top()) {
        component->m_data[0] = kEnable;
    } else {
        component->m_data[0] = kDisable;
    }
    component->m_data[1] = State_Blend;
    component->m_data[2] = kEndCommands;

    component = &m_priorityStacks.m_stacks[LevelIndex(display.m_recordingMode, 1)].Top();
    entries = DisplayProgram::GetComponentMinNumOfEntries(display.m_recordingMode, (DisplayProgram::LevelId)1);
    component->m_flag4 = 1;
    component->m_flag6 = entries * 4 + 8;
    component->m_flag5 = 0x10;
    component->m_data[0] = 0;

    component = &m_priorityStacks.m_stacks[LevelIndex(display.m_recordingMode, 2)].Top();
    entries = DisplayProgram::GetComponentMinNumOfEntries(display.m_recordingMode, (DisplayProgram::LevelId)2);
    component->m_flag4 = 0;
    component->m_flag6 = entries * 4 + 8;

    component = &m_priorityStacks.m_stacks[LevelIndex(display.m_recordingMode, 3)].Top();
    entries = DisplayProgram::GetComponentMinNumOfEntries(display.m_recordingMode, (DisplayProgram::LevelId)3);
    component->m_flag4 = 2;
    component->m_flag6 = entries * 4 + 8;
    component->m_data[0] = kSetClip;
    component->m_data[1] = m_clip.m_x;
    component->m_data[2] = m_clip.m_y;
    component->m_data[3] = m_clip.m_width;
    component->m_data[4] = m_clip.m_height;
    component->m_data[5] = kEndCommands;

    component = &m_priorityStacks.m_stacks[LevelIndex(display.m_recordingMode, 4)].Top();
    entries = DisplayProgram::GetComponentMinNumOfEntries(display.m_recordingMode, (DisplayProgram::LevelId)4);
    component->m_flag4 = 2;
    component->m_flag6 = entries * 4 + 8;
    component->m_data[0] = kSetBlend;
    component->m_data[1] = m_state.m_blendArg.Top();
    component->m_data[2] = kEndCommands;

    component = &m_priorityStacks.m_stacks[LevelIndex(display.m_recordingMode, 5)].Top();
    entries = DisplayProgram::GetComponentMinNumOfEntries(display.m_recordingMode, (DisplayProgram::LevelId)5);
    component->m_flag4 = 2;
    component->m_flag6 = entries * 4 + 8;
    if (m_state.m_alphaTest.Top()) {
        component->m_data[0] = kEnable;
    } else {
        component->m_data[0] = kDisable;
    }
    component->m_data[1] = State_AlphaTest;
    component->m_data[2] = kEndCommands;

    const Color_ARGB_fixed& color = m_state.m_state2.Top().m_color;
    component = &m_priorityStacks.m_stacks[LevelIndex(display.m_recordingMode, 6)].Top();
    entries = DisplayProgram::GetComponentMinNumOfEntries(display.m_recordingMode, (DisplayProgram::LevelId)6);
    component->m_flag4 = 2;
    component->m_flag6 = entries * 4 + 8;
    component->m_data[0] = kSetColor;
    component->m_data[1] = color.a;
    component->m_data[2] = color.r;
    component->m_data[3] = color.g;
    component->m_data[4] = color.b;
    component->m_data[5] = kEndCommands;

    component = &m_priorityStacks.m_stacks[LevelIndex(display.m_recordingMode, 7)].Top();
    entries = DisplayProgram::GetComponentMinNumOfEntries(display.m_recordingMode, (DisplayProgram::LevelId)7);
    component->m_flag4 = 0;
    component->m_flag6 = entries * 4 + 8;

    // depth test for the layered modes
    bool depth = previousMode == IDisplayProgram::Mode_2 || previousMode == IDisplayProgram::Mode_5;
    if (depth) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    glDepthMask(depth);
    m_depthEnabled = depth;
    m_depthWriting = depth;
    m_lastTexture = 0;
    m_textureBound = 0;
    glDisable(GL_TEXTURE_2D);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    m_priorityStacks.Run(display);
    if (m_depthEnabled) {
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        m_depthEnabled = 0;
        m_depthWriting = 0;
    }
    m_recording->m_currPos = previousMode;
    m_recording = NULL;
}

void CGraphics2d_Lite_OGLES::SetOrigin(Origin origin)
{
    m_origin = origin;
}

void CGraphics2d_Lite_OGLES::SetShaderMode(ShaderMode)
{
}

void CGraphics2d_Lite_OGLES::SetShader(ICShader*)
{
}

void CGraphics2d_Lite_OGLES::ConsiderAddingToDisplayList(unsigned int command, int count, ...)
{
    m_recording->m_field22 = 0;
    // transforms are applied to the recorded vertices instead
    switch (command) {
    case kScale:
    case kPopTransform:
    case kTranslate:
    case kRotate:
    case kLoadIdentity:
    case kMultiplyTransform:
    case kPushTransform:
    case kLoadTransform:
        return;
    }
    va_list arguments;
    va_start(arguments, count);
    int level;
    switch (command) {
    case kEnable:
    case kDisable: {
        int state = va_arg(arguments, int);
        if (state == State_AlphaTest) {
            level = LevelIndex(m_recording->m_recordingMode, 5);
        } else if (state == State_Blend) {
            level = LevelIndex(m_recording->m_recordingMode, DisplayProgram::LevelId_Blend);
        } else {
            return;
        }
        break;
    }
    case kPushState:
    case kPopState: {
        int state = va_arg(arguments, int);
        if (state == State_AlphaTest) {
            level = LevelIndex(m_recording->m_recordingMode, 5);
        } else if (state == State_Blend) {
            level = LevelIndex(m_recording->m_recordingMode, DisplayProgram::LevelId_Blend);
        } else if (state == State_2) {
            level = LevelIndex(m_recording->m_recordingMode, 6);
        } else {
            return;
        }
        if (level < 0) {
            return;
        }
        // the state stacks follow the pushes and pops
        if (command == kPushState) {
            m_priorityStacks.m_stacks[level].PushTop();
        } else if (command == kPopState) {
            m_priorityStacks.m_stacks[level].m_top--;
        }
        return;
    }
    case kSetColor:
        level = LevelIndex(m_recording->m_recordingMode, 6);
        break;
    case kSetClip:
        level = LevelIndex(m_recording->m_recordingMode, 3);
        break;
    case kSetBlend:
        level = LevelIndex(m_recording->m_recordingMode, 4);
        break;
    default:
        return;
    }
    if (level < 0) {
        return;
    }
    // the caller writes the command over the stack top
    m_recording->m_field22 = 1;
    TCStack<Component>& stack = m_priorityStacks.m_stacks[level];
    Component& component = stack.m_data[stack.m_top - 1];
    component.m_flag4 = 2;
    component.m_flag6 = MinCmpLen(m_recording->m_recordingMode)[level];
    m_recording->m_poolPos = component.m_data;
    component.m_data[count + 1] = kEndCommands;
}

/** Packed 8 bit channel of a 16.16 color channel. */
static inline unsigned char PackChannel(int channel)
{
    int value = channel >> 8;
    if (value < 0) {
        return 0;
    }
    if (value > 0xff) {
        return 0xff;
    }
    return value;
}

void CGraphics2d_Lite_OGLES::SetColor(int alpha, int red, int green, int blue)
{
    AbstractState::Entry20& entry = m_state.m_state2.Top();
    entry.m_color.a = alpha;
    entry.m_color.r = red;
    entry.m_color.g = green;
    entry.m_color.b = blue;
    entry.m_argb = (PackChannel(alpha) << 24) | (PackChannel(red) << 16) | (PackChannel(green) << 8) |
                   PackChannel(blue);
    if (CK_GL_IMMEDIATE()) {
        glColor4x(red, green, blue, alpha);
    }
    if (m_recording != NULL) {
        if (m_recording->m_currPos == IDisplayProgram::Mode_1) {
            CK_DL_RESERVE(5);
        } else if (m_recording->m_currPos > IDisplayProgram::Mode_1) {
            ConsiderAddingToDisplayList(kSetColor, 4, entry.m_color.a, entry.m_color.r, entry.m_color.g,
                                        entry.m_color.b);
        }
        if (m_recording != NULL && m_recording->m_field22) {
            CK_DL_WRITE(kSetColor);
            CK_DL_WRITE(entry.m_color.a);
            CK_DL_WRITE(entry.m_color.r);
            CK_DL_WRITE(entry.m_color.g);
            CK_DL_WRITE(entry.m_color.b);
        }
    }
}

void CGraphics2d_Lite_OGLES::SetColor(const Color_ARGB_fixed& color)
{
    AbstractState::Entry20& entry = m_state.m_state2.Top();
    entry.m_color = color;
    entry.m_argb = (PackChannel(entry.m_color.a) << 24) | (PackChannel(entry.m_color.r) << 16) |
                   (PackChannel(entry.m_color.g) << 8) | PackChannel(entry.m_color.b);
    if (CK_GL_IMMEDIATE()) {
        glColor4x(entry.m_color.r, entry.m_color.g, entry.m_color.b, entry.m_color.a);
    }
    if (m_recording != NULL) {
        if (m_recording->m_currPos == IDisplayProgram::Mode_1) {
            CK_DL_RESERVE(5);
        } else if (m_recording->m_currPos > IDisplayProgram::Mode_1) {
            ConsiderAddingToDisplayList(kSetColor, 4, entry.m_color.a, entry.m_color.r, entry.m_color.g,
                                        entry.m_color.b);
        }
        if (m_recording != NULL && m_recording->m_field22) {
            CK_DL_WRITE(kSetColor);
            CK_DL_WRITE(entry.m_color.a);
            CK_DL_WRITE(entry.m_color.r);
            CK_DL_WRITE(entry.m_color.g);
            CK_DL_WRITE(entry.m_color.b);
        }
    }
}

void CGraphics2d_Lite_OGLES::SetColor(unsigned int argb)
{
    AbstractState::Entry20& entry = m_state.m_state2.Top();
    entry.m_argb = argb;
    entry.m_color.Make(ColorFormat_A8R8G8B8, &argb);
    if (CK_GL_IMMEDIATE()) {
        glColor4x(entry.m_color.r, entry.m_color.g, entry.m_color.b, entry.m_color.a);
    }
    if (m_recording != NULL) {
        if (m_recording->m_currPos == IDisplayProgram::Mode_1) {
            CK_DL_RESERVE(5);
        } else if (m_recording->m_currPos > IDisplayProgram::Mode_1) {
            ConsiderAddingToDisplayList(kSetColor, 4, entry.m_color.a, entry.m_color.r, entry.m_color.g,
                                        entry.m_color.b);
        }
        if (m_recording != NULL && m_recording->m_field22) {
            CK_DL_WRITE(kSetColor);
            CK_DL_WRITE(entry.m_color.a);
            CK_DL_WRITE(entry.m_color.r);
            CK_DL_WRITE(entry.m_color.g);
            CK_DL_WRITE(entry.m_color.b);
        }
    }
}

/** Record a state command with one argument (Enable / Disable / PushState / PopState / SetBlendArg). */
#define CK_DL_RECORD_1(command, argument)                                                           \
    if (m_recording != NULL) {                                                                      \
        if (m_recording->m_currPos == IDisplayProgram::Mode_1) {                                    \
            CK_DL_RESERVE(2);                                                                       \
        } else if (m_recording->m_currPos > IDisplayProgram::Mode_1) {                              \
            ConsiderAddingToDisplayList(command, 1, argument);                                      \
        }                                                                                           \
        if (m_recording != NULL && m_recording->m_field22) {                                        \
            CK_DL_WRITE(command);                                                                   \
            CK_DL_WRITE(argument);                                                                  \
        }                                                                                           \
    }

void CGraphics2d_Lite_OGLES::Enable(State state)
{
    GLenum capability;
    switch (state) {
    case State_ColorKey:
        m_state.m_colorKey.Top() = 1;
        goto record;
    case State_4:
        m_state.m_state4.Top() = 1;
        goto record;
    case State_AlphaTest:
        m_state.m_alphaTest.Top() = 1;
        capability = GL_ALPHA_TEST;
        break;
    case State_Blend:
        m_state.m_blend.Top() = 1;
        capability = GL_BLEND;
        break;
    default:
        goto record;
    }
    if (CK_GL_IMMEDIATE()) {
        glEnable(capability);
    }
record:
    CK_DL_RECORD_1(kEnable, state);
}

void CGraphics2d_Lite_OGLES::Disable(State state)
{
    GLenum capability;
    switch (state) {
    case State_ColorKey:
        m_state.m_colorKey.Top() = 0;
        goto record;
    case State_4:
        m_state.m_state4.Top() = 0;
        goto record;
    case State_AlphaTest:
        m_state.m_alphaTest.Top() = 0;
        capability = GL_ALPHA_TEST;
        break;
    case State_Blend:
        m_state.m_blend.Top() = 0;
        capability = GL_BLEND;
        break;
    default:
        goto record;
    }
    if (CK_GL_IMMEDIATE()) {
        glDisable(capability);
    }
record:
    CK_DL_RECORD_1(kDisable, state);
}

void CGraphics2d_Lite_OGLES::PushState(State state)
{
    m_state.Push(state);
    CK_DL_RECORD_1(kPushState, state);
}

void CGraphics2d_Lite_OGLES::PopState(State state)
{
    m_state.Pop(state);
    unsigned char enabled;
    GLenum capability;
    if (state == State_AlphaTest) {
        enabled = m_state.m_alphaTest.Top();
        capability = GL_ALPHA_TEST;
    } else if (state == State_Blend) {
        enabled = m_state.m_blend.Top();
        SetBlendArg((BlendArg)m_state.m_blendArg.Top());
        capability = GL_BLEND;
    } else if (state == State_2) {
        const Color_ARGB_fixed& color = m_state.m_state2.Top().m_color;
        glColor4x(color.r, color.g, color.b, color.a);
        goto record;
    } else {
        goto record;
    }
    if (enabled) {
        if (CK_GL_IMMEDIATE()) {
            glEnable(capability);
        }
    } else {
        if (CK_GL_IMMEDIATE()) {
            glDisable(capability);
        }
    }
record:
    CK_DL_RECORD_1(kPopState, state);
}

void CGraphics2d_Lite_OGLES::SetBlendArg(BlendArg arg)
{
    m_state.m_blendArg.Top() = arg;
    GLenum source = GL_SRC_ALPHA;
    GLenum destination = GL_ONE_MINUS_SRC_ALPHA;
    switch (m_state.m_blendArg.Top()) {
    case BlendArg_0:
    case BlendArg_2:
        break;
    case BlendArg_1:
    case BlendArg_3:
        destination = GL_ONE;
        break;
    case BlendArg_4:
        source = GL_ONE;
        destination = GL_ONE;
        break;
    default:
        source = GL_ZERO;
        destination = GL_ZERO;
        break;
    }
    if (CK_GL_IMMEDIATE()) {
        glBlendFunc(source, destination);
    }
    CK_DL_RECORD_1(kSetBlend, arg);
}

void CGraphics2d_Lite_OGLES::LoadIdentity()
{
    m_transformFlags.Top() = 0;
    if (CK_GL_IMMEDIATE()) {
        glLoadIdentity();
    }
    if (m_trackTransform) {
        m_transformStack.Top() = Matrix();
    }
    if (m_recording != NULL) {
        if (m_recording->m_currPos == IDisplayProgram::Mode_1) {
            CK_DL_RESERVE(1);
        } else if (m_recording->m_currPos > IDisplayProgram::Mode_1) {
            ConsiderAddingToDisplayList(kLoadIdentity, 0);
        }
        if (m_recording != NULL && m_recording->m_field22) {
            CK_DL_WRITE(kLoadIdentity);
        }
    }
}

/** Record a transform with the matrix and translation words. */
#define CK_DL_RECORD_MATRIX(command)                                                                \
    if (m_recording != NULL) {                                                                      \
        if (m_recording->m_currPos == IDisplayProgram::Mode_1) {                                    \
            CK_DL_RESERVE(7);                                                                       \
        } else if (m_recording->m_currPos > IDisplayProgram::Mode_1) {                              \
            ConsiderAddingToDisplayList(command, 6, FixedWord(matrix, 0), FixedWord(matrix, 1),     \
                                        FixedWord(matrix, 2), FixedWord(matrix, 3),                 \
                                        FixedWord(translation, 0), FixedWord(translation, 1));      \
        }                                                                                           \
        if (m_recording != NULL && m_recording->m_field22) {                                        \
            CK_DL_WRITE(command);                                                                   \
            CK_DL_WRITE(FixedWord(matrix, 0));                                                      \
            CK_DL_WRITE(FixedWord(matrix, 1));                                                      \
            CK_DL_WRITE(FixedWord(matrix, 2));                                                      \
            CK_DL_WRITE(FixedWord(matrix, 3));                                                      \
            CK_DL_WRITE(FixedWord(translation, 0));                                                 \
            CK_DL_WRITE(FixedWord(translation, 1));                                                 \
        }                                                                                           \
    }

void CGraphics2d_Lite_OGLES::LoadTransform(const CMatrix2d* matrix, const CVector2d* translation)
{
    // column major GL matrix, written row by row
    GLfixed transform[16];
    transform[0] = FixedWord(matrix, 0);
    transform[4] = FixedWord(matrix, 2);
    transform[8] = 0;
    transform[12] = FixedWord(translation, 0);
    transform[1] = FixedWord(matrix, 1);
    transform[5] = FixedWord(matrix, 3);
    transform[9] = 0;
    transform[13] = FixedWord(translation, 1);
    transform[2] = 0;
    transform[6] = 0;
    transform[10] = kFixedOne;
    transform[14] = 0;
    transform[3] = 0;
    transform[7] = 0;
    transform[11] = 0;
    transform[15] = kFixedOne;
    m_transformFlags.Top() |= kTransformTranslate;
    m_transformFlags.Top() |= kTransformRotate;
    m_transformFlags.Top() |= kTransformScale;
    if (CK_GL_IMMEDIATE()) {
        glLoadMatrixx(transform);
    }
    if (m_trackTransform) {
        int x = FixedWord(translation, 0);
        int y = FixedWord(translation, 1);
        Matrix& top = m_transformStack.Top();
        top.m[0][0] = transform[0];
        top.m[0][1] = transform[4];
        top.m[1][0] = transform[1];
        top.m[1][1] = transform[5];
        top.m[2][0] = x;
        top.m[2][1] = y;
    }
    CK_DL_RECORD_MATRIX(kLoadTransform);
}

void CGraphics2d_Lite_OGLES::MultiplyTransform(const CMatrix2d* matrix, const CVector2d* translation)
{
    // column major GL matrix, written row by row
    GLfixed transform[16];
    transform[0] = FixedWord(matrix, 0);
    transform[4] = FixedWord(matrix, 2);
    transform[8] = 0;
    transform[12] = FixedWord(translation, 0);
    transform[1] = FixedWord(matrix, 1);
    transform[5] = FixedWord(matrix, 3);
    transform[9] = 0;
    transform[13] = FixedWord(translation, 1);
    transform[2] = 0;
    transform[6] = 0;
    transform[10] = kFixedOne;
    transform[14] = 0;
    transform[3] = 0;
    transform[7] = 0;
    transform[11] = 0;
    transform[15] = kFixedOne;
    m_transformFlags.Top() |= kTransformTranslate;
    m_transformFlags.Top() |= kTransformRotate;
    m_transformFlags.Top() |= kTransformScale;
    if (CK_GL_IMMEDIATE()) {
        glMultMatrixx(transform);
    }
    if (m_trackTransform) {
        m_transformStack.Top().multiply(transform[0], transform[4], transform[1], transform[5],
                                        FixedWord(translation, 0), FixedWord(translation, 1));
    }
    CK_DL_RECORD_MATRIX(kMultiplyTransform);
}

void CGraphics2d_Lite_OGLES::Translate(int x, int y)
{
    m_transformFlags.Top() |= kTransformTranslate;
    if (CK_GL_IMMEDIATE()) {
        glTranslatex(x, y, 0);
    }
    if (m_trackTransform) {
        Matrix& top = m_transformStack.Top();
        int xx = smult16(x, top.m[0][0]);
        int yx = smult16(y, top.m[0][1]);
        int xy = smult16(x, top.m[1][0]);
        int yy = smult16(y, top.m[1][1]);
        top.m[2][0] = top.m[2][0] + xx + yx;
        top.m[2][1] = yy + xy + top.m[2][1];
    }
    if (m_recording != NULL) {
        if (m_recording->m_currPos == IDisplayProgram::Mode_1) {
            CK_DL_RESERVE(3);
        } else if (m_recording->m_currPos > IDisplayProgram::Mode_1) {
            ConsiderAddingToDisplayList(kTranslate, 2, x, y);
        }
        if (m_recording != NULL && m_recording->m_field22) {
            CK_DL_WRITE(kTranslate);
            CK_DL_WRITE(x);
            CK_DL_WRITE(y);
        }
    }
}

void CGraphics2d_Lite_OGLES::Rotate(int angle)
{
    m_transformFlags.Top() |= kTransformRotate;
    if (CK_GL_IMMEDIATE()) {
        glRotatex(angle, 0, 0, kFixedOne);
    }
    if (m_trackTransform) {
        Matrix& top = m_transformStack.Top();
        int sine = CMathFixed::Sin(angle);
        int cosine = CMathFixed::Sin(CMathFixed::Degrees_90 + angle);
        Matrix rotation;
        rotation.m[0][0] = cosine;
        rotation.m[0][1] = -sine;
        rotation.m[1][0] = sine;
        rotation.m[1][1] = cosine;
        rotation.m[2][1] = 0;
        rotation.m[2][0] = 0;
        top.precat_R(&rotation);
    }
    if (m_recording != NULL) {
        if (m_recording->m_currPos == IDisplayProgram::Mode_1) {
            CK_DL_RESERVE(2);
        } else if (m_recording->m_currPos > IDisplayProgram::Mode_1) {
            ConsiderAddingToDisplayList(kRotate, 1, angle);
        }
        if (m_recording != NULL && m_recording->m_field22) {
            CK_DL_WRITE(kRotate);
            CK_DL_WRITE(angle);
        }
    }
}

void CGraphics2d_Lite_OGLES::Scale(int x, int y)
{
    m_transformFlags.Top() |= kTransformScale;
    if (CK_GL_IMMEDIATE()) {
        glScalex(x, y, kFixedOne);
    }
    if (m_trackTransform) {
        Matrix& top = m_transformStack.Top();
        for (int row = 0; row < 2; row++) {
            top.m[row][0] = smult16(top.m[row][0], x);
            top.m[row][1] = smult16(top.m[row][1], y);
        }
    }
    if (m_recording != NULL) {
        if (m_recording->m_currPos == IDisplayProgram::Mode_1) {
            CK_DL_RESERVE(3);
        } else if (m_recording->m_currPos > IDisplayProgram::Mode_1) {
            ConsiderAddingToDisplayList(kScale, 2, x, y);
        }
        if (m_recording != NULL && m_recording->m_field22) {
            CK_DL_WRITE(kScale);
            CK_DL_WRITE(x);
            CK_DL_WRITE(y);
        }
    }
}

void CGraphics2d_Lite_OGLES::PushTransform()
{
    m_transformFlags.PushTop();
    if (CK_GL_IMMEDIATE()) {
        glPushMatrix();
    }
    if (m_trackTransform) {
        m_transformStack.PushTop();
    }
    if (m_recording != NULL) {
        if (m_recording->m_currPos == IDisplayProgram::Mode_1) {
            CK_DL_RESERVE(1);
        } else if (m_recording->m_currPos > IDisplayProgram::Mode_1) {
            ConsiderAddingToDisplayList(kPushTransform, 0);
        }
        if (m_recording != NULL && m_recording->m_field22) {
            CK_DL_WRITE(kPushTransform);
        }
    }
}

void CGraphics2d_Lite_OGLES::PopTransform()
{
    m_transformFlags.m_top--;
    if (CK_GL_IMMEDIATE()) {
        glPopMatrix();
    }
    if (m_trackTransform) {
        m_transformStack.m_top--;
    }
    if (m_recording != NULL) {
        if (m_recording->m_currPos == IDisplayProgram::Mode_1) {
            CK_DL_RESERVE(1);
        } else if (m_recording->m_currPos > IDisplayProgram::Mode_1) {
            ConsiderAddingToDisplayList(kPopTransform, 0);
        }
        if (m_recording != NULL && m_recording->m_field22) {
            CK_DL_WRITE(kPopTransform);
        }
    }
}

void CGraphics2d_Lite_OGLES::GetTransform(const CMatrix2d** matrix, const CVector2d** translation)
{
    const Matrix& top = m_transformStack.Top();
    if (matrix != NULL) {
        SetFixedWord(&m_returnMatrix, 0, top.m[0][0]);
        SetFixedWord(&m_returnMatrix, 2, top.m[0][1]);
        SetFixedWord(&m_returnMatrix, 1, top.m[1][0]);
        SetFixedWord(&m_returnMatrix, 3, top.m[1][1]);
        *matrix = &m_returnMatrix;
    }
    if (translation != NULL) {
        SetFixedWord(&m_returnTranslation, 0, top.m[2][0]);
        SetFixedWord(&m_returnTranslation, 1, top.m[2][1]);
        *translation = &m_returnTranslation;
    }
}

void CGraphics2d_Lite_OGLES::SetVertexPositionCoords(int componentCount, unsigned int type, int stride,
                                                     const void* coords)
{
    m_coordCount = componentCount;
    m_coordType = type;
    m_coordStride = stride;
    m_coords = coords;
}

void CGraphics2d_Lite_OGLES::SetClip(unsigned short x, unsigned short y, unsigned short width,
                                      unsigned short height)
{
    m_clip.m_x = x;
    m_clip.m_y = y;
    m_clip.m_width = width;
    m_clip.m_height = height;
    if (m_rendering) {
        UpdateActiveClipRect_HW();
    }
    if (m_recording != NULL) {
        if (m_recording->m_currPos == IDisplayProgram::Mode_1) {
            CK_DL_RESERVE(5);
        } else if (m_recording->m_currPos > IDisplayProgram::Mode_1) {
            ConsiderAddingToDisplayList(kSetClip, 4, x, y, width, height);
        }
        if (m_recording != NULL && m_recording->m_field22) {
            CK_DL_WRITE(kSetClip);
            CK_DL_WRITE(x);
            CK_DL_WRITE(y);
            CK_DL_WRITE(width);
            CK_DL_WRITE(height);
        }
    }
}

void CGraphics2d_Lite_OGLES::UpdateActiveClipRect_HW()
{
    ICRenderSurface* target = GraphicsInstance()->GetTargetSurface();
    if (target == NULL) {
        return;
    }
    unsigned int width = 0;
    unsigned int height = 0;
    target->GetWidthAndHeight(width, height);
    m_activeClip = CRectangle(0, 0, width, height);
    if (m_clip.m_x == 0 && m_clip.m_y == 0 && (unsigned short)m_clip.m_width == (width & 0xffff) &&
        (unsigned short)m_clip.m_height == (height & 0xffff)) {
        if (CK_GL_IMMEDIATE()) {
            glDisable(GL_SCISSOR_TEST);
        }
        return;
    }
    m_activeClip.Clip(m_clip);
    if (CK_GL_IMMEDIATE()) {
        glEnable(GL_SCISSOR_TEST);
    }
    if (m_activeClip.m_width == 0 || m_activeClip.m_height == 0) {
        if (CK_GL_IMMEDIATE()) {
            glScissor(0, 0, 0, 0);
        }
        return;
    }
    int clipWidth = m_activeClip.m_width;
    int clipHeight = m_activeClip.m_height;
    if (m_origin == Origin_0) {
        if (CK_GL_IMMEDIATE()) {
            glScissor(m_activeClip.m_x, m_activeClip.m_y, clipWidth, clipHeight);
        }
        return;
    }
    // top left origin: map to the GL window of the device orientation
    if ((target->m_flags & ICRenderSurface::Flag_Targetable) != 0 && CApplet::m_pCore->m_deviceOrientationSet) {
        switch (CApplet::m_pCore->m_deviceOrientation) {
        case 1:
            break;
        case 2:
            if (CK_GL_IMMEDIATE()) {
                glScissor(m_activeClip.m_y, m_activeClip.m_x, clipHeight, clipWidth);
            }
            return;
        case 3:
            if (CK_GL_IMMEDIATE()) {
                glScissor(width - clipWidth - m_activeClip.m_x, m_activeClip.m_y, clipWidth, clipHeight);
            }
            return;
        case 4:
            if (CK_GL_IMMEDIATE()) {
                glScissor(height - clipHeight - m_activeClip.m_y, width - clipWidth - m_activeClip.m_x, clipHeight,
                          clipWidth);
            }
            return;
        default:
            return;
        }
    }
    // portrait
    if (CK_GL_IMMEDIATE()) {
        glScissor(m_activeClip.m_x, height - m_activeClip.m_y - clipHeight, clipWidth, clipHeight);
    }
}

void CGraphics2d_Lite_OGLES::SetClip(const CRectangle& clip)
{
    m_clip.m_x = clip.m_x;
    m_clip.m_y = clip.m_y;
    m_clip.m_width = clip.m_width;
    m_clip.m_height = clip.m_height;
    if (m_rendering) {
        UpdateActiveClipRect_HW();
    }
    if (m_recording != NULL) {
        if (m_recording->m_currPos == IDisplayProgram::Mode_1) {
            CK_DL_RESERVE(5);
        } else if (m_recording->m_currPos > IDisplayProgram::Mode_1) {
            ConsiderAddingToDisplayList(kSetClip, 4, (int)clip.m_x, (int)clip.m_y, (int)clip.m_width,
                                        (int)clip.m_height);
        }
        if (m_recording != NULL && m_recording->m_field22) {
            CK_DL_WRITE(kSetClip);
            CK_DL_WRITE(clip.m_x);
            CK_DL_WRITE(clip.m_y);
            CK_DL_WRITE(clip.m_width);
            CK_DL_WRITE(clip.m_height);
        }
    }
}

const CRectangle& CGraphics2d_Lite_OGLES::GetClip() const
{
    return m_clip;
}

/** Recorded vertex (x, y) through the CPU transform of the recording (display list modes 2..5). */
#define CK_TRANSFORM_RECORDED(x, y)                                                                 \
    if (m_recording != NULL && m_recording->m_currPos > IDisplayProgram::Mode_1) {                 \
        unsigned int flags = m_transformFlags.Top();                                                \
        const Matrix& transform = m_transformStack.Top();                                           \
        if ((flags & (kTransformScale | kTransformRotate)) == 0) {                                  \
            if (flags == kTransformTranslate) {                                                     \
                y = y + (short)((unsigned int)transform.m[2][1] >> 16);                             \
                x = x + (short)((unsigned int)transform.m[2][0] >> 16);                             \
            }                                                                                       \
        } else {                                                                                    \
            int fixedX = x << 16;                                                                   \
            int fixedY = y << 16;                                                                   \
            x = (short)((unsigned int)(smult16(fixedX, transform.m[0][0]) +                         \
                                       smult16(fixedY, transform.m[0][1]) + transform.m[2][0]) >> 16); \
            y = (short)((unsigned int)(smult16(fixedX, transform.m[1][0]) +                         \
                                       smult16(fixedY, transform.m[1][1]) + transform.m[2][1]) >> 16); \
        }                                                                                           \
    }

void CGraphics2d_Lite_OGLES::Draw(ICRenderSurface& surface, Flip flip, const CRectangle* source)
{
    if (!(surface.m_flags & ICRenderSurface::Flag_Initialized)) {
        return;
    }
    CRenderSurface_OGLES_Texture& texture = static_cast<CRenderSurface_OGLES_Texture&>(surface);
    CRenderSurface_OGLES_Texture* owner = texture.GetSourceTexture();
    CRenderSurfaceBuffer* buffer = owner->m_buffers[CRenderSurface::Buffer_Color];
    if (buffer == NULL || owner->m_textureCount == 0) {
        return;
    }
    if (m_state.m_state4.Top()) {
        HandleConfigStateBasedOnSrcFormat(buffer->m_format);
    }
    // a plain (mode 1) display list cannot hold vertices
    if (m_recording != NULL && m_recording->m_currPos == IDisplayProgram::Mode_1) {
        return;
    }
    bool recording = m_recording != NULL && m_recording->m_currPos > IDisplayProgram::Mode_1;

    const ICGraphics::Description* description = GraphicsInstance()->GetDescription(ICGraphics::Abstraction_Hardware);
    const int tileWidth = description->m_maxRenderTargetWidth;
    const int tileHeight = description->m_maxRenderTargetHeight;
    if (tileWidth <= 0 || tileHeight <= 0) {
        return;
    }
    const int imageWidth = buffer->m_imageWidth;
    const int imageHeight = buffer->m_imageHeight;
    int sourceX = 0;
    int sourceY = 0;
    int sourceWidth = imageWidth;
    int sourceHeight = imageHeight;
    const bool flipX = (flip & Flip_Horizontal) != 0;
    const bool flipY = (flip & Flip_Vertical) != 0;
    if (source != NULL) {
        CRectangle clipped = *source;
        sourceX = clipped.m_x;
        sourceY = clipped.m_y;
        if (clipped.m_width <= 0 || clipped.m_height <= 0 || sourceX >= imageWidth || sourceX + clipped.m_width <= 0 ||
            sourceY >= imageHeight || sourceY + clipped.m_height <= 0) {
            return;
        }
        // The mirrored destination is based on the visible source rectangle for tiled images.
        if (owner->m_textureCount > 1 && (flipX || flipY) && imageWidth <= 0x7fff && imageHeight <= 0x7fff) {
            int result = CBlitUtil::ClipSubRectangle(sourceX, sourceY, 0, 0, imageWidth, imageHeight,
                                                     clipped.m_x, clipped.m_y, clipped.m_width, clipped.m_height,
                                                     0, 0);
            if (result & CBlitUtil::ClipResult_Empty) {
                return;
            }
        }
        sourceWidth = clipped.m_width;
        sourceHeight = clipped.m_height;
    }
    const bool powerOfTwo = (texture.m_flags & ICRenderSurface::Flag_PowerOfTwo) != 0;

    int shorts;
    int stride;
    bool depth = false;
    if (recording) {
        shorts = m_texturedShorts;
        stride = m_texturedStride;
        depth = (signed char)m_texturedDecl[0].m_fieldC == 3;
        m_priorityStacks.m_stacks[LevelIndex(m_recording->m_recordingMode, DisplayProgram::LevelId_Texture)].PushTop();
    } else {
        GLenum environment = GL_REPLACE;
        if (m_state.m_blend.Top() && m_state.m_blendArg.Top() < BlendArg_2) {
            environment = GL_MODULATE;
        }
        if (m_textureEnvironment != environment) {
            m_textureEnvironment = environment;
            glTexEnvx(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, environment);
        }
        glColor4x(kFixedOne, kFixedOne, kFixedOne, m_state.m_state2.Top().m_color.a);
        shorts = 4;
        stride = m_quadBuffer.m_pool.m_reserve;
    }

    // Surfaces are stored bottom-up like GL render targets: the decoder's default flipY puts image row i
    // into buffer row (imageHeight - 1 - i), and InitializeTextureObjects cuts the tiles in buffer order.
    // So a tile row covers buffer rows [bufferY, bufferY + height) and texture v = 0 is its lowest image row.
    int index = 0;
    for (int bufferY = 0; bufferY < imageHeight; bufferY += tileHeight) {
        int height = tileHeight;
        if (imageHeight - bufferY < height) {
            height = imageHeight - bufferY;
        }
        const int tileBottom = imageHeight - bufferY;          // image row just below this tile
        const int tileTop = tileBottom - height;               // first image row of this tile
        int top = tileTop;
        if (sourceY > top) {
            top = sourceY;
        }
        int bottom = tileBottom;
        if (sourceY + sourceHeight < bottom) {
            bottom = sourceY + sourceHeight;
        }
        for (int x = 0; x < imageWidth; x += tileWidth, ++index) {
            int width = tileWidth;
            if (imageWidth - x < width) {
                width = imageWidth - x;
            }
            int left = x;
            if (sourceX > left) {
                left = sourceX;
            }
            int right = x + width;
            if (sourceX + sourceWidth < right) {
                right = sourceX + sourceWidth;
            }
            if (left >= right || top >= bottom || index >= owner->m_textureCount) {
                continue;
            }
            const GLuint name = owner->GetTextureName(index);
            if (name == 0) {
                continue;
            }
            // 4.12 texture coordinates, kept one unit inside the drawn texels
            int textureWidth = width;
            int textureHeight = height;
            if (powerOfTwo) {
                textureWidth = 1 << TextureShift(width);
                textureHeight = 1 << TextureShift(height);
            }
            short u0 = (short)(((left - x) << 12) / textureWidth + 1);
            short u1 = (short)(((right - x) << 12) / textureWidth - 1);
            short v0 = (short)(((tileBottom - top) << 12) / textureHeight - 1);
            short v1 = (short)(((tileBottom - bottom) << 12) / textureHeight + 1);
            short x0 = (short)(left - sourceX);
            short x1 = (short)(right - sourceX);
            short y0 = (short)(top - sourceY);
            short y1 = (short)(bottom - sourceY);
            if (flipX) {
                x0 = (short)(sourceWidth - x0);
                x1 = (short)(sourceWidth - x1);
            }
            if (flipY) {
                y0 = (short)(sourceHeight - y0);
                y1 = (short)(sourceHeight - y1);
            }
            short x0y0 = x0;
            short y0x0 = y0;
            short x0y1 = x0;
            short y1x0 = y1;
            short x1y0 = x1;
            short y0x1 = y0;
            short x1y1 = x1;
            short y1x1 = y1;

            short* positions;
            short* coordinates;
            CIndexVertexBuffer* recorded = NULL;
            if (recording) {
                CK_TRANSFORM_RECORDED(x0y0, y0x0);
                CK_TRANSFORM_RECORDED(x0y1, y1x0);
                CK_TRANSFORM_RECORDED(x1y0, y0x1);
                CK_TRANSFORM_RECORDED(x1y1, y1x1);
                IDisplayProgram::Mode mode = m_recording->m_recordingMode;
                TCStack<Component>& textures = m_priorityStacks.m_stacks[LevelIndex(mode, DisplayProgram::LevelId_Texture)];
                Component& component = textures.m_data[textures.m_top - 1];
                component.m_flag4 = 1;
                component.m_flag6 = DisplayProgram::GetComponentMinNumOfEntries(mode, DisplayProgram::LevelId_Texture) * 4 + 8;
                component.m_flag5 = 0x20;
                component.m_data[0] = name;
                if (m_recording->m_currPos == IDisplayProgram::Mode_4 || m_recording->m_currPos == IDisplayProgram::Mode_5) {
                    ConsiderDisplayProgramAutoAdvanceLayerAndUpdateInfo();
                }
                int* saved = m_recording->m_poolPos;
                m_recording->m_poolPos = m_savedPoolPos;
                recorded = static_cast<CIndexVertexBuffer*>(m_priorityStacks.AquireVtxBuffer(
                    *m_recording, MinCmpLen(mode), 0x4a, stride * 4, kQuadBufferKey, m_texturedDecl, 0xc,
                    kUnsignedShortVertex));
                m_savedPoolPos = m_recording->m_poolPos;
                m_recording->m_poolPos = saved;
                if (recorded == NULL) {
                    continue;
                }
                const CVertexAttribute::Decl* attributes = reinterpret_cast<const CVertexAttribute::Decl*>(recorded->m_attributes);
                positions = reinterpret_cast<short*>(recorded->m_pool.m_cursor + attributes[0].m_field4);
                coordinates = reinterpret_cast<short*>(recorded->m_pool.m_cursor + attributes[1].m_field4);
            } else {
                m_quadBuffer.m_pool.m_cursor = m_quadBuffer.m_pool.m_data;
                if (m_quadBuffer.m_pool.m_capacity < stride * 4) {
                    m_quadBuffer.m_pool.ResizeInternal(stride * 4);
                }
                const CVertexAttribute::Decl* attributes = reinterpret_cast<const CVertexAttribute::Decl*>(m_quadBuffer.m_attributes);
                positions = reinterpret_cast<short*>(m_quadBuffer.m_pool.m_cursor + attributes[0].m_field4);
                coordinates = reinterpret_cast<short*>(m_quadBuffer.m_pool.m_cursor + attributes[1].m_field4);
            }

            // triangle strip (x0, y0) (x0, y1) (x1, y0) (x1, y1)
            positions[0] = x0y0;
            positions[1] = y0x0;
            positions[shorts] = x0y1;
            positions[shorts + 1] = y1x0;
            positions[shorts * 2] = x1y0;
            positions[shorts * 2 + 1] = y0x1;
            positions[shorts * 3] = x1y1;
            positions[shorts * 3 + 1] = y1x1;
            if (depth) {
                TCStack<Component>& layers =
                    m_priorityStacks.m_stacks[LevelIndex(m_recording->m_recordingMode, DisplayProgram::LevelId_Layer)];
                short z = (short)((m_primitiveIndex | (layers.Top().m_data[0] << 12)) + 0x8001);
                positions[2] = z;
                positions[shorts + 2] = z;
                positions[shorts * 2 + 2] = z;
                positions[shorts * 3 + 2] = z;
            }
            coordinates[0] = u0;
            coordinates[1] = v0;
            coordinates[shorts] = u0;
            coordinates[shorts + 1] = v1;
            coordinates[shorts * 2] = u1;
            coordinates[shorts * 2 + 1] = v0;
            coordinates[shorts * 3] = u1;
            coordinates[shorts * 3 + 1] = v1;

            if (recording) {
                GLushort* indices = reinterpret_cast<GLushort*>(recorded->m_indices.m_cursor);
                GLushort first = (GLushort)((recorded->m_pool.m_cursor - recorded->m_pool.m_data) / recorded->m_pool.m_reserve);
                indices[0] = first;
                indices[1] = first + 1;
                indices[2] = first + 2;
                indices[3] = first + 1;
                indices[4] = first + 2;
                indices[5] = first + 3;
                recorded->m_indices.m_cursor += recorded->m_indices.m_reserve * 6;
                recorded->m_pool.m_cursor += recorded->m_pool.m_reserve * 4;
            } else {
                m_boundTexture = name;
                glBindTexture(GL_TEXTURE_2D, name);
                glEnable(GL_TEXTURE_2D);
                glEnableClientState(GL_TEXTURE_COORD_ARRAY);
                m_textureEnabled = 1;
                glVertexPointer(2, GL_SHORT, stride, positions);
                glTexCoordPointer(2, GL_SHORT, stride, coordinates);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }
            m_primitiveIndex++;
        }
    }
    if (recording) {
        m_priorityStacks.m_stacks[LevelIndex(m_recording->m_recordingMode, DisplayProgram::LevelId_Texture)].m_top--;
        return;
    }
    const Color_ARGB_fixed& color = m_state.m_state2.Top().m_color;
    glColor4x(color.r, color.g, color.b, color.a);
}

void CGraphics2d_Lite_OGLES::Draw(Primitive primitive, int first, int count)
{
    if (m_state.m_state4.Top()) {
        // translucent colors blend like an A8R8G8B8 source, opaque ones like an opaque format
        RSBFormat format;
        if (m_state.m_state2.Top().m_color.a < 0xfae2) {
            format = (RSBFormat)0xc4404;
        } else {
            format = (RSBFormat)0xb0304;
        }
        HandleConfigStateBasedOnSrcFormat(format);
    }
    if (m_state.m_alphaTest.Top() && m_state.m_state2.Top().m_color.a == 0) {
        return;
    }
    if (m_state.m_colorKey.Top() && (m_state.m_state2.Top().m_argb & 0xffffff) == 0xff00ff) {
        return;
    }
    if (CK_GL_IMMEDIATE()) {
        glDisable(GL_TEXTURE_2D);
        if (CK_GL_IMMEDIATE()) {
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        }
    }
    bool restoreAlphaTest = false;
    m_textureEnabled = 0;
    if (m_state.m_alphaTest.Top()) {
        restoreAlphaTest = true;
        if (CK_GL_IMMEDIATE()) {
            glDisable(GL_ALPHA_TEST);
        }
    }
    if (primitive == Primitive_Rectangles) {
        DrawHardwareRectangles(first, count);
    } else if (primitive == Primitive_Polygon) {
        DrawHardwarePolygon(first, count);
    } else if (primitive == Primitive_Lines) {
        DrawHardwareLines(first, count);
    }
    if (restoreAlphaTest) {
        if (CK_GL_IMMEDIATE()) {
            glEnable(GL_ALPHA_TEST);
        }
    }
}

/** Start recording a primitive: an untextured texture component and the vertex buffer. */
#define CK_PRIMITIVE_RECORD_BEGIN(kind, vertexBytes, indexBytes, indexType)                         \
    IDisplayProgram::Mode mode = m_recording->m_recordingMode;                                      \
    const int* lengths = MinCmpLen(mode);                                                           \
    m_priorityStacks.m_stacks[LevelIndex(mode, DisplayProgram::LevelId_Texture)].PushTop();         \
    TCStack<Component>& textures = m_priorityStacks.m_stacks[LevelIndex(mode, DisplayProgram::LevelId_Texture)]; \
    Component& component = textures.m_data[textures.m_top - 1];                                     \
    component.m_flag4 = 1;                                                                          \
    component.m_flag6 = DisplayProgram::GetComponentMinNumOfEntries(mode, DisplayProgram::LevelId_Texture) * 4 + 8; \
    component.m_flag5 = 0x20;                                                                       \
    component.m_data[0] = 0;                                                                        \
    CVertexAttribute::Decl decl[2];                                                                 \
    decl[0].m_field4 = 0;                                                                           \
    decl[0].m_type = kShortVertex;                                                                  \
    bool depth = m_recording->m_currPos == IDisplayProgram::Mode_2;                                 \
    if (!depth) {                                                                                   \
        decl[0].m_fieldC = m_coordCount;                                                            \
        decl[0].m_stride = 4;                                                                       \
    } else {                                                                                        \
        decl[0].m_fieldC = 3;                                                                       \
        decl[0].m_stride = 8;                                                                       \
    }                                                                                               \
    decl[1].m_type = 0;                                                                             \
    if (m_recording->m_currPos == IDisplayProgram::Mode_4 || m_recording->m_currPos == IDisplayProgram::Mode_5) { \
        ConsiderDisplayProgramAutoAdvanceLayerAndUpdateInfo();                                      \
    }                                                                                               \
    int* saved = m_recording->m_poolPos;                                                            \
    m_recording->m_poolPos = m_savedPoolPos;                                                        \
    CVertexBuffer* buffer = m_priorityStacks.AquireVtxBuffer(*m_recording, lengths, kind, vertexBytes, \
                                                             kPrimitiveBufferKey, decl, indexBytes, indexType); \
    m_savedPoolPos = m_recording->m_poolPos;                                                        \
    m_recording->m_poolPos = saved;

/** Depth word of a recorded primitive. */
#define CK_PRIMITIVE_DEPTH()                                                                        \
    (short)((m_primitiveIndex | (m_priorityStacks.m_stacks[LevelIndex(m_recording->m_recordingMode,  \
                                    DisplayProgram::LevelId_Layer)].Top().m_data[0] << 12)) + 0x8001)

/** Write recorded vertex (x, y) of coordinate `index` at the buffer cursor (with its depth word). */
#define CK_RECORD_VERTEX(buffer, x, y, index, depth)                                                    {                                                                                                       int offset = reinterpret_cast<VertexAttribute*>((buffer)->m_attributes)[0].m_offset;                unsigned char* cursor = (buffer)->m_pool.m_cursor;                                                  CK_TRANSFORM_RECORDED(x, y);                                                                        *reinterpret_cast<short*>(cursor + offset) = x;                                                     short* position = reinterpret_cast<short*>(cursor + offset);                                        position[1] = y;                                                                                    if (m_coordCount == 3) {                                                                                position[2] = *reinterpret_cast<const short*>(                                                          static_cast<const unsigned char*>(m_coords) + m_coordStride * (index) + 4);                 } else if (depth) {                                                                                     position[2] = CK_PRIMITIVE_DEPTH();                                                             }                                                                                               }

void CGraphics2d_Lite_OGLES::DrawHardwareLines(int first, int count)
{
    if (CK_GL_IMMEDIATE()) {
        glVertexPointer(m_coordCount, GLVertexType(m_coordType), m_coordStride, m_coords);
        if (CK_GL_IMMEDIATE()) {
            glDrawArrays(GL_LINES, first, count);
        }
        return;
    }
    // recording: an untextured texture component and a vertex buffer of lines
    int vertexBytes = m_coordStride * count;
    const int* lengths = MinCmpLen(m_recording->m_recordingMode);
    m_priorityStacks.m_stacks[LevelIndex(m_recording->m_recordingMode, DisplayProgram::LevelId_Texture)].PushTop();
    Component& component =
        m_priorityStacks.m_stacks[LevelIndex(m_recording->m_recordingMode, DisplayProgram::LevelId_Texture)].Top();
    int entries = DisplayProgram::GetComponentMinNumOfEntries(m_recording->m_recordingMode, DisplayProgram::LevelId_Texture);
    component.m_flag4 = 1;
    component.m_flag6 = entries * 4 + 8;
    component.m_flag5 = 0x20;
    component.m_data[0] = 0;
    VertexAttribute decl[2];
    decl[0].m_offset = 0;
    decl[0].m_type = kShortVertex;
    bool depth;
    if (m_recording->m_currPos != IDisplayProgram::Mode_2) {
        decl[0].m_count = m_coordCount;
        decl[0].m_stride = 4;
        depth = false;
    } else {
        decl[0].m_count = 3;
        decl[0].m_stride = 8;
        depth = true;
    }
    decl[1].m_type = 0;
    if (m_recording->m_currPos == IDisplayProgram::Mode_4 || m_recording->m_currPos == IDisplayProgram::Mode_5) {
        ConsiderDisplayProgramAutoAdvanceLayerAndUpdateInfo();
    }
    int* saved = m_recording->m_poolPos;
    m_recording->m_poolPos = m_savedPoolPos;
    CVertexBuffer* buffer = m_priorityStacks.AquireVtxBuffer(*m_recording, lengths, 0x30, vertexBytes, kPrimitiveBufferKey,
                                                             reinterpret_cast<CVertexAttribute::Decl*>(decl), 0, 0);
    m_savedPoolPos = m_recording->m_poolPos;
    m_recording->m_poolPos = saved;
    if (m_coordType == kShortVertex) {
        int ended = 0;
        for (int index = first; index < first + count; index++) {
            const short* coordinate = reinterpret_cast<const short*>(static_cast<const unsigned char*>(m_coords) + m_coordStride * index);
            short x = coordinate[0];
            short y = coordinate[1];
            CK_RECORD_VERTEX(buffer, x, y, index, depth);
            buffer->m_pool.m_cursor += buffer->m_pool.m_reserve;
            ended++;
            if (ended == 2) {
                m_primitiveIndex++;
                ended = 0;
            }
        }
    } else if (m_coordType == kByteVertex) {
        int ended = 0;
        for (int index = first; index < first + count; index++) {
            const signed char* coordinate =
                reinterpret_cast<const signed char*>(static_cast<const unsigned char*>(m_coords) + m_coordStride * index);
            short x = coordinate[0];
            short y = coordinate[1];
            CK_RECORD_VERTEX(buffer, x, y, index, depth);
            buffer->m_pool.m_cursor += buffer->m_pool.m_reserve;
            ended++;
            if (ended == 2) {
                m_primitiveIndex++;
                ended = 0;
            }
        }
    }
    if (m_recording != NULL && m_recording->m_currPos > IDisplayProgram::Mode_1) {
        m_priorityStacks.m_stacks[LevelIndex(m_recording->m_recordingMode, DisplayProgram::LevelId_Texture)].m_top--;
    }
}

void CGraphics2d_Lite_OGLES::DrawHardwarePolygon(int first, int count)
{
    if (CK_GL_IMMEDIATE()) {
        glVertexPointer(m_coordCount, GLVertexType(m_coordType), m_coordStride, m_coords);
        if (CK_GL_IMMEDIATE()) {
            glDrawArrays(GL_TRIANGLE_FAN, first, count);
        }
        return;
    }
    // recording: an untextured texture component and the fan as indexed triangles
    int vertexBytes = m_coordStride * count;
    int triangles = count - 2;
    const int* lengths = MinCmpLen(m_recording->m_recordingMode);
    m_priorityStacks.m_stacks[LevelIndex(m_recording->m_recordingMode, DisplayProgram::LevelId_Texture)].PushTop();
    Component& component =
        m_priorityStacks.m_stacks[LevelIndex(m_recording->m_recordingMode, DisplayProgram::LevelId_Texture)].Top();
    int entries = DisplayProgram::GetComponentMinNumOfEntries(m_recording->m_recordingMode, DisplayProgram::LevelId_Texture);
    component.m_flag4 = 1;
    component.m_flag6 = entries * 4 + 8;
    component.m_flag5 = 0x20;
    component.m_data[0] = 0;
    VertexAttribute decl[2];
    decl[0].m_offset = 0;
    decl[0].m_type = kShortVertex;
    bool depth;
    if (m_recording->m_currPos != IDisplayProgram::Mode_2) {
        decl[0].m_count = m_coordCount;
        decl[0].m_stride = 4;
        depth = false;
    } else {
        decl[0].m_count = 3;
        decl[0].m_stride = 8;
        depth = true;
    }
    decl[1].m_type = 0;
    if (m_recording->m_currPos == IDisplayProgram::Mode_4 || m_recording->m_currPos == IDisplayProgram::Mode_5) {
        ConsiderDisplayProgramAutoAdvanceLayerAndUpdateInfo();
    }
    int* saved = m_recording->m_poolPos;
    m_recording->m_poolPos = m_savedPoolPos;
    CVertexBuffer* buffer = m_priorityStacks.AquireVtxBuffer(*m_recording, lengths, 0x40, vertexBytes, kPrimitiveBufferKey,
                                                             reinterpret_cast<CVertexAttribute::Decl*>(decl),
                                                             triangles * 12, kUnsignedShortVertex);
    m_savedPoolPos = m_recording->m_poolPos;
    m_recording->m_poolPos = saved;
    if (m_recording != NULL && m_recording->m_currPos > IDisplayProgram::Mode_1 && triangles > 0) {
        CIndexVertexBuffer* indexed = static_cast<CIndexVertexBuffer*>(buffer);
        short* indices = reinterpret_cast<short*>(indexed->m_indices.m_cursor);
        short base = (short)((buffer->m_pool.m_cursor - buffer->m_pool.m_data) / buffer->m_pool.m_reserve);
        for (int triangle = 0; triangle < triangles; triangle++) {
            short corner = (short)(base + triangle);
            indices[0] = base;
            indices[1] = corner + 1;
            indices[2] = corner + 2;
            indices += 3;
            indexed->m_indices.m_cursor += indexed->m_indices.m_reserve * 3;
        }
    }
    if (m_coordType == kShortVertex) {
        for (int index = first; index < first + count; index++) {
            const short* coordinate = reinterpret_cast<const short*>(static_cast<const unsigned char*>(m_coords) + m_coordStride * index);
            short x = coordinate[0];
            short y = coordinate[1];
            CK_RECORD_VERTEX(buffer, x, y, index, depth);
            buffer->m_pool.m_cursor += buffer->m_pool.m_reserve;
        }
    } else if (m_coordType == kByteVertex) {
        for (int index = first; index < first + count; index++) {
            const signed char* coordinate =
                reinterpret_cast<const signed char*>(static_cast<const unsigned char*>(m_coords) + m_coordStride * index);
            short x = coordinate[0];
            short y = coordinate[1];
            CK_RECORD_VERTEX(buffer, x, y, index, depth);
            buffer->m_pool.m_cursor += buffer->m_pool.m_reserve;
        }
    } else {
        goto done;
    }
    m_primitiveIndex++;
done:
    if (m_recording != NULL && m_recording->m_currPos > IDisplayProgram::Mode_1) {
        m_priorityStacks.m_stacks[LevelIndex(m_recording->m_recordingMode, DisplayProgram::LevelId_Texture)].m_top--;
    }
}

/** One rectangle (x0, y0)-(x1, y1) as the strip (x0, y0) (x0, y1) (x1, y0) (x1, y1), drawn or recorded. */
#define CK_RECTANGLE(x0, y0, x1, y1, index)                                                         \
    {                                                                                               \
        int offset = reinterpret_cast<VertexAttribute*>(buffer->m_attributes)[0].m_offset;          \
        short* position = reinterpret_cast<short*>(buffer->m_pool.m_cursor + offset);               \
        short ax = x0;                                                                              \
        short ay = y0;                                                                              \
        short bx = x0;                                                                              \
        short by = y1;                                                                              \
        short cx = x1;                                                                              \
        short cy = y0;                                                                              \
        short dx = x1;                                                                              \
        short dy = y1;                                                                              \
        if (m_recording != NULL && m_recording->m_currPos > IDisplayProgram::Mode_1) {             \
            unsigned int flags = m_transformFlags.Top();                                            \
            const Matrix& transform = m_transformStack.Top();                                       \
            if ((flags & (kTransformScale | kTransformRotate)) == 0) {                              \
                if (flags == kTransformTranslate) {                                                 \
                    short translateY = (short)((unsigned int)transform.m[2][1] >> 16);              \
                    ay = ay + translateY;                                                           \
                    by = by + translateY;                                                           \
                    cy = cy + translateY;                                                           \
                    dy = dy + translateY;                                                           \
                    short translateX = (short)((unsigned int)transform.m[2][0] >> 16);              \
                    ax = ax + translateX;                                                           \
                    bx = bx + translateX;                                                           \
                    cx = cx + translateX;                                                           \
                    dx = dx + translateX;                                                           \
                }                                                                                   \
            } else {                                                                                \
                int fixedX0 = (x0) << 16;                                                           \
                int fixedY0 = (y0) << 16;                                                           \
                int fixedX1 = (x1) << 16;                                                           \
                int fixedY1 = (y1) << 16;                                                           \
                ax = (short)((unsigned int)(smult16(fixedX0, transform.m[0][0]) +                   \
                    smult16(fixedY0, transform.m[0][1]) + transform.m[2][0]) >> 16);                \
                ay = (short)((unsigned int)(smult16(fixedX0, transform.m[1][0]) +                   \
                    smult16(fixedY0, transform.m[1][1]) + transform.m[2][1]) >> 16);                \
                bx = (short)((unsigned int)(smult16(fixedX0, transform.m[0][0]) +                   \
                    smult16(fixedY1, transform.m[0][1]) + transform.m[2][0]) >> 16);                \
                by = (short)((unsigned int)(smult16(fixedX0, transform.m[1][0]) +                   \
                    smult16(fixedY1, transform.m[1][1]) + transform.m[2][1]) >> 16);                \
                cx = (short)((unsigned int)(smult16(fixedX1, transform.m[0][0]) +                   \
                    smult16(fixedY0, transform.m[0][1]) + transform.m[2][0]) >> 16);                \
                cy = (short)((unsigned int)(smult16(fixedX1, transform.m[1][0]) +                   \
                    smult16(fixedY0, transform.m[1][1]) + transform.m[2][1]) >> 16);                \
                dx = (short)((unsigned int)(smult16(fixedX1, transform.m[0][0]) +                   \
                    smult16(fixedY1, transform.m[0][1]) + transform.m[2][0]) >> 16);                \
                dy = (short)((unsigned int)(smult16(fixedX1, transform.m[1][0]) +                   \
                    smult16(fixedY1, transform.m[1][1]) + transform.m[2][1]) >> 16);                \
            }                                                                                       \
        }                                                                                           \
        position[0] = ax;                                                                           \
        position[1] = ay;                                                                           \
        position[shorts] = bx;                                                                      \
        position[shorts + 1] = by;                                                                  \
        position[shorts * 2] = cx;                                                                  \
        position[shorts * 2 + 1] = cy;                                                              \
        position[shorts * 3] = dx;                                                                  \
        position[shorts * 3 + 1] = dy;                                                              \
        if (m_coordCount == 3) {                                                                    \
            short z = *reinterpret_cast<const short*>(                                              \
                static_cast<const unsigned char*>(m_coords) + m_coordStride * (index) + 4);         \
            position[2] = z;                                                                        \
            position[shorts + 2] = z;                                                               \
            position[shorts * 2 + 2] = z;                                                           \
            position[shorts * 3 + 2] = z;                                                           \
        } else if (depth) {                                                                         \
            short z = CK_PRIMITIVE_DEPTH();                                                         \
            position[2] = z;                                                                        \
            position[shorts + 2] = z;                                                               \
            position[shorts * 2 + 2] = z;                                                           \
            position[shorts * 3 + 2] = z;                                                           \
        }                                                                                           \
        if (m_recording != NULL && m_recording->m_currPos > IDisplayProgram::Mode_1) {             \
            CIndexVertexBuffer* indexed = static_cast<CIndexVertexBuffer*>(buffer);                 \
            short* indices = reinterpret_cast<short*>(indexed->m_indices.m_cursor);                 \
            short base = (short)((buffer->m_pool.m_cursor - buffer->m_pool.m_data) /                \
                                 buffer->m_pool.m_reserve);                                         \
            indices[0] = base;                                                                      \
            indices[1] = base + 1;                                                                  \
            indices[2] = base + 2;                                                                  \
            indices[3] = base + 1;                                                                  \
            indices[4] = base + 2;                                                                  \
            indices[5] = base + 3;                                                                  \
            indexed->m_indices.m_cursor += indexed->m_indices.m_reserve * 6;                        \
            buffer->m_pool.m_cursor += buffer->m_pool.m_reserve * 4;                                \
        }                                                                                           \
        if (CK_GL_IMMEDIATE()) {                                                                    \
            glVertexPointer(m_coordCount, GL_SHORT, m_coordCount * 2, buffer->m_pool.m_data +      \
                            reinterpret_cast<VertexAttribute*>(buffer->m_attributes)[0].m_offset);  \
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);                                                  \
        }                                                                                           \
        m_primitiveIndex++;                                                                         \
    }

void CGraphics2d_Lite_OGLES::DrawHardwareRectangles(int first, int count)
{
    CVertexBuffer* buffer;
    bool depth = false;
    if (!CK_GL_IMMEDIATE()) {
        // recording: an untextured texture component and an indexed vertex buffer
        IDisplayProgram::Mode mode = m_recording->m_recordingMode;
        m_priorityStacks.m_stacks[LevelIndex(mode, DisplayProgram::LevelId_Texture)].PushTop();
        TCStack<Component>& textures = m_priorityStacks.m_stacks[LevelIndex(mode, DisplayProgram::LevelId_Texture)];
        Component& component = textures.m_data[textures.m_top - 1];
        component.m_flag4 = 1;
        component.m_flag6 = DisplayProgram::GetComponentMinNumOfEntries(mode, DisplayProgram::LevelId_Texture) * 4 + 8;
        component.m_flag5 = 0x20;
        component.m_data[0] = 0;
        CVertexAttribute::Decl decl[2];
        decl[0].m_field4 = 0;
        decl[0].m_type = kShortVertex;
        depth = m_recording->m_currPos == IDisplayProgram::Mode_2;
        if (!depth) {
            decl[0].m_fieldC = m_coordCount;
            decl[0].m_stride = 4;
        } else {
            decl[0].m_fieldC = 3;
            decl[0].m_stride = 8;
        }
        decl[1].m_type = 0;
        if (m_recording->m_currPos == IDisplayProgram::Mode_4 || m_recording->m_currPos == IDisplayProgram::Mode_5) {
            ConsiderDisplayProgramAutoAdvanceLayerAndUpdateInfo();
        }
        int* saved = m_recording->m_poolPos;
        m_recording->m_poolPos = m_savedPoolPos;
        buffer = m_priorityStacks.AquireVtxBuffer(*m_recording, MinCmpLen(mode), 0x40, m_coordStride * count,
                                                  kPrimitiveBufferKey, decl, (count >> 1) * 12, kUnsignedShortVertex);
        m_savedPoolPos = m_recording->m_poolPos;
        m_recording->m_poolPos = saved;
        if (buffer == NULL) {
            m_priorityStacks.m_stacks[LevelIndex(mode, DisplayProgram::LevelId_Texture)].m_top--;
            return;
        }
    } else {
        // immediate: the corners go through m_vertexBuffer
        buffer = &m_vertexBuffer;
        CVertexAttribute::Decl* attributes = reinterpret_cast<CVertexAttribute::Decl*>(m_vertexBuffer.m_attributes);
        attributes[0].m_field0 = 0;
        attributes[0].m_field4 = 0;
        attributes[0].m_type = m_coordType;
        attributes[0].m_fieldC = m_coordCount;
        attributes[0].m_stride = 0;
        attributes[0].m_fieldE = 1;
        attributes[0].m_fieldF = 0;
        m_vertexBuffer.m_pool.m_reserve = m_coordStride;
        m_vertexBuffer.m_pool.m_cursor = m_vertexBuffer.m_pool.m_data;
        if (m_vertexBuffer.m_pool.m_capacity < m_coordStride * count) {
            m_vertexBuffer.m_pool.ResizeInternal(m_coordStride * count);
        }
    }

    const VertexAttribute& attribute = reinterpret_cast<VertexAttribute*>(buffer->m_attributes)[0];
    int shorts = attribute.m_count;
    if (attribute.m_type != kFixedVertex && shorts == 3) {
        shorts++;
    }
    // odd counts read one coordinate past the end, as in the original
    if (m_coordType == kShortVertex) {
        for (int index = first; index < first + count; index += 2) {
            const unsigned char* upper = static_cast<const unsigned char*>(m_coords) + m_coordStride * index;
            const short* corner0 = reinterpret_cast<const short*>(upper);
            const short* corner1 = reinterpret_cast<const short*>(upper + m_coordStride);
            short x0 = corner0[0];
            short y0 = corner0[1];
            short x1 = corner1[0];
            short y1 = corner1[1];
            CK_RECTANGLE(x0, y0, x1, y1, index);
        }
    } else if (m_coordType == kByteVertex) {
        for (int index = first; index < first + count; index += 2) {
            const unsigned char* upper = static_cast<const unsigned char*>(m_coords) + m_coordStride * index;
            const signed char* corner0 = reinterpret_cast<const signed char*>(upper);
            const signed char* corner1 = reinterpret_cast<const signed char*>(upper + m_coordStride);
            short x0 = corner0[0];
            short y0 = corner0[1];
            short x1 = corner1[0];
            short y1 = corner1[1];
            CK_RECTANGLE(x0, y0, x1, y1, index);
        }
    }
    if (m_recording != NULL && m_recording->m_currPos > IDisplayProgram::Mode_1) {
        m_priorityStacks.m_stacks[LevelIndex(m_recording->m_recordingMode, DisplayProgram::LevelId_Texture)].m_top--;
    }
}

void CGraphics2d_Lite_OGLES::HandleConfigStateBasedOnSrcFormat(RSBFormat format)
{
    bool alphaBlend = false;
    if ((format & 0x4000) != 0) {
        switch ((unsigned int)format) {
        case 0x803d5400:
        case 0x80475400:
        case 0x80495400:
        case 0x65402:
        case 0x75402:
        case 0xc4404:
        case 0xd4404:
        case 0xe4404:
        case 0xf4404:
            alphaBlend = true;
            break;
        case 0x45402:
        case 0x55402:
            // alpha tested, not blended
            if (!m_state.m_alphaTest.Top()) {
                Enable(State_AlphaTest);
            }
            goto disableBlend;
        default:
            break;
        }
    }
    if (alphaBlend) {
        if (!m_state.m_alphaTest.Top()) {
            Enable(State_AlphaTest);
        }
        if (!m_state.m_blend.Top()) {
            Enable(State_Blend);
        }
        if (m_state.m_blendArg.Top() != BlendArg_2) {
            SetBlendArg(BlendArg_2);
        }
        return;
    }
    if (m_state.m_alphaTest.Top()) {
        Disable(State_AlphaTest);
    }
disableBlend:
    if (m_state.m_blend.Top()) {
        Disable(State_Blend);
    }
}
