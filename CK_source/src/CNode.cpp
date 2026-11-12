// CNode.cpp - lifecycle and rendering hooks of a CK application node.
#include "CNode.h"
#include "ICInAppPurchase.h"

CNode::CNode()
    : m_nodeId(0x02642625)
    , m_interrupt(0)
    , m_displayProgram(NULL)
    , m_displayMode(ICGraphics2d::IDisplayProgram::Mode_0)
{
}

CNode::~CNode()
{
    if (m_displayProgram != NULL) {
        delete m_displayProgram;
        m_displayProgram = NULL;
    }
}

void CNode::DestroyGraphicsOptimizations()
{
    if (m_displayProgram != NULL) {
        delete m_displayProgram;
        m_displayProgram = NULL;
    }
}

int CNode::InitGraphicsOptimizations(ICGraphics2d::IDisplayProgram::Mode mode,
                                     unsigned short capacity, unsigned int layers,
                                     unsigned short capacity12, unsigned short capacity10,
                                     unsigned short capacity14)
{
    ICGraphics::Abstraction abstraction = ICGraphics2d::GetInstance()->GetActiveAbstractionLayer();
    m_displayProgram = ICGraphics2d::IDisplayProgram::CreateInstance(abstraction);
    if (m_displayProgram == NULL) {
        return 0;
    }
    ICGraphics2d::IDisplayProgram::CreationInfo info;
    info.m_capacity0 = capacity;
    info.m_capacity1 = capacity;
    info.m_mode = 0;
    info.m_layers = layers;
    info.m_flag0c = capacity12 != 0;
    if (capacity12 != 0) {
        info.m_capacity10 = capacity10;
        info.m_capacity12 = capacity12;
        info.m_capacity14 = capacity14;
    }
    int result = m_displayProgram->Initialize(info);
    m_displayMode = mode;
    return result;
}

int CNode::Start()
{
    return 0;
}

void CNode::Stop()
{
}

void CNode::Activate()
{
}

void CNode::Deactivate()
{
}

int CNode::HandleEvent(unsigned int eventId, unsigned int param1, unsigned int param2)
{
    return 0;
}

int CNode::HandleUpdate(int elapsedMillis)
{
    return 0;
}

int CNode::HandleRender()
{
    return 0;
}

void CNode::SetInterrupt(int interrupt)
{
    if (m_interrupt == 0) {
        m_interrupt = interrupt;
    }
}

int CNode::GetInterrupt()
{
    return m_interrupt;
}

void CNode::ClearInterrupt()
{
    m_interrupt = 0;
}

void CNode::RenderBegin()
{
    ICGraphics* graphics = ICGraphics::GetInstance();
    ICGraphics2d* graphics2d = ICGraphics2d::GetInstance();
    graphics->RenderBegin(NULL);
    graphics2d->RenderBegin(1);
    graphics2d->Enable(ICGraphics2d::State_4);
    if (graphics2d->GetActiveAbstractionLayer() == ICGraphics::Abstraction_Software) {
        graphics2d->Enable(ICGraphics2d::State_ColorKey);
    }
    if (m_displayProgram != NULL) {
        m_displayProgram->SetLayer(0);
        graphics2d->DisplayListBegin(*m_displayProgram, m_displayMode);
    }
    SyncExternalSystemsWithRenderBegin();
}

void CNode::RenderEnd()
{
    ICGraphics* graphics = ICGraphics::GetInstance();
    ICGraphics2d* graphics2d = ICGraphics2d::GetInstance();
    if (m_displayProgram != NULL) {
        graphics2d->DisplayListEnd();
        graphics2d->Run(*m_displayProgram);
    }
    graphics2d->RenderEnd();
    graphics->RenderEnd();
}

void CNode::SyncExternalSystemsWithRenderBegin()
{
}
