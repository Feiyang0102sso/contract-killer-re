// CNGSLoginFlow.cpp - gServe login step scheduling and event dispatch.
#include "CNGSLoginFlow.h"
#include "CNGS.h"
#include "CNGSLocalUser.h"
#include "CNGSUtil.h"
#include "CNotificationHandler.h"
#include "GameCenter_Facade.h"
#include <string.h>

enum {
    DefaultStepTimeoutMs = 180000,
    MinimumStepTimeoutMs = 120000,
    NetworkTimeStepId = 0x81a8233f,
    SocialNetworkStepId = 0x90e4b4fa,
    FriendsListStepId = 0xe0f9e913
};

const char* eventDescriptions[] = {
    "LFW_None", "LFW_Success", "LFW_Cancel", "LFW_Fail", "LFW_Paused",
    "LFW_Resume", "LFW_ForceStopped", "LFW_VersionSuccess", "LFW_VersionInvalid",
    "LFW_SessionInvalid", "LFW_ServeConnectionError", "LFW_NetworkConnectionError",
    "LFW_AccountConflict", "LFW_PrimaryUserChanged", "LFW_Timeout"
};

CNGSLoginFlow::CNGSLoginFlow() : CSingleton(ClassId), m_currentStep(-1)
{
    m_activeNetwork = -1;
    m_remainingTimeout = 0;
    m_timeoutReported = 0;
    m_timeoutsEnabled = 0;
    Reset();
}

void CNGSLoginFlow::Reset()
{
    CNGSUtil::DebugLog("Login Flow - Reset");
    m_currentStep = -1;
    m_remainingTimeout = 0;
    m_timeoutReported = 0;
    for (int index = 0; index < m_steps.Size(); index++) {
        if (m_steps[index] != NULL) {
            delete m_steps[index];
            m_steps[index] = NULL;
        }
    }
    m_steps.Clear();
}

CNGSLoginFlow::~CNGSLoginFlow()
{
    Reset();
}

unsigned char CNGSLoginFlow::IsRunning()
{
    return m_currentStep >= 0 && m_currentStep < m_steps.Size();
}

void CNGSLoginFlow::Queue(CNGSLoginStep* step)
{
    m_steps.Add(step);
}

void CNGSLoginFlow::Start()
{
    CNGSUtil::DebugLog("Login Flow - Started");
    if (m_steps.Size() > 0) {
        PrepareStep(0);
    }
}

void CNGSLoginFlow::PrepareStep(int step)
{
    CNGSUtil::DebugLog("Login Flow - PrepareStep %i, %i", step, m_currentStep);
    if (step != -1) {
        m_currentStep = step;
    } else {
        step = m_currentStep;
    }
    if (step < 0 || step >= m_steps.Size()) {
        return;
    }
    if (m_timeoutsEnabled) {
        m_timeoutReported = 0;
        m_remainingTimeout = m_steps[step]->GetTimeout();
        if (m_remainingTimeout == -1) {
            m_remainingTimeout = DefaultStepTimeoutMs;
        } else if (m_remainingTimeout < (int)MinimumStepTimeoutMs) {  // signed in the original (bgt)
            m_remainingTimeout = MinimumStepTimeoutMs;
        }
    }
    CNGSUtil::DebugLog("Login Flow - Preparing %s", m_steps[m_currentStep]->GetName());
    m_steps[m_currentStep]->Prepare();
}

void CNGSLoginFlow::OnEvent(eLoginFlowEvent event, const char* caller)
{
    if (IsRunning()) {
        CNGSUtil::DebugLog("CNGSLoginFlow::OnEvent[%d]: %s, %s", m_currentStep,
                          eventDescriptions[event + 1], caller);
        if (!m_steps[m_currentStep]->isEventValid(event, caller)) {
            CNGSUtil::DebugLog("CNGSLoginFlow::OnEvent: Invalid");
            return;
        }
    }
    if (IsRunning()) {
        if (event == LoginFlowTimeout) {
            m_timeoutReported = 1;
            event = LoginFlowFail;
        }
        m_steps[m_currentStep]->OnEvent(event, caller);
    }
}

