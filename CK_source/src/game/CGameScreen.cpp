#include "CGameScreen.h"

#include "App.h"
#include "CBH_SurfaceManager.h"
#include "CDH_LevelUpScreen.h"
#include "CDH_TextScreen.h"
#include "CHuntingInfo.h"
#include "CIntermediateState.h"
#include "CSaveManager.h"
#include "CSwerveGame.h"
#include "DGHelper.h"

namespace {
enum State {
    GPSMap = 1, HuntDayOver, HuntDayCompleted, EnterMission, LeaveMission, Mission, LevelUp = 8, Informant
};
const char DialogBackground[] = "IDB_DIALOG_WINDOW_BG";
}

CGameScreen::CGameScreen()
    : Window(false), m_previousState(-1), m_missionSurface(NULL), m_mapCache(NULL)
{
}

CGameScreen::~CGameScreen()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_surfaceManager->ReleaseSurface(DialogBackground);
    delete m_mapCache;
    m_mapCache = NULL;
}

void CGameScreen::Init()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_surfaceManager->CreateSurface(DialogBackground, true);
    ChangeState(GPSMap, STATE_REPLACE);
}

void CGameScreen::Shutdown()
{
}

void CGameScreen::Activate()
{
}

void CGameScreen::Deactivate()
{
}

void CGameScreen::OnCommand(Event& event)
{
    if (event.m_id != (int)0x97204784 && event.m_id != 0x121683a7) {
        return;
    }
    event.Clear();
    if (m_currentState == Mission) {
        App* app = static_cast<App*>(WindowApp::m_instance);
        CSwerveGame* game = app->m_helper->m_swerveGame;
        if ((game != NULL && game->m_currentState != 4) || app->m_timeScale == 0.0f) {
            return;
        }
        SetInterrupt(3);
    } else {
        SetInterrupt(2);
    }
}

void CGameScreen::OnCreateState(Window** window, int state)
{
    *window = NULL;
    switch (state) {
    case GPSMap: {
        delete m_mapCache;
        m_mapCache = new CGPSMapScreen::CachedData();
        CGPSMapScreen* map = new CGPSMapScreen(m_mapCache);
        if (m_previousState == 2) {
            map->SkipTutorialAutoStart();
        }
        *window = map;
        break;
    }
    case HuntDayOver: {
        CDH_TextScreen* screen = new CDH_TextScreen("IDS_TEXT_GAME_OVER_HUNT_DAY_TITLE", "IDS_TEXT_GAME_OVER_HUNT_DAY");
        screen->setButtons(NULL, "IDS_DONE");
        *window = screen;
        break;
    }
    case HuntDayCompleted: {
        CDH_TextScreen* screen = new CDH_TextScreen();
        screen->setCaptionText(XString(L"Free hunt completed"), false);
        screen->setText(XString(L"You successfuly complete free hunt!"));
        screen->setButtons(NULL, "IDS_DONE");
        *window = screen;
        break;
    }
    case EnterMission:
    case LeaveMission:
        *window = new CIntermediateState();
        break;
    case Mission: {
        App* app = static_cast<App*>(WindowApp::m_instance);
        app->m_saveManager->PauseSaving();
        *window = new CSwerveGame(m_missionSurface, -1);
        m_missionSurface = NULL;
        break;
    }
    case LevelUp:
        *window = new CDH_LevelUpScreen();
        break;
    case Informant: {
        CDH_TextScreen* screen = new CDH_TextScreen();
        screen->setText("Informant found");
        screen->setButtons("IDS_NO", "IDS_YES");
        *window = screen;
        break;
    }
    }
}

void CGameScreen::OnStateInterrupt(int state, Window* window)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    GamePlayManager& gameplay = app->m_helper->m_gamePlayManager;
    switch (state) {
    case GPSMap: {
        int result = window->GetInterrupt();
        if (result == 2) {
            if (m_mapCache->m_huntDayElapsed < CGPSMapScreen::CDH_GPS_SCREEN_HUNT_DAY_DURATION_MS) {
                gameplay.BreakGame();
                SetInterrupt(2);
            } else if (app->m_helper->GetGameType() == 1) {
                gameplay.FinishGame(true);
                ChangeState(HuntDayOver, STATE_PUSH);
            }
        } else if (result == 1) {
            if (static_cast<CGPSMapScreen*>(window)->GetState() != 0) {
                ChangeState(EnterMission, STATE_REPLACE);
            } else {
                ChangeState(Informant, STATE_PUSH);
            }
        }
        break;
    }
    case HuntDayOver:
    case HuntDayCompleted:
    case LevelUp:
        if (gameplay.GetLevelUp()) {
            gameplay.ResetLevelUp();
            ChangeState(LevelUp, STATE_PUSH);
        } else {
            SetInterrupt(1);
        }
        break;
    case EnterMission:
        ChangeState(Mission, STATE_REPLACE);
        break;
    case LeaveMission:
        ChangeState(GPSMap, STATE_REPLACE);
        break;
    case Mission:
        app->m_saveManager->ResumeSaving();
        m_previousState = window->GetInterrupt();
        app->m_huntingInfo->AfterSceneActivity();
        ChangeState(LeaveMission, STATE_REPLACE);
        break;
    case Informant:
        if (window->GetInterrupt() == 2) {
            ChangeState(0, STATE_POP);
        }
        break;
    }
}

typedef char AssertGameScreenSize[sizeof(CGameScreen) == 0xb0 ? 1 : -1];
