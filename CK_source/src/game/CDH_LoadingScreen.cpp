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
    : Window(false), m_tapToContinue(tapToContinue), m_title(NULL), m_loginPending(false),
      m_bar(NULL), m_initialDelay(0), m_elapsed(0), m_fieldbc(false), m_preloadFailed(false),
      m_resBank(NULL), m_state(Delay), m_restartRequested(false), m_profileRequested(false)
{
    CResource* title = NULL;
    CResourceManager::GetInstance()->CreateResource("SUR_TITLE", &title);
    if (title != NULL) {
        m_title = static_cast<ICRenderSurface*>(title->GetData());
    }
}

CDH_LoadingScreen::~CDH_LoadingScreen()
{
    if (m_title != NULL) {
        CResourceManager::GetInstance()->ReleaseResource("SUR_TITLE");
    }
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
    App* app = static_cast<App*>(WindowApp::m_instance);
    switch (m_state) {
    case Delay:
        m_elapsed += app->m_updateFrameTime;
        if (m_elapsed >= m_initialDelay) {
            app->InitPlayerIndependentGameData();
            m_state = Login;
            m_bar->setState(CDH_LoadingBarWidget::STATE_CONNECTING);
        }
        break;
    case Login: {
        CNGSLoginFlow* flow = CNGSLoginFlow::GetInstance();
        CGServeHandler* handler = app->m_gserveHandler;
        bool online = app->m_networkAvailability->IsAvailable();
        // Original: the startup login only waits for a running flow.
        if (!m_loginPending && !handler->m_offlineAccepted) {
            if (!online) {
                app->m_gserveHandler->ShowOfflineModeDialog();
                m_loginPending = true;
            } else if (!flow->IsRunning()) {
                app->StartLogin(App::LoginAtStartup, false);
                m_loginPending = true;
            }
        }
        if (!online && !handler->m_offlineAccepted && flow->IsRunning()) {
            flow->Reset();
            app->m_gserveHandler->ShowOfflineModeDialog();
            m_loginPending = true;
        }
        if (m_loginPending && (online || handler->m_offlineAccepted) && !flow->IsRunning()) {
            m_loginPending = false;
        }
        if (m_loginPending) {
            return;
        }
        if (!app->m_field3a1 && !m_restartRequested && !m_profileRequested) {
            app->StartLogin(App::LoginLoadProfile, false);
            m_profileRequested = true;
            m_loginPending = true;
            return;
        }
        app->m_saveManager->PauseByGServe(0);
        app->InitGameData();
        m_state = Preload;
        m_bar->setState(CDH_LoadingBarWidget::STATE_LOADING);
        break;
    }
    case Preload:
        if (m_resBank != NULL) {
            m_resBank->HandleUpdate(app->m_updateFrameTime);
            m_preloadFailed = m_resBank->GetState() == CResBank::Failed;
            m_bar->m_progress = m_resBank->GetProgressPercent();
        } else {
            m_bar->m_progress = 100;
        }
        if (m_bar->m_progress >= 100) {
            if (!m_tapToContinue) {
                SetInterrupt(1);
            } else {
                m_state = WaitForTap;
                m_bar->setState(CDH_LoadingBarWidget::STATE_TAP);
            }
        }
        break;
    case WaitForTap:
        break;
    }
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

typedef char AssertLoadingScreenSize[sizeof(CDH_LoadingScreen) == 0xcc ? 1 : -1];
