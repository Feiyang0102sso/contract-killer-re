// CGraphics2d_Lite_OGLES.cpp - GLES1 drawing and display program playback.
#include "CGraphics2d_Lite_OGLES.h"

#include <OpenGLES/ES1/glext.h>
#include <math.h>
#include <stdarg.h>
#include <string.h>
#include "CGraphics2d_Lite_HAL.h"
#include "CRenderSurface_OGLES_Texture.h"
#include "CVertexBuffer.h"
#include "ICCore.h"

namespace {
    const int kFixedOne = 65536;
    const unsigned int kByteVertex = 0x2f978;
    const unsigned int kUnsignedByteVertex = 0x23f978;
    const unsigned int kFixedVertex = 0x30ee34;
    const unsigned int kShortVertex = 0x3f9726;
    const unsigned int kUnsignedShortVertex = 0x13f9726;

    const unsigned int kEnable = 0xdc7991b1;
    const unsigned int kDisable = 0xc6fb25f8;
    const unsigned int kPushState = 0xbb855bf6;
    const unsigned int kPopState = 0xa9b130f8;
    const unsigned int kSetBlend = 0x9b25d779;
    const unsigned int kSetColor = 0x93e08c5f;
    const unsigned int kSetClip = 0xd13e0bbd;
    const unsigned int kEndCommands = 0xbca5ef3b;

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

    int Min(int first, int second)
    {
        if (first < second) {
            return first;
        }
        return second;
    }

    int Max(int first, int second)
    {
        if (first > second) {
            return first;
        }
        return second;
    }

    int FixedWord(const void* storage)
    {
        int value;
        memcpy(&value, storage, sizeof(value));
        return value;
    }

    void StoreFixedWord(void* storage, int value)
    {
        memcpy(storage, &value, sizeof(value));
    }

    int TextureExtent(int extent, bool powerOfTwo)
    {
        if (!powerOfTwo) {
            return extent;
        }
        int padded = 1;
        while (padded < extent) {
            padded <<= 1;
        }
        return padded;
    }

    void SetCapability(GLenum capability, bool enabled)
    {
        if (enabled) {
            glEnable(capability);
        } else {
            glDisable(capability);
        }
    }
}

// level -> table slot, and minimum component entries per slot (CK 1.5.2 data)
static const int CGraphics2d_Lite_OGLES_DisplayProgram_Mode_PaintersAlgorithmAutoAdvanceLayers[8] = {
    1, 0, 2, 3, 4, 5, 6, 7
};
static const int CGraphics2d_Lite_OGLES_DisplayProgram_Mode_PaintersAlgorithmAutoAdvanceLayersLikelyMostOptimal[8] = {
    0, 1, 2, 3, 4, 5, 6, 7
};
static const int CGraphics2d_Lite_OGLES_DisplayProgram_Mode_PaintersAlgorithmAutoAdvanceLayers_CmpMinNumOfEntries[8] = {
    1, 3, 1, 6, 3, 3, 6, 2
};
static const int CGraphics2d_Lite_OGLES_DisplayProgram_Mode_PaintersAlgorithmAutoAdvanceLayersLikelyMostOptimal_CmpMinNumOfEntries[8] = {
    3, 1, 1, 6, 3, 3, 6, 2
};

static const int DepthComponentLengths[8] = { 20, 12, 12, 32, 20, 20, 32, 16 };
static const int CompactComponentLengths[8] = { 12, 20, 12, 32, 20, 20, 32, 16 };

static const CVertexAttribute::Decl DepthTextureVertices[] = {
    { 0, 0, kShortVertex, 3, 12, 1, 0 },
    { 1, 8, kShortVertex, 2, 12, 1, 0 },
    { 0, 0, 0, 0, 0, 0, 0 }
};
static const CVertexAttribute::Decl DepthVertices[] = {
    { 0, 0, kShortVertex, 3, 8, 1, 0 },
    { 0, 0, 0, 0, 0, 0, 0 }
};
static const CVertexAttribute::Decl CompactTextureVertices[] = {
    { 0, 0, kShortVertex, 2, 8, 1, 0 },
    { 1, 4, kShortVertex, 2, 8, 1, 0 },
    { 0, 0, 0, 0, 0, 0, 0 }
};
static const CVertexAttribute::Decl CompactVertices[] = {
    { 0, 0, kShortVertex, 2, 4, 1, 0 },
    { 0, 0, 0, 0, 0, 0, 0 }
};

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
    if (m_graphics != NULL) {
        CGraphics2d_Lite_HAL* graphics = static_cast<CGraphics2d_Lite_HAL*>(m_graphics);
        CGraphics2d_Lite_OGLES* device = static_cast<CGraphics2d_Lite_OGLES*>(graphics->m_device);
        if (device->m_recording == this && device->IsRecording()) {
            Component& component = device->ComponentAtLevel(1);
            component.m_flag4 = 1;
            component.m_flag5 = 0x10;
            component.m_flag6 = GetComponentMinNumOfEntries(m_recordingMode, static_cast<LevelId>(1)) * 4 + 8;
            component.m_data[0] = layer;
        }
    }
}

int CGraphics2d_Lite_OGLES::DisplayProgram::GetLayer()
{
    return m_layer;
}

