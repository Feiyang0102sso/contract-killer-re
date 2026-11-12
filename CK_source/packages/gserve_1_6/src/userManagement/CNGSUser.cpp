// CNGSUser.cpp - common part of the local and the remote users: credentials file handling
// and accessors.
#include "CNGSUser.h"
#include "CNGS.h"
#include "CNGSFactory.h"
#include "CNGSFromServerMessageQ.h"
#include "CNGSRemoteUser.h"
#include "CNGSLocalUser.h"
#include "CNotificationHandler.h"
#include "CObjectMap.h"
#include "CObjectMapObject.h"
#include "CFileUtil.h"
#include "ICFileMgr.h"
#include "CNGSAttributeManager.h"






CNGSUser::CNGSUser(const CNGSUserCredentials* credentials)
{
    if (credentials != NULL) {
        m_credentials = *credentials;
    }
}

CNGSUser::~CNGSUser()
{
}

class CNGSUserFunctor : public CNGSServerRequestFunctor {
public:
    typedef void (CNGSUser::*Callback)(CObjectMap* response, CNGSUserFunctor* functor);

    CNGSUserFunctor(CNGSUser* user, Callback callback, int includeFiles)
        : m_user(user)
        , m_callback(callback)
        , m_includeFiles(includeFiles)
    {
    }

    virtual ~CNGSUserFunctor() {}

    virtual void requestCompleteCallback(CObjectMap* response)
    {
        if (m_callback) {
            (m_user->*m_callback)(response, this);
        }
    }

    CNGSUser* m_user;
    int m_reserved;
    Callback m_callback;
    int m_includeFiles;
};

CObjectMapObject* CNGSUser::CreateAvatarIdFromServerRequest()
{
    CObjectMapObject* message = new CObjectMapObject();
    InsertHeaderIntoMessageObject(message, "ngs.avatar.query", 1);
    message->addEntry(CStrWChar("user"), m_credentials.GetUserObject());
    return message;
}

void CNGSUser::GetAvatarIdAndContinueDeepPopulate(bool includeFiles)
{
    AddReadRequestOutstanding();
    CObjectMap* message = new CObjectMap(CreateAvatarIdFromServerRequest());
    CNGSUserFunctor* functor = new CNGSUserFunctor(this, &CNGSUser::HandleGetAvatarIdAndContinueDeepPopulateResponse,
                                                 includeFiles);
    SendMessageToServer(message, "resources/avatar/queryavatarid", functor);
}

void CNGSUser::HandleGetAvatarIdAndContinueDeepPopulateResponse(CObjectMap* response, CNGSUserFunctor* functor)
{
    if (WasErrorInResponse(response, functor, "HandleGetAvatarIdAndContinueDeepPopulateResponse")) {
        SetIsValid(0);
    } else {
        CObjectMapValue* id = response->getDataAt(CStrWChar("list,0,id"), CStrWChar(","));
        if (id != NULL && id->getType() == CObjectMapValue::TYPE_INT) {
            m_credentials.m_avatarId = (int)static_cast<CObjectMapInt*>(id)->m_value;
        }
        if (m_credentials.m_avatarId < 1) {
            SetIsValid(0);
            CNotificationHandler* handler = CNGS::GetInstance()->GetLocalUser()->m_notificationHandler;
            handler->HandleGuestDeepPopulated(static_cast<CNGSRemoteUser*>(this), 0);
        } else {
            SetIsValid(1);
        }
    }
    CompleteReadRequestOutstanding();
    if (m_credentials.m_avatarId > 0) {
        static_cast<CNGSRemoteUser*>(this)->deepPopulate(functor->m_includeFiles != 0);
    }
}

void CNGSUser::SaveCredentials(const wchar_t* fileName)
{
    CStrWChar name(fileName);
    m_credentials.writeToFile(name, false);
    m_credentials.writeToFile(name, true);
}

