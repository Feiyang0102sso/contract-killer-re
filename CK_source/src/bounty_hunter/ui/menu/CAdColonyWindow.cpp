#include "CAdColonyWindow.h"

#include "App.h"
#include "AppSettings.h"
#include "CBH_Player.h"
#include "CCheckBox.h"
#include "CDialogWindow.h"
#include "CFontMgr.h"
#include "SScreenAdaptingConstant.h"

namespace {
const int PlayVideo = -0x170d674;
const int CancelVideo = -0x57785c74;
const int DisableFeaturedInstall = 0x5320af;
}

CAdColonyWindow::CAdColonyWindow(bool featuredInstall)
    : Window(false), m_credits(AdColony_Facade::getAwardAmount(NULL)), m_skipFeatured(NULL),
      m_soundEnabled(false), m_musicEnabled(false), m_finished(false), m_playing(false)
{
    if (m_credits < 1) {
        ShowErrorDialog();
        return;
    }
    const char* title = "IDS_ADCOLONY_SHOW_VIDEO_DIALOG_TITLE";
    const char* message = "IDS_ADCOLONY_SHOW_VIDEO_DIALOG_TEXT";
    if (featuredInstall) {
        title = "IDS_ADCOLONY_FEATURED_INSTALL_TITLE";
        message = "IDS_ADCOLONY_FEATURED_INSTALL_TEXT";
        if (m_credits == 1) {
            message = "IDS_ADCOLONY_FEATURED_INSTALL_ONE_CREDIT_TEXT";
        }
    } else if (m_credits == 1) {
        message = "IDS_ADCOLONY_EARN_ONE_CREDIT";
    }

    CTextDialog* dialog = new CTextDialog();
    dialog->SetTitleText(title);
    dialog->SetMessageText(XString::Format(ResString(message).c_str(), m_credits));
    dialog->SetButtons(CDialogWindow::BUTTON_OK, CDialogWindow::BUTTON_IGNORE);
    dialog->SetButtonsCommands(PlayVideo, CancelVideo, -1);
    if (featuredInstall) {
        Window* option = new Window(false);
        option->SetPercentHeight(10, 0, 0);
        option->SetPercentWidth(50, 0, 0);
        const SScreenAdaptingConstant bottom(50, 100);
        option->SetOutsetSpacing(0, 0, 0, bottom());

        CFont* font = CFontMgr::GetInstance()->GetFont(15);
        TextWindow* label = new TextWindow(ResString("IDS_ADCOLONY_CHECKBOX_DISABLE_FEATURED_INSTALL"), font);
        const SScreenAdaptingConstant left(5, 10);
        label->SetOutsetSpacing(left(), 0, 0, 0);
        option->AddToFront(label, 1, 0);

        m_skipFeatured = new CCheckBox(0);
        option->AddToFront(m_skipFeatured, 0, 0);
        option->SetLayoutType(LAYOUT_SIMPLE);
        option->SetAlign(ALIGN_LEFT | ALIGN_VCENTER);
        dialog->AddToFront(option);
    }
    static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddModal(dialog);
}

void CAdColonyWindow::Shutdown()
{
    AdColony_Facade::cancelVideoCallback(this);
    if (m_playing) {
        AppSettings* settings = static_cast<App*>(WindowApp::m_instance)->m_settings;
        settings->SetMusicEnabled(m_musicEnabled);
        settings->SetSoundEnabled(m_soundEnabled);
    }
}

void CAdColonyWindow::OnVideoEnded(bool completed, const char*)
{
    if (!m_playing) {
        return;
    }
    m_playing = false;
    if (completed) {
        CBH_Player::GetInstance()->AddCredits(m_credits);
        ShowCreditsEarnedDialog(m_credits);
    } else {
        ShowErrorDialog();
    }

    AppSettings* settings = static_cast<App*>(WindowApp::m_instance)->m_settings;
    settings->SetMusicEnabled(m_musicEnabled);
    settings->SetSoundEnabled(m_soundEnabled);
}

void CAdColonyWindow::ShowCreditsEarnedDialog(int credits)
{
    const char* text = "IDS_ADCOLONY_VIDEO_SHOWED_TEXT";
    if (credits < 2) {
        text = "IDS_ADCOLONY_ONE_CREDIT_EARNED_TEXT";
    }
    CTextDialog* dialog = new CTextDialog();
    dialog->SetTitleText("IDS_ADCOLONY_VIDEO_SHOWED_TITLE");
    dialog->SetMessageText(XString::Format(ResString(text).c_str(), credits));
    dialog->SetButtons(CDialogWindow::BUTTON_OK);
    static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddModal(dialog);
    m_finished = true;
}

void CAdColonyWindow::ShowErrorDialog()
{
    CTextDialog* dialog = new CTextDialog();
    dialog->SetTitleText("IDS_ADCOLONY_ERROR_TITLE");
    dialog->SetMessageText(ResString("IDS_ADCOLONY_ERROR_TEXT"));
    dialog->SetButtons(CDialogWindow::BUTTON_OK);
    static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddModal(dialog);
    m_finished = true;
}

void CAdColonyWindow::OnCommand(Event& event)
{
    if (m_finished || m_playing || (event.m_id != PlayVideo && event.m_id != CancelVideo)) {
        return;
    }
    if (m_skipFeatured != NULL && m_skipFeatured->IsCheched()) {
        WindowApp::HandleTunnelCommand(DisableFeaturedInstall, 0, 0, 0);
    }
    m_skipFeatured = NULL;
    if (event.m_id == CancelVideo) {
        m_finished = true;
    } else {
        AppSettings* settings = static_cast<App*>(WindowApp::m_instance)->m_settings;
        m_soundEnabled = settings->IsSoundEnabled();
        m_musicEnabled = settings->IsMusicEnabled();
        settings->SetMusicEnabled(false);
        settings->SetSoundEnabled(false);
        m_playing = true;
        if (AdColony_Facade::isReady(NULL)) {
            AdColony_Facade::showVideo(this, NULL, false, false);
        } else {
            OnVideoEnded(false, NULL);
        }
    }
    event.Clear();
}

void CAdColonyWindow::Update()
{
    Window::Update();
    if (m_finished) {
        Close();
    }
}
