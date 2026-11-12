// CNGS.cpp - gServe root singleton.
#include <string.h>
#include "CNGS.h"
#include "CNGSFactory.h"
#include "CNGSServerRequest.h"
#include "CNGSSession.h"
#include "CNGSUserCredentials.h"
#include "CNGSLocalUser.h"
#include "CNGSFromServerMessageQ.h"
#include "CNGSContentManager.h"
#include "CNGSLoginFlow.h"
#include "CNGSLockManager.h"
#include "CProfileManager.h"
#include "CNGS_Platform.h"
#include "CNGSSocialInterface.h"
#include "CNotificationHandler.h"
#include "CNGSOfferManager.h"
#include "CNGSUtil.h"


/** Default period of the automatic server message poll (ms). */
static const unsigned int DEFAULT_AUTO_POLL_INTERVAL_MS = 120000;

/** Time since the last automatic message poll (ms). */
static unsigned int s_autoPollTimerMs = 0;
static const int LOGIN_STALL_WARNING_MS = 60000;
static const char* const SESSION_LOG_SEPARATOR =
    "##################################################################################################";

CNGS::CNGS()
    : CSingleton(ClassId)
    , m_factory(NULL)
    , m_autoPollIntervalMs(DEFAULT_AUTO_POLL_INTERVAL_MS)
{
    m_networkState = NETWORK_CONNECTED;
    m_serverURL = new char[1];
    m_serverURL[0] = '\0';
    m_localUser = NULL;
    m_profileManager = NULL;
    m_version = L"-1.-1.-1";
    CNGSUtil::DebugLog(SESSION_LOG_SEPARATOR);
}

CNGS::~CNGS()
{
    destroy();
}

void CNGS::destroy()
{
    CNGSServerRequest::DeleteAll();
    if (m_serverURL != NULL) {
        delete[] m_serverURL;
        m_serverURL = NULL;
    }
    if (m_factory != NULL) {
        delete m_factory;
        m_factory = NULL;
    }
    if (m_localUser != NULL) {
        delete m_localUser;
        m_localUser = NULL;
    }
    if (m_profileManager != NULL) {
        delete m_profileManager;
        m_profileManager = NULL;
    }
}

void CNGS::setVersion(CStrWChar& major, CStrWChar& minor, CStrWChar& build)
{
    m_version = major + L"." + minor + L"." + build;
}

CStrWChar CNGS::getVersionString()
{
    CStrWChar version;
    version += m_version;
    return version;
}

void CNGS::SetServerURL(const char* url)
{
    if (m_serverURL != NULL) {
        delete[] m_serverURL;
        m_serverURL = NULL;
    }
    int length = strlen(url);
    m_serverURL = new char[length + 2];
    strcpy(m_serverURL, url);
}

char* CNGS::GetServerURL()
{
    return m_serverURL;
}

CNGSLocalUser* CNGS::GetLocalUser()
{
    if (m_localUser == NULL) {
        CNGSFactory* factory = CNGS::GetInstance()->m_factory;
        if (factory != NULL) {
            CNGSUserCredentials credentials;
            CStrWChar fileName(CREDENTIAL_FILE_NAME);
            credentials.readFromFile(fileName, false);
            m_localUser = factory->createLocalUser(credentials);
        }
    }
    return m_localUser;
}

CNGSFactory* CNGS::GetFactory()
{
    return m_factory;
}

CProfileManager* CNGS::GetProfileManager()
{
    if (m_profileManager == NULL) {
        m_profileManager = new CProfileManager(NULL);
    }
    return m_profileManager;
}

void CNGS::SetFactory(CNGSFactory* factory)
{
    m_factory = factory;
}

void CNGS::SetNetworkConnectionState(eNetworkConnectionState state, const char* reason)
{
    eNetworkConnectionState previousState = m_networkState;
    m_networkState = state;
    CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
    if (previousState == NETWORK_CONNECTED) {
        if (m_networkState != NETWORK_CONNECTED) {
            user->CNGSHandleServerConnectionError(reason);
        }
    } else {
        if (m_networkState == NETWORK_CONNECTION_ERROR) {
            user->CNGSHandleServerConnectionError(reason);
        }
        if (m_networkState == NETWORK_CONNECTED) {
            user->CNGSHandleServerConnectionRestore(previousState);
        }
    }
}

