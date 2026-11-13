// CK game-side gServe notifications.
#include "CNotificationHandler.h"
#include "AdBannerSizes.h"
#include "ICInAppPurchase.h"
#include "App.h"
#include "CBHFriend.h"
#include "CBH_Player.h"
#include "CBHUtil_Cocoa.h"
#include "CFileData1.h"
#include "ICFile.h"
#include "CGameApp.h"
#include "CGameAnalytics.h"
#include "CGServeHandler.h"
#include "CLoginSteps.h"
#include "CNGS.h"
#include "CNGSFromServerMessage.h"
#include "CNGSFromServerMessageQ.h"
#include "CNGSLocalUser.h"
#include "CNGSOfferManager.h"
#include "CNGSRemoteUser.h"
#include "CNGSRemoteUserList.h"
#include "CNGS_Platform.h"
#include "CNewsFeedManager.h"
#include "CPrivateData1.h"
#include "CProfileManager.h"
#include "CStdUtil_Cocoa.h"
#include "ICFileMgr.h"

extern const wchar_t SAVE_FILE_NAME[];

namespace {
App* CurrentApp()
{
    return static_cast<App*>(WindowApp::m_instance);
}
}

CNotificationHandler::CNotificationHandler()
    : m_localSoftCurrency(0), m_localHardCurrency(0), m_localExperience(0),
      m_serverSoftCurrency(0), m_serverHardCurrency(0), m_serverExperience(0)
{
    // stored after the two strings in the original
    m_loginState = 0;
}

CNotificationHandler::~CNotificationHandler()
{
}

unsigned char CNotificationHandler::HandleUserLogin(int result, int network, const wchar_t*)
{
    if (network == SOCIAL_INTERFACE_FACEBOOK) {
        m_loginState = 0;
    }
    if (!CNGSLoginFlow::GetInstance()->IsRunning()) {
        CurrentApp()->m_gserveHandler->CloseCurrentSocialNetworkConnectDialog();
        if (network == SOCIAL_INTERFACE_FACEBOOK) {
            WindowApp::HandleTunnelCommand((int)0xb3593b79, 0, 0, 0);
        } else if (network == SOCIAL_INTERFACE_GAME_CENTER) {
            WindowApp::HandleTunnelCommand((int)0xbb215081, 0, 0, 0);
        }
    }
    // analytics: 1 Facebook, 2 Game Center, 3 both networks
    int networks = 0;
    if (network == SOCIAL_INTERFACE_FACEBOOK) {
        networks = !CNGS::GetInstance()->GetLocalUser()->isAuthenticated(SOCIAL_INTERFACE_GAME_CENTER) ? 1 : 3;
    } else if (network == SOCIAL_INTERFACE_GAME_CENTER) {
        networks = !CNGS::GetInstance()->GetLocalUser()->isAuthenticated(SOCIAL_INTERFACE_FACEBOOK) ? 2 : 3;
    }
    CGameAnalytics::logGservLogin(networks);
    void* found = NULL;
    CApplet::m_pApp->m_singletons->Find(CNGS::ClassId, &found);
    CNGS* gserve = static_cast<CNGS*>(found);
    if (gserve == NULL) {
        gserve = new CNGS();
    }
    gserve->GetLocalUser()->m_messageQueue->ProcessPromptMessages();
    return 1;
}

unsigned char CNotificationHandler::HandleUserLogout(unsigned char, int network)
{
    int networks = 0;
    if (network == SOCIAL_INTERFACE_FACEBOOK) {
        WindowApp::HandleTunnelCommand(0x47f46330, 0, 0, 0);
        networks = 1;
    } else if (network == SOCIAL_INTERFACE_GAME_CENTER) {
        WindowApp::HandleTunnelCommand((int)0xf8a2664d, 0, 0, 0);
        networks = 2;
    }
    CGameAnalytics::logGservLogout(networks);
    return 1;
}

unsigned char CNotificationHandler::HandlePrimaryUserChanged(int)
{
    CurrentApp()->m_field3a0 = true;
    CGameApp::GetInstance()->m_accountChanged = true;
    CurrentApp()->StartLogin(App::LoginSwitchAccount, true);
    return 1;
}