void CGraphics2d_Lite_OGLES::DisplayProgram::ExecuteSpecial(Component* component)
{
    CGraphics2d_Lite_HAL* graphics = static_cast<CGraphics2d_Lite_HAL*>(m_graphics);
    CGraphics2d_Lite_OGLES* device = static_cast<CGraphics2d_Lite_OGLES*>(graphics->m_device);
    unsigned int kind = component->m_flag5 >> 4;
    if (kind == 2) {
        GLuint texture = component->m_data[0];
        if (texture == 0) {
            if (device->m_textureEnabled) {
                device->m_textureEnabled = false;
                glDisable(GL_TEXTURE_2D);
                glDisableClientState(GL_TEXTURE_COORD_ARRAY);
            }
        } else {
            if (!device->m_textureEnabled) {
                device->m_textureEnabled = true;
                glEnable(GL_TEXTURE_2D);
                glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            }
            if (device->m_boundTexture != texture) {
                device->m_boundTexture = texture;
                glBindTexture(GL_TEXTURE_2D, texture);
            }
        }
        return;
    }
    if (kind != 3 && kind != 4) {
        return;
    }

    GLenum primitive = GL_LINES;
    if (kind == 4) {
        primitive = GL_TRIANGLES;
    }
    DrawCommand state;
    device->SetCommandState(state, primitive, device->m_boundTexture);
    device->ApplyCommandState(state);
    GLenum environment = GL_REPLACE;
    if (state.blend && state.blendArg < BlendArg_2) {
        environment = GL_MODULATE;
    }
    if (device->m_depthEnabled && device->m_depthWriting == state.blend) {
        device->m_depthWriting = !state.blend;
        glDepthMask(device->m_depthWriting);
    }
    if (device->m_textureEnvironment != environment) {
        device->m_textureEnvironment = environment;
        glTexEnvx(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, environment);
    }
    bool alphaOnly = environment == GL_MODULATE && (component->m_flag5 & 8) != 0 &&
        (state.color.r != kFixedOne || state.color.g != kFixedOne || state.color.b != kFixedOne);
    if (alphaOnly) {
        glColor4x(kFixedOne, kFixedOne, kFixedOne, state.color.a);
    }

    CVertexBuffer* buffer = reinterpret_cast<CVertexBuffer*>(component->m_data[0]);
    const CVertexAttribute::Decl* attributes = reinterpret_cast<const CVertexAttribute::Decl*>(buffer->m_attributes);
    const CVertexAttribute::Decl& position = attributes[0];
    int stride = buffer->m_pool.m_reserve;
    glVertexPointer(position.m_fieldC, GLVertexType(position.m_type), stride,
                    buffer->m_pool.m_data + position.m_field4);
    if ((component->m_flag5 & 2) != 0) {
        const CVertexAttribute::Decl& coordinate = attributes[1];
        glTexCoordPointer(coordinate.m_fieldC, GLVertexType(coordinate.m_type), stride,
                          buffer->m_pool.m_data + coordinate.m_field4);
    }
    if (buffer->m_field4 == CVertexBuffer::Type_IndexVertex) {
        CIndexVertexBuffer* indexed = static_cast<CIndexVertexBuffer*>(buffer);
        const CIndexVertexPool& indices = indexed->m_indices;
        int count = (indices.m_cursor - indices.m_data) / indices.m_reserve;
        glDrawElements(primitive, count, GLVertexType(indexed->m_indexType), indices.m_data);
    } else {
        int count = (buffer->m_pool.m_cursor - buffer->m_pool.m_data) / stride;
        glDrawArrays(primitive, 0, count);
    }
    if (alphaOnly) {
        glColor4x(state.color.r, state.color.g, state.color.b, state.color.a);
    }
}

typedef char DisplayProgramSize[sizeof(CGraphics2d_Lite_OGLES::DisplayProgram) == 0x54 ? 1 : -1];

CGraphics2d_Lite_OGLES::CGraphics2d_Lite_OGLES()
    : m_recording(NULL)
    , m_priorityStacks()
    , m_primitiveIndex(0)
    , m_depthEnabled(false)
    , m_depthWriting(false)
    , m_textureEnabled(false)
    , m_boundTexture(0)
    , m_textureEnvironment(GL_REPLACE)
    , m_origin(Origin_TopLeft)
    , m_blendArg(BlendArg_2)
    , m_colorWord(0xffffffff)
    , m_coords(NULL)
    , m_coordType(0)
    , m_coordStride(0)
    , m_coordCount(0)
    , m_rendering(false)
{
    memset(m_states, 0, sizeof(m_states));
    memset(&m_target, 0, sizeof(m_target));
    m_clip = m_target;
    m_activeClip = m_target;
    m_color = Color_ARGB_fixed::White;
    LoadIdentity();
}

CGraphics2d_Lite_OGLES::~CGraphics2d_Lite_OGLES()
{
    m_transformStack.Destroy();
    m_priorityStacks.Destroy();
}

unsigned char CGraphics2d_Lite_OGLES::Initialize()
{
    Destroy();
    m_transformStack.Initialize(32);
    m_blendArg = BlendArg_2;
    SetColor(0xffffffffu);
    LoadIdentity();
    return 1;
}

void CGraphics2d_Lite_OGLES::Destroy()
{
    m_recording = NULL;
    m_priorityStacks.Destroy();
    m_transformStack.Destroy();
    for (int state = 0; state < 5; ++state) {
        m_stateStack[state].Clear();
        m_states[state] = false;
    }
    m_colorStack.Clear();
    m_colorWordStack.Clear();
    m_blendStack.Clear();
    m_vertices.Clear();
    m_coords = NULL;
    m_rendering = false;
}

void CGraphics2d_Lite_OGLES::SetActiveAbstractionLayer(ICGraphics::Abstraction)
{
}

ICGraphics::Abstraction CGraphics2d_Lite_OGLES::GetActiveAbstractionLayer()
{
    return ICGraphics::Abstraction_Hardware;
}

