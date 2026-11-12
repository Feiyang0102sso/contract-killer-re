#include "CSettingsWindow.h"
#include "App.h"
#include "AppMedia.h"
#include "AppSettings.h"
#include "BaseDialog.h"
#include "CFontMgr.h"
#include "CGameAnalytics.h"
#include "CHelpScreen.h"
#include "CImageButton.h"
#include "CSwerveGame.h"
#include "CDH_RadioButtonWidget.h"
#include "CDH_RadioGroupWidget.h"
#include "CTutorialManager.h"

namespace {
enum SettingsCommand {
    Settings = 0x0c000066,
    Continue = 0x3b0ba067,
    AskReturnToGPS = 0x24b25450,
    Objective = 0x4cb1c63c,
    Closed = 0x13179047,
    Done = (int)0xb3aa22e5,
    Back = 0x6daa228b,
    Help = (int)0xb916cc2a,
    ToggleSound = 0x3b0ba068,
    ToggleMusic,
    ToggleFireButton,
    TogglePush,
    ToggleFacebook
};

XString SettingText(const char* label, bool enabled)
{
    const char* value = "IDS_OFF";
    if (enabled) {
        value = "IDS_ON";
    }
    return Window::ResString(label) + XString(L"  ") + Window::ResString(value);
}
}

CSettingsWindow::CSettingsWindow(bool pauseMenu, bool returnToGPS)
    : Window(false), m_sound(NULL), m_music(NULL), m_fireButton(NULL), m_push(NULL),
      m_fieldb4(NULL), m_facebook(NULL), m_content(NULL), m_pauseMenu(pauseMenu), m_returnToGPS(returnToGPS)
{
    SetLayoutType(LAYOUT_SIMPLE);
    SetPercentWidth(54, 0, 0);
    SetHeightByContent(0, 0);
    SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    m_contentHost = new Window(false);
    m_contentHost->SetPercentWidth(100, 0, 0);
    m_contentHost->SetHeightByContent(0, 0);
    m_contentHost->SetLayoutType(LAYOUT_TABLE);
    AddToFront(m_contentHost);
    m_title = new TextWindow(ResString("IDS_MENU_SETTINGS").Upper(), CFontMgr::GetInstance()->GetFont(15));
    m_title->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
    m_contentHost->AddToFront(m_title, 0, 0);
    if (m_pauseMenu) {
        ShowContent(CreatePauseMenu());
    } else {
        ShowContent(CreateSettingMenu());
    }
}

CSettingsWindow::~CSettingsWindow()
{
}

CImageButton* CSettingsWindow::AddItem(Window* content, const char* label, int command, int row)
{
    CImageButton* button = new CImageButton();
    button->SetPercentWidth(100, 0, 0);
    button->SetDesiredHeight(App::Scale() * 24);
    button->SetText(ResString(label).Upper(), ALIGN_HCENTER | ALIGN_VCENTER, NULL, 0, 0);
    button->m_command = command;
    content->AddToFront(button, 0, row);
    return button;
}

Window* CSettingsWindow::CreatePauseMenu()
{
    m_title->SetText(ResString("IDS_PAUSED").Upper(), CFontMgr::GetInstance()->GetFont(15));
    Window* content = new Window(false);
    content->SetPercentWidth(100, 0, 0);
    content->SetHeightByContent(0, 0);
    content->SetLayoutType(LAYOUT_TABLE);
    AddItem(content, "IDS_CONTINUE", Continue, 0);
    int row = 1;
    if (m_returnToGPS) {
        AddItem(content, "IDS_MENU_RETURN_TO_GPS", AskReturnToGPS, row++);
    }
    if (m_pauseMenu && CTutorialManager::IsShowObjective()) {
        AddItem(content, "IDS_MENU_OBJECTIVE", Objective, row++);
    }
    AddItem(content, "IDS_MENU_SETTINGS", Settings, row);
    return content;
}

Window* CSettingsWindow::CreateSettingMenu()
{
    m_title->SetText(ResString("IDS_MENU_SETTINGS").Upper(), CFontMgr::GetInstance()->GetFont(15));
    Window* content = new Window(false);
    content->SetPercentWidth(100, 0, 0);
    content->SetHeightByContent(0, 0);
    content->SetLayoutType(LAYOUT_TABLE);
    m_sound = AddItem(content, "IDS_OPTIONS_SOUND", ToggleSound, 0);
    m_music = AddItem(content, "IDS_OPTIONS_MUSIC", ToggleMusic, 1);
    m_fireButton = AddItem(content, "IDS_OPTIONS_FIRE_BUTTON", ToggleFireButton, 2);
    m_push = AddItem(content, "IDS_OPTIONS_NOTIFICATIONS", TogglePush, 3);
    m_facebook = AddItem(content, "IDS_FB", ToggleFacebook, 4);
    AddItem(content, "IDS_MENU_HELP", Help, 5);
    AddItem(content, "IDS_MENU_BACK", Back, 6);
    AddItem(content, "IDS_BTN_DONE", Done, 7);
    UpdateValues();
    return content;
}