unsigned char CNotificationHandler::HandleFriendListUpdate(unsigned char, int count)
{
    WindowApp::HandleTunnelCommand((int)0xe4fd16fb, count, 0, 0);
    return 1;
}

unsigned char CNotificationHandler::HandleFriendDeepPopulated(CNGSRemoteUser* user, unsigned char)
{
    CBHFriend* friendUser = static_cast<CBHFriend*>(user);
    friendUser->setPopulated(1);
    for (int index = 0; index < friendUser->m_publicData.m_guildFriends.size(); index++) {
        int id = friendUser->m_publicData.m_guildFriends[index];
        if (CNGS::GetInstance()->GetLocalUser()->GetClientID() == id &&
            !CurrentApp()->m_publicData->isInGuildFriendList(id)) {
            CurrentApp()->m_publicData->addGuildFriend(friendUser->GetClientID());
        }
    }
    WindowApp::HandleTunnelCommand(4, 0, 0, 0);
    return 1;
}

unsigned char CNotificationHandler::HandleSessionInvalid()
{
    // the result is not used (calls kept as in the original)
    if (CNGS::GetInstance()->GetLocalUser()->isAuthenticated(SOCIAL_INTERFACE_GAME_CENTER) ||
        CNGS::GetInstance()->GetLocalUser()->isAuthenticated(SOCIAL_INTERFACE_FACEBOOK)) {
    }
    CurrentApp()->m_field3a0 = true;
    CurrentApp()->m_gserveHandler->ShowSocialNetworkSessionInvalidDialog();
    return 1;
}

unsigned char CNotificationHandler::HandleVersionInvalid()
{
    if (!CNGSLoginFlow::GetInstance()->IsRunning()) {
        WindowApp::HandleTunnelCommand(0x6458fa9c, 0, 0, 0);
    }
    return 1;
}

unsigned char CNotificationHandler::HandleProfileLoaded(unsigned char, const TCVector<int>&)
{
    if (CNGSLoginFlow::GetInstance()->IsRunning()) {
        CNGSLoginFlow::GetInstance()->OnEvent(LoginFlowSuccess, "HandleProfileLoaded");
    }
    return 1;
}

unsigned char CNotificationHandler::HandleProfileDataConflict(const TCVector<int>& dataIds)
{
    m_localDataText = L"";
    m_serverDataText = L"";
    if (dataIds.Size() < 1) {
        if (CNGSLoginFlow::GetInstance()->IsRunning()) {
            CNGSLoginFlow::GetInstance()->OnEvent(LoginFlowSuccess, "HandleProfileDataConflict");
        }
        return 1;
    }
    unsigned char conflict = 0;
    int index = 0;
    do {
        if (dataIds[index] < 1000) {
            continue;
        }
        CSaveRestoreInterface* local = CurrentApp()->m_profileManager->getManagedObjectFromBuffer(
            PROFILE_DATA_SAVED_LOCAL, dataIds[index]);
        CSaveRestoreInterface* server = CurrentApp()->m_profileManager->getManagedObjectFromBuffer(
            PROFILE_DATA_SYNCED, dataIds[index]);

        if (local->getDataSaveStatus() == PROFILE_DATA_INVALIDATED) {
            int collection = dataIds[index];
            if (collection == 1000) {
                if (static_cast<CPublicData1*>(local)->m_level == 0) {
                    goto selectServer;
                }
            } else if (collection == 1001 && static_cast<CPrivateData1*>(local)->getExperience() == 0) {
                goto selectServer;
            }
        }
        conflict = 1;
        if (dataIds[index] == 1001) {
            CPrivateData1* localData = static_cast<CPrivateData1*>(local);
            CPrivateData1* serverData = static_cast<CPrivateData1*>(server);
            m_localDataText += localData->asString() + "\n";
            m_serverDataText += serverData->asString() + "\n";
            m_localSoftCurrency = localData->getSoftCurrency();
            m_localHardCurrency = localData->getHardCurrency();
            m_localExperience = localData->getExperience();
            m_serverSoftCurrency = serverData->getSoftCurrency();
            m_serverHardCurrency = serverData->getHardCurrency();
            m_serverExperience = serverData->getExperience();
        }
    } while (++index < dataIds.Size());
    if (conflict) {
        CurrentApp()->m_gserveHandler->CloseCurrentSocialNetworkConnectDialog();
        CGameApp::gserveDataConflict = true;
        WindowApp::HandleTunnelCommand(0x07e17dc8, 0, 0, 0);
        return 1;
    }
    goto resumeLogin;

selectServer:
    {
        TCVector<int> collections;
        CurrentApp()->m_profileManager->getAllRegisteredCollections(collections);
        CurrentApp()->m_gserveHandler->SelectProfileServerData(collections);
        CurrentApp()->m_field3a0 = false;
    }

resumeLogin:
    if (CNGSLoginFlow::GetInstance()->IsRunning()) {
        CNGSLoginFlow::GetInstance()->OnEvent(LoginFlowSuccess, "HandleProfileDataConflict");
    }
    return 1;
}

