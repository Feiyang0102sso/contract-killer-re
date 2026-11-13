// CK game-side gServe notifications.
#include "CNotificationHandler.h"
#include "AdBannerSizes.h"
#include "ICInAppPurchase.h"
#include "App.h"
#include "CBHFriend.h"
#include "CBH_Player.h"
#include "CBHUtil_Cocoa.h"
#include "CFileData1.h"
#include "CFileUtil.h"
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
    CNGS::GetInstance()->GetLocalUser()->m_messageQueue->ProcessPromptMessages();
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
    App* app = CurrentApp();
    bool conflict = false;
    for (int index = 0; index < dataIds.Size(); index++) {
        int collection = dataIds[index];
        if (collection < 1000) {
            continue;
        }
        CSaveRestoreInterface* local =
            app->m_profileManager->getManagedObjectFromBuffer(PROFILE_DATA_SAVED_LOCAL, collection);
        CSaveRestoreInterface* server =
            app->m_profileManager->getManagedObjectFromBuffer(PROFILE_DATA_SYNCED, collection);
        if (local == NULL || server == NULL) {
            continue;
        }
        if (local->getDataSaveStatus() == PROFILE_DATA_INVALIDATED) {
            bool empty = false;
            if (collection == 1000) {
                empty = static_cast<CPublicData1*>(local)->m_level == 0;
            } else if (collection == 1001) {
                empty = static_cast<CPrivateData1*>(local)->getExperience() == 0;
            }
            if (empty) {
                TCVector<int> collections;
                app->m_profileManager->getAllRegisteredCollections(collections);
                app->m_gserveHandler->SelectProfileServerData(collections);
                app->m_field3a0 = false;
                conflict = false;
                break;
            }
        }
        conflict = true;
        if (collection == 1001) {
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
    }
    if (conflict) {
        CurrentApp()->m_gserveHandler->CloseCurrentSocialNetworkConnectDialog();
        CGameApp::gserveDataConflict = true;
        WindowApp::HandleTunnelCommand(0x07e17dc8, 0, 0, 0);
    } else if (CNGSLoginFlow::GetInstance()->IsRunning()) {
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
    CNewsFeedManager* news = CurrentApp()->m_newsFeedManager;
    TCVector<long long> ids;
    queue->GetAllMessagesByUserAndType(ids, 0, 0, CNGSFromServerMessage::CNGS_MSG_SUBTYPE_ANY);
    for (int index = 0; index < ids.Size(); index++) {
        CNGSFromServerMessage* message = queue->GetMessageById(ids[index]);
        if (message == NULL) {
            continue;
        }
        long long type = message->m_type;
        if (type == 40 || type == 41 || type == 100) {
            bool alreadyQueued = false;
            for (int gift = 0; gift < m_giftMessageIds.Size(); gift++) {
                if (m_giftMessageIds[gift] == message->m_messageId) {
                    alreadyQueued = true;
                    break;
                }
            }
            if (!alreadyQueued) {
                m_giftMessageIds.Add(message->m_messageId);
            }
            if (news == NULL || alreadyQueued) {
                continue;
            }
            if (message->m_subType == CNGSFromServerMessage::CNGS_MSG_SUBTYPE_AWARD ||
                message->m_subType == CNGSFromServerMessage::CNGS_MSG_SUBTYPE_GIFT) {
                CNGSMessageAward* award = static_cast<CNGSMessageAward*>(message);
                int item = (int)award->m_itemType - 1;
                if (type == 41 || message->m_subType == CNGSFromServerMessage::CNGS_MSG_SUBTYPE_AWARD) {
                    if (type == 41) {
                        WindowApp::HandleTunnelCommand(0x6df3535f, 0, 0, 0);
                    }
                    news->AddServerAwardMessage(message->GetMessageText(), (int)award->m_currencyCommon,
                                               (int)award->m_currencyRare, (int)award->m_experience,
                                               item, (int)message->m_messageId);
                } else if (award->m_itemType > 0) {
                    news->AddGiftReceivedMessage((int)message->m_senderId, item, (int)message->m_messageId);
                }
            }
            continue;
        }
        if (type == 300) {
            continue;
        }
        if (type == 1008 || type == 1011) {
            if (news != NULL) {
                CStrWChar text = message->GetMessageText();
                int challenge = XString(text.GetWCharPtr()).SubString(1, text.GetLength() - 2).ToInt();
                if (type == 1008) {
                    news->AddChallengeInviteMessage((int)message->m_senderId, challenge, (int)message->m_messageId);
                } else {
                    news->AddFriendCompletedChallengeMessage((int)message->m_senderId, challenge, text,
                                                             (int)message->m_messageId);
                }
            }
            continue;
        }
        if (type >= 1000 && !HandleGameSpecificMessage(type, message->GetMessageText(), message->m_senderId)) {
            continue;
        }
        if (queue->MarkMessageForAck(message->m_messageId)) {
            delete message;
        }
    }
    WindowApp::HandleTunnelCommand(0x6e7d5209, 0, 0, 0);
    return 1;
}

unsigned char CNotificationHandler::HandleGameSpecificMessage(long long type, const CStrWChar& text,
                                                              long long senderId)
{
    App* app = CurrentApp();
    CNewsFeedManager* news = app->m_newsFeedManager;
    switch (type) {
    case 1001:
    case 1004:
        if (news != NULL) {
            news->AddTargettedMessage((int)senderId, text, type == 1001);
        }
        break;
    case 1002: {
        CNGSRemoteUserList* friends = CNGS::GetInstance()->GetLocalUser()->m_remoteUserLists[0];
        if (news != NULL && friends->findRemoteUserByID((int)senderId) != NULL) {
            CStrWChar id = text.GetSubString(1, text.GetLength() - 1);
            news->AddMissionInviteMessage((int)senderId, XString(id.GetWCharPtr()).ToInt());
        }
        break;
    }
    case 1005:
        if (news != NULL && !app->m_publicData->isInGuildFriendList((int)senderId)) {
            news->AddGuildInviteMessage((int)senderId, text);
        }
        break;
    case 1006:
        app->m_publicData->addGuildFriend((int)senderId);
        if (news != NULL) {
            news->AddGuildInviteResponseMessage((int)senderId, true);
        }
        return 1;
    case 1007:
        if (news != NULL) {
            news->AddGuildInviteResponseMessage((int)senderId, false);
        }
        return 1;
    case 1009:
    case 1010:
        if (news != NULL) {
            int challenge = XString(text.GetWCharPtr()).SubString(1, text.GetLength() - 2).ToInt();
            news->AddChallengeResponseMessage((int)senderId, challenge, type == 1009);
        }
        return 1;
    }
    return 0;
}

void CNotificationHandler::RemoveGameSpecificMessage(int messageId)
{
    // the original looks the message up only after acknowledging it and keeps the gift id list
    CNGSFromServerMessageQ* queue = CNGS::GetInstance()->GetLocalUser()->m_messageQueue;
    if (messageId >= 0 && queue->MarkMessageForAck(messageId)) {
        delete queue->GetMessageById(messageId);
    }
}

void CNotificationHandler::OnEnterForeground()
{
    App* app = CurrentApp();
    if (m_loginState != 1 && app->m_field398 == SOCIAL_INTERFACE_FACEBOOK) {
        m_loginState = 2;
        app->m_field398 = -1;
        CNGSLoginFlow* flow = CNGSLoginFlow::GetInstance();
        if (flow->IsRunning()) {
            flow->PrepareStep(-1);
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
    App* app = CurrentApp();
    CNGSLoginFlow* flow = CNGSLoginFlow::GetInstance();
    CGServeHandler* handler = app->m_gserveHandler;
    const int network = SOCIAL_INTERFACE_GAME_CENTER;
    const unsigned int socialStep = 0x90e4b4fa;
    if (CBHUtil_Cocoa::IsGameCenterAuthenticated()) {
        if (flow->IsRunning() && handler->m_pendingSwitchNetwork == network) {
            handler->m_pendingSwitchNetwork = 0;
            app->StartLogin(App::LoginGameCenter, true);
        } else if (!CNGS::GetInstance()->GetLocalUser()->isAuthenticated(network)) {
            if (!flow->IsRunning()) {
                app->StartLogin(App::LoginGameCenter, true);
                return 1;
            }
            CNGSLoginStep* active = flow->GetCurrentStep();
            CNGSLoginStep* previous = flow->GetStep(flow->GetStepCount() - 2);
            if (active != NULL && active->GetClassId() != socialStep &&
                previous != NULL && previous->GetClassId() != socialStep) {
                flow->Queue(new GameCenterLoginStep());
                flow->Queue(new LoadFriendsListStep(network));
            }
        }
    } else if (handler->m_pendingSwitchNetwork == network) {
        handler->m_pendingSwitchNetwork = 0;
        if (flow->IsRunning()) {
            if (flow->m_activeNetwork == network) {
                flow->NextStep();
            } else {
                flow->ExcuteStep(-1);
            }
        }
    }
    return 1;
}

unsigned char CNotificationHandler::HandleProfileDataFormatIncorrect(const CStrWChar& name,
    const CStrWChar&, unsigned char*& data, unsigned int& size)
{
    CFileData1 legacy;
    if (CStdUtil_Cocoa::WcsICmp(name.GetWCharPtr(), legacy.getFilename().GetWCharPtr()) != 0) {
        return 0;
    }
    CStrWChar path;
    CFileUtil::GetApplicationDataPathForFile(path, L"savebh.dat");
    unsigned int byteCount = ICFileMgr::GetInstance()->Size(path.GetWCharPtr());
    if (byteCount == 0) {
        return 0;
    }
    unsigned char* bytes = new unsigned char[byteCount];
    unsigned char read = CFileUtil::ReadFile(path.GetWCharPtr(), bytes, byteCount);
    if (read) {
        legacy.setData(bytes, byteCount);
        legacy.setDataSaveStatus(PROFILE_DATA_SAVED_LOCAL);
    }
    delete[] bytes;
    if (!read) {
        return 0;
    }
    data = NULL;
    size = 0;
    return CurrentApp()->m_profileManager->WriteAttributesToRawFileData(&legacy, data, size);
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
