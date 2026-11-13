// CGraphics2d_Lite_HAL.cpp - lifecycle and forwarding for the active CK 2D device.
#include "CGraphics2d_Lite_HAL.h"

typedef char CGraphics2d_Lite_HAL_ARMv7_Size[(sizeof(CGraphics2d_Lite_HAL) == 0x18) ? 1 : -1];

CGraphics2d_Lite_HAL::CGraphics2d_Lite_HAL()
{
    m_abstraction = ICGraphics::Abstraction_Hardware;
}

CGraphics2d_Lite_HAL::~CGraphics2d_Lite_HAL()
{
}

unsigned char CGraphics2d_Lite_HAL::Initialize()
{
    if (m_initialized) {
        Destroy();
    }
    InitializeHardware();
    if (!m_softwareReady && !m_hardwareReady) {
        Destroy();
        return 0;
    }
    SetActiveAbstractionLayer(m_hardwareReady ? ICGraphics::Abstraction_Hardware
                                              : ICGraphics::Abstraction_Software);
    m_initialized = 1;
    return 1;
}

void CGraphics2d_Lite_HAL::Destroy()
{
    if (m_device != NULL) {
        m_device->Destroy();
        delete m_device;
        m_device = NULL;
    }
    m_hardwareReady = 0;
    m_initialized = 0;
}

void CGraphics2d_Lite_HAL::SetActiveAbstractionLayer(ICGraphics::Abstraction)
{
    // CK's iOS build has one device.
}

ICGraphics::Abstraction CGraphics2d_Lite_HAL::GetActiveAbstractionLayer()
{
    return ICGraphics::Abstraction_Hardware;
}

void CGraphics2d_Lite_HAL::RenderBegin(unsigned char writeAlpha)
{
    m_device->RenderBegin(writeAlpha);
}

void CGraphics2d_Lite_HAL::RenderEnd()
{
    m_device->RenderEnd();
}

void CGraphics2d_Lite_HAL::DisplayListBegin(IDisplayProgram& program, IDisplayProgram::Mode mode)
{
    m_device->DisplayListBegin(program, mode);
}

void CGraphics2d_Lite_HAL::DisplayListEnd()
{
    m_device->DisplayListEnd();
}

void CGraphics2d_Lite_HAL::Run(IDisplayProgram& program)
{
    m_device->Run(program);
}

void CGraphics2d_Lite_HAL::SetOrigin(Origin origin)
{
    m_device->SetOrigin(origin);
}

void CGraphics2d_Lite_HAL::SetShaderMode(ShaderMode mode)
{
    m_device->SetShaderMode(mode);
}

void CGraphics2d_Lite_HAL::SetShader(ICShader* shader)
{
    m_device->SetShader(shader);
}

void CGraphics2d_Lite_HAL::SetColor(int alpha, int red, int green, int blue)
{
    m_device->SetColor(alpha, red, green, blue);
}

void CGraphics2d_Lite_HAL::SetColor(const Color_ARGB_fixed& color)
{
    m_device->SetColor(color);
}

void CGraphics2d_Lite_HAL::SetColor(unsigned int argb)
{
    m_device->SetColor(argb);
}

void CGraphics2d_Lite_HAL::Enable(State state)
{
    m_device->Enable(state);
}

void CGraphics2d_Lite_HAL::Disable(State state)
{
    m_device->Disable(state);
}

void CGraphics2d_Lite_HAL::PushState(State state)
{
    m_device->PushState(state);
}

void CGraphics2d_Lite_HAL::PopState(State state)
{
    m_device->PopState(state);
}

void CGraphics2d_Lite_HAL::SetBlendArg(BlendArg arg)
{
    m_device->SetBlendArg(arg);
}

void CGraphics2d_Lite_HAL::LoadIdentity()
{
    m_device->LoadIdentity();
}

void CGraphics2d_Lite_HAL::LoadTransform(const CMatrix2d* matrix, const CVector2d* translation)
{
    m_device->LoadTransform(matrix, translation);
}

void CGraphics2d_Lite_HAL::MultiplyTransform(const CMatrix2d* matrix, const CVector2d* translation)
{
    m_device->MultiplyTransform(matrix, translation);
}

void CGraphics2d_Lite_HAL::Translate(int x, int y)
{
    m_device->Translate(x, y);
}

void CGraphics2d_Lite_HAL::Rotate(int angle)
{
    m_device->Rotate(angle);
}

void CGraphics2d_Lite_HAL::Scale(int x, int y)
{
    m_device->Scale(x, y);
}

void CGraphics2d_Lite_HAL::PushTransform()
{
    m_device->PushTransform();
}

void CGraphics2d_Lite_HAL::PopTransform()
{
    m_device->PopTransform();
}

void CGraphics2d_Lite_HAL::GetTransform(const CMatrix2d** matrix, const CVector2d** translation)
{
    m_device->GetTransform(matrix, translation);
}

void CGraphics2d_Lite_HAL::SetVertexPositionCoords(int componentCount, unsigned int type, int stride,
                                 const void* coords)
{
    m_device->SetVertexPositionCoords(componentCount, type, stride, coords);
}

void CGraphics2d_Lite_HAL::SetClip(unsigned short x, unsigned short y, unsigned short width, unsigned short height)
{
    m_device->SetClip(x, y, width, height);
}

void CGraphics2d_Lite_HAL::SetClip(const CRectangle& clip)
{
    m_device->SetClip(clip);
}

const CRectangle& CGraphics2d_Lite_HAL::GetClip() const
{
    return m_device->GetClip();
}

void CGraphics2d_Lite_HAL::Draw(ICRenderSurface& surface, Flip flip, const CRectangle* source)
{
    m_device->Draw(surface, flip, source);
}

void CGraphics2d_Lite_HAL::Draw(ICRenderText2d& text)
{
    m_device->Draw(text);
}

void CGraphics2d_Lite_HAL::Draw(Primitive primitive, int first, int count)
{
    m_device->Draw(primitive, first, count);
}