unsigned char CNotificationHandler::HandleAddExperienceGiftMessage(long long experience, int, const CStrWChar&)
{
    CBH_Player::GetInstance()->AddXp((int)experience, GAIN_SOURCE_IAP);
    return 1;
}

unsigned char CNotificationHandler::HandleAddRareCurrencyGiftMessage(long long credits, int, const CStrWChar&)
{
    CBH_Player::GetInstance()->AddCredits((int)credits);
    return 1;
}

unsigned char CNotificationHandler::HandleAddCommonCurrencyGiftMessage(long long cash, int, const CStrWChar&)
{
    CBH_Player::GetInstance()->AddMoney((int)cash, GAIN_SOURCE_IAP);
    return 1;
}

unsigned char CNotificationHandler::HandleIncentive(unsigned char, int amount)
{
    WindowApp::HandleTunnelCommand((int)0x9937390f, amount, 0, 0);
    return 1;
}

unsigned char CNotificationHandler::HandlePostMessage(unsigned char success, int)
{
    if (success) {
        WindowApp::HandleTunnelCommand((int)0xa728ca73, 0, 0, 0);
    } else {
        WindowApp::HandleTunnelCommand(0x70b135b5, 0, 0, 0);
    }
    return 1;
}

unsigned char CNotificationHandler::HandleInitializeGluOffers(unsigned char success, int)
{
    if (success) {
        COfferManager::GetInstance()->getUnconsumedIncentives();
    }
    if (CNGSLoginFlow::GetInstance()->IsRunning()) {
        CNGSLoginFlow::GetInstance()->OnEvent(LoginFlowSuccess, "HandleInitializeGluOffers");
    }
    return 1;
}

unsigned char CNotificationHandler::HandleSendInvite(unsigned char success, int)
{
    if (success) {
        WindowApp::HandleTunnelCommand((int)0x940a3e98, 0, 0, 0);
    } else {
        WindowApp::HandleTunnelCommand((int)0xcdb8030a, 0, 0, 0);
    }
    return 1;
}

unsigned char CNotificationHandler::HandleOldUnackedMessages()
{
    HandleNewMessagesReceived();
    return 0;
}

