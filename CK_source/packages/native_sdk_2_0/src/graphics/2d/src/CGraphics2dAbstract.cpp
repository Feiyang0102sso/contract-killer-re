// CGraphics2dAbstract.cpp - shared bitmap and system font drawing.
#include "CGraphics2d_Lite_OGLES.h"

#include "CGraphicFont.h"
#include "ARM_math.h"
#include "CRenderText2d.h"

CGraphics2dAbstract::DisplayProgram::DisplayProgram(ICGraphics::Abstraction abstraction)
{
    m_abstraction = abstraction;
    m_graphics = NULL;
}

CGraphics2dAbstract::DisplayProgram::~DisplayProgram()
{
    Destroy();
}

void CGraphics2dAbstract::DisplayProgram::Destroy()
{
    DestroyCore();
    m_graphics = NULL;
}

unsigned char CGraphics2dAbstract::DisplayProgram::Initialize(const CreationInfo& info)
{
    Destroy();

    unsigned char mode1;
    unsigned char mode2;
    if (info.m_mode == 1) {
        mode1 = 1;
        mode2 = 0;
    } else if (info.m_mode == 2) {
        mode2 = 1;
        mode1 = 0;
    } else {
        mode1 = 0;
        mode2 = 0;
    }
    InitializeCore(info.m_capacity0, info.m_capacity1, mode1, mode2, info.m_layers);
    m_graphics = ICGraphics2d::GetInstance();

    unsigned short special;
    if (info.m_flag0c == 0) {
        // sized from the device description
        const ICGraphics::Description* description = ICGraphics::GetInstance()->GetDescription(m_abstraction);
        m_textCapacity = description->m_maxVertexAttributes * 20;
        special = description->m_maxVertexAttributes;
    } else {
        m_textCapacity = info.m_capacity10 * info.m_capacity12;
        special = info.m_capacity14;
    }
    m_specialCapacity = special * 2;
    return 1;
}

void CGraphics2dAbstract::DisplayProgram::ExecuteText(Component* component)
{
    // the component data is a list of (command key, arguments) ending with the end key
    const int* data = component->m_data;
    for (;;) {
        const int* command = data;
        int key = *data;
        data = command + 1;
        switch (key) {
        case (int)0xdc7991b1:   // Enable
            m_graphics->Enable((ICGraphics2d::State)command[1]);
            data = command + 2;
            break;
        case (int)0xc6fb25f8:   // Disable
            m_graphics->Disable((ICGraphics2d::State)command[1]);
            data = command + 2;
            break;
        case (int)0xbb855bf6:   // PushState
            m_graphics->PushState((ICGraphics2d::State)command[1]);
            data = command + 2;
            break;
        case (int)0xa9b130f8:   // PopState
            m_graphics->PopState((ICGraphics2d::State)command[1]);
            data = command + 2;
            break;
        case (int)0x9b25d779:   // SetBlendArg
            m_graphics->SetBlendArg((ICGraphics2d::BlendArg)command[1]);
            data = command + 2;
            break;
        case (int)0x93e08c5f:   // SetColor
            m_graphics->SetColor(command[1], command[2], command[3], command[4]);
            data = command + 5;
            break;
        case (int)0xd13e0bbd:   // SetClip
            m_graphics->SetClip((unsigned short)command[1], (unsigned short)command[2],
                                (unsigned short)command[3], (unsigned short)command[4]);
            data = command + 5;
            break;
        case (int)0xbca5ef3b:   // end of the list
            return;
        }
    }
}

ICGraphics::Abstraction CGraphics2dAbstract::DisplayProgram::GetAbstraction()
{
    return m_abstraction;
}

void CGraphics2dAbstract::AbstractState::Initialize(unsigned int depth)
{
    m_alphaTest.Initialize(depth);
    m_blend.Initialize(depth);
    m_blendArg.Initialize(depth);
    m_state2.Initialize(depth);
    m_colorKey.Initialize(depth);
    m_state4.Initialize(depth);
}

void CGraphics2dAbstract::AbstractState::Destroy()
{
    m_alphaTest.Destroy();
    m_blend.Destroy();
    m_blendArg.Destroy();
    m_state2.Destroy();
    m_colorKey.Destroy();
    m_state4.Destroy();
}

