// Game-specific gServe login steps used by CK 1.5.2.
#include "CLoginSteps.h"
#define AD_BANNER_SIZES_NO_MARGINS
#include "AdBannerSizes.h"
#undef AD_BANNER_SIZES_NO_MARGINS
#include "App.h"
#include "AppSettings.h"
#include "CFileData1.h"
#include "CGameAnalytics.h"
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
App* CurrentApp()
{
    return static_cast<App*>(WindowApp::m_instance);
}
}

void DeviceOnlineCheckStep::Excute()
{
    CNGSLoginFlow::GetInstance()->NextStep();
}

void DeviceOnlineCheckStep::OnEvent(eLoginFlowEvent, const char*)
{
}

void VersionCheckStep::OnEvent(eLoginFlowEvent event, const char*)
{
    if (event == LoginFlowVersionInvalid) {
        WindowApp::HandleTunnelCommand(0x6458fa9c, 0, 0, 0);
        return;
    }
    if (event == LoginFlowVersionSuccess) {
        CNGSLoginFlow::GetInstance()->NextStep();
        return;
    }

    CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
    bool hasIdentity = (CNGS_Platform::GetInstance()->isGameCenterAvailable() &&
                        user->GetSocialNetworkIdentity(SOCIAL_INTERFACE_GAME_CENTER).GetLength() > 0) ||
                       user->GetSocialNetworkIdentity(SOCIAL_INTERFACE_FACEBOOK).GetLength() > 0;
    if (hasIdentity) {
        CurrentApp()->m_gserveHandler->ShowSocialNetworkConnectionErrorDialog();
    } else {
        CNGSLoginFlow::GetInstance()->NextStep();
    }
}

