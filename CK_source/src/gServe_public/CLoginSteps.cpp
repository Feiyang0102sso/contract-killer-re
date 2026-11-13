// Game-specific gServe login steps used by CK 1.5.2.
#include "CLoginSteps.h"
#define AD_BANNER_SIZES_NO_MARGINS
#include "AdBannerSizes.h"
#undef AD_BANNER_SIZES_NO_MARGINS
#include "App.h"
#include "AppSettings.h"
#include "CFileData1.h"
#include "CGServeHandler.h"
#include "CNGS.h"
#include "CNGSLocalUser.h"
#include "CNGSOfferManager.h"

// The original initializes the offer manager class ID before the margins.
static AdBannerMarginsX s_adBannerMarginsX;
static AdBannerMarginsY s_adBannerMarginsY;

#include "CNGS_Platform.h"
#include "CNetworkAvailability.h"
#include "CNotificationHandler.h"
#include "COverrideManager.h"
#include "CProfileManager.h"
#include "CSaveManager.h"

namespace {
const unsigned int OverrideStepId = 0xd538bed3;

App* CurrentApp()
{
    return static_cast<App*>(WindowApp::m_instance);
}

void HandleSocialLoginEvent(eLoginFlowEvent event, int network)
{
    switch (event) {
    case LoginFlowFail:
        CNGS::GetInstance()->GetLocalUser()->Logout(network);
        CurrentApp()->m_gserveHandler->ShowSocialNetworkConnectionErrorDialog();
        break;
    case LoginFlowServerConnectionError:
    case LoginFlowNetworkConnectionError:
        CurrentApp()->m_gserveHandler->ShowSocialNetworkConnectionErrorDialog();
        break;
    case LoginFlowAccountConflict:
        CurrentApp()->m_gserveHandler->ShowSocialNetworkAccountsConflictDialogs();
        break;
    case LoginFlowSuccess:
    case LoginFlowCancel:
        if (CNGSLoginFlow::GetInstance()->m_activeNetwork == network) {
            CNGSLoginFlow::GetInstance()->NextStep();
        }
        break;
    default:
        break;
    }
}
}

// The base constructors (CNGSLoginFlow.h) set id, name, network and timeout.
DeviceOnlineCheckStep::DeviceOnlineCheckStep()
{
}

void DeviceOnlineCheckStep::Excute()
{
    CNGSLoginFlow::GetInstance()->NextStep();
}

void DeviceOnlineCheckStep::OnEvent(eLoginFlowEvent, const char*)
{
}

VersionCheckStep::VersionCheckStep()
{
}

void VersionCheckStep::OnEvent(eLoginFlowEvent event, const char*)
{
    if (event == LoginFlowVersionSuccess) {
        CNGSLoginFlow::GetInstance()->NextStep();
    } else if (event == LoginFlowVersionInvalid) {
        WindowApp::HandleTunnelCommand(0x6458fa9c, 0, 0, 0);
    } else {
        CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
        if (CNGS_Platform::GetInstance()->isGameCenterAvailable() &&
            user->GetSocialNetworkIdentity(SOCIAL_INTERFACE_GAME_CENTER).GetLength() > 0) {
            CurrentApp()->m_gserveHandler->ShowSocialNetworkConnectionErrorDialog();
        } else if (user->GetSocialNetworkIdentity(SOCIAL_INTERFACE_FACEBOOK).GetLength() > 0) {
            CurrentApp()->m_gserveHandler->ShowSocialNetworkConnectionErrorDialog();
        } else {
            CNGSLoginFlow::GetInstance()->NextStep();
        }
    }
}

FacebookLoginStep::FacebookLoginStep(bool force)
    : CNGSLoginStep_SocialNetwork(SOCIAL_INTERFACE_FACEBOOK), m_force(force)
{
}

void FacebookLoginStep::Prepare()
{
    App* app = CurrentApp();
    CNGSLoginFlow::GetInstance()->m_activeNetwork = SOCIAL_INTERFACE_FACEBOOK;
    CNotificationHandler* handler = app->m_notificationHandler;
    CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
    if (app->m_gserveHandler->m_pendingSwitchNetwork == SOCIAL_INTERFACE_FACEBOOK) {
        app->m_gserveHandler->m_pendingSwitchNetwork = 0;
    } else if (user->GetSocialNetworkIdentity(SOCIAL_INTERFACE_FACEBOOK).GetLength() == 0 ||
               (handler != NULL && handler->m_loginState == 2)) {
        if (CurrentApp()->m_gserveHandler->NeedToShowFacebookLogin()) {
            CurrentApp()->m_gserveHandler->ShowFacebookLoginDialog();
            return;
        }
        if (CurrentApp()->m_gserveHandler->NeedToShowLoginRequired()) {
            CurrentApp()->m_gserveHandler->ShowLoginRequired();
            return;
        }
    }
    Excute();
}