void CGraphics2dAbstract::AbstractState::Push(ICGraphics2d::State state)
{
    switch (state) {
    case ICGraphics2d::State_AlphaTest:
        m_alphaTest.PushTop();
        break;
    case ICGraphics2d::State_Blend:
        m_blend.PushTop();
        m_blendArg.PushTop();
        break;
    case ICGraphics2d::State_2:
        m_state2.PushTop();
        break;
    case ICGraphics2d::State_ColorKey:
        m_colorKey.PushTop();
        break;
    case ICGraphics2d::State_4:
        m_state4.PushTop();
        break;
    }
}

void CGraphics2dAbstract::AbstractState::Pop(ICGraphics2d::State state)
{
    switch (state) {
    case ICGraphics2d::State_AlphaTest:
        m_alphaTest.m_top--;
        break;
    case ICGraphics2d::State_Blend:
        m_blend.m_top--;
        m_blendArg.m_top--;
        break;
    case ICGraphics2d::State_2:
        m_state2.m_top--;
        break;
    case ICGraphics2d::State_ColorKey:
        m_colorKey.m_top--;
        break;
    case ICGraphics2d::State_4:
        m_state4.m_top--;
        break;
    }
}

void CGraphics2dAbstract::Matrix::precat_R(Matrix* other)
{
    Matrix result;
    for (int column = 0; column < 2; column++) {
        for (int row = 0; row < 2; row++) {
            result.m[row][column] = smult16(other->m[0][column], m[row][0]) +
                                    smult16(other->m[1][column], m[row][1]);
        }
    }
    for (int column = 0; column < 2; column++) {
        for (int row = 0; row < 2; row++) {
            m[row][column] = result.m[row][column];
        }
    }
}

void CGraphics2dAbstract::Matrix::multiply(int a, int b, int c, int d, int tx, int ty)
{
    Matrix other;
    other.m[0][0] = a;
    other.m[0][1] = b;
    other.m[1][0] = c;
    other.m[1][1] = d;
    // the translation goes through the current linear part
    m[2][0] += smult16(tx, m[0][0]) + smult16(ty, m[0][1]);
    m[2][1] += smult16(tx, m[1][0]) + smult16(ty, m[1][1]);

    // linear part: this = this * other (precat_R written out)
    Matrix result;
    for (int column = 0; column < 2; column++) {
        for (int row = 0; row < 2; row++) {
            result.m[row][column] = smult16(other.m[0][column], m[row][0]) +
                                    smult16(other.m[1][column], m[row][1]);
        }
    }
    for (int column = 0; column < 2; column++) {
        for (int row = 0; row < 2; row++) {
            m[row][column] = result.m[row][column];
        }
    }
}

void CGraphics2dAbstract::Draw(ICRenderText2d& text)
{
    const CRenderText2d::TextModel& model = static_cast<CRenderText2d&>(text).m_model;
    if (!(model.m_flags & CRenderText2d::Flag_BitmapFont)) {
        if ((model.m_flags & CRenderText2d::Flag_SystemFont) && model.m_surface != NULL) {
            Draw(*model.m_surface, Flip_None, NULL);
        }
        return;
    }
    if (model.m_text == NULL) {
        return;
    }

    const CGraphicFont& font = *static_cast<const CGraphicFont*>(model.m_font);
    ICRenderSurface* atlas = font.m_image;
    const wchar_t* characters = static_cast<const wchar_t*>(model.m_text);
    ICGraphics2d* renderer = ICGraphics2d::GetInstance();
    int x = 0;
    for (unsigned int index = 0; index < model.m_length; ++index) {
        const CGraphicFont::ControlChar* control = font.GetControlChar(characters[index]);
        if (control != NULL) {
            x += control->m_advance + font.m_metrics->m_charSpacing;
            continue;
        }
        const CGraphicFont::Glyph* glyph = font.GetChar(characters[index]);
        if (glyph != NULL) {
            const CRectangle rectangle(glyph->m_x, glyph->m_y, glyph->m_width, glyph->m_height);
            PushTransform();
            Translate((x + glyph->m_offsetX) * 65536, glyph->m_offsetY * 65536);
            renderer->Draw(*atlas, Flip_None, &rectangle);
            PopTransform();
            x += glyph->m_advance + font.m_metrics->m_charSpacing;
        }
    }
}
