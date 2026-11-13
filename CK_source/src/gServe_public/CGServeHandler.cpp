// CK account login progress and network dialogs.
#include "CGServeHandler.h"
#include "App.h"
#include "AppSettings.h"
#include "CBHUtil_Cocoa.h"
#include "CDialogWindow.h"
#include "CImportantDialog.h"
#include "CConflictLogoutPrompt.h"
#include "CSaveConflictWindow.h"
#include "CGameApp.h"
#include "CNGS.h"
#include "CNGSLocalUser.h"
#include "CNGSLoginFlow.h"
#include "CNGSSocialInterface.h"
#include "CNGS_Platform.h"
#include "CNotificationHandler.h"
#include "CPrivateData1.h"
#include "CProfileManager.h"
#include "CProgressSystem.h"
#include "CSaveManager.h"
// Header statics of the original unit (static initializer: banner tables, then two class ids).
#include "AdBannerSizes.h"
#include "CNGSOfferManager.h"
#include "ICInAppPurchase.h"

namespace {
const int LoginLater = (int)0x83461980;
const int LoginNow = (int)0xc7b99830;
const int OfflineAccepted = (int)0x8b50a871;
const int RestartAfterLogin = 0x42650932;
const int LoginFinished = 3;
const int LogoutFacebook = 0x09f954a7;
const int LogoutGameCenter = 0x194eae3b;
const int AskGameCenter = 0x5be239f5;
const int ShowProfileConflict = 0x07e17dc8;
const int ShowProfileConflictHelp = 0x7d8ce451;
const int ShowAccountConflictHelp = (int)0x8a79f26d;
const int AskUseLocal = 0x7b7b47f1;
const int AskUseServer = (int)0xb64f7bf5;
const int UseLocal = 0x522fba3b;
const int UseServer = 0x1efcf942;
const int ForceFacebook = LoginNow + 1;

App* CurrentApp()
{
    return static_cast<App*>(WindowApp::m_instance);
}

// Explicit inline helper (name guessed): the original has CloseCurrentSocialNetworkConnectDialog() out of
// line and the same code expanded in all six callers of this file, which needs the inline hint.
inline bool CloseLoginProgressDialog()
{
    Window* dialog = CurrentApp()->m_loginProgressDialog;
    CurrentApp()->m_field398 = -1;
    if (dialog != NULL) {
        dialog->Close();
        CurrentApp()->m_loginProgressDialog = NULL;
        return true;
    }
    return false;
}
}

// Facebook application of CK (setAppID in Init)
static const wchar_t FACEBOOK_API_SECRET[] = L"";
static const wchar_t FACEBOOK_APP_ID[] = L"209864595695358";

CGServeHandler::CGServeHandler()
{
}

CGServeHandler::~CGServeHandler()
{
}

void CGServeHandler::Init()
{
    m_offlineAccepted = false;
    CNGSSocialInterface* facebook = CNGS_Platform::GetInstance()->getSocialInterface(SOCIAL_INTERFACE_FACEBOOK);
    facebook->setAppID(CStrWChar(FACEBOOK_APP_ID), CStrWChar(FACEBOOK_API_SECRET));
    m_accountConflictDialog = NULL;
    m_pendingSwitchNetwork = 0;
}

void CGServeHandler::HandleUpdate()
{
    if (!CNGSLoginFlow::GetInstance()->IsRunning() && m_wasLoginRunning) {
        CloseLoginProgressDialog();
        WindowApp::HandleTunnelCommand(LoginFinished, 0, 0, 0);
    }
    m_wasLoginRunning = CNGSLoginFlow::GetInstance()->IsRunning();
}

bool CGServeHandler::CloseCurrentSocialNetworkConnectDialog()
{
    return CloseLoginProgressDialog();
}

