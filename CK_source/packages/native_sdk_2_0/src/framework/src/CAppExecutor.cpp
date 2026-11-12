// CAppExecutor.cpp - application main loop driver.
#include "CAppExecutor.h"

#include "CApplet.h"
#include "CApp.h"
#include "ICCore.h"
#include "CExecutableRegistry.h"
#include "CStdUtil_Cocoa.h"
#include "TapjoyInterface.h"

                                                                          

CAppExecutor::CAppExecutor()
{
    m_classId = ClassId;
    m_mode = Mode_Running;
    m_modeQualifier = ModeQualifier_Starting;
    m_registry = NULL;
    m_flag18 = 0;
    m_maxFrameMillis = 1000;
    m_frameMillis = 0;
    m_frameTime = 0;
    m_lastFrameMillis = 0;
}

CAppExecutor::~CAppExecutor()
{
    Destroy();
}

void CAppExecutor::Destroy()
{
    if (m_registry != NULL) {
        ReleaseRegistry();
        if (m_registry != NULL) {
            delete m_registry;
        }
        m_registry = NULL;
    }
}

unsigned char CAppExecutor::Init()
{
    m_registry = new CExecutableRegistry();
    if (m_registry == NULL) {
        return 0;
    }
    return InitRegistry() == 0;
}

void CAppExecutor::SetTimerMillis(int milliseconds)
{
    ICCore* core = (ICCore*)CApplet::m_pCore;
    core->SetTickRate(milliseconds);
}

void CAppExecutor::Run()
{
    int now = 0;
    int mode = m_mode;

    if (mode == Mode_Running) {
        now = CStdUtil_Cocoa::GetUpTimeMS();
        int elapsed = now - m_lastFrameMillis;
        int frameMillis = m_maxFrameMillis;
        if (elapsed < frameMillis) {
            frameMillis = elapsed;
        }
        m_frameMillis = frameMillis;
        m_frameTime = (frameMillis << 16) / 1000;
        TapjoyInterface::tick(frameMillis);
    }

    if (!TapjoyInterface::interfaceIsOpen()) {
        OnExecute();
    }

    if (mode == Mode_Running) {
        m_lastFrameMillis = now;
    }
}

void CAppExecutor::OnStart()
{
    SetMode(Mode_Running);
    SetModeQualifier(ModeQualifier_Starting);
    m_lastFrameMillis = CStdUtil_Cocoa::GetUpTimeMS();
    Run();
    SetModeQualifier(ModeQualifier_None);
}

void CAppExecutor::OnStop()
{
    SetMode(Mode_Running);
    SetModeQualifier(ModeQualifier_Stopping);
    Run();
}

void CAppExecutor::OnSuspend()
{
    SetMode(Mode_Running);
    SetModeQualifier(ModeQualifier_Suspending);
    Run();
}

void CAppExecutor::OnResume()
{
    SetMode(Mode_Resuming);
    SetModeQualifier(ModeQualifier_Resuming);
    m_lastFrameMillis = CStdUtil_Cocoa::GetUpTimeMS();
    Run();
    SetModeQualifier(ModeQualifier_None);
}

       