unsigned char CNotificationHandler::HandleNewMessagesReceived()
{
    CNGSFromServerMessageQ* queue = CNGS::GetInstance()->GetLocalUser()->m_messageQueue;
    TCVector<long long> ids;
    queue->GetAllMessagesByUserAndType(ids, 0, 0, CNGSFromServerMessage::CNGS_MSG_SUBTYPE_ANY);
    for (int index = 0; index < ids.Size(); index++) {
        CNGSFromServerMessage* message = queue->GetMessageById(ids[index]);
        if (message == NULL) {
            continue;
        }
        long long type = message->m_type;
        switch (type) {
        case 41:
            m_giftMessageIds.Add(ids[index]);
            if (message->m_subType == CNGSFromServerMessage::CNGS_MSG_SUBTYPE_AWARD ||
                message->m_subType == CNGSFromServerMessage::CNGS_MSG_SUBTYPE_GIFT) {
                CNewsFeedManager* news = CurrentApp()->m_newsFeedManager;
                if (news != NULL) {
                    CNGSMessageAward* award = static_cast<CNGSMessageAward*>(message);
                    int rare = (int)award->m_currencyRare;
                    int experience = (int)award->m_experience;
                    int common = (int)award->m_currencyCommon;
                    int item = (int)award->m_itemType;
                    WindowApp::HandleTunnelCommand(0x6df3535f, 0, 0, 0);
                    CStrWChar text = message->GetMessageText();
                    news->AddServerAwardMessage(text, common, rare, experience, item - 1, (int)message->m_messageId);
                }
            }
            break;
        case 40:
        case 100:
            m_giftMessageIds.Add(ids[index]);
            if (message->m_subType == CNGSFromServerMessage::CNGS_MSG_SUBTYPE_AWARD) {
                CNewsFeedManager* news = CurrentApp()->m_newsFeedManager;
                if (news != NULL) {
                    CNGSMessageAward* award = static_cast<CNGSMessageAward*>(message);
                    int rare = (int)award->m_currencyRare;
                    int common = (int)award->m_currencyCommon;
                    int item = (int)award->m_itemType;
                    int experience = (int)award->m_experience;
                    CStrWChar text = message->GetMessageText();
                    news->AddServerAwardMessage(text, common, rare, experience, item - 1, (int)message->m_messageId);
                }
            } else if (message->m_subType == CNGSFromServerMessage::CNGS_MSG_SUBTYPE_GIFT) {
                CNewsFeedManager* news = CurrentApp()->m_newsFeedManager;
                if (news != NULL) {
                    CNGSMessageGift* gift = static_cast<CNGSMessageGift*>(message);
                    if (gift->m_itemType > 0) {
                        news->AddGiftReceivedMessage((int)message->m_senderId, (int)gift->m_itemType - 1,
                                                     (int)message->m_messageId);
                    }
                }
            }
            break;
        case 300:
            break;
        case 1008: {
            CNewsFeedManager* news = CurrentApp()->m_newsFeedManager;
            if (news != NULL) {
                CStrWChar text = message->GetMessageText();
                int sender = (int)message->m_senderId;
                XString challengeText(text.GetWCharPtr());
                int challenge = challengeText.SubString(1, challengeText.Length() - 2).ToInt();
                news->AddChallengeInviteMessage(sender, challenge, (int)message->m_messageId);
            }
            break;
        }
        case 1011: {
            CNewsFeedManager* news = CurrentApp()->m_newsFeedManager;
            if (news != NULL) {
                CStrWChar text = message->GetMessageText();
                int sender = (int)message->m_senderId;
                XString challengeText(text.GetWCharPtr());
                int challenge = challengeText.SubString(1, challengeText.Length() - 2).ToInt();
                news->AddFriendCompletedChallengeMessage(sender, challenge, text, (int)message->m_messageId);
            }
            break;
        }
        default:
            if (type >= 1000) {
                CStrWChar text = message->GetMessageText();
                if (!HandleGameSpecificMessage(type, text, message->m_senderId)) {
                    break;
                }
            }
            if (queue->MarkMessageForAck(message->m_messageId)) {
                delete message;
            }
            break;
        }
    }
    WindowApp::HandleTunnelCommand(0x6e7d5209, 0, 0, 0);
    return 1;
}

