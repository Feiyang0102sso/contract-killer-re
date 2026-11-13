#include "CSettingsWindow.h"
#include "App.h"
#include "AppMedia.h"
#include "AppSettings.h"
#include "BaseDialog.h"
#include "CBaseScreen.h"
#include "CFontMgr.h"
#include "CHelpScreen.h"
#include "CImageButton.h"
#include "CSwerveGame.h"
#include "CDH_RadioButtonWidget.h"
#include "CDH_RadioGroupWidget.h"
#include "SScreenAdaptingConstant.h"
#include "CTutorialManager.h"
#include "CSaveManager.h"
#include "DGHelper.h"

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
    ResetProgress = 0x74146762
};

inline __attribute__((always_inline)) void AddFrameTile(Window* frame, ImageWindow::EFillType fill, const char* resource, int column, int row)
{
    ImageWindow* tile = new ImageWindow(fill, ImageRes(resource));
    tile->SetLayoutType(Window::LAYOUT_TABLE);
    tile->SetCellPos(column, row, 1, 1);
    frame->AddToFront(tile);
}

// The header frame builds each tile with the default ctor and SetImage.
inline __attribute__((always_inline)) void AddHeaderTile(Window* frame, ImageWindow::EFillType fill, const char* resource, int column)
{
    ImageWindow* tile = new ImageWindow();
    tile->SetFillType(fill);
    tile->SetImage(ImageRes(resource));
    tile->SetLayoutType(Window::LAYOUT_TABLE);
    tile->SetCellPos(column, 0, 1, 1);
    frame->AddToFront(tile);
}
}

CSettingsWindow::CSettingsWindow(bool pauseMenu, bool returnToGPS)
    : Window(false), m_pauseMenu(pauseMenu), m_returnToGPS(returnToGPS)
{
    SetWidthByContent(0, 0);
    SetHeightByContent(0, 0);

    Window* header = new Window(false);
    header->SetPercentWidth(100, 0, 0);
    header->SetHeightByContent(0, 0);
    header->SetLayoutType(LAYOUT_TABLE);
    header->SetCellPos(0, 0, 1, 1);
    Window* headerFrame = new Window(false);
    headerFrame->SetPercentWidth(50, 0, 0);
    headerFrame->SetHeightByContent(0, 0);
    headerFrame->SetAlign(ALIGN_BOTTOM);
    headerFrame->SetAlign(ALIGN_HCENTER | ALIGN_BOTTOM);
    AddHeaderTile(headerFrame, ImageWindow::FILL_NONE, "SUR_FRAME_PAUSE_LEFT_TOP", 0);
    AddHeaderTile(headerFrame, ImageWindow::FILL_HORIZONTAL, "SUR_FRAME_PAUSE_TOP", 1);
    AddHeaderTile(headerFrame, ImageWindow::FILL_NONE, "SUR_FRAME_PAUSE_RIGHT_TOP", 2);
    header->AddToFront(headerFrame);
    m_title = new TextWindow(ResString("IDS_MENU_SETTINGS").Upper(), CFontMgr::GetInstance()->GetFont(15));
    m_title->SetLayoutType(LAYOUT_SIMPLE);
    m_title->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
    header->AddToFront(m_title);
    AddToFront(header);

    Window* host = new Window(false);
    host->SetPercentWidth(100, 0, 0);
    host->SetHeightByContent(0, 0);
    host->SetLayoutType(LAYOUT_TABLE);
    host->SetCellPos(0, 1, 1, 1);
    AddFrameTile(host, ImageWindow::FILL_NONE, "SUR_FRAME_PAUSE_LEFT_TOP", 0, 0);
    // The top edge sits in its own cell window and stretches over it.
    Window* top = new Window(false);
    top->SetLayoutType(LAYOUT_TABLE);
    top->SetCellPos(1, 0, 1, 1);
    host->AddToFront(top);
    ImageWindow* topTile = new ImageWindow(ImageWindow::FILL_HORIZONTAL, ImageRes("SUR_FRAME_PAUSE_TOP"));
    topTile->SetPercentWidth(100, 0, 0);
    topTile->SetLayoutType(LAYOUT_TABLE);
    topTile->SetCellPos(0, 0, 1, 1);
    top->AddToFront(topTile);
    AddFrameTile(host, ImageWindow::FILL_NONE, "SUR_FRAME_PAUSE_RIGHT_TOP", 2, 0);
    AddFrameTile(host, ImageWindow::FILL_VERTICAL, "SUR_FRAME_PAUSE_LEFT", 0, 1);
    AddFrameTile(host, ImageWindow::FILL_BOTH, "SUR_FRAME_PAUSE_CENTER", 1, 1);
    AddFrameTile(host, ImageWindow::FILL_VERTICAL, "SUR_FRAME_PAUSE_RIGHT", 2, 1);
    AddFrameTile(host, ImageWindow::FILL_NONE, "SUR_FRAME_PAUSE_LEFT_DOWN", 0, 2);
    AddFrameTile(host, ImageWindow::FILL_HORIZONTAL, "SUR_FRAME_PAUSE_DOWN", 1, 2);
    AddFrameTile(host, ImageWindow::FILL_NONE, "SUR_FRAME_PAUSE_RIGHT_DOWN", 2, 2);

    ImageWindow* background = new ImageWindow(ImageWindow::FILL_NONE, ImageRes("SUR_FRAME_PAUSE_BG"));
    background->SetLayoutType(LAYOUT_SIMPLE);
    background->SetAlign(ALIGN_LEFT | ALIGN_BOTTOM);
    int bottomInset = SScreenAdaptingConstant(10, 15, 15)();
    int leftInset = SScreenAdaptingConstant(10, 15, 15)();
    background->SetOutsetSpacing(0, 0, bottomInset, leftInset);
    host->AddToFront(background);

    Window* content;
    if (m_pauseMenu) {
        content = CreatePauseMenu();
    } else {
        content = CreateSettingMenu();
    }
    m_content = content;
    m_contentHost = host;
    ShowContent(content);
    AddToFront(host);
}

