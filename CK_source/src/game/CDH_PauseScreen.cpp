#include "CDH_PauseScreen.h"
#include "App.h"
#include "AppSettings.h"
#include "CDH_MenuScreen.h"
#include "CDH_OptionsScreen.h"
#include "CDialogWindow.h"
#include "CSettingsWindow.h"
#include "CSwerveGame.h"
#include "GamePlayHUD.h"
#include <string.h>

namespace {
const int ReturnToGPS = 0x24b25450;
const int ConfirmReturn = 0x24b25451;
const int Back = (int)0x97204784;
const int SettingsClosed = 0x13179047;
enum State { PauseMenu = 1, Options = 2, Cheats = 3, ExitGame = 4, ConfirmReturnToGPS = 5 };
const char* const CHEAT_MENU_ITEMS[] = {
    "IDS_CHEAT_SHOW_WEAPON", "IDS_CHEAT_SHOW_ANIMALS", "IDS_CHEAT_SHOW_TREES",
    "IDS_CHEAT_SHOW_BRANCHES", "IDS_CHEAT_SHOW_GROUND"
};
const char* const EXIT_CONFIRM_MENU_ITEMS[] = { "IDS_YES", "IDS_NO" };
}

CDH_PauseScreen::CDH_PauseScreen(bool soundOff, bool allowQuit)
    : m_items(NULL), m_itemCount(0), m_quitMission(false)
{
    m_pauseDialog = new CSettingsWindow(soundOff, allowQuit);
    m_pauseDialog->SetLayoutType(LAYOUT_SIMPLE);
    m_pauseDialog->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    AddModal(m_pauseDialog);
    AppSettings* settings = static_cast<App*>(WindowApp::m_instance)->m_settings;
    settings->m_field1f = !settings->m_field1f;
    if (CSwerveGame::m_pInstance != NULL && CSwerveGame::m_pInstance->m_hud != NULL) {
        CSwerveGame::m_pInstance->m_hud->ResetFireButton();
    }
}

CDH_PauseScreen::~CDH_PauseScreen()
{
    AppSettings* settings = static_cast<App*>(WindowApp::m_instance)->m_settings;
    settings->m_field1f = !settings->m_field1f;
}

Window* CDH_PauseScreen::PopulateSofkeys()
{
    return new SoftkeysContainer(this);
}

void CDH_PauseScreen::OnCreateState(Window** window, int state)
{
    *window = NULL;
    if (state == Options) {
        *window = new CDH_OptionsScreen();
    } else if (state == PauseMenu) {
        CDH_MenuScreen* menu = new CDH_MenuScreen();
        menu->setTitleDelayed("IDS_PAUSE_MENU");
        menu->setMenuItems(m_items, m_itemCount);
        *window = menu;
    } else if (state == Cheats) {
        CDH_MenuScreen* menu = new CDH_MenuScreen();
        menu->setTitleDelayed("IDS_MENU_CHEAT");
        menu->setMenuItems(CHEAT_MENU_ITEMS, 5);
        *window = menu;
    } else if (state == ExitGame) {
        CDH_MenuScreen* menu = new CDH_MenuScreen();
        menu->m_showBack = false;
        menu->setTitleDelayed("IDS_EXIT_MENU");
        menu->setMenuItems(EXIT_CONFIRM_MENU_ITEMS, 2);
        *window = menu;
    } else if (state == ConfirmReturnToGPS) {
        CDialogWindow* confirm = new CDialogWindow(true);
        confirm->SetTitleText("IDS_DIALOG_TITLE_WARNING");
        confirm->SetMessageText(ResString("IDS_RETURN_TO_GPS_TEXT"));
        confirm->SetButtons(CDialogWindow::BUTTON_OK, CDialogWindow::BUTTON_IGNORE);
        confirm->SetButtonsCommands(ConfirmReturn, Back, -1);
        *window = confirm;
    }
}

void CDH_PauseScreen::OnStateInterrupt(int state, Window* window)
{
    int result = window->GetInterrupt();
    if (state == PauseMenu) {
        if (result == 2) {
            static_cast<App*>(WindowApp::m_instance)->m_timeScale = 1.0f;
            SetInterrupt(2);
        } else if (result == 1) {
            int index = static_cast<CDH_MenuScreen*>(window)->m_selectedIndex;
            if (index < 0 || index >= m_itemCount) {
                return;
            }
            const char* item = m_items[index];
            if (strcmp(item, "IDS_MENU_CONTINUE") == 0) {
                SetInterrupt(2);
            } else if (strcmp(item, "IDS_MENU_RETURN_TO_GPS") == 0) {
                m_quitMission = true;
                SetInterrupt(1);
            } else if (strcmp(item, "IDS_MENU_OPTIONS") == 0) {
                ChangeState(Options, STATE_PUSH);
            } else if (strcmp(item, "IDS_MENU_CHEAT") == 0) {
                ChangeState(Cheats, STATE_PUSH);
            } else if (strcmp(item, "IDS_MENU_EXIT") == 0) {
                ChangeState(ExitGame, STATE_PUSH);
            }
        }
    } else if (state == ExitGame && result == 1) {
        int index = static_cast<CDH_MenuScreen*>(window)->m_selectedIndex;
        if (index == 0) {
            m_quitMission = false;
            SetInterrupt(1);
        } else if (index == 1) {
            PopState();
        }
    } else if ((state == Options && (result == 1 || result == 2))
               || ((state == Cheats || state == ExitGame) && result == 2)
               || state == ConfirmReturnToGPS) {
        PopState();
    }
}

void CDH_PauseScreen::OnCommand(Event& event)
{
    switch (event.m_id) {
    case ReturnToGPS:
        if (CSwerveGame::m_pInstance != NULL) {
            SetInterrupt(ConfirmMissionExit);
        } else {
            ChangeState(ConfirmReturnToGPS, STATE_PUSH);
        }
        event.Clear();
        break;
    case ConfirmReturn:
        m_quitMission = true;
        SetInterrupt(1);
        event.Clear();
        break;
    case Back:
        if (m_currentState == ConfirmReturnToGPS) {
            PopState();
            event.Clear();
            break;
        }
        // The pause screen is popped when its settings pane closes.
    case SettingsClosed:
        SetInterrupt(2);
        break;
    }
}

typedef char AssertPauseScreenSize[sizeof(CDH_PauseScreen) == 0xb4 ? 1 : -1];
