// CDH_OptionsScreen.cpp - apply selected options or confirm a save reset.
#include "CDH_OptionsScreen.h"

#include "App.h"
#include "AppSettings.h"
#include "CDH_ButtonWidget.h"
#include "CDH_RadioButtonWidget.h"
#include "CDH_RadioGroupWidget.h"
#include "CDH_SoundManager.h"
#include "CDH_TextScreen.h"
#include "CFontMgr.h"
#include "DGHelper.h"

namespace {
const int OptionItem = static_cast<int>(0x9720a3cbu);
const int Back = static_cast<int>(0x97204784u);
const int Done = 0x07b18a47;
const int LeftButton = 0x21204844;
}

CDH_OptionsScreen::CDH_OptionsScreen()
    : Window(false), m_sound(NULL), m_music(NULL), m_fireButton(NULL)
{
    AddNewState(OptionItems);
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_soundManager->PlaySound("IDM_MENU_WINDOW_OPEN_CLOSE", false, false);
}

CDH_OptionsScreen::~CDH_OptionsScreen()
{
}

Window* CDH_OptionsScreen::AddButtonItem(XString label, int row)
{
    CDH_ButtonWidget* button = new CDH_ButtonWidget(label, 5);
    button->m_command = OptionItem;
    button->m_userData = row;
    button->SetCellPos(0, row, 1, 1);
    button->setContainerAlign(ALIGN_LEFT);
    return button;
}

CDH_RadioGroupWidget* CDH_OptionsScreen::AddSelectorItem(int row, int selected)
{
    CDH_RadioGroupWidget* group = new CDH_RadioGroupWidget();
    group->SetCellPos(1, row, 1, 1);

    CDH_RadioButtonWidget* off = new CDH_RadioButtonWidget();
    off->Init(false, ResString("IDS_OPTION_OFF"), 1);
    off->SetCellPos(0, 0, 1, 1);
    group->AddButton(off);

    CDH_RadioButtonWidget* on = new CDH_RadioButtonWidget();
    on->Init(false, ResString("IDS_OPTION_ON"), 1);
    on->SetCellPos(1, 0, 1, 1);
    group->AddButton(on);
    group->SetValue(selected);
    return group;
}

CDH_BaseScreen* CDH_OptionsScreen::createOptionItemsScreen()
{
    CDH_BaseScreen* screen = new CDH_BaseScreen();
    screen->setCaptionText("IDS_MENU_OPTIONS", false);
    screen->setButtons("IDS_BACK", "IDS_BTN_DONE");
    Window* body = new Window(false);
    body->SetAlign(ALIGN_LEFT);
    body->SetDesiredWidth(300);
    body->SetDesiredHeight(200);
    CFontMgr::GetInstance()->GetFont(6);
    body->AddToFront(AddButtonItem(ResString("IDS_MENU_CHEAT"), 0));
    body->AddToFront(AddButtonItem(ResString("IDS_MENU_RESET_DATA"), 1));
    body->AddToFront(AddButtonItem(ResString("IDS_MENU_SOUND"), 2));
    body->AddToFront(AddButtonItem(ResString("IDS_MENU_MUSIC"), 3));
    body->AddToFront(AddButtonItem(ResString("IDS_MENU_FIRE_BUTTON"), 4));

    m_music = AddSelectorItem(3, static_cast<App*>(WindowApp::m_instance)->m_settings->IsMusicEnabled());
    m_sound = AddSelectorItem(2, static_cast<App*>(WindowApp::m_instance)->m_settings->IsSoundEnabled());
    m_fireButton = AddSelectorItem(4, static_cast<App*>(WindowApp::m_instance)->m_settings->IsFireButtonEnabled());
    body->AddToFront(m_music);
    body->AddToFront(m_sound);
    body->AddToFront(m_fireButton);
    screen->setLayoutWidget(body, CDH_WidgetScreen::Body);
    return screen;
}

void CDH_OptionsScreen::Init()
{
    // The original constructor queues the initial state.
}

void CDH_OptionsScreen::OnCommand(Event& event)
{
    if (event.m_id == OptionItem && event.m_param1 == 1) {
        App* app = static_cast<App*>(WindowApp::m_instance);
        app->m_soundManager->PlaySound("IDM_MENU_WINDOW_BUTTON", false, false);
        ChangeState(ConfirmReset, STATE_PUSH);
        event.Clear();
    } else if (event.m_id == Back || event.m_id == Done || event.m_id == LeftButton) {
        SetInterrupt(2);
        event.Clear();
    }
}

void CDH_OptionsScreen::OnCreateState(Window** window, int state)
{
    Window* created = NULL;
    if (state == ConfirmReset) {
        CDH_TextScreen* confirm = new CDH_TextScreen();
        confirm->setText("IDS_DLG_TEXT_RESET_DATA");
        confirm->setButtons("IDS_NO", "IDS_YES");
        created = confirm;
    } else if (state == OptionItems) {
        created = createOptionItemsScreen();
    }
    *window = created;
}

void CDH_OptionsScreen::OnStateInterrupt(int state, Window* window)
{
    if (state == OptionItems) {
        int result = window->GetInterrupt();
        if (result != 2) {
            if (result != 1) {
                return;
            }
            AppSettings* settings = static_cast<App*>(WindowApp::m_instance)->m_settings;
            settings->SetSoundEnabled(m_sound->GetValue() != 0);
            settings = static_cast<App*>(WindowApp::m_instance)->m_settings;
            settings->SetMusicEnabled(m_music->GetValue() != 0);
            settings = static_cast<App*>(WindowApp::m_instance)->m_settings;
            settings->SetFireButtonEnabled(m_fireButton->GetValue() != 0);
            static_cast<App*>(WindowApp::m_instance)->m_soundManager->PlaySound("IDM_MENU_WINDOW_BUTTON", false, false);
            SetInterrupt(1);
        } else {
            static_cast<App*>(WindowApp::m_instance)->m_soundManager->PlaySound("IDM_MENU_WINDOW_BUTTON", false, false);
            SetInterrupt(2);
        }
    } else if (state == ConfirmReset) {
        static_cast<App*>(WindowApp::m_instance)->m_soundManager->PlaySound("IDM_MENU_WINDOW_BUTTON", false, false);
        int result = window->GetInterrupt();
        if (result != 2) {
            if (result != 1) {
                return;
            }
            static_cast<App*>(WindowApp::m_instance)->m_helper->m_gamePlayManager.Reset();
        }
        ChangeState(0, STATE_POP);
    }
}