CSettingsWindow::~CSettingsWindow()
{
}

inline __attribute__((always_inline)) CImageButton* CSettingsWindow::AddItem(Window* content, const char* label, int command, int row)
{
    CImageButton* button = new CImageButton();
    button->SetText(ResString(label).Upper(), ALIGN_HCENTER | ALIGN_VCENTER, NULL, 0, 0);
    button->SetCellPos(0, row, 1, 1);
    button->SetCommandParameters(command, 0, 0, 0);
    content->AddToFront(button);
    return button;
}

Window* CSettingsWindow::CreatePauseMenu()
{
    SetPercentWidth(40, 0, 0);
    m_title->SetText(ResString("IDS_PAUSED").Upper(), CFontMgr::GetInstance()->GetFont(15));
    Window* content = new Window(false);
    content->SetPercentWidth(90, 0, 0);
    content->SetInsetSpacing(App::Scale() * 20, 0);
    content->SetHeightByContent(0, 0);
    content->SetLayoutType(LAYOUT_SIMPLE);
    content->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    AddItem(content, "IDS_CONTINUE", Continue, 0);
    content->AddToFront(CBaseScreen::createLineSplitter(0, 1));
    int row = 2;
    if (m_returnToGPS) {
        AddItem(content, "IDS_MENU_RETURN_TO_GPS", AskReturnToGPS, 2);
        content->AddToFront(CBaseScreen::createLineSplitter(0, 3));
        row = 4;
    }
    if (m_pauseMenu && static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsShowObjective()) {
        AddItem(content, "IDS_MENU_OBJECTIVE", Objective, row);
        ++row;
        content->AddToFront(CBaseScreen::createLineSplitter(0, row));
        ++row;
    }
    AddItem(content, "IDS_MENU_SETTINGS", Settings, row);
    return content;
}