void CGraphics2d_Lite_OGLES::RenderBegin(unsigned char writeAlpha)
{
    CGraphics_OGLES* hardware = CGraphics_OGLES::GetHardware();
    m_target.m_width = 0;
    m_target.m_height = 0;
    if (hardware->m_target != NULL) {
        unsigned int width = 0;
        unsigned int height = 0;
        hardware->m_target->GetWidthAndHeight(width, height);
        m_target.m_width = width;
        m_target.m_height = height;
        hardware->SetWidthAndHeightMappedViewportAndOrthoProjection(
            *static_cast<CRenderSurface*>(hardware->m_target));
    }
    m_clip = m_target;
    m_activeClip = m_target;

    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_DITHER);
    glDisable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    glDepthRangex(0, kFixedOne);
    glDisable(GL_CULL_FACE);
    glDisable(GL_NORMALIZE);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_FOG);
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_LIGHTING);
    glShadeModel(GL_FLAT);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    glAlphaFuncx(GL_GREATER, 0);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, writeAlpha != 0);
    if (hardware->m_target != NULL) {
        SetCapability(GL_MULTISAMPLE, (hardware->m_target->m_flags & ICRenderSurface::Flag_MultisampleMask) != 0);
    }

    const ICGraphics::Description& description = hardware->m_description;
    for (unsigned int unit = 1; unit < description.m_maxTextureUnits; ++unit) {
        glActiveTexture(GL_TEXTURE0 + unit);
        glClientActiveTexture(GL_TEXTURE0 + unit);
        glDisable(GL_TEXTURE_2D);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }
    glActiveTexture(GL_TEXTURE0);
    glClientActiveTexture(GL_TEXTURE0);
    glDisable(GL_TEXTURE_2D);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    LoadIdentity();
    m_rendering = true;
}

void CGraphics2d_Lite_OGLES::RenderEnd()
{
    m_rendering = false;
}

void CGraphics2d_Lite_OGLES::DisplayListBegin(IDisplayProgram& program, IDisplayProgram::Mode mode)
{
    DisplayProgram& display = static_cast<DisplayProgram&>(program);
    display.ClearCore();
    display.m_currPos = mode;
    display.m_recordingMode = mode;
    m_recording = &display;
    m_primitiveIndex = 0;
    if (!IsRecording()) {
        return;
    }
    m_priorityStacks.Initialize(DisplayProgram::LevelId_Count, 32, 50);
    m_previousComponents[0].m_flag4 = 0;
    PriorityStacks::ConditionalMgr& conditions = m_priorityStacks.m_conditionals;
    int blendLevel = 0;
    int layerLevel = 1;
    if (mode == IDisplayProgram::Mode_3 || mode == IDisplayProgram::Mode_4) {
        blendLevel = 1;
        layerLevel = 0;
    }
    conditions.m_values[blendLevel].m_op = 1;
    conditions.m_values[blendLevel].m_value = kDisable;
    conditions.m_results[blendLevel].m_true = 0;
    conditions.m_values[layerLevel].m_op = 3;
    conditions.m_values[layerLevel].m_value = -1;
    conditions.m_results[layerLevel].m_true = 0;

    InitializeDisplayProgramStates();
    display.SetLayer(display.GetLayer());
    display.m_field22 = 1;
    Enable(State_Blend);
}

void CGraphics2d_Lite_OGLES::DisplayListEnd()
{
    if (m_recording != NULL) {
        m_recording->m_field22 = 0;
    }
    m_recording = NULL;
}

void CGraphics2d_Lite_OGLES::Run(IDisplayProgram& program)
{
    DisplayProgram& display = static_cast<DisplayProgram&>(program);
    if (!m_rendering || display.m_firstComponent == NULL) {
        return;
    }
    int previousMode = display.m_currPos;
    display.m_currPos = IDisplayProgram::Mode_1;
    m_recording = &display;
    InitializeDisplayProgramStates();
    ComponentAtLevel(1).m_data[0] = 0;

    m_depthEnabled = previousMode == IDisplayProgram::Mode_2 || previousMode == IDisplayProgram::Mode_5;
    m_depthWriting = m_depthEnabled;
    SetCapability(GL_DEPTH_TEST, m_depthEnabled);
    glDepthMask(m_depthWriting);
    m_textureEnabled = false;
    m_boundTexture = 0;
    glDisable(GL_TEXTURE_2D);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    // Packed texture coordinates have twelve fractional bits.
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glScalex(0x10, 0x10, kFixedOne);
    glMatrixMode(GL_MODELVIEW);
    m_priorityStacks.Run(display);
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    if (m_depthEnabled) {
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        m_depthEnabled = false;
        m_depthWriting = false;
    }
    display.m_currPos = previousMode;
    m_recording = NULL;
}

bool CGraphics2d_Lite_OGLES::IsRecording() const
{
    return m_recording != NULL && m_recording->m_currPos >= IDisplayProgram::Mode_2 &&
           m_recording->m_currPos <= IDisplayProgram::Mode_5;
}

CGraphics2d_Lite_OGLES::Component& CGraphics2d_Lite_OGLES::ComponentAtLevel(int level)
{
    if (level < 2 && (m_recording->m_recordingMode == IDisplayProgram::Mode_3 ||
                      m_recording->m_recordingMode == IDisplayProgram::Mode_4)) {
        level = 1 - level;
    }
    TCStack<Component>& stack = m_priorityStacks.m_stacks[level];
    return stack.m_data[stack.m_top - 1];
}

void CGraphics2d_Lite_OGLES::SetTextComponent(int level, unsigned int command, const int* arguments, int count)
{
    Component& component = ComponentAtLevel(level);
    component.m_flag4 = 2;
    component.m_flag6 = DisplayProgram::GetComponentMinNumOfEntries(
        m_recording->m_recordingMode, static_cast<DisplayProgram::LevelId>(level)) * 4 + 8;
    component.m_data[0] = command;
    for (int index = 0; index < count; ++index) {
        component.m_data[index + 1] = arguments[index];
    }
    component.m_data[count + 1] = kEndCommands;
}