void CNGSLoginFlow::ExcuteStep(int step)
{
    CNGSUtil::DebugLog("Login Flow - ExecuteStep %i, %i", step, m_currentStep);
    if (step != -1) {
        m_currentStep = step;
    } else {
        step = m_currentStep;
    }
    if (step >= 0 && step < m_steps.Size()) {
        CNGSUtil::DebugLog("Login Flow - Executing %s", m_steps[step]->GetName());
        m_steps[m_currentStep]->Excute();
    }
}

void CNGSLoginFlow::NextStep()
{
    CNGSUtil::DebugLog("Login Flow - NextStep");
    if (IsRunning()) {
        m_timeoutReported = 0;
        m_currentStep++;
        CNGS::GetInstance()->GetLocalUser()->m_notificationHandler->HandleLoginFlowNextStep();
        if (m_currentStep != m_steps.Size()) {
            PrepareStep(m_currentStep);
        } else {
            CNGSUtil::DebugLog("Login Flow - Finished");
        }
    }
}

int CNGSLoginFlow::GetRemainingTimeout(int elapsedMs)
{
    int remaining = m_remainingTimeout - elapsedMs;
    if (remaining < 0) {
        remaining = 0;
    }
    m_remainingTimeout = remaining;
    return remaining;
}

CNGSLoginStep* CNGSLoginFlow::GetCurrentStep()
{
    if (m_currentStep < 0 || m_currentStep >= m_steps.Size()) {
        return NULL;
    }
    return m_steps[m_currentStep];
}

int CNGSLoginFlow::GetStepCount()
{
    return m_steps.Size();
}

CNGSLoginStep* CNGSLoginFlow::GetStep(int step)
{
    if (step < 0 || step >= m_steps.Size()) {
        return NULL;
    }
    return m_steps[step];
}

void CNGSLoginStep_CheckVersion::Excute()
{
    CNGS::GetInstance()->GetLocalUser()->CheckServerVersionCompatibility();
}

void CNGSLoginStep_CheckVersion::OnEvent(eLoginFlowEvent event, const char* caller)
{
    if (event == LoginFlowVersionSuccess) {
        CNGSLoginFlow::GetInstance()->NextStep();
    }
}

void CNGSLoginStep_SocialNetwork::Excute()
{
    if (m_snType == 1 && !GameCenter_Facade::GetInstance()->isGameCenterSupported()) {
        CNGSLoginFlow::GetInstance()->NextStep();
    }
    if (CNGS::GetInstance()->GetLocalUser()->isAuthenticated(m_snType)) {
        CNGSLoginFlow::GetInstance()->NextStep();
    } else {
        CNGS::GetInstance()->GetLocalUser()->Login(m_snType);
    }
}

void CNGSLoginStep_SocialNetwork::OnEvent(eLoginFlowEvent event, const char* caller)
{
    if (CNGSLoginFlow::GetInstance()->m_activeNetwork == m_snType) {
        CNGSLoginFlow::GetInstance()->NextStep();
    }
}

void CNGSLoginStep_LoadFriendsList::Excute()
{
    if (CNGS::GetInstance()->GetLocalUser()->isAuthenticated(m_snType)) {
        CNGS::GetInstance()->GetLocalUser()->LoadFriendListFromServer(m_snType);
    } else {
        CNGSLoginFlow::GetInstance()->NextStep();
    }
}

void CNGSLoginStep_LoadFriendsList::OnEvent(eLoginFlowEvent event, const char* caller)
{
    if (event == LoginFlowSuccess) {
        CNGSLoginFlow::GetInstance()->NextStep();
    }
}

bool CNGSLoginStep::isEventValid(eLoginFlowEvent event, const char* caller)
{
    const char* expectedCaller;
    switch (m_classId) {
    case NetworkTimeStepId:
        expectedCaller = "handleResponseNetworkTime";
        break;
    case SocialNetworkStepId:
        if (event == LoginFlowCancel) {
            return true;
        }
        expectedCaller = "CNGSHandleUserLogin";
        break;
    case FriendsListStepId:
        expectedCaller = "CNGSHandleFriendListUpdate";
        break;
    default:
        return true;
    }
    return strcmp(caller, expectedCaller) == 0;
}