bool CGServeHandler::OnCommand(Event& event)
{
    switch (event.m_id) {
    case LoginNow:
    case ForceFacebook: {
        if (CurrentApp()->m_socialConnectDialog != NULL) {
            CurrentApp()->m_socialConnectDialog->Close();
            CurrentApp()->m_socialConnectDialog = NULL;
        }
        CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
        if (user->isValid() && user->isAuthenticated(SOCIAL_INTERFACE_FACEBOOK)) {
            CDialogWindow* dialog = new CDialogWindow(true);
            dialog->SetButtons(CDialogWindow::BUTTON_OK, CDialogWindow::BUTTON_IGNORE);
            dialog->SetMessageText(Window::ResString("IDS_SWITCH_USER_TEXT_FB"));
            dialog->SetTitleText(Window::ResString("IDS_SWITCH_USER_TITLE"));
            dialog->SetButtonsCommands(LogoutFacebook, -1, -1);
            CurrentApp()->m_mainLayer->AddModal(dialog);
        } else if (m_pendingSwitchNetwork == SOCIAL_INTERFACE_FACEBOOK) {
            CurrentApp()->StartLogin(App::LoginBothNetworks, true);
        } else if (CNGSLoginFlow::GetInstance()->IsRunning()) {
            CNGSLoginFlow::GetInstance()->ExcuteStep(-1);
        } else if (event.m_id == LoginNow) {
            CurrentApp()->StartLogin(App::LoginFacebook, true);
        } else {
            CurrentApp()->StartLogin(App::LoginForceFacebook, true);
        }
        break;
    }
    case 2:
        if (CNGSLoginFlow::GetInstance()->IsRunning()) {
            CNGSLoginFlow::GetInstance()->ExcuteStep(-1);
        }
        App::m_bIsLoginOK = true;
        break;
    case ShowProfileConflict:
        ShowSocialNetworkProfileDataConflict();
        break;
    case LogoutFacebook:
        CNGS::GetInstance()->GetLocalUser()->Logout(SOCIAL_INTERFACE_FACEBOOK);
        if (m_accountConflictDialog != NULL) {
            m_accountConflictDialog->Close();
        }
        m_pendingSwitchNetwork = SOCIAL_INTERFACE_FACEBOOK;
        WindowApp::HandleTunnelCommand(LoginLater, 0, 0, 0);
        break;
    case LogoutGameCenter:
        if (m_accountConflictDialog != NULL) {
            m_accountConflictDialog->Close();
        }
        m_pendingSwitchNetwork = SOCIAL_INTERFACE_GAME_CENTER;
        CBHUtil_Cocoa::LaunchGameCenter();
        break;
    case UseLocal: {
        CurrentApp()->m_socialConnectDialog = NULL;
        CGameApp::gserveDataConflict = false;
        TCVector<int> collections;
        CurrentApp()->m_profileManager->getAllRegisteredCollections(collections);
        CurrentApp()->m_profileManager->useDataFromLocal(collections);
        CurrentApp()->m_profileManager->save(collections, true);
        CurrentApp()->m_privateData->setContentFileConflictStatus(CONTENT_FILE_CONFLICT_RESOLVED);
        if (CNGSLoginFlow::GetInstance()->IsRunning()) {
            CNGSLoginFlow::GetInstance()->OnEvent(LoginFlowSuccess, "EventId_GServeProfileDataUseLocalConfirm");
        }
        break;
    }
    case UseServer: {
        CurrentApp()->m_socialConnectDialog = NULL;
        CGameApp::gserveDataConflict = false;
        TCVector<int> collections;
        CurrentApp()->m_profileManager->getAllRegisteredCollections(collections);
        SelectProfileServerData(collections);
        WindowApp::HandleTunnelCommand(RestartAfterLogin, 0, 0, 0);
        CurrentApp()->m_saveManager->PauseByGServe(1);
        break;
    }
    case AskGameCenter: {
        CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
        if (user->isValid() && user->isAuthenticated(SOCIAL_INTERFACE_GAME_CENTER)) {
            CApplet::m_pCore->LaunchURL(L"gamecenter:", 0);
        } else if (CBHUtil_Cocoa::IsGameCenterAuthenticated() == true) {
            CurrentApp()->StartLogin(App::LoginGameCenter, true);
        } else {
            CBHUtil_Cocoa::LaunchGameCenter();
        }
        break;
    }
    case LoginLater:
        CurrentApp()->m_socialConnectDialog = NULL;
        if (m_pendingSwitchNetwork == SOCIAL_INTERFACE_FACEBOOK) {
            m_pendingSwitchNetwork = 0;
            CurrentApp()->StartLogin(App::LoginGameCenter, true);
        } else {
            CNGSLoginFlow::GetInstance()->OnEvent(LoginFlowCancel, "EventId_GServeLoginFacebookLater");
        }
        break;
    case ShowAccountConflictHelp:
        ShowSocialNetworkAccountsConflictHelpDialogs();
        break;
    case AskUseServer:
        ShowSocialNetworkProfileDataConflictUseServer();
        break;
    case ShowProfileConflictHelp:
        ShowSocialNetworkProfileDataConflictHelp();
        break;
    case AskUseLocal:
        ShowSocialNetworkProfileDataConflictUseLocal();
        break;
    case 1:
        CNGSLoginFlow::GetInstance()->Reset();
        break;
    }
    return false;
}

void CGServeHandler::ShowSocialNetworkAccountsConflictHelpDialogs()
{
    CImportantDialog* dialog = new CImportantDialog();
    dialog->SetTitle(Window::ResString("IDS_MENU_HELP"));
    dialog->SetMessageText(Window::ResString("IDS_ACCOUNT_SAVE_CONFLICT_INSTRUCTIONS"));
    dialog->SetPercentWidth(70, 0, 0);
    dialog->SetPercentHeight(70, 0, 0);
    dialog->SetLayoutType(Window::LAYOUT_SIMPLE);
    dialog->SetAlign(0x24);
    dialog->SetCommand(0x2ddd8a7d);
    CurrentApp()->m_mainLayer->AddModal(dialog);
}

