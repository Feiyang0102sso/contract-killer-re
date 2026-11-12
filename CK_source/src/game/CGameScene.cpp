// The CK game scene switches between preload, gameplay and the pause screens.
#include "CGameScene.h"
#include "App.h"
#include "CDH_LoadingScreen.h"
#include "CDH_PauseScreen.h"
#include "CDH_SoundManager.h"
#include "CGameApp.h"
#include "CGameScreen.h"
#include "CGluGamesNetwork.h"
#include "CSwerveGame.h"
#include "CTutorialManager.h"
#include "DGHelper.h"
#include "ICMediaPlayer.h"
// Header statics of the original unit (static initializer: banner tables, then a class id).
#include "AdBannerSizes.h"
#include "ICInAppPurchase.h"

bool CGameScene::m_bGGNShowed;
CGluGamesNetwork* CGameScene::m_pGGN;

CGameScene::CGameScene() : Window(false)
{
}

CGameScene::~CGameScene()
{
}

void CGameScene::Init()
{
    if (CDemoMgr::GetInstance()->IsDemo()) {
        ChangeState(Demo, STATE_PUSH);
    } else {
        ChangeState(Loading, STATE_PUSH);
    }
}

void CGameScene::Shutdown()
{
    m_resBank.removeAllKeysets();
}

bool CGameScene::HandleEvent(unsigned int, unsigned int, unsigned int)
{
    return false;
}

void CGameScene::Update()
{
    Window::Update();
}

void CGameScene::OnPause()
{
}

void CGameScene::OnCreateState(Window** window, int state)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_helper->m_gamePlayManager.m_gameType = (game_type_t)1;
    Window* created = NULL;
    switch (state) {
    case Loading: {
        CDH_LoadingScreen* screen = new CDH_LoadingScreen(false);
        screen->SetResInfo(&m_resBank, "KEYSET_RES_PRELOAD", 0);
        created = screen;
        break;
    }
    case Game:
        created = new CGameScreen();
        break;
    case Pause:
        app->m_timeScale = 0.0f;
        created = new CDH_PauseScreen(false, false);
        break;
    case PauseWithSoundOff:
        app->m_soundManager->StopAll();
        ICMediaPlayer::GetInstance()->Stop(0);
        ICMediaPlayer::GetInstance()->StopVibrate(0);
        app->m_timeScale = 0.0f;
        bool allowQuit = app->m_helper->GetGameType() == 1 && !CTutorialManager::IsPlayerInTutorial();
        created = new CDH_PauseScreen(true, allowQuit);
        break;
    }
    *window = created;
}

void CGameScene::OnStateInterrupt(int state, Window* window)
{
    int result = window->GetInterrupt();
    App* app = static_cast<App*>(WindowApp::m_instance);
    switch (state) {
    case Loading:
        if (result == 1) {
            ChangeState(Game, STATE_REPLACE);
        } else {
            ChangeState(PauseWithSoundOff, STATE_PUSH);
        }
        break;
    case Game:
        if (result == 1) {
            SetInterrupt(1);
        } else if (result == 2) {
            ChangeState(Pause, STATE_PUSH);
        } else if (result == 3) {
            ChangeState(PauseWithSoundOff, STATE_PUSH);
        }
        break;
    case Pause:
        if (result == 1) {
            SetInterrupt(2);
        } else if (result == 2) {
            PopState();
        }
        app->m_timeScale = 1.0f;
        break;
    case PauseWithSoundOff:
        if (result == CDH_PauseScreen::ConfirmMissionExit) {
            app->m_timeScale = 1.0f;
            PopState();
            app->m_helper->m_swerveGame->ShowReturnToGpsConfirmation();
        } else if (result == 1) {
            app->m_timeScale = 1.0f;
            if (static_cast<CDH_PauseScreen*>(window)->m_quitMission) {
                PopState();
                app->m_helper->m_swerveGame->FinishGameWithResult(BountyResultPending, true);
            } else {
                SetInterrupt(2);
            }
        } else if (result == 2) {
            app->m_timeScale = 1.0f;
            PopState();
        }
        break;
    }
}

void CGameScene::AtExitGGN(void*)
{
    CGameApp::GetInstance()->SetRenderingEnabled(1);
    m_bGGNShowed = false;
}

void CGameScene::ShowGGN()
{
    if (m_pGGN == NULL) {
        m_pGGN = new CGluGamesNetwork("bounty", AtExitGGN, NULL, 0, false);
    }
    m_pGGN->Show();
    CGameApp::GetInstance()->SetRenderingEnabled(0);
    m_bGGNShowed = true;
}

bool CGameScene::IsGGNShowed()
{
    return m_bGGNShowed;
}

typedef char AssertGameSceneSize[sizeof(CGameScene) == 0xc8 ? 1 : -1];