unsigned char CNotificationHandler::HandleGameSpecificMessage(long long type, const CStrWChar& text,
                                                              long long senderId)
{
    switch (type) {
    case 1001:
    case 1004: {
        CNewsFeedManager* news = CurrentApp()->m_newsFeedManager;
        if (news != NULL) {
            if (type == 1001) {
                news->AddTargettedMessage((int)senderId, text, true);
            } else {
                news->AddTargettedMessage((int)senderId, text, false);
            }
        }
        return 0;
    }
    case 1005: {
        App* app = CurrentApp();
        CNewsFeedManager* news = app->m_newsFeedManager;
        if (news != NULL && !app->m_publicData->isInGuildFriendList((int)senderId)) {
            news->AddGuildInviteMessage((int)senderId, text);
        }
        return 0;
    }
    case 1006: {
        CurrentApp()->m_publicData->addGuildFriend((int)senderId);
        CNewsFeedManager* news = CurrentApp()->m_newsFeedManager;
        if (news != NULL) {
            news->AddGuildInviteResponseMessage((int)senderId, true);
        }
        return 1;
    }
    case 1007: {
        CNewsFeedManager* news = CurrentApp()->m_newsFeedManager;
        if (news != NULL) {
            news->AddGuildInviteResponseMessage((int)senderId, false);
        }
        return 1;
    }
    case 1002: {
        CNGSRemoteUserList* friends = CNGS::GetInstance()->GetLocalUser()->m_remoteUserLists[0];
        for (int index = 0; index < friends->m_users.Size(); ++index) {
            if (friends->m_users[index]->GetClientID() == senderId) {
                CStrWChar mission = text.GetSubString(1, text.GetLength() - 2);
                const wchar_t* wide = mission.GetWCharPtr();
                char number[12];
                wcstombs(number, wide, wcslen(wide) + 1);
                int missionId = atoi(number);
                CurrentApp()->m_newsFeedManager->AddMissionInviteMessage((int)senderId, missionId);
            }
        }
        return 0;
    }
    case 1009: {
        CNewsFeedManager* news = CurrentApp()->m_newsFeedManager;
        if (news != NULL) {
            XString challengeText(text.GetWCharPtr());
            int challenge = challengeText.SubString(1, challengeText.Length() - 2).ToInt();
            news->AddChallengeResponseMessage((int)senderId, challenge, true);
        }
        return 1;
    }
    case 1010: {
        CNewsFeedManager* news = CurrentApp()->m_newsFeedManager;
        if (news != NULL) {
            XString challengeText(text.GetWCharPtr());
            int challenge = challengeText.SubString(1, challengeText.Length() - 2).ToInt();
            news->AddChallengeResponseMessage((int)senderId, challenge, false);
        }
        return 1;
    }
    }
    return 0;
}

void CNotificationHandler::RemoveGameSpecificMessage(int messageId)
{
    // the original looks the message up only after acknowledging it and keeps the gift id list
    CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
    if (messageId >= 0) {
        CNGSFromServerMessageQ* queue = user->m_messageQueue;
        if (queue->MarkMessageForAck(messageId)) {
            delete queue->GetMessageById(messageId);
        }
    }
}

void CNotificationHandler::OnEnterForeground()
{
    if (m_loginState != 1 && CurrentApp()->m_field398 == SOCIAL_INTERFACE_FACEBOOK) {
        m_loginState = 2;
        CurrentApp()->m_field398 = -1;
        if (CNGSLoginFlow::GetInstance()->IsRunning()) {
            CNGSLoginFlow::GetInstance()->PrepareStep(-1);
        }
    }
    m_loginState = 0;
}

unsigned char CNotificationHandler::HandleFacebookLoginPageClose()
{
    if (m_loginState == 3) {
        m_loginState = 1;
        return 1;
    }
    return 0;
}

unsigned char CNotificationHandler::HandleProfileContentFileDownloaded(unsigned char, int)
{
    WindowApp::HandleTunnelCommand(0x42650932, 0, 0, 0);
    return 1;
}