void CGraphics2d_Lite_OGLES::InitializeDisplayProgramStates()
{
    for (int level = 0; level < DisplayProgram::LevelId_Count; ++level) {
        TCStack<Component>& stack = m_priorityStacks.m_stacks[level];
        stack.m_top = 1;
        Component& component = ComponentAtLevel(level);
        component = Component();
        component.m_flag6 = DisplayProgram::GetComponentMinNumOfEntries(
            m_recording->m_recordingMode, static_cast<DisplayProgram::LevelId>(level)) * 4 + 8;
    }

    const int blend = State_Blend;
    SetTextComponent(0, m_states[State_Blend] ? kEnable : kDisable, &blend, 1);
    Component& layer = ComponentAtLevel(1);
    layer.m_flag4 = 1;
    layer.m_flag5 = 0x10;
    layer.m_data[0] = m_recording->m_layer;
    const int clip[] = { m_clip.m_x, m_clip.m_y, m_clip.m_width, m_clip.m_height };
    SetTextComponent(3, kSetClip, clip, 4);
    const int blendArg = m_blendArg;
    SetTextComponent(4, kSetBlend, &blendArg, 1);
    const int alpha = State_AlphaTest;
    SetTextComponent(5, m_states[State_AlphaTest] ? kEnable : kDisable, &alpha, 1);
    const int color[] = { m_color.a, m_color.r, m_color.g, m_color.b };
    SetTextComponent(6, kSetColor, color, 4);
}

void CGraphics2d_Lite_OGLES::ConsiderAddingToDisplayList(unsigned int command, int count, ...)
{
    m_recording->m_field22 = 0;
    if (!IsRecording()) {
        return;
    }
    int arguments[4];
    va_list values;
    va_start(values, count);
    for (int index = 0; index < count; ++index) {
        arguments[index] = va_arg(values, int);
    }
    va_end(values);

    int level;
    switch (command) {
    case kSetClip:
        level = 3;
        break;
    case kSetBlend:
        level = 4;
        break;
    case kSetColor:
        level = 6;
        break;
    case kEnable:
    case kDisable:
    case kPushState:
    case kPopState:
        switch (arguments[0]) {
        case State_AlphaTest:
            level = 5;
            break;
        case State_Blend:
            level = 0;
            break;
        case State_2:
            if (command != kPushState && command != kPopState) {
                return;
            }
            level = 6;
            break;
        default:
            return;
        }
        break;
    default:
        return;
    }

    if (command == kPushState || command == kPopState) {
        if (level == 0 && (m_recording->m_recordingMode == IDisplayProgram::Mode_3 ||
                           m_recording->m_recordingMode == IDisplayProgram::Mode_4)) {
            level = 1;
        }
        TCStack<Component>& state = m_priorityStacks.m_stacks[level];
        if (command == kPushState) {
            state.PushTop();
        } else if (state.m_top > 1) {
            --state.m_top;
        }
        return;
    }
    m_recording->m_field22 = 1;
    SetTextComponent(level, command, arguments, count);
}