eNetworkConnectionState CNGS::GetNetworkConnectionState()
{
    return m_networkState;
}

void CNGS::HandleUpdate(int elapsedMs)
{
    CNGSServerRequest::HandleUpdate(elapsedMs);
    CNGSContentManager::GetInstance()->HandleUpdate(elapsedMs);

    CNGSLocalUser* user = GetLocalUser();
    if (user == NULL) {
        return;
    }
    if (user->m_session != NULL) {
        user->m_session->tick(elapsedMs);
    }
    if (user->isAuthenticated(0) && user->m_session->isSessionKeyValid() == true &&
        !CNGSLoginFlow::GetInstance()->IsRunning()) {
        // Poll the server for new messages, otherwise send the queued acknowledgements.
        if (m_autoPollIntervalMs != 0 && s_autoPollTimerMs > m_autoPollIntervalMs &&
            user->isReady() && user->m_messageQueue->m_messageIdsToAck.Size() == 0) {
            if (user->m_messageQueue->SyncAllFromServer(NULL)) {
                s_autoPollTimerMs = 0;
                user->m_notificationHandler->HandleAutoPollStart();
            }
        } else {
            s_autoPollTimerMs += elapsedMs;
            user->m_messageQueue->AckMessages(NULL);
        }
        return;
    }
    if (CNGSLoginFlow::GetInstance()->IsRunning() && CNGSLoginFlow::GetInstance()->m_timeoutsEnabled) {
        int remaining = CNGSLoginFlow::GetInstance()->GetRemainingTimeout(elapsedMs);
        CNGSLoginFlow* flow = CNGSLoginFlow::GetInstance();
        if (remaining == 0) {
            flow->NextStep();
        } else if (!flow->m_timeoutReported && remaining < LOGIN_STALL_WARNING_MS) {
            CNGSLoginFlow::GetInstance()->OnEvent(LoginFlowTimeout, "Login Flow Stall Detected");
        }
    }
}

void CNGS::Pause()
{
    CNGSLocalUser* user = GetLocalUser();
    if (user != NULL && user->isValid()) {
        if (user->m_session != NULL) {
            user->m_session->clearNetworkTime();
        }
    }
    CNGSLockManager::GetInstance()->suspend();
}

void CNGS::Resume()
{
    CNGSLocalUser* user = GetLocalUser();
    if (user != NULL && user->isValid()) {
        CNGSSession* session = user->GetSession();
        if (session != NULL) {
            session->clearNetworkTime();
            session->SyncAllFromServer(NULL);
        }
    }
    if (user != NULL) {
        CNGS_Platform::GetInstance()->getSocialInterface(SOCIAL_INTERFACE_FACEBOOK)->resume();
        user->m_messageQueue->ProcessPromptMessages();
    }
    CNGSLockManager::GetInstance()->resume();
}

void CNGS::Reset(unsigned char logoutNetworks)
{
    if (m_localUser != NULL && logoutNetworks) {
        for (int network = 0; network <= 5; network++) {
            if (!m_localUser->isAuthenticated(network)) {
                continue;
            }
            if (network == SOCIAL_INTERFACE_GAME_CENTER) {
                CNGS_Platform::GetInstance()->getSocialInterface(network)->finishLogout();
            } else if (network != 0) {
                CNGS_Platform::GetInstance()->getSocialInterface(network)->logout();
            }
        }
    }
    CNGS_Platform::GetInstance()->Reset();
    COfferManager::GetInstance()->resetOffers();
    CNGSLoginFlow::GetInstance()->Reset();
    GetProfileManager()->Reset();
    CNGSContentManager::GetInstance()->Reset();
    CNGSLockManager::GetInstance()->reset();
    destroy();
    CNGSUtil::DebugLog(SESSION_LOG_SEPARATOR);
}

