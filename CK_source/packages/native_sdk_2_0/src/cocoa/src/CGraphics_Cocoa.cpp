#include "CGraphics_HAL.h"

#include "CEvent.h"
#include "CEventListener.h"
#include "CGraphics_OGLES_Cocoa.h"
#include "CMathFixed.h"
#include "CStringToKey.h"
#include "ICCore.h"
#include "ICRenderSurface.h"

class CGraphics_Cocoa : public CGraphics_HAL {
public:
    virtual ~CGraphics_Cocoa();
    virtual unsigned char Initialize(const Param* params);
    virtual void Destroy();
    virtual void SwapBuffers();
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
    ICRenderSurface* window = device->m_window;
    if (window != NULL) {
        const ICRenderSurface::InitParam params[] = {
            { ICRenderSurface::InitParam_Window, 1 }, { ICRenderSurface::InitParam_Targetable, 1 },
            { ICRenderSurface::InitParam_ColorFormat, 0 }, { ICRenderSurface::InitParam_Multisample, device->m_samples },
            { 0, 0 }
        };
        window->Initialize(params);
    }
    device->m_window->Bind();
    return 1;
}

ICGraphics* ICGraphics::CreateInstance()
{
    return new CGraphics_Cocoa;
}

CGraphics_Cocoa::~CGraphics_Cocoa()
{
    // Preserve both vtable stores before the base destructor, as in the original.
    __asm__ __volatile__("" : : : "memory");
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
                m_softwareOnly = true;
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
    CEventListener* listener = new CEventListener();
    m_listener = listener;
    if (listener == NULL) {
        Destroy();
        return 0;
    }
    unsigned int listenerHandle = CStringToKey("Graphics", 0);
    listener->m_object = this;
    listener->m_handle = listenerHandle | 0x80000000;
    listener->m_callback = HandleEvent;
    m_listener->Register(EVENT_RESUME, 0x3fffffff);
    m_listener->Register(EVENT_ORIENTATION_CHANGED, 0x3fffffff);
    m_listener->Register(EVENT_SUSPEND, 0x3fffffff);

    InitializeHardware(params);
    if (!m_softwareReady && !m_hardwareReady) {
        Destroy();
        return 0;
    }
    Abstraction abstraction = Abstraction_Software;
    if (m_hardwareReady) {
        abstraction = Abstraction_Hardware;
    }
    SetDefaultActiveAbstractionLayer(abstraction);
    ICRenderSurface::Targetability targetability = ICRenderSurface::Targetability_1;
    if (m_hardwareReady) {
        targetability = ICRenderSurface::Targetability_2;
        if (m_softwareReady) {
            targetability = ICRenderSurface::Targetability_3;
        }
    }
    m_window = ICRenderSurface::CreateInstance(abstraction, ICRenderSurface::Type_Window, targetability);
    ICCore* core = CApplet::m_pCore;
    core->m_windowToDisplayScale.x = m_scaleX;
    core->m_windowToDisplayScale.y = m_scaleY;
    // The original reads the X scale before dividing Y, then divides X.
    int scaleX = m_scaleX;
    core->m_displayToWindowScale.y = CMathFixed::Div(0x10000, m_scaleY);
    __asm__ __volatile__("" : "+r"(scaleX) : : "memory");
    core->m_displayToWindowScale.x = CMathFixed::Div(0x10000, scaleX);

    if (m_window == NULL) {
        Destroy();
        return 0;
    }
    const ICRenderSurface::InitParam surfaceParams[] = {
        { ICRenderSurface::InitParam_Window, 1 }, { ICRenderSurface::InitParam_Targetable, 1 },
        { ICRenderSurface::InitParam_ColorFormat, 0 }, { ICRenderSurface::InitParam_Multisample, m_samples },
        { 0, 0 }
    };
    if (!m_window->Initialize(surfaceParams)) {
        Destroy();
        return 0;
    }
    m_window->Bind();
    ICRenderSurface::Description description;
    m_window->GetDescription(description);
    m_samples = description.m_multisample;
    SetDefaultColorFormat(m_window->GetColorFormat());
    SetClearColor(0, Color_ARGB_fixed::Black.r, Color_ARGB_fixed::Black.g, Color_ARGB_fixed::Black.b);
    SetClearDepth(CMathFixed::One);
    SetClearStencil(0);
    m_initialized = true;
    return 1;
}

void CGraphics_Cocoa::Destroy()
{
    if (m_window != NULL) {
        delete m_window;
        m_window = NULL;
    }
    DestroyHardware();
    if (m_listener != NULL) {
        m_listener->UnregisterAll();
        delete m_listener;
        m_listener = NULL;
    }
    m_softwareOnly = false;
    m_scaleX = 0x10000;
    m_scaleY = 0x10000;
    m_background = Color_A8R8G8B8_t::Black.value;
    m_windowFlag = false;
    m_initialized = false;
}

void CGraphics_Cocoa::SwapBuffers()
{
    if (m_target != NULL) {
        m_hardware->SwapBuffers();
    }
}