void CNGSUser::LoadCredentials(const wchar_t* fileName)
{
    CStrWChar name(fileName);
    m_credentials.readFromFile(name, false);
}

void CNGSUser::ResetCredentials(const wchar_t* fileName)
{
    m_credentials.reset();
    if (fileName != NULL) {
        DeleteCredentialsFile(fileName);
    }
}

void CNGSUser::DeleteCredentialsFile(const wchar_t* fileName)
{
    CStrWChar path;
    CFileUtil::GetApplicationDataPathForFile(path, fileName);
    ICFileMgr::GetInstance()->Delete(path.GetWCharPtr());
}

int CNGSUser::GetClientID() const
{
    return m_credentials.m_userId;
}

void CNGSUser::SendMessage(CObjectMapValue* content, unsigned int duration, long long type,
                          unsigned char push, CNGSPushNotificationDetails* details)
{
    CNGSLocalUser* local = CNGS::GetInstance()->GetLocalUser();
    CNGSFromServerMessageQ* queue = local->m_messageQueue;
    TCVector<CObjectMapObject*> envelopes;
    envelopes.Add(queue->CreateEnvelopeObject(m_credentials.m_userId, content, duration, type, push, details));
    queue->PostEnvelopes(envelopes);
}

void CNGSUser::SendGift(CStrWChar text, long long itemType, int quantity, unsigned int duration,
                       unsigned char deductOnReceipt, unsigned char push, CNGSPushNotificationDetails* details)
{
    CNGSFromServerMessageQ* queue = CNGS::GetInstance()->GetLocalUser()->m_messageQueue;
    CObjectMapObject* payload = queue->CreateGiftMsgObject(text, deductOnReceipt, itemType, quantity, 0, 0, 0);
    SendMessage(payload, duration, 40, push, details);
    delete payload;
}

void CNGSUser::SetNickName(const CStrWChar& nickName)
{
    m_credentials.m_nickName = nickName;
}

CStrWChar CNGSUser::GetNickName() const
{
    return m_credentials.m_nickName;
}

CStrWChar CNGSUser::GetEmail() const
{
    return m_credentials.m_email;
}

void CNGSUser::SetIDForSocialNetwork(eSocialNetworkType network, const CStrWChar& id)
{
    m_credentials.setIDForSocialNetwork(network, id);
}

CStrWChar CNGSUser::GetIDForSocialNetwork(eSocialNetworkType network) const
{
    return m_credentials.getIDForSocialNetwork(network);
}

CStrWChar CNGSUser::GetAliasForSocialNetwork(eSocialNetworkType network) const
{
    return m_credentials.getAliasForSocialNetwork(network);
}

void CNGSUser::SetAliasForSocialNetwork(eSocialNetworkType network, const CStrWChar& alias)
{
    m_credentials.setAliasForSocialNetwork(network, alias);
}

void CNGSUser::setFriendMsgTimestampLocally(int userId, unsigned long long timestamp)
{
    CNGSUserCredentials credentials;
    credentials.m_userId = userId;
    CNGSRemoteUser* user = CNGS::GetInstance()->GetFactory()->getRemoteUserByCredentials(credentials);
    if (user != NULL) {
        user->m_friendMsgTimestamp = timestamp;
    }
}

CObjectMapObject* CNGSUser::GetUserObject()
{
    return m_credentials.GetUserObject();
}

void CNGSUser::UpdateCredentials(const CNGSUserCredentials& credentials)
{
    if (m_credentials.mergeWith(credentials, true) != 1) {
        m_credentials = credentials;
    }
}

void CNGSUser::GetSKUBonusItems(CStrWChar& sku, TCVector<CNGSAttribute*>& items)
{
}

CStrWChar CNGSUser::GetSKUBonusNameForItem(CNGSAttribute* item)
{
    return CStrWChar(L"??");
}

void CNGSUser::DebugPrint()
{
    m_credentials.debugPrint();
}