void FacebookLoginStep::Prepare()
{
    CNGSLoginFlow::GetInstance()->m_activeNetwork = SOCIAL_INTERFACE_FACEBOOK;
    CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
    if (CurrentApp()->m_gserveHandler->m_pendingSwitchNetwork == SOCIAL_INTERFACE_FACEBOOK) {
        CurrentApp()->m_gserveHandler->m_pendingSwitchNetwork = 0;
    } else if (user->GetSocialNetworkIdentity(SOCIAL_INTERFACE_FACEBOOK).GetLength() == 0 ||
               user->m_notificationHandler->m_loginState == 2) {
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
    if (CNGS::GetInstance()->GetLocalUser()->isAuthenticated(m_snType)) {
        CNGSLoginFlow::GetInstance()->NextStep();
        return;
    }

    CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
    if (user->m_notificationHandler != NULL && user->m_notificationHandler->m_loginState == 2) {
        CNGSLoginFlow::GetInstance()->NextStep();
        return;
    }

    if (CurrentApp()->m_gserveHandler->NeedToShowFacebookLogin() ||
        CurrentApp()->m_gserveHandler->NeedToShowLoginRequired() || m_force) {
        user = CNGS::GetInstance()->GetLocalUser();
        if (user->m_notificationHandler != NULL) {
            user->m_notificationHandler->m_loginState = 3;
        }
        CurrentApp()->m_field398 = SOCIAL_INTERFACE_FACEBOOK;
        CNGS::GetInstance()->GetLocalUser()->Login(m_snType);
        CurrentApp()->m_settings->SetFBReminderEnabled(true);
        CurrentApp()->m_settings->Save();
    } else {
        CNGSLoginFlow::GetInstance()->NextStep();
    }
}

void FacebookLoginStep::OnEvent(eLoginFlowEvent event, const char*)
{
    CurrentApp()->m_field398 = -1;
    CNGS::GetInstance()->GetLocalUser()->m_notificationHandler->m_loginState = 0;

    if (event == LoginFlowServerConnectionError || event == LoginFlowNetworkConnectionError) {
        CurrentApp()->m_gserveHandler->ShowSocialNetworkConnectionErrorDialog();
        return;
    }
    if (event <= LoginFlowVersionInvalid) {
        if (event == LoginFlowFail) {
            CNGS::GetInstance()->GetLocalUser()->Logout(m_snType);
            CurrentApp()->m_gserveHandler->ShowSocialNetworkConnectionErrorDialog();
            return;
        }
    } else {
        if (event == LoginFlowSessionInvalid || event == LoginFlowPrimaryUserChanged) {
            return;
        }
        if (event == LoginFlowAccountConflict) {
            CurrentApp()->m_gserveHandler->ShowSocialNetworkAccountsConflictDialogs();
            return;
        }
    }
    if (event == LoginFlowSuccess || event == LoginFlowCancel) {
        if (CNGSLoginFlow::GetInstance()->m_activeNetwork == m_snType) {
            CNGSLoginFlow::GetInstance()->NextStep();
        }
    }
}

void GameCenterLoginStep::Prepare()
{
    CNGSLoginFlow::GetInstance()->m_activeNetwork = SOCIAL_INTERFACE_GAME_CENTER;
    CNGSLoginFlow::GetInstance()->ExcuteStep(-1);
}

void GameCenterLoginStep::OnEvent(eLoginFlowEvent event, const char*)
{
    if (event == LoginFlowServerConnectionError || event == LoginFlowNetworkConnectionError) {
        CurrentApp()->m_gserveHandler->ShowSocialNetworkConnectionErrorDialog();
        return;
    }
    if (event <= LoginFlowVersionInvalid) {
        if (event == LoginFlowFail) {
            CNGS::GetInstance()->GetLocalUser()->Logout(m_snType);
            CurrentApp()->m_gserveHandler->ShowSocialNetworkConnectionErrorDialog();
            return;
        }
    } else {
        if (event == LoginFlowSessionInvalid || event == LoginFlowPrimaryUserChanged) {
            return;
        }
        if (event == LoginFlowAccountConflict) {
            CurrentApp()->m_gserveHandler->ShowSocialNetworkAccountsConflictDialogs();
            return;
        }
    }
    if (event == LoginFlowSuccess || event == LoginFlowCancel) {
        if (CNGSLoginFlow::GetInstance()->m_activeNetwork == m_snType) {
            CNGSLoginFlow::GetInstance()->NextStep();
        }
    }
}

void LoadFriendsListStep::OnEvent(eLoginFlowEvent event, const char*)
{
    if (event == LoginFlowServerConnectionError || event == LoginFlowNetworkConnectionError) {
        CurrentApp()->m_gserveHandler->ShowSocialNetworkConnectionErrorDialog();
    } else if (event == LoginFlowSuccess) {
        if (m_snType == SOCIAL_INTERFACE_FACEBOOK) {
            CGameAnalytics::logGservFbImported();
        } else if (m_snType == SOCIAL_INTERFACE_GAME_CENTER) {
            CGameAnalytics::logGservGcImported();
        }
        CNGSLoginFlow::GetInstance()->NextStep();
    }
}

void LoadPrivateGameDataStep::Excute()
{
    CNGSLoginFlow::GetInstance()->NextStep();
}

void LoadPrivateGameDataStep::OnEvent(eLoginFlowEvent, const char*)
{
}

void LoadGameDataStep::Excute()
{
    TCVector<int> collections;
    CProfileManager* profiles = CurrentApp()->m_profileManager;
    profiles->getAllRegisteredCollections(collections);
    if (!profiles->load(collections)) {
        CurrentApp()->m_field3a1 = true;
        CNGSLoginFlow::GetInstance()->NextStep();
    }
}

void LoadGameDataStep::OnEvent(eLoginFlowEvent event, const char*)
{
    if (event == LoginFlowSuccess) {
        CFileData1* file = CurrentApp()->m_fileData;
        if (file != NULL && file->m_size > 0) {
            unsigned char* data = new unsigned char[file->m_size];
            memcpy(data, file->m_data, file->m_size);
            CurrentApp()->m_saveManager->SetData(data, file->m_size);
            delete[] data;
        }
        CurrentApp()->m_field3a1 = true;
        CNGSLoginFlow::GetInstance()->NextStep();
    } else if (event == LoginFlowServerConnectionError || event == LoginFlowNetworkConnectionError) {
        if (CurrentApp()->m_socialConnectDialog != NULL) {
            CurrentApp()->m_socialConnectDialog->Close();
            CurrentApp()->m_socialConnectDialog = NULL;
        }
        CurrentApp()->m_gserveHandler->ShowSocialNetworkConnectionErrorDialog();
    }
}

void InitIncentiveOffersStep::Excute()
{
    if (COfferManager::GetInstance()->isInitialized()) {
        CNGSLoginFlow::GetInstance()->NextStep();
    } else {
        COfferManager::GetInstance()->initializeGluOffers();
    }
}

void InitIncentiveOffersStep::OnEvent(eLoginFlowEvent, const char*)
{
    CNGSLoginFlow::GetInstance()->NextStep();
}

void GameReloadStep::Excute()
{
    CurrentApp()->m_gserveHandler->ShowSocialNetworkPrimaryUserChanged();
}

void GameReloadStep::OnEvent(eLoginFlowEvent, const char*)
{
}

void CheckOverrideDataStep::Excute()
{
    if (CurrentApp()->m_networkAvailability->IsAvailable()) {
        CurrentApp()->m_overrideManager->checkUpdates();
        if (CurrentApp()->m_overrideManager->IsBusy()) {
            return;
        }
    }
    CNGSLoginFlow::GetInstance()->NextStep();
}

void CheckOverrideDataStep::OnEvent(eLoginFlowEvent, const char*)
{
    CNGSLoginFlow::GetInstance()->NextStep();
}