void FacebookLoginStep::Excute()
{
    App* app = CurrentApp();
    CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
    CNotificationHandler* handler = app->m_notificationHandler;
    if (user->isAuthenticated(m_snType) || (handler != NULL && handler->m_loginState == 2) ||
        (!m_force && !CurrentApp()->m_gserveHandler->NeedToShowFacebookLogin() && !CurrentApp()->m_gserveHandler->NeedToShowLoginRequired())) {
        CNGSLoginFlow::GetInstance()->NextStep();
        return;
    }
    if (handler != NULL) {
        handler->m_loginState = 3;
    }
    app->m_field398 = SOCIAL_INTERFACE_FACEBOOK;
    user->Login(m_snType);
    app->m_settings->SetFBReminderEnabled(true);
    app->m_settings->Save();
}

void FacebookLoginStep::OnEvent(eLoginFlowEvent event, const char*)
{
    App* app = CurrentApp();
    app->m_field398 = -1;
    if (app->m_notificationHandler != NULL) {
        app->m_notificationHandler->m_loginState = 0;
    }
    HandleSocialLoginEvent(event, m_snType);
}

GameCenterLoginStep::GameCenterLoginStep() : CNGSLoginStep_SocialNetwork(SOCIAL_INTERFACE_GAME_CENTER)
{
}

void GameCenterLoginStep::Prepare()
{
    CNGSLoginFlow* flow = CNGSLoginFlow::GetInstance();
    flow->m_activeNetwork = SOCIAL_INTERFACE_GAME_CENTER;
    flow->ExcuteStep(-1);
}

void GameCenterLoginStep::OnEvent(eLoginFlowEvent event, const char*)
{
    HandleSocialLoginEvent(event, m_snType);
}

LoadFriendsListStep::LoadFriendsListStep(int network) : CNGSLoginStep_LoadFriendsList(network)
{
}

void LoadFriendsListStep::OnEvent(eLoginFlowEvent event, const char*)
{
    if (event == LoginFlowServerConnectionError || event == LoginFlowNetworkConnectionError) {
        CurrentApp()->m_gserveHandler->ShowSocialNetworkConnectionErrorDialog();
    } else if (event == LoginFlowSuccess) {
        CNGSLoginFlow::GetInstance()->NextStep();
    }
}

LoadPrivateGameDataStep::LoadPrivateGameDataStep()
{
}

void LoadPrivateGameDataStep::Excute()
{
    CNGSLoginFlow::GetInstance()->NextStep();
}

void LoadPrivateGameDataStep::OnEvent(eLoginFlowEvent, const char*)
{
}

LoadGameDataStep::LoadGameDataStep()
{
}

void LoadGameDataStep::Excute()
{
    App* app = CurrentApp();
    TCVector<int> collections;
    app->m_profileManager->getAllRegisteredCollections(collections);
    if (!app->m_profileManager->load(collections)) {
        app->m_field3a1 = true;
        CNGSLoginFlow::GetInstance()->NextStep();
    }
}

void LoadGameDataStep::OnEvent(eLoginFlowEvent event, const char*)
{
    App* app = CurrentApp();
    if (event == LoginFlowSuccess) {
        CFileData1* file = app->m_fileData;
        if (file != NULL && file->m_size > 0) {
            unsigned char* data = new unsigned char[file->m_size];
            memcpy(data, file->m_data, file->m_size);
            app->m_saveManager->SetData(data, file->m_size);
            delete[] data;
        }
        app->m_field3a1 = true;
        CNGSLoginFlow::GetInstance()->NextStep();
    } else if (event == LoginFlowServerConnectionError || event == LoginFlowNetworkConnectionError) {
        if (app->m_socialConnectDialog != NULL) {
            app->m_socialConnectDialog->Close();
            app->m_socialConnectDialog = NULL;
        }
        CurrentApp()->m_gserveHandler->ShowSocialNetworkConnectionErrorDialog();
    }
}

InitIncentiveOffersStep::InitIncentiveOffersStep()
{
}

void InitIncentiveOffersStep::Excute()
{
    COfferManager* offers = COfferManager::GetInstance();
    if (!offers->isInitialized()) {
        offers->initializeGluOffers();
    } else {
        CNGSLoginFlow::GetInstance()->NextStep();
    }
}

void InitIncentiveOffersStep::OnEvent(eLoginFlowEvent, const char*)
{
    CNGSLoginFlow::GetInstance()->NextStep();
}

GameReloadStep::GameReloadStep()
{
}

void GameReloadStep::Excute()
{
    CurrentApp()->m_gserveHandler->ShowSocialNetworkPrimaryUserChanged();
}

void GameReloadStep::OnEvent(eLoginFlowEvent, const char*)
{
}

CheckOverrideDataStep::CheckOverrideDataStep()
{
    m_classId = OverrideStepId;
}

void CheckOverrideDataStep::Excute()
{
    App* app = CurrentApp();
    if (app->m_networkAvailability->IsAvailable()) {
        app->m_overrideManager->checkUpdates();
        if (app->m_overrideManager->IsBusy()) {
            return;
        }
    }
    CNGSLoginFlow::GetInstance()->NextStep();
}

void CheckOverrideDataStep::OnEvent(eLoginFlowEvent, const char*)
{
    CNGSLoginFlow::GetInstance()->NextStep();
}