unsigned char CNotificationHandler::HandleGameCenterAppLogin(unsigned char)
{
    const int network = SOCIAL_INTERFACE_GAME_CENTER;
    const unsigned int socialStep = 0x90e4b4fa;
    if (CBHUtil_Cocoa::IsGameCenterAuthenticated() == true) {
        if (!CNGSLoginFlow::GetInstance()->IsRunning()) {
            if (CNGS::GetInstance()->GetLocalUser()->isAuthenticated(network)) {
                return 1;
            }
            CurrentApp()->StartLogin(App::LoginGameCenter, true);
        } else {
            CGServeHandler* handler = CurrentApp()->m_gserveHandler;
            if (handler->m_pendingSwitchNetwork == network) {
                handler->m_pendingSwitchNetwork = 0;
                CurrentApp()->StartLogin(App::LoginGameCenter, true);
            } else {
                if (CNGS::GetInstance()->GetLocalUser()->isAuthenticated(network)) {
                    return 1;
                }
                CNGSLoginStep* active = CNGSLoginFlow::GetInstance()->GetCurrentStep();
                if (active == NULL || active->GetClassId() == socialStep) {
                    return 1;
                }
                CNGSLoginFlow* flow = CNGSLoginFlow::GetInstance();
                CNGSLoginStep* previous = flow->GetStep(CNGSLoginFlow::GetInstance()->GetStepCount() - 2);
                if (previous == NULL || previous->GetClassId() == socialStep) {
                    return 1;
                }
                flow = CNGSLoginFlow::GetInstance();
                flow->Queue(new GameCenterLoginStep());
                flow = CNGSLoginFlow::GetInstance();
                flow->Queue(new LoadFriendsListStep(network));
            }
        }
    } else if (CurrentApp()->m_gserveHandler->m_pendingSwitchNetwork == network) {
        CurrentApp()->m_gserveHandler->m_pendingSwitchNetwork = 0;
        if (CNGSLoginFlow::GetInstance()->IsRunning()) {
            int activeNetwork = CNGSLoginFlow::GetInstance()->m_activeNetwork;
            CNGSLoginFlow* flow = CNGSLoginFlow::GetInstance();
            if (activeNetwork == network) {
                flow->NextStep();
            } else {
                flow->ExcuteStep(-1);
            }
        }
    }
    return 1;
}

unsigned char CNotificationHandler::HandleProfileDataFormatIncorrect(const CStrWChar& name,
    const CStrWChar& subDirectory, unsigned char*& data, unsigned int& size)
{
    // The original migration leaves the temporary envelope's payload allocated.
    unsigned char legacyStorage[sizeof(CFileData1)] __attribute__((aligned(__alignof__(CFileData1))));
    CFileData1& legacy = *new (legacyStorage) CFileData1();
    if (CStdUtil_Cocoa::WcsICmp(name.GetWCharPtr(), legacy.getFilename().GetWCharPtr()) != 0) {
        return 0;
    }

    CStrWChar path(ICFileMgr::GetInstance()->GetApplicationDataPath());
    path += ICFileMgr::GetInstance()->GetPathSeparator() + subDirectory;
    path += ICFileMgr::GetInstance()->GetPathSeparator() + CStrWChar(SAVE_FILE_NAME);
    ICFile* file = ICFileMgr::GetInstance()->Open(path.GetWCharPtr(), ICFileMgr::AccessRead);
    if (file == NULL) {
        return 0;
    }
    unsigned int byteCount = ICFileMgr::GetInstance()->Size(path.GetWCharPtr());
    unsigned char* bytes = static_cast<unsigned char*>(malloc(byteCount));
    unsigned int read = file->Read(bytes, byteCount);
    ICFileMgr::GetInstance()->Close(file);
    legacy.setData(bytes, read);
    legacy.setDataSaveStatus(PROFILE_DATA_SAVED_LOCAL);
    data = NULL;
    size = 0;
    CurrentApp()->m_profileManager->WriteAttributesToRawFileData(&legacy, data, size);
    if (bytes != NULL) {
        free(bytes);
    }
    return 1;
}

// The ARMv7 binary returns these flags without side effects.
unsigned char CNotificationHandler::HandlePrimaryUserLogout()
{
    return 1;
}

unsigned char CNotificationHandler::HandleGuestDeepPopulated(CNGSRemoteUser* user, unsigned char success)
{
    return 1;
}

unsigned char CNotificationHandler::HandleSessionError()
{
    return 1;
}

