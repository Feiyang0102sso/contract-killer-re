#include "CGraphics_HAL.h"

#include "CEvent.h"
#include "CEventListener.h"
#include "CGraphics_OGLES_Cocoa.h"
#include "CStringToKey.h"
#include "ICCore.h"
#include "ICRenderSurface.h"

class CGraphics_Cocoa : public CGraphics_HAL {
public:
    virtual ~CGraphics_Cocoa();
    virtual unsigned char Initialize(const Param* params);
    virtual void Destroy();
    virtual void SwapBuffers();
    unsigned char InitializeWindowSurface();
};

static const unsigned int EVENT_RESUME = 0x75424363;
static const unsigned int EVENT_ORIENTATION_CHANGED = 0x747ab255;
static const unsigned int EVENT_SUSPEND = 0x32b482c3;

static unsigned char HandleEvent(CEvent& event, void* listener)
{
    if (event.m_eventId != EVENT_ORIENTATION_CHANGED) {
        return 0;
    }
    CGraphics_Cocoa* device = static_cast<CGraphics_Cocoa*>(static_cast<CEventListener*>(listener)->m_object);
    if (device->InitializeWindowSurface()) {
        device->m_window->Bind();
    }
    return 1;
}

ICGraphics* ICGraphics::CreateInstance()
{
    return new CGraphics_Cocoa();
}

CGraphics_Cocoa::~CGraphics_Cocoa()
{
}

unsigned char CGraphics_Cocoa::Initialize(const Param* params)
{
    if (m_initialized) {
        Destroy();
    }
    for (const Param* param = params; param->m_key != 0; param++) {
        switch (param->m_key) {
        case 2:
            if ((unsigned char)param->m_value != 0) {
                Destroy();
                return 0;
            }
            break;
        case 3:
            m_background = param->m_value;
            break;
        case 4:
            m_scaleX = param->m_value;
            break;
        case 5:
            m_scaleY = param->m_value;
            break;
        case 6:
            m_samples = param->m_value;
            break;
        }
    }
    if (m_scaleX == 0 || m_scaleY == 0) {
        Destroy();
        return 0;
    }

    m_listener = new CEventListener();
    m_listener->m_object = this;
    m_listener->m_handle = CStringToKey("Graphics", 0) | 0x80000000;
    m_listener->m_callback = HandleEvent;
    m_listener->Register(EVENT_RESUME, 0x3fffffff);
    m_listener->Register(EVENT_ORIENTATION_CHANGED, 0x3fffffff);
    m_listener->Register(EVENT_SUSPEND, 0x3fffffff);

    if (!InitializeHardware(params)) {
        Destroy();
        return 0;
    }
    SetDefaultActiveAbstractionLayer(Abstraction_Hardware);
    m_window = ICRenderSurface::CreateInstance(Abstraction_Hardware, ICRenderSurface::Type_Window,
                                              ICRenderSurface::Targetability_2);
    ICCore* core = CApplet::m_pCore;
    core->m_windowToDisplayScale.x = m_scaleX;
    core->m_windowToDisplayScale.y = m_scaleY;
    core->m_displayToWindowScale.x = (1LL << 32) / m_scaleX;
    core->m_displayToWindowScale.y = (1LL << 32) / m_scaleY;

    if (!InitializeWindowSurface()) {
        Destroy();
        return 0;
    }
    m_window->Bind();
    m_target = m_window;
    ICRenderSurface::Description description;
    m_window->GetDescription(description);
    m_samples = description.m_multisample;
    SetDefaultColorFormat(m_window->GetColorFormat());
    SetClearColor(0, 0, 0, 0);
    SetClearDepth(0x10000);
    SetClearStencil(0);
    m_initialized = true;
    return 1;
}

unsigned char CGraphics_Cocoa::InitializeWindowSurface()
{
    if (m_window == NULL) {
        return 0;
    }
    const ICRenderSurface::InitParam params[] = {
        { ICRenderSurface::InitParam_Window, 1 }, { ICRenderSurface::InitParam_Targetable, 1 },
        { ICRenderSurface::InitParam_ColorFormat, 0 }, { ICRenderSurface::InitParam_Multisample, m_samples }, { 0, 0 }
    };
    return m_window->Initialize(params);
}

void CGraphics_Cocoa::Destroy()
{
    delete m_window;
    m_window = NULL;
    m_target = NULL;
    DestroyHardware();
    if (m_listener != NULL) {
        m_listener->UnregisterAll();
        delete m_listener;
        m_listener = NULL;
    }
    ResetConfiguration();
}

void CGraphics_Cocoa::SwapBuffers()
{
    if (m_target != NULL) {
        m_hardware->SwapBuffers();
    }
}
