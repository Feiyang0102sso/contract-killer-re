#include "CDH_PauseScreen.h"
#include "App.h"
#include "AppSettings.h"
#include "CDH_MenuScreen.h"
#include "CDH_OptionsScreen.h"
#include "CSettingsWindow.h"
#include "CSwerveGame.h"
#include "GamePlayHUD.h"
#include <string.h>

namespace {
const int Back = (int)0x97204784;
const int SettingsClosed = 0x13179047;
enum State { PauseMenu = 1, Options = 2, Cheats = 3, ExitGame = 4 };
}

static const char* const CHEAT_MENU_ITEMS[] = {
    "IDS_CHEAT_SHOW_WEAPON", "IDS_CHEAT_SHOW_ANIMALS", "IDS_CHEAT_SHOW_TREES",
    "IDS_CHEAT_SHOW_BRANCHES", "IDS_CHEAT_SHOW_GROUND"
};
static const char* const EXIT_CONFIRM_MENU_ITEMS[] = { "IDS_YES", "IDS_NO" };

CDH_PauseScreen::CDH_PauseScreen(bool soundOff, bool allowQuit)
    : m_items(NULL)
{
    m_pauseDialog = new CSettingsWindow(soundOff, allowQuit);
    m_pauseDialog->SetLayoutType(LAYOUT_SIMPLE);
    m_pauseDialog->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    AddModal(m_pauseDialog);
    AppSettings* settings = static_cast<App*>(WindowApp::m_instance)->m_settings;
    settings->m_field1f = !settings->m_field1f;
    if (CSwerveGame::m_pInstance != NULL) {
        CSwerveGame::m_pInstance->m_hud->SetFireHeld(false);
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
    Window* created = NULL;
    switch (state) {
    case PauseMenu: {
        CDH_MenuScreen* menu = new CDH_MenuScreen();
        menu->setTitleDelayed("IDS_PAUSE_MENU");
        menu->setMenuItems(m_items, m_itemCount);
        created = menu;
        break;
    }
    case Cheats: {
        CDH_MenuScreen* menu = new CDH_MenuScreen();
        menu->setTitleDelayed("IDS_MENU_CHEAT");
        menu->setMenuItems(CHEAT_MENU_ITEMS, 5);
        created = menu;
        break;
    }
    case Options:
        created = new CDH_OptionsScreen();
        break;
    case ExitGame: {
        CDH_MenuScreen* menu = new CDH_MenuScreen();
        menu->m_showBack = false;
        menu->setTitleDelayed("IDS_EXIT_MENU");
        menu->setMenuItems(EXIT_CONFIRM_MENU_ITEMS, 2);
        created = menu;
        break;
    }
    }
    *window = created;
}

void CDH_PauseScreen::OnStateInterrupt(int state, Window* window)
{
    switch (state) {
    case PauseMenu: {
        int result = window->GetInterrupt();
        if (result == 2) {
            static_cast<App*>(WindowApp::m_instance)->m_timeScale = 1.0f;
            SetInterrupt(2);
        } else if (result == 1) {
            int index = static_cast<CDH_MenuScreen*>(window)->m_selectedIndex;
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
        return;
    }
    case Options: {
        int result = window->GetInterrupt();
        switch (result) {
        case 2:
        case 1:
            break;
        default:
            return;
        }
        break;
    }
    case Cheats:
        if (window->GetInterrupt() != 2) {
            return;
        }
        break;
    case ExitGame: {
        int result = window->GetInterrupt();
        if (result != 2) {
            if (result != 1) {
                return;
            }
            int index = static_cast<CDH_MenuScreen*>(window)->m_selectedIndex;
            const char* item = EXIT_CONFIRM_MENU_ITEMS[index];
            if (strcmp(item, "IDS_YES") == 0) {
                m_quitMission = false;
                SetInterrupt(1);
                return;
            }
            if (strcmp(item, "IDS_NO") != 0) {
                return;
            }
        }
        break;
    }
    default:
        return;
    }
    PopState();
}

void CDH_PauseScreen::OnCommand(Event& event)
{
    // The pause screen is popped when its settings pane closes.
    if (event.m_id == Back || event.m_id == SettingsClosed) {
        SetInterrupt(2);
    }
}

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AssertPauseScreenSize[sizeof(CDH_PauseScreen) == 0xb4 ? 1 : -1];
#endif
