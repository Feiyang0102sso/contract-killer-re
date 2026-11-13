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
static const char BOUNTY_GAME_CODE[] = "bounty";

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
    static_cast<App*>(WindowApp::m_instance)->m_helper->m_gamePlayManager.m_gameType = (game_type_t)1;
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
        static_cast<App*>(WindowApp::m_instance)->m_timeScale = 0.0f;
        created = new CDH_PauseScreen(false, false);
        break;
    case PauseWithSoundOff: {
        static_cast<App*>(WindowApp::m_instance)->m_soundManager->StopAll();
        ICMediaPlayer::GetInstance()->Stop(0);
        ICMediaPlayer::GetInstance()->StopVibrate(0);
        static_cast<App*>(WindowApp::m_instance)->m_timeScale = 0.0f;
        game_type_t gameType = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameType();
        bool inTutorial = static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial();
        created = new CDH_PauseScreen(true, (gameType == 1) & !inTutorial);
        break;
    }
    }
    *window = created;
}

void CGameScene::OnStateInterrupt(int state, Window* window)
{
    switch (state) {
    case Loading:
        if (window->GetInterrupt() == 1) {
            ChangeState(Game, STATE_REPLACE);
            return;
        }
        break;
    case Game: {
        int result = window->GetInterrupt();
        if (result == 3) {
            break;
        }
        if (result == 2) {
            ChangeState(Pause, STATE_PUSH);
            return;
        }
        if (result == 1) {
            SetInterrupt(1);
        }
        return;
    }
    case Pause: {
        int result = window->GetInterrupt();
        if (result == 1) {
            SetInterrupt(2);
        } else if (result == 2) {
            PopState();
        }
        static_cast<App*>(WindowApp::m_instance)->m_timeScale = 1.0f;
        return;
    }
    case PauseWithSoundOff: {
        int result = window->GetInterrupt();
        if (result != 1) {
            if (result == 2) {
                static_cast<App*>(WindowApp::m_instance)->m_timeScale = 1.0f;
                PopState();
            }
            return;
        }
        static_cast<App*>(WindowApp::m_instance)->m_timeScale = 1.0f;
        if (static_cast<CDH_PauseScreen*>(window)->m_quitMission) {
            PopState();
            static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->FinishGameWithResult(
                BountyResultPending, true);
        } else {
            SetInterrupt(2);
        }
        return;
    }
    default:
        return;
    }
    ChangeState(PauseWithSoundOff, STATE_PUSH);
}

void CGameScene::AtExitGGN(void*)
{
    CGameApp::GetInstance()->SetRenderingEnabled(1);
    m_bGGNShowed = false;
}

void CGameScene::ShowGGN()
{
    if (m_pGGN == NULL) {
        m_pGGN = new CGluGamesNetwork(BOUNTY_GAME_CODE, AtExitGGN, NULL, 0, false);
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