void CSettingsWindow::UpdateValues()
{
    AppSettings* settings = static_cast<App*>(WindowApp::m_instance)->m_settings;
    m_sound->UpdateText(SettingText("IDS_OPTIONS_SOUND", settings->IsSoundEnabled()));
    m_music->UpdateText(SettingText("IDS_OPTIONS_MUSIC", settings->IsMusicEnabled()));
    m_fireButton->UpdateText(SettingText("IDS_OPTIONS_FIRE_BUTTON", settings->IsFireButtonEnabled()));
    m_push->UpdateText(SettingText("IDS_OPTIONS_NOTIFICATIONS", settings->IsLocalPushEnabled()));
    m_facebook->UpdateText(SettingText("IDS_FB", settings->IsFBReminderEnabled()));
}

void CSettingsWindow::ShowContent(Window* content)
{
    if (m_content != NULL) {
        m_content->Close();
    }
    m_content = content;
    m_contentHost->AddToFront(content, 0, 1);
}

void CSettingsWindow::OnStartupEasy()
{
    EasyParams params(this, (EasyLib::EMethod)26);
    params.SetMoving(m_x, m_parent->m_height, m_x, m_y);
    params.SetFullTime(0.4f);
    StartEasy(params, 0);
}

void CSettingsWindow::OnEasyFinished()
{
}

void CSettingsWindow::OnClosed()
{
    WindowApp::HandleTunnelCommand(Closed, 0, 0, 0);
}

void CSettingsWindow::OnCommand(Event& event)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    AppSettings* settings = app->m_settings;
    switch (event.m_id) {
    case Continue:
        Close();
        break;
    case Objective:
        if (CSwerveGame::m_pInstance != NULL) {
            CSwerveGame::m_pInstance->ShowObjective();
            Close();
        }
        break;
    case Settings:
        ShowContent(CreateSettingMenu());
        break;
    case Help:
        app->m_mainLayer->AddModal(new CHelpScreen());
        WindowApp::HandleTunnelCommand(0x718a0676, 0, 0, 0);
        break;
    case Done:
        settings->Save();
        app->m_media->Update();
        // The same button returns to the pause menu when opened from a mission.
    case Back:
        if (m_pauseMenu) {
            ShowContent(CreatePauseMenu());
        } else {
            Close();
        }
        break;
    case ToggleSound:
        settings->SetSoundEnabled(!settings->IsSoundEnabled());
        CGameAnalytics::logSoundSwitch(settings->IsSoundEnabled());
        UpdateValues();
        break;
    case ToggleMusic:
        settings->SetMusicEnabled(!settings->IsMusicEnabled());
        CGameAnalytics::logMusicSwitch(settings->IsMusicEnabled());
        UpdateValues();
        break;
    case ToggleFireButton:
        settings->SetFireButtonEnabled(!settings->IsFireButtonEnabled());
        CGameAnalytics::logFireButtonSwitch(settings->IsFireButtonEnabled());
        UpdateValues();
        break;
    case TogglePush:
        settings->SetLocalPushEnabled(!settings->IsLocalPushEnabled());
        UpdateValues();
        break;
    case ToggleFacebook:
        settings->SetFBReminderEnabled(!settings->IsFBReminderEnabled());
        UpdateValues();
        break;
    default:
        return;
    }
    event.Clear();
}

void CSettingsWindow::Paint(ICGraphics2d& graphics)
{
    FillRect(graphics, 0, 0, m_width, m_height, 0xe0201414);
    Window::Paint(graphics);
}

typedef char AssertSettingsWindowSize[sizeof(CSettingsWindow) == 0xcc ? 1 : -1];

Window* CSettingsWindow::CreateRadioGroup(CDH_RadioGroupWidget** group, const char* label, const char* onLabel,
                                          const char* offLabel, bool value)
{
    Window* row = new Window(false);
    row->SetPercentWidth(100, 0, 0);
    row->SetHeightByContent(0, 0);
    TextWindow* caption = new TextWindow(ResString(label), CFontMgr::GetInstance()->GetFont(15));
    caption->SetOutsetSpacing(0, 0, App::Scale() * 10, 0);
    row->AddToFront(caption);

    CDH_RadioGroupWidget* radio = new CDH_RadioGroupWidget();
    radio->SetOutsetSpacing(0, 15, 0, 0);
    radio->SetCellPos(1, 0, 1, 1);
    CDH_RadioButtonWidget* onButton = new CDH_RadioButtonWidget();
    onButton->Init(true, ResString(onLabel).Upper(), 15);
    onButton->SetCellPos(0, 0, 1, 1);
    radio->AddButton(onButton);
    CDH_RadioButtonWidget* offButton = new CDH_RadioButtonWidget();
    offButton->SetCellPos(1, 0, 1, 1);
    offButton->Init(false, ResString(offLabel).Upper(), 15);
    radio->AddButton(offButton);
    radio->SetValue(value);
    row->AddToFront(radio);
    *group = radio;
    return row;
}