inline void CGraphics2d_Lite_OGLES::ConsiderDisplayProgramAutoAdvanceLayerAndUpdateInfo()
{
    int stateCount = m_priorityStacks.m_stackCount - 1;
    if (m_previousComponents[0].m_flag4 != 0) {
        if (m_recording->m_currPos == IDisplayProgram::Mode_5 &&
            static_cast<unsigned int>(ComponentAtLevel(0).m_data[0]) == kDisable) {
            return;
        }
        int level = 0;
        for (; level < stateCount; ++level) {
            if (!(m_previousComponents[level] == ComponentAtLevel(level))) {
                break;
            }
        }
        if (level == stateCount) {
            return;
        }
        m_recording->SetLayer(m_recording->GetLayer() + 1);
    }
    for (int level = 0; level < stateCount; ++level) {
        m_previousComponents[level] = ComponentAtLevel(level);
    }
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

void CGraphics2d_Lite_OGLES::SetColor(int alpha, int red, int green, int blue)
{
    if (IsRecording()) {
        ConsiderAddingToDisplayList(kSetColor, 4, alpha, red, green, blue);
    }
    m_color.a = alpha;
    m_color.r = red;
    m_color.g = green;
    m_color.b = blue;
    m_colorWord = (static_cast<unsigned int>(alpha >> 8) & 0xff) << 24;
    m_colorWord |= (static_cast<unsigned int>(red >> 8) & 0xff) << 16;
    m_colorWord |= (static_cast<unsigned int>(green >> 8) & 0xff) << 8;
    m_colorWord |= static_cast<unsigned int>(blue >> 8) & 0xff;
    // Full intensity is exactly 0x10000.
    if (alpha == kFixedOne) m_colorWord |= 0xff000000;
    if (red == kFixedOne) m_colorWord |= 0x00ff0000;
    if (green == kFixedOne) m_colorWord |= 0x0000ff00;
    if (blue == kFixedOne) m_colorWord |= 0x000000ff;
}

void CGraphics2d_Lite_OGLES::SetColor(const Color_ARGB_fixed& color)
{
    SetColor(color.a, color.r, color.g, color.b);
}

void CGraphics2d_Lite_OGLES::SetColor(unsigned int argb)
{
    m_colorWord = argb;
    m_color.Make(ColorFormat_A8R8G8B8, &argb);
}

void CGraphics2d_Lite_OGLES::Enable(State state)
{
    if (IsRecording()) {
        ConsiderAddingToDisplayList(kEnable, 1, state);
    }
    if (state != State_2) {
        m_states[state] = true;
    }
}

void CGraphics2d_Lite_OGLES::Disable(State state)
{
    if (IsRecording()) {
        ConsiderAddingToDisplayList(kDisable, 1, state);
    }
    if (state != State_2) {
        m_states[state] = false;
    }
}

void CGraphics2d_Lite_OGLES::PushState(State state)
{
    if (IsRecording()) {
        ConsiderAddingToDisplayList(kPushState, 1, state);
    }
    if (state == State_2) {
        m_colorStack.Add(m_color);
        m_colorWordStack.Add(m_colorWord);
    } else {
        m_stateStack[state].Add(m_states[state]);
        if (state == State_Blend) {
            m_blendStack.Add(m_blendArg);
        }
    }
}

void CGraphics2d_Lite_OGLES::PopState(State state)
{
    if (IsRecording()) {
        ConsiderAddingToDisplayList(kPopState, 1, state);
    }
    if (state == State_2) {
        if (m_colorStack.Size() != 0) {
            m_color = m_colorStack[--m_colorStack.m_size];
            m_colorWord = m_colorWordStack[--m_colorWordStack.m_size];
        }
    } else if (m_stateStack[state].Size() != 0) {
        TCVector<int>& stack = m_stateStack[state];
        m_states[state] = stack[--stack.m_size] != 0;
        if (state == State_Blend) {
            m_blendArg = m_blendStack[--m_blendStack.m_size];
        }
    }
}

void CGraphics2d_Lite_OGLES::SetBlendArg(BlendArg arg)
{
    if (IsRecording()) {
        ConsiderAddingToDisplayList(kSetBlend, 1, arg);
    }
    m_blendArg = arg;
}

void CGraphics2d_Lite_OGLES::LoadIdentity()
{
    m_transform = Matrix();
}

void CGraphics2d_Lite_OGLES::LoadTransform(const CMatrix2d* matrix, const CVector2d* translation)
{
    // The CK 2D interface uses the fixed-point words of these math types.
    m_transform.m[0][0] = FixedWord(&matrix->m[0]);
    m_transform.m[1][0] = FixedWord(&matrix->m[1]);
    m_transform.m[0][1] = FixedWord(&matrix->m[2]);
    m_transform.m[1][1] = FixedWord(&matrix->m[3]);
    m_transform.m[2][0] = FixedWord(&translation->m_x);
    m_transform.m[2][1] = FixedWord(&translation->m_y);
}

void CGraphics2d_Lite_OGLES::MultiplyTransform(const CMatrix2d* matrix, const CVector2d* translation)
{
    m_transform.multiply(FixedWord(&matrix->m[0]), FixedWord(&matrix->m[2]),
                         FixedWord(&matrix->m[1]), FixedWord(&matrix->m[3]),
                         FixedWord(&translation->m_x), FixedWord(&translation->m_y));
}

void CGraphics2d_Lite_OGLES::Translate(int x, int y)
{
    m_transform.multiply(kFixedOne, 0, 0, kFixedOne, x, y);
}

void CGraphics2d_Lite_OGLES::Rotate(int angle)
{
    const float radians = angle * (3.14159265358979323846f / (180.0f * kFixedOne));
    const int sine = static_cast<int>(sinf(radians) * kFixedOne);
    const int cosine = static_cast<int>(cosf(radians) * kFixedOne);
    m_transform.multiply(cosine, -sine, sine, cosine, 0, 0);
}

void CGraphics2d_Lite_OGLES::Scale(int x, int y)
{
    m_transform.multiply(x, 0, 0, y, 0, 0);
}

void CGraphics2d_Lite_OGLES::PushTransform()
{
    m_transformStack.m_data[m_transformStack.m_top - 1] = m_transform;
    m_transformStack.PushTop();
}

void CGraphics2d_Lite_OGLES::PopTransform()
{
    if (m_transformStack.m_top > 1) {
        --m_transformStack.m_top;
        m_transform = m_transformStack.m_data[m_transformStack.m_top - 1];
    }
}

void CGraphics2d_Lite_OGLES::GetTransform(const CMatrix2d** matrix, const CVector2d** translation)
{
    if (matrix != NULL) {
        StoreFixedWord(&m_returnMatrix.m[0], m_transform.m[0][0]);
        StoreFixedWord(&m_returnMatrix.m[1], m_transform.m[1][0]);
        StoreFixedWord(&m_returnMatrix.m[2], m_transform.m[0][1]);
        StoreFixedWord(&m_returnMatrix.m[3], m_transform.m[1][1]);
        *matrix = &m_returnMatrix;
    }
    if (translation != NULL) {
        StoreFixedWord(&m_returnTranslation.m_x, m_transform.m[2][0]);
        StoreFixedWord(&m_returnTranslation.m_y, m_transform.m[2][1]);
        *translation = &m_returnTranslation;
    }
}

void CGraphics2d_Lite_OGLES::SetVertexPositionCoords(int componentCount, unsigned int type, int stride,
                                                     const void* coords)
{
    m_coordCount = componentCount;
    m_coordType = type;
    m_coordStride = stride;
    m_coords = static_cast<const unsigned char*>(coords);
}

void CGraphics2d_Lite_OGLES::SetClip(unsigned short x, unsigned short y, unsigned short width,
                                      unsigned short height)
{
    m_clip.m_x = x;
    m_clip.m_y = y;
    m_clip.m_width = width;
    m_clip.m_height = height;
    if (IsRecording()) {
        ConsiderAddingToDisplayList(kSetClip, 4, x, y, width, height);
    }
    UpdateActiveClipRect_HW();
}

void CGraphics2d_Lite_OGLES::SetClip(const CRectangle& clip)
{
    m_clip = clip;
    if (IsRecording()) {
        ConsiderAddingToDisplayList(kSetClip, 4, clip.m_x, clip.m_y, clip.m_width, clip.m_height);
    }
    UpdateActiveClipRect_HW();
}

void CGraphics2d_Lite_OGLES::UpdateActiveClipRect_HW()
{
    ICRenderSurface* target = ICGraphics::GetInstance()->GetTargetSurface();
    if (target == NULL) {
        return;
    }
    unsigned int width = 0;
    unsigned int height = 0;
    target->GetWidthAndHeight(width, height);
    m_target.m_x = 0;
    m_target.m_y = 0;
    m_target.m_width = width;
    m_target.m_height = height;
    m_activeClip = m_target;
    m_activeClip.Clip(m_clip);
    if (!IsRecording() && m_rendering) {
        ApplyClip(m_clip);
    }
}

const CRectangle& CGraphics2d_Lite_OGLES::GetClip() const
{
    return m_clip;
}

CGraphics2d_Lite_OGLES::Vertex CGraphics2d_Lite_OGLES::TransformVertex(float x, float y, float u, float v) const
{
    Vertex vertex;
    const float unit = kFixedOne;
    vertex.x = (m_transform.m[0][0] * x + m_transform.m[0][1] * y + m_transform.m[2][0]) / unit;
    vertex.y = (m_transform.m[1][0] * x + m_transform.m[1][1] * y + m_transform.m[2][1]) / unit;
    vertex.u = u;
    vertex.v = v;
    return vertex;
}

CGraphics2d_Lite_OGLES::Vertex CGraphics2d_Lite_OGLES::ReadVertex(int index) const
{
    const unsigned char* source = m_coords + index * m_coordStride;
    Vertex vertex = { 0.0f, 0.0f, 0.0f, 0.0f };
    switch (m_coordType) {
    case kByteVertex:
        vertex.x = static_cast<signed char>(source[0]);
        vertex.y = static_cast<signed char>(source[1]);
        break;
    case kUnsignedByteVertex:
        vertex.x = source[0];
        vertex.y = source[1];
        break;
    case kShortVertex: {
        short point[2];
        memcpy(point, source, sizeof(point));
        vertex.x = point[0];
        vertex.y = point[1];
        break;
    }
    case kUnsignedShortVertex: {
        unsigned short point[2];
        memcpy(point, source, sizeof(point));
        vertex.x = point[0];
        vertex.y = point[1];
        break;
    }
    case kFixedVertex:
        vertex.x = FixedWord(source) / static_cast<float>(kFixedOne);
        vertex.y = FixedWord(source + sizeof(int)) / static_cast<float>(kFixedOne);
        break;
    }
    return vertex;
}

void CGraphics2d_Lite_OGLES::SetCommandState(DrawCommand& command, GLenum primitive, GLuint texture) const
{
    command.primitive = primitive;
    command.texture = texture;
    command.color = m_color;
    command.clip = m_clip;
    command.blendArg = m_blendArg;
    command.blend = m_states[State_Blend];
    command.alphaTest = m_states[State_AlphaTest];
}

void CGraphics2d_Lite_OGLES::ApplyClip(const CRectangle& clip) const
{
    if (clip.m_x == 0 && clip.m_y == 0 && clip.m_width == m_target.m_width &&
        clip.m_height == m_target.m_height) {
        glDisable(GL_SCISSOR_TEST);
        return;
    }

    CRectangle bounds;
    if (&clip == &m_clip) {
        bounds = m_activeClip;
    } else {
        bounds = m_target;
        bounds.Clip(clip);
    }
    glEnable(GL_SCISSOR_TEST);
    if (bounds.m_width == 0 || bounds.m_height == 0) {
        glScissor(0, 0, 0, 0);
        return;
    }

    if (m_origin == Origin_0) {
        glScissor(bounds.m_x, bounds.m_y, bounds.m_width, bounds.m_height);
        return;
    }

    CGraphics_OGLES* hardware = CGraphics_OGLES::GetHardware();
    ICCore::eDeviceOrientation orientation = ICCore::DeviceOrientation_Portrait;
    if (hardware->m_target != NULL && (hardware->m_target->m_flags & ICRenderSurface::Flag_Targetable) &&
        CApplet::m_pCore->m_deviceOrientationSet) {
        orientation = CApplet::m_pCore->m_deviceOrientation;
    }
    const int x = bounds.m_x;
    const int y = bounds.m_y;
    const int width = bounds.m_width;
    const int height = bounds.m_height;
    switch (orientation) {
    case ICCore::DeviceOrientation_LandscapeRight:
        glScissor(y, x, height, width);
        break;
    case ICCore::DeviceOrientation_PortraitUpsideDown:
        glScissor(m_target.m_width - x - width, y, width, height);
        break;
    case ICCore::DeviceOrientation_LandscapeLeft:
        glScissor(m_target.m_height - y - height, m_target.m_width - x - width, height, width);
        break;
    case ICCore::DeviceOrientation_Portrait:
        glScissor(x, m_target.m_height - y - height, width, height);
        break;
    default:
        break;
    }
}

void CGraphics2d_Lite_OGLES::ApplyCommandState(const DrawCommand& command) const
{
    ApplyClip(command.clip);
    SetCapability(GL_ALPHA_TEST, command.alphaTest);
    SetCapability(GL_BLEND, command.blend);
    switch (command.blendArg) {
    case BlendArg_0:
    case BlendArg_2:
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        break;
    case BlendArg_1:
    case BlendArg_3:
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        break;
    case BlendArg_4:
        glBlendFunc(GL_ONE, GL_ONE);
        break;
    }
    glColor4x(command.color.r, command.color.g, command.color.b, command.color.a);
}

void CGraphics2d_Lite_OGLES::Execute(const DrawCommand& command, const Vertex* vertices) const
{
    ApplyCommandState(command);
    if (command.texture != 0) {
        GLint environment = GL_REPLACE;
        if (command.blend && command.blendArg < BlendArg_2) {
            environment = GL_MODULATE;
        }
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, environment);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, command.texture);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), &vertices[0].u);
    } else {
        glDisable(GL_TEXTURE_2D);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }
    glVertexPointer(2, GL_FLOAT, sizeof(Vertex), &vertices[0].x);
    if (command.primitive == GL_TRIANGLE_STRIP) {
        for (int first = 0; first < command.count; first += 4) {
            glDrawArrays(GL_TRIANGLE_STRIP, first, 4);
        }
    } else {
        glDrawArrays(command.primitive, 0, command.count);
    }
}