Window* CSettingsWindow::CreateSettingMenu()
{
    SetPercentWidth(70, 0, 0);
    m_title->SetText(XString(L"Settings"), CFontMgr::GetInstance()->GetFont(15));
    Window* content = new Window(false);
    content->SetPercentWidth(90, 0, 0);
    content->SetHeightByContent(0, 0);
    content->SetInsetSpacing(App::Scale() * 20, 0, 0, 0);
    content->SetLayoutType(LAYOUT_SIMPLE);
    content->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    bool enabled = static_cast<App*>(WindowApp::m_instance)->m_settings->IsSoundEnabled();
    Window* row = CreateRadioGroup(&m_sound, "IDS_MENU_SOUND", "IDS_ON", "IDS_OFF", !enabled);
    row->SetAlign(ALIGN_RIGHT);
    row->SetCellPos(0, 0, 1, 1);
    content->AddToFront(row);

    enabled = static_cast<App*>(WindowApp::m_instance)->m_settings->IsMusicEnabled();
    row = CreateRadioGroup(&m_music, "IDS_MENU_MUSIC", "IDS_ON", "IDS_OFF", !enabled);
    row->SetLayoutType(LAYOUT_TABLE);
    row->SetCellPos(0, 1, 1, 1);
    content->AddToFront(row);

    enabled = static_cast<App*>(WindowApp::m_instance)->m_settings->IsFireButtonEnabled();
    row = CreateRadioGroup(&m_fireButton, "IDS_MENU_FIRE_BUTTON", "IDS_ON", "IDS_OFF", !enabled);
    row->SetLayoutType(LAYOUT_TABLE);
    row->SetCellPos(0, 2, 1, 1);
    content->AddToFront(row);

    enabled = static_cast<App*>(WindowApp::m_instance)->m_settings->IsLocalPushEnabled();
    row = CreateRadioGroup(&m_push, "IDS_MENU_PUSHES", "IDS_ON", "IDS_OFF", !enabled);
    row->SetLayoutType(LAYOUT_TABLE);
    row->SetCellPos(0, 3, 1, 1);
    content->AddToFront(row);

    enabled = static_cast<App*>(WindowApp::m_instance)->m_settings->IsFBReminderEnabled();
    row = CreateRadioGroup(&m_facebook, "IDS_MENU_FACEBOOK_REMINDER", "IDS_ON", "IDS_OFF", !enabled);
    row->SetLayoutType(LAYOUT_TABLE);
    row->SetCellPos(0, 4, 1, 1);
    content->AddToFront(row);

    Window* helpRow = new Window(false);
    helpRow->SetHeightByContent(0, 0);
    helpRow->SetLayoutType(LAYOUT_SIMPLE);
    helpRow->SetCellPos(0, 5, 1, 1);
    CImageButton* help = new CImageButton();
    help->SetText(ResString("IDS_MENU_HELP"), ALIGN_HCENTER | ALIGN_VCENTER, NULL, 0, 0);
    help->SetLayoutType(LAYOUT_SIMPLE);
    help->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    help->SetCommandParameters(Help, 0, 0, 0);
    helpRow->AddToFront(help);
    content->AddToFront(helpRow);

    Window* buttons = new Window(false);
    buttons->SetHeightByContent(0, 0);
    buttons->SetLayoutType(LAYOUT_TABLE);
    buttons->SetCellPos(0, 6, 1, 1);
    CImageButton* back = new CImageButton();
    back->SetText(ResString("IDS_MENU_BACK").Upper(), ALIGN_HCENTER | ALIGN_TOP, NULL, 0, 0);
    back->SetLayoutType(LAYOUT_SIMPLE);
    back->SetAlign(ALIGN_LEFT | ALIGN_BOTTOM);
    back->SetCommandParameters(Back, 0, 0, 0);
    buttons->AddToFront(back);
    CImageButton* done = new CImageButton();
    done->SetText(ResString("IDS_BTN_DONE"), ALIGN_HCENTER | ALIGN_TOP, NULL, 0, 0);
    done->SetLayoutType(LAYOUT_SIMPLE);
    done->SetAlign(ALIGN_RIGHT | ALIGN_BOTTOM);
    done->SetCommandParameters(Done, 0, 0, 0);
    buttons->AddToFront(done);
    content->AddToFront(buttons);
    return content;
}