unsigned char CNotificationHandler::HandleGenericServerError()
{
    return 1;
}

unsigned char CNotificationHandler::HandleServerConnectionError()
{
    return 1;
}

unsigned char CNotificationHandler::HandleServerConnectionRestore()
{
    return 1;
}

unsigned char CNotificationHandler::HandleVersionSuccess()
{
    return 1;
}

unsigned char CNotificationHandler::HandleProfileSaved(unsigned char success, const TCVector<int>& dataIds)
{
    return 1;
}

unsigned char CNotificationHandler::HandleAddItemsGiftMessage(long long messageId, long long itemId, int amount, const CStrWChar& text)
{
    return 1;
}

unsigned char CNotificationHandler::HandleGift(int giftType, const CStrWChar& text, int amount)
{
    return 1;
}

unsigned char CNotificationHandler::HandleGenericNotifyMessage(int messageId, const CStrWChar& text)
{
    return 1;
}

unsigned char CNotificationHandler::HandleContentFileUploaded(unsigned char success)
{
    return 1;
}

unsigned char CNotificationHandler::HandleContentFileDownloadedSelf(unsigned char success, const unsigned char* data, int size)
{
    return 1;
}

unsigned char CNotificationHandler::HandleContentFileDownloadedFriend(unsigned char success, int userId, const unsigned char* data, int size)
{
    return 1;
}

unsigned char CNotificationHandler::HandleContentFileDeleted(unsigned char success)
{
    return 1;
}

unsigned char CNotificationHandler::HandleFriendPicUpdate(unsigned char success, int userId)
{
    return 1;
}

unsigned char CNotificationHandler::HandleGluOffersReset(unsigned char success, int count)
{
    return 1;
}

unsigned char CNotificationHandler::HandleAutoPollStart()
{
    return 1;
}

unsigned char CNotificationHandler::HandleRandomNonFriendListUpdate(unsigned char success, int count)
{
    return 1;
}

unsigned char CNotificationHandler::HandleProfileSaved(ProfileDataSaveStatus status, const TCVector<int>& dataIds)
{
    return 1;
}

unsigned char CNotificationHandler::HandleReceivedEnvelope()
{
    return 1;
}

unsigned char CNotificationHandler::HandleProfileContentFileUploaded(unsigned char success, int collection)
{
    return 1;
}

unsigned char CNotificationHandler::HandleLoginFlowNextStep()
{
    return 1;
}

unsigned char CNotificationHandler::HandleLockMgrGetSharedData(const CNGSLockMgrResponse& response, const CNGSLock* lock)
{
    return 0;
}

unsigned char CNotificationHandler::HandleLockMgrSaveSharedData(const CNGSLockMgrResponse& response, const CNGSLock* lock)
{
    return 0;
}

unsigned char CNotificationHandler::HandleLockMgrQueryAllSharedData(const CNGSLockMgrResponse& response, TCVector<CNGSSharedData*>& data)
{
    return 0;
}

unsigned char CNotificationHandler::HandleLockMgrReleaseLock(const CNGSLockMgrResponse& response, const CNGSLock* lock)
{
    return 0;
}

unsigned char CNotificationHandler::HandleLockMgrQueryLock(const CNGSLockMgrResponse& response, const CNGSLock* previous, const CNGSLock* current)
{
    return 0;
}

unsigned char CNotificationHandler::HandleFeaturedGameSelected(unsigned char success, int gameId)
{
    return 1;
}

unsigned char CNotificationHandler::HandleFileDownloadedMessage(unsigned char success, const CNGSDirectFileDownloadRequest& request)
{
    return 0;
}

unsigned char CNotificationHandler::HandleBatchFileDownloadStartedMessage(const CStrWChar& batch, int count)
{
    return 0;
}

unsigned char CNotificationHandler::HandleBatchFileDownloadCompleteMessage(const CStrWChar& batch, int count)
{
    return 0;
}

unsigned char CNotificationHandler::HandleGetAvatarIdFromServerForRemoteUserResponse(const CNGSUser& user, unsigned char success)
{
    return 0;
}