void CGraphics2d_Lite_OGLES::Submit(DrawCommand& command)
{
    command.count = m_vertices.Size();
    if (!IsRecording()) {
        if (m_rendering) {
            Execute(command, m_vertices.m_data);
        }
        return;
    }

    bool triangleFan = command.primitive == GL_TRIANGLE_FAN;
    bool quads = command.primitive == GL_TRIANGLE_STRIP;
    int vertexCount = command.count;
    if (triangleFan) {
        if (vertexCount < 3) {
            return;
        }
        vertexCount = 3 * (vertexCount - 2);
    }
    const bool textured = command.texture != 0;
    const bool depth = m_recording->m_recordingMode == IDisplayProgram::Mode_2 ||
                       m_recording->m_recordingMode == IDisplayProgram::Mode_5;
    const CVertexAttribute::Decl* attributes;
    const int* lengths;
    if (depth) {
        attributes = textured ? DepthTextureVertices : DepthVertices;
        lengths = DepthComponentLengths;
    } else {
        attributes = textured ? CompactTextureVertices : CompactVertices;
        lengths = CompactComponentLengths;
    }
    const int stride = attributes[0].m_stride;

    TCStack<Component>& textures = m_priorityStacks.m_stacks[2];
    textures.PushTop();
    Component& texture = ComponentAtLevel(2);
    texture.m_flag4 = 1;
    texture.m_flag5 = 0x20;
    texture.m_data[0] = command.texture;
    Component alphaState = ComponentAtLevel(5);
    if (command.alphaTest != m_states[State_AlphaTest]) {
        const int alpha = State_AlphaTest;
        SetTextComponent(5, command.alphaTest ? kEnable : kDisable, &alpha, 1);
    }
    if (m_recording->m_currPos == IDisplayProgram::Mode_4 || m_recording->m_currPos == IDisplayProgram::Mode_5) {
        ConsiderDisplayProgramAutoAdvanceLayerAndUpdateInfo();
    }
    int kind = 0x40;
    int verticesPerPrimitive = 3;
    if (command.primitive == GL_LINES) {
        kind = 0x30;
        verticesPerPrimitive = 2;
    }
    if (textured) {
        kind |= 2;
    }
    unsigned int indexType = 0;
    int indexBytes = 0;
    if (quads) {
        verticesPerPrimitive = 4;
        indexType = kUnsignedShortVertex;
        indexBytes = (vertexCount / 4) * 6 * sizeof(GLushort);
    }
    CVertexBuffer* buffer = m_priorityStacks.AquireVtxBuffer(
        *m_recording, lengths, kind, vertexCount * stride, command.texture, attributes, indexBytes, indexType);
    if (buffer != NULL) {
        int firstVertex = (buffer->m_pool.m_cursor - buffer->m_pool.m_data) / stride;
        for (int index = 0; index < vertexCount; ++index) {
            int source = index;
            if (triangleFan) {
                const int triangle = index / 3;
                const int corner = index % 3;
                source = 0;
                if (corner != 0) {
                    source = triangle + corner;
                }
            }
            const Vertex& vertex = m_vertices[source];
            short* position = reinterpret_cast<short*>(buffer->m_pool.m_cursor);
            position[0] = static_cast<short>(vertex.x);
            position[1] = static_cast<short>(vertex.y);
            if (depth) {
                unsigned int layer = m_recording->GetLayer();
                position[2] = static_cast<short>(0x8001u + ((layer << 12) | m_primitiveIndex));
            }
            if (textured) {
                short* coordinate = reinterpret_cast<short*>(buffer->m_pool.m_cursor + attributes[1].m_field4);
                coordinate[0] = static_cast<short>(vertex.u * 4096.0f);
                coordinate[1] = static_cast<short>(vertex.v * 4096.0f);
            }
            buffer->m_pool.m_cursor += stride;
            if (index % verticesPerPrimitive == verticesPerPrimitive - 1) {
                ++m_primitiveIndex;
            }
        }
        if (quads) {
            CIndexVertexPool& pool = static_cast<CIndexVertexBuffer*>(buffer)->m_indices;
            GLushort* indices = reinterpret_cast<GLushort*>(pool.m_cursor);
            for (int index = firstVertex; index < firstVertex + vertexCount; index += 4) {
                indices[0] = index;
                indices[1] = index + 1;
                indices[2] = index + 2;
                indices[3] = index + 2;
                indices[4] = index + 1;
                indices[5] = index + 3;
                indices += 6;
            }
            pool.m_cursor += indexBytes;
        }
    }
    ComponentAtLevel(5) = alphaState;
    --textures.m_top;
}

