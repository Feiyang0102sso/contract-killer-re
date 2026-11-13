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
const char* const DialogBackground = "IDB_DIALOG_WINDOW_BG";
}

CGameScreen::CGameScreen()
    : Window(false), m_previousState(-1), m_missionSurface(NULL), m_mapCache(NULL)
{
}

CGameScreen::~CGameScreen()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_surfaceManager->ReleaseSurface(DialogBackground);
    if (m_mapCache != NULL) {
        delete m_mapCache;
        m_mapCache = NULL;
    }
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
            event.Clear();
            return;
        }
        SetInterrupt(3);
    } else {
        SetInterrupt(2);
    }
}

void CGameScreen::OnCreateState(Window** window, int state)
{
    Window* created = NULL;
    switch (state) {
    case HuntDayOver: {
        CDH_TextScreen* screen = new CDH_TextScreen("IDS_TEXT_GAME_OVER_HUNT_DAY_TITLE", "IDS_TEXT_GAME_OVER_HUNT_DAY");
        screen->setButtons(NULL, "IDS_DONE");
        created = screen;
        break;
    }
    case HuntDayCompleted: {
        CDH_TextScreen* screen = new CDH_TextScreen();
        screen->setCaptionText(XString(L"Free hunt completed"), false);
        screen->setText(XString(L"You successfuly complete free hunt!"));
        screen->setButtons(NULL, "IDS_DONE");
        created = screen;
        break;
    }
    case LevelUp:
        created = new CDH_LevelUpScreen();
        break;
    case GPSMap:
        if (m_mapCache != NULL) {
            delete m_mapCache;
            m_mapCache = NULL;
        }
        m_mapCache = new CGPSMapScreen::CachedData();
        created = new CGPSMapScreen(m_mapCache);
        break;
    case Informant: {
        CDH_TextScreen* screen = new CDH_TextScreen();
        screen->setText("Informant found");
        screen->setButtons("IDS_NO", "IDS_YES");
        created = screen;
        break;
    }
    case EnterMission:
    case LeaveMission:
        created = new CIntermediateState();
        break;
    case Mission:
        static_cast<App*>(WindowApp::m_instance)->m_saveManager->PauseSaving();
        created = new CSwerveGame(m_missionSurface, -1);
        m_missionSurface = NULL;
        break;
    }
    *window = created;
}

void CGameScreen::OnStateInterrupt(int state, Window* window)
{
    int nextState;
    switch (state) {
    case HuntDayOver:
    case HuntDayCompleted:
    case LevelUp:
        if (!static_cast<App*>(WindowApp::m_instance)->m_helper->m_gamePlayManager.GetLevelUp()) {
            SetInterrupt(1);
            return;
        }
        static_cast<App*>(WindowApp::m_instance)->m_helper->m_gamePlayManager.ResetLevelUp();
        ChangeState(LevelUp, STATE_PUSH);
        return;
    case GPSMap: {
        int result = window->GetInterrupt();
        if (result != 2) {
            if (result != 1) {
                return;
            }
            if (static_cast<CGPSMapScreen*>(window)->GetState() == 0) {
                ChangeState(Informant, STATE_PUSH);
                return;
            }
            ChangeState(EnterMission, STATE_REPLACE);
            return;
        }
        DGHelper* helper = static_cast<App*>(WindowApp::m_instance)->m_helper;
        if (m_mapCache->m_huntDayElapsed >= CGPSMapScreen::CDH_GPS_SCREEN_HUNT_DAY_DURATION_MS) {
            if (helper->GetGameType() != 1) {
                return;
            }
            static_cast<App*>(WindowApp::m_instance)->m_helper->m_gamePlayManager.FinishGame(true);
            ChangeState(HuntDayOver, STATE_PUSH);
        } else {
            helper->m_gamePlayManager.BreakGame();
            SetInterrupt(2);
        }
        return;
    }
    case Informant:
        if (window->GetInterrupt() == 2) {
            ChangeState(0, STATE_POP);
        }
        return;
    case EnterMission:
        ChangeState(Mission, STATE_REPLACE);
        return;
    case LeaveMission:
        ChangeState(GPSMap, STATE_REPLACE);
        return;
    case Mission:
        static_cast<App*>(WindowApp::m_instance)->m_saveManager->ResumeSaving();
        static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->AfterSceneActivity();
        nextState = LeaveMission;
        break;
    default:
        return;
    }
    ChangeState(nextState, STATE_REPLACE);
}

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AssertGameScreenSize[sizeof(CGameScreen) == 0xb0 ? 1 : -1];
#endif