void CGServeHandler::SelectProfileServerData(const TCVector<int>& collections)
{
    App* app = CurrentApp();
    AppSettings* settings = app->m_settings;
    app->m_field3a0 = true;
    CurrentApp()->m_profileManager->useDataFromServer(collections);
    settings->SetSoundEnabled(app->m_privateData->getSoundSetting() != 0);
    settings->SetMusicEnabled(app->m_privateData->getMusicSetting() != 0);
    settings->SetFireButtonEnabled(app->m_privateData->getFireBtnSetting() != 0);
    settings->SetLocalPushEnabled(app->m_privateData->getNotificationSetting() != 0);
    settings->Save();
}

void CGServeHandler::ShowSocialNetworkProfileDataConflictHelp()
{
    CImportantDialog* dialog = new CImportantDialog();
    dialog->SetMessageText(Window::ResString("IDS_ACCOUNT_CONFLICT_INSTRUCTIONS"));
    dialog->SetTitle(Window::ResString("IDS_DIALOG_TITLE_WARNING"));
    dialog->SetCommand(ShowProfileConflict);
    dialog->SetPercentWidth(60, 0, 0);
    dialog->SetPercentHeight(60, 0, 0);
    CurrentApp()->m_mainLayer->AddModal(dialog);
    CurrentApp()->m_socialConnectDialog = dialog;
}

void CGServeHandler::ShowSocialNetworkProfileDataConflict()
{
    CNotificationHandler* handler = CNGS::GetInstance()->GetLocalUser()->m_notificationHandler;
    CProgressSystem* progress = CurrentApp()->InitProgressSystem();
    CSaveConflictWindow::SConflictInfo info;
    info.m_localSoftCurrency = handler->m_localSoftCurrency;
    info.m_localHardCurrency = handler->m_localHardCurrency;
    info.m_localLevel = progress->GetLevel(handler->m_localExperience);
    info.m_serverSoftCurrency = handler->m_serverSoftCurrency;
    info.m_serverHardCurrency = handler->m_serverHardCurrency;
    info.m_serverLevel = progress->GetLevel(handler->m_serverExperience);
    CSaveConflictWindow* window = new CSaveConflictWindow(info);
    window->SetLayoutType(Window::LAYOUT_SIMPLE);
    window->SetAlign(0x24);
    CurrentApp()->m_mainLayer->AddModal(window);
    CurrentApp()->m_socialConnectDialog = window;
}

void CGServeHandler::ShowSocialNetworkProfileDataConflictUseLocal()
{
    CDialogWindow* dialog = new CDialogWindow(true);
    dialog->SetButtons(CDialogWindow::BUTTON_OK, CDialogWindow::BUTTON_IGNORE);
    dialog->SetMessageText(Window::ResString("IDS_USE_LOCAL_SAVE_CONFIRMATION"));
    dialog->SetTitleText(Window::ResString("IDS_ARE_YOU_SURE"));
    dialog->SetButtonsCommands(UseLocal, ShowProfileConflict, -1);
    CurrentApp()->m_mainLayer->AddModal(dialog);
    CurrentApp()->m_socialConnectDialog = dialog;
}

void CGServeHandler::ShowSocialNetworkProfileDataConflictUseServer()
{
    CDialogWindow* dialog = new CDialogWindow(true);
    dialog->SetButtons(CDialogWindow::BUTTON_OK, CDialogWindow::BUTTON_IGNORE);
    dialog->SetMessageText(Window::ResString("IDS_USE_SERVER_SAVE_CONFIRMATION"));
    dialog->SetTitleText(Window::ResString("IDS_ARE_YOU_SURE"));
    dialog->SetButtonsCommands(UseServer, ShowProfileConflict, -1);
    CurrentApp()->m_mainLayer->AddModal(dialog);
    CurrentApp()->m_socialConnectDialog = dialog;
}

void CGServeHandler::ShowFacebookLoginDialog()
{
    CFacebookLoginDialog* dialog = new CFacebookLoginDialog();
    dialog->SetButtons(CDialogWindow::BUTTON_LATER, CDialogWindow::BUTTON_OK);
    dialog->SetMessageText(Window::ResString("IDS_GSERV_LOGIN_FB"));
    dialog->SetButtonsCommands(LoginLater, LoginNow, -1);
    CurrentApp()->m_mainLayer->AddModal(dialog);
    CurrentApp()->m_socialConnectDialog = dialog;
}