void CGraphics2d_Lite_OGLES::Draw(ICRenderSurface& surface, Flip flip, const CRectangle* source)
{
    if (!(surface.m_flags & ICRenderSurface::Flag_Initialized) ||
        surface.GetAbstraction() != ICGraphicsAbstract::Abstraction_Hardware) {
        return;
    }
    CRenderSurface_OGLES_Texture& texture = static_cast<CRenderSurface_OGLES_Texture&>(surface);
    CRenderSurface_OGLES_Texture* owner = texture.GetSourceTexture();
    CRenderSurfaceBuffer* buffer = owner->m_buffers[CRenderSurface::Buffer_Color];
    if (buffer == NULL || owner->m_textureCount == 0) {
        return;
    }
    if (m_states[State_4]) {
        HandleConfigStateBasedOnSrcFormat(buffer->m_format);
    }

    const ICGraphics::Description& description = CGraphics_OGLES::GetHardware()->m_description;
    const int tileWidth = description.m_maxRenderTargetWidth;
    const int tileHeight = description.m_maxRenderTargetHeight;
    if (tileWidth <= 0 || tileHeight <= 0) {
        return;
    }
    const int imageWidth = buffer->m_imageWidth;
    const int imageHeight = buffer->m_imageHeight;
    int sourceX = 0;
    int sourceY = 0;
    int sourceWidth = imageWidth;
    int sourceHeight = imageHeight;
    if (source != NULL) {
        sourceX = source->m_x;
        sourceY = source->m_y;
        sourceWidth = source->m_width;
        sourceHeight = source->m_height;
    }
    const bool flipX = (flip & Flip_Horizontal) != 0;
    const bool flipY = (flip & Flip_Vertical) != 0;
    const bool powerOfTwo = (texture.m_flags & ICRenderSurface::Flag_PowerOfTwo) != 0;

    int index = 0;
    for (int y = 0; y < imageHeight; y += tileHeight) {
        const int height = Min(tileHeight, imageHeight - y);
        const int top = Max(y, sourceY);
        const int bottom = Min(y + height, sourceY + sourceHeight);
        for (int x = 0; x < imageWidth; x += tileWidth, ++index) {
            const int width = Min(tileWidth, imageWidth - x);
            const int left = Max(x, sourceX);
            const int right = Min(x + width, sourceX + sourceWidth);
            if (left >= right || top >= bottom || index >= owner->m_textureCount) {
                continue;
            }
            const GLuint name = owner->GetTextureName(index);
            if (name == 0) {
                continue;
            }
            const float textureWidth = TextureExtent(width, powerOfTwo);
            const float textureHeight = TextureExtent(height, powerOfTwo);
            const float u0 = (left - x) / textureWidth;
            const float v0 = (top - y) / textureHeight;
            const float u1 = (right - x) / textureWidth;
            const float v1 = (bottom - y) / textureHeight;
            float x0 = left - sourceX;
            float x1 = right - sourceX;
            float y0 = top - sourceY;
            float y1 = bottom - sourceY;
            if (flipX) {
                x0 = sourceWidth - x0;
                x1 = sourceWidth - x1;
            }
            if (flipY) {
                y0 = sourceHeight - y0;
                y1 = sourceHeight - y1;
            }

            DrawCommand command;
            SetCommandState(command, GL_TRIANGLE_STRIP, name);
            command.color.r = kFixedOne;
            command.color.g = kFixedOne;
            command.color.b = kFixedOne;
            m_vertices.Clear();
            m_vertices.Add(TransformVertex(x0, y0, u0, v0));
            m_vertices.Add(TransformVertex(x1, y0, u1, v0));
            m_vertices.Add(TransformVertex(x0, y1, u0, v1));
            m_vertices.Add(TransformVertex(x1, y1, u1, v1));
            Submit(command);
        }
    }
}