void CSettingsWindow::ShowContent(Window* content)
{
    // the original neither closes the previous content nor remembers the new one
    EasyParams params(this, EasyLib::IN_CUBIC, EasyLib::IN_CUBIC);
    params.SetFullTime(2.5f);
    params.SetOpacity(0.0f, 1.0f);
    m_contentHost->AddToFront(content);
    content->StartEasy(params, FLAG_SHOW_AFTER_EASY);
}

void CSettingsWindow::OnStartupEasy()
{
    EasyParams params(this, EasyLib::IN_CUBIC, EasyLib::IN_CUBIC);
    params.SetFullTime(0.5f);
    params.SetOpacity(0.0f, 1.0f);
    StartEasy(params, FLAG_SHOW_AFTER_EASY);
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
    switch (event.m_id) {
    case Settings:
        if (m_content != NULL) {
            m_content->ReverseEasy(FLAG_CLOSE_AFTER_EASY);
        }
        m_content = CreateSettingMenu();
        ShowContent(m_content);
        return;
    case Help: {
        CHelpScreen* help = new CHelpScreen();
        static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddModal(help);
        return;
    }
    case ResetProgress:
        static_cast<App*>(WindowApp::m_instance)->m_saveManager->PauseSaving();
        static_cast<App*>(WindowApp::m_instance)->m_saveManager->Reset();
        WindowApp::HandleTunnelCommand(DisplayWindow::COMMAND_EXIT_CONSUMED, 0, 0, 0);
        event.Clear();
        return;
    case Back:
        if (m_pauseMenu) {
            m_content->ReverseEasy(FLAG_CLOSE_AFTER_EASY);
            m_content = CreatePauseMenu();
            ShowContent(m_content);
        } else {
            ReverseEasy(FLAG_CLOSE_AFTER_EASY);
        }
        return;
    case Continue:
        ReverseEasy(FLAG_CLOSE_AFTER_EASY);
        event.Clear();
        return;
    case AskReturnToGPS:
        static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->ShowReturnToGpsConfirmation();
        Close();
        event.Clear();
        return;
    case Objective:
        static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->ShowObjective();
        Close();
        return;
    case Done: {
        AppSettings* settings = static_cast<App*>(WindowApp::m_instance)->m_settings;
        settings->SetMusicEnabled(m_music->GetValue() == 0);
        static_cast<App*>(WindowApp::m_instance)->m_settings->SetSoundEnabled(m_sound->GetValue() == 0);
        static_cast<App*>(WindowApp::m_instance)->m_media->Update();
        static_cast<App*>(WindowApp::m_instance)->m_settings->SetFireButtonEnabled(m_fireButton->GetValue() == 0);
        static_cast<App*>(WindowApp::m_instance)->m_settings->SetLocalPushEnabled(m_push->GetValue() == 0);
        static_cast<App*>(WindowApp::m_instance)->m_settings->SetFBReminderEnabled(m_facebook->GetValue() == 0);
        static_cast<App*>(WindowApp::m_instance)->m_settings->Save();
        // Original: re-dispatches a Back command to itself.
        Event back(Event::NONE, Back, 0, 0, 0);
        OnCommand(back);
        return;
    }
    default:
        return;
    }
}

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AssertSettingsWindowSize[sizeof(CSettingsWindow) == 0xcc ? 1 : -1];
#endif

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