void CGServeHandler::ShowSocialNetworkConnectDialogs(int network)
{
    CloseLoginProgressDialog();
    CDialogWindow* dialog = new CDialogWindow(true);
    dialog->SetMessageText(Window::ResString("IDS_CONNECTING"));
    switch (network) {
    case SOCIAL_INTERFACE_FACEBOOK:
        dialog->SetTitleText(Window::ResString("IDS_FB"));
        break;
    case SOCIAL_INTERFACE_GAME_CENTER:
        dialog->SetTitleText(Window::ResString("IDS_GC"));
        break;
    case -1:
        dialog->SetTitleText(Window::ResString("IDS_CONNECTING"));
        break;
    }
    CurrentApp()->m_mainLayer->AddModal(dialog);
    CurrentApp()->m_loginProgressDialog = dialog;
    CurrentApp()->m_field398 = network;
}

void CGServeHandler::ShowSocialNetworkAccountsConflictDialogs()
{
    CurrentApp()->m_saveManager->PauseByGServe(1);
    m_accountConflictDialog = new CConflictLogoutPrompt();
    m_accountConflictDialog->SetLayoutType(Window::LAYOUT_SIMPLE);
    m_accountConflictDialog->SetAlign(0x24);
    m_accountConflictDialog->SetTitle(Window::ResString("IDS_DIALOG_TITLE_WARNING"));
    m_accountConflictDialog->SetPercentWidth(70, 0, 0);
    m_accountConflictDialog->SetPercentHeight(80, 0, 0);
    CurrentApp()->m_mainLayer->AddModal(m_accountConflictDialog);
}

void CGServeHandler::ShowSocialNetworkPrimaryUserChanged()
{
    CloseLoginProgressDialog();
    CDialogWindow* dialog = new CDialogWindow(true);
    dialog->SetMessageText(Window::ResString("IDS_ACCOUNT_LOGOUT_DESC"));
    dialog->SetTitleText("");
    dialog->SetButtons(CDialogWindow::BUTTON_OK);
    dialog->SetButtonsCommands(RestartAfterLogin, -1, -1);
    CurrentApp()->m_mainLayer->AddModal(dialog);
    CGameApp::GetInstance()->m_accountChanged = false;
}

void CGServeHandler::ShowSocialNetworkSessionInvalidDialog()
{
    CloseLoginProgressDialog();
    CDialogWindow* dialog = new CDialogWindow(true);
    dialog->SetButtons(CDialogWindow::BUTTON_OK);
    dialog->SetButtonsCommands(RestartAfterLogin, -1, -1);
    dialog->SetMessageText(Window::ResString("IDS_SESSION_INVALID"));
    CurrentApp()->m_mainLayer->AddModal(dialog);
}

void CGServeHandler::ShowSocialNetworkConnectionErrorDialog()
{
    App::m_bIsLoginOK = false;
    CloseLoginProgressDialog();
    CDialogWindow* dialog = new CDialogWindow(true);
    dialog->SetButtons(CDialogWindow::BUTTON_RETRY, CDialogWindow::BUTTON_ACKNOWLEDGE);
    dialog->SetButtonsCommands(2, 1, -1);
    dialog->SetMessageText(Window::ResString("IDS_CONNECTION_ERROR_MESSAGE"));
    CurrentApp()->m_mainLayer->AddModal(dialog);
}

void CGServeHandler::ShowOfflineModeDialog()
{
    CloseLoginProgressDialog();
    CDialogWindow* dialog = new CDialogWindow(true);
    dialog->SetButtons(CDialogWindow::BUTTON_OK);
    dialog->SetTitleText(Window::ResString("IDS_ACCOUNT_OFFLINE_TITLE"));
    dialog->SetMessageText(Window::ResString("IDS_ACCOUNT_OFFLINE_DESC"));
    dialog->SetButtonsCommands(OfflineAccepted, -1, -1);
    CurrentApp()->m_mainLayer->AddModal(dialog);
}

void CGServeHandler::ShowLoginRequired()
{
    CDialogWindow* dialog = new CDialogWindow(true);
    dialog->SetButtons(CDialogWindow::BUTTON_LATER, CDialogWindow::BUTTON_OK);
    dialog->SetMessageText(Window::ResString("IDS_GSERV_LOGIN_REQUIRED"));
    dialog->SetButtonsCommands(LoginLater, LoginNow, -1);
    CurrentApp()->m_mainLayer->AddModal(dialog);
    CurrentApp()->m_socialConnectDialog = dialog;
}

bool CGServeHandler::NeedToShowFacebookLogin()
{
    return CurrentApp()->m_settings->IsFBReminderEnabled();
}

bool CGServeHandler::NeedToShowLoginRequired()
{
    return !CNGS::GetInstance()->GetLocalUser()->isValid();
}

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AssertGServeHandlerSize[sizeof(CGServeHandler) == 0x14 ? 1 : -1];
#endif