void CGraphics2d_Lite_OGLES::Draw(Primitive primitive, int first, int count)
{
    if (m_coords == NULL || m_coordCount < 2 || count <= 0) {
        return;
    }
    if (m_coordType != kByteVertex && m_coordType != kUnsignedByteVertex &&
        m_coordType != kShortVertex && m_coordType != kUnsignedShortVertex && m_coordType != kFixedVertex) {
        return;
    }
    if (m_states[State_4]) {
        RSBFormat format = (RSBFormat)0xb0304;
        if (m_color.a < 0xfae2) {
            format = RSBFormat_A8R8G8B8;
        }
        HandleConfigStateBasedOnSrcFormat(format);
    }
    if ((m_states[State_AlphaTest] && m_color.a == 0) ||
        (m_states[State_ColorKey] && (m_colorWord & 0xffffff) == 0xff00ff)) {
        return;
    }

    switch (primitive) {
    case Primitive_Lines:
        DrawHardwareLines(first, count);
        break;
    case Primitive_Polygon:
        DrawHardwarePolygon(first, count);
        break;
    case Primitive_Rectangles:
        DrawHardwareRectangles(first, count);
        break;
    }
}

void CGraphics2d_Lite_OGLES::DrawUntexturedVertices(GLenum primitive, int first, int count)
{
    DrawCommand command;
    SetCommandState(command, primitive, 0);
    m_vertices.Clear();
    m_vertices.EnsureCapacity(count);
    for (int index = first; index < first + count; ++index) {
        const Vertex point = ReadVertex(index);
        m_vertices.Add(TransformVertex(point.x, point.y, 0, 0));
    }
    command.alphaTest = false;
    Submit(command);
}

void CGraphics2d_Lite_OGLES::DrawHardwareLines(int first, int count)
{
    DrawUntexturedVertices(GL_LINES, first, count);
}

void CGraphics2d_Lite_OGLES::DrawHardwarePolygon(int first, int count)
{
    DrawUntexturedVertices(GL_TRIANGLE_FAN, first, count);
}

void CGraphics2d_Lite_OGLES::DrawHardwareRectangles(int first, int count)
{
    DrawCommand command;
    SetCommandState(command, GL_TRIANGLE_STRIP, 0);
    m_vertices.Clear();
    m_vertices.EnsureCapacity((count / 2) * 4);
    for (int index = first; index + 1 < first + count; index += 2) {
        const Vertex upper = ReadVertex(index);
        const Vertex lower = ReadVertex(index + 1);
        const Vertex a = TransformVertex(upper.x, upper.y, 0, 0);
        const Vertex b = TransformVertex(lower.x, upper.y, 0, 0);
        const Vertex c = TransformVertex(upper.x, lower.y, 0, 0);
        const Vertex d = TransformVertex(lower.x, lower.y, 0, 0);
        m_vertices.Add(a);
        m_vertices.Add(b);
        m_vertices.Add(c);
        m_vertices.Add(d);
    }
    command.alphaTest = false;
    if (m_vertices.Size() != 0) {
        Submit(command);
    }
}

void CGraphics2d_Lite_OGLES::HandleConfigStateBasedOnSrcFormat(RSBFormat format)
{
    switch (format) {
    case 0x45402:
    case 0x55402:
        Enable(State_AlphaTest);
        Disable(State_Blend);
        break;
    case 0x65402:
    case 0x75402:
    case 0xc4404:
    case 0xd4404:
    case 0xe4404:
    case 0xf4404:
    case 0x803d5400:
    case 0x80475400:
    case 0x80495400:
        Enable(State_AlphaTest);
        Enable(State_Blend);
        SetBlendArg(BlendArg_2);
        break;
    default:
        Disable(State_AlphaTest);
        Disable(State_Blend);
        break;
    }
}
