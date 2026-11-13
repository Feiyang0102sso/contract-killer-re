#include "CDH_LoadingScreen.h"
#include "App.h"
#include "CDH_LoadingBarWidget.h"
#include "CNGSLoginFlow.h"
#include "CNetworkAvailability.h"
#include "CResBank.h"
#include "CResourceManager.h"
#include "CSaveManager.h"
// Header statics of the original unit (static initializer: banner tables, then a class id).
#include "AdBannerSizes.h"
#include "ICInAppPurchase.h"

CDH_LoadingScreen::CDH_LoadingScreen(bool tapToContinue)
    : Window(false)
{
    m_tapToContinue = tapToContinue;
    m_state = Delay;
    m_initialDelay = 0;
    m_elapsed = 0;
    m_fieldbc = false;
    m_preloadFailed = false;
    m_resBank = NULL;
    CResourceManager* resources = CResourceManager::GetInstance();
    CResource* title = NULL;
    resources->CreateResource("SUR_TITLE", &title);
    m_title = static_cast<ICRenderSurface*>(title->GetData());
    m_loginPending = false;
    m_restartRequested = false;
    m_profileRequested = false;
}

CDH_LoadingScreen::~CDH_LoadingScreen()
{
    CResourceManager::GetInstance()->ReleaseResource("SUR_TITLE");
}

void CDH_LoadingScreen::Init()
{
    m_bar = new CDH_LoadingBarWidget();
    AddToFront(m_bar);
}

void CDH_LoadingScreen::SetResInfo(CResBank* bank, const char* keyset, int delay)
{
    m_resBank = bank;
    if (bank != NULL && keyset != NULL) {
        bank->load(keyset);
    }
    m_initialDelay = delay;
}

void CDH_LoadingScreen::OnPointerPressed(Event& event)
{
    if (m_state == WaitForTap) {
        SetInterrupt(1);
    }
    event.Clear();
}

void CDH_LoadingScreen::Update()
{
    State state = m_state;
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (state != Login) {
        int elapsed = app->m_updateFrameTime;
        switch (state) {
        case Delay:
            m_elapsed = (int)((unsigned int)m_elapsed + elapsed);
            if (m_elapsed >= m_initialDelay) {
                static_cast<App*>(WindowApp::m_instance)->InitPlayerIndependentGameData();
                m_state = Login;
                m_bar->setState(CDH_LoadingBarWidget::STATE_CONNECTING);
            }
            return;
        case Preload:
            if (m_resBank != NULL) {
                m_resBank->HandleUpdate(elapsed);
                int bankState = m_resBank->GetState();
                CDH_LoadingBarWidget* bar = m_bar;
                if (bankState == CResBank::Failed) {
                    bar->m_progress = 100;
                    m_preloadFailed = true;
                } else {
                    bar->m_progress = m_resBank->GetProgressPercent();
                }
            }
            if (m_bar->m_progress >= 100) {
                if (m_tapToContinue) {
                    m_state = WaitForTap;
                    m_bar->setState(CDH_LoadingBarWidget::STATE_TAP);
                } else {
                    SetInterrupt(1);
                }
            }
            return;
        default:
            return;
        }
    }

    // The startup login only waits for a running flow.
    // The original checks the whole byte for the offline and profile flags.
    if (!m_loginPending && reinterpret_cast<const unsigned char&>(app->m_gserveHandler->m_offlineAccepted) == 0) {
        if (!app->m_networkAvailability->IsAvailable()) {
            static_cast<App*>(WindowApp::m_instance)->m_gserveHandler->ShowOfflineModeDialog();
            m_loginPending = true;
        } else if (!CNGSLoginFlow::GetInstance()->IsRunning()) {
            static_cast<App*>(WindowApp::m_instance)->StartLogin(App::LoginAtStartup, false);
            m_loginPending = true;
        }
    }
    if (!static_cast<App*>(WindowApp::m_instance)->m_networkAvailability->IsAvailable()
        && reinterpret_cast<const unsigned char&>(
            static_cast<App*>(WindowApp::m_instance)->m_gserveHandler->m_offlineAccepted) == 0
        && CNGSLoginFlow::GetInstance()->IsRunning()) {
        CNGSLoginFlow::GetInstance()->Reset();
        static_cast<App*>(WindowApp::m_instance)->m_gserveHandler->ShowOfflineModeDialog();
        m_loginPending = true;
    }
    if (m_loginPending
        && (static_cast<App*>(WindowApp::m_instance)->m_networkAvailability->IsAvailable()
            || reinterpret_cast<const unsigned char&>(
                static_cast<App*>(WindowApp::m_instance)->m_gserveHandler->m_offlineAccepted) != 0)
        && !CNGSLoginFlow::GetInstance()->IsRunning()) {
        m_loginPending = false;
    }
    bool pending = m_loginPending;
    if (!pending) {
        App* profileApp = static_cast<App*>(WindowApp::m_instance);
        if (reinterpret_cast<const unsigned char&>(profileApp->m_field3a1) == 0
            && reinterpret_cast<const unsigned char&>(m_restartRequested) == 0
            && reinterpret_cast<const unsigned char&>(m_profileRequested) == 0) {
            profileApp->StartLogin(App::LoginLoadProfile, false);
            m_loginPending = true;
            m_profileRequested = true;
            return;
        }
    }
    if (pending) {
        return;
    }
    static_cast<App*>(WindowApp::m_instance)->m_saveManager->PauseByGServe(0);
    static_cast<App*>(WindowApp::m_instance)->InitGameData();
    m_state = Preload;
    m_bar->setState(CDH_LoadingBarWidget::STATE_LOADING);
}

void CDH_LoadingScreen::Paint(ICGraphics2d& graphics)
{
    // the original draws without checking the title image
    DrawImage(graphics, m_title, 0, 0, 0, 0);
}

void CDH_LoadingScreen::OnCommand(Event& event)
{
    switch (event.m_id) {
    case DisplayWindow::COMMAND_RESTART:
        m_restartRequested = true;
        break;
    case (int)0x8b50a871:
        static_cast<App*>(WindowApp::m_instance)->m_gserveHandler->CloseCurrentSocialNetworkConnectDialog();
        static_cast<App*>(WindowApp::m_instance)->m_gserveHandler->m_offlineAccepted = true;
        break;
    case (int)0x87cad340:
    case (int)0xb079653a:
    case 0x00247405:
        m_loginPending = false;
        break;
    }
}

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AssertLoadingScreenSize[sizeof(CDH_LoadingScreen) == 0xcc ? 1 : -1];
#endif
