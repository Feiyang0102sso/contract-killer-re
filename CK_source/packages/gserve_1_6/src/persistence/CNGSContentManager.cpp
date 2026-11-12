// CNGSContentManager.cpp - content file upload / download of the gServe user (b025x).
#include "CNGSContentManager.h"
#include "CNGS.h"
#include "CNGSFactory.h"
#include "CNGSHeader.h"
#include "CNGSJSONParser.h"
#include "CNGSLocalUser.h"
#include "CNGSRemoteUser.h"
#include "CNGSRemoteUserList.h"
#include "CNGSServerRequest.h"
#include "CNGSTokens.h"
#include "CNGSUtil.h"
#include "CObjectMap.h"
#include "CObjectMapObject.h"
#include "CProfileManager.h"
#include "CNotificationHandler.h"
#include "CHttpTransport_gServe.h"
#include "ICStdUtil.h"
#include "CStrChar.h"
#include "CUtf.h"
#include "NPMalloc.h"
#include <string.h>


/** Upload reply callback: calls a CNGSContentManager member with the reply. */
class CNGSContentRequestFunctor : public CNGSServerRequestFunctor {
public:
    typedef void (CNGSContentManager::*Callback)(CObjectMap* response, CNGSContentRequestFunctor* functor);

    CNGSContentRequestFunctor(CNGSContentManager* target, Callback callback)
        : m_target(target)
        , m_callback(callback)
        , m_context(NULL)
    {
    }

    virtual ~CNGSContentRequestFunctor() {}

    virtual void requestCompleteCallback(CObjectMap* response)
    {
        if (m_callback) {
            (m_target->*m_callback)(response, this);
        }
    }

    CNGSContentManager* m_target;       // +0x08
    Callback m_callback;                // +0x0c
    void* m_context;                    // +0x14 always NULL in gserve (meaning unknown; sizeof is 0x18)
};

CNGSContentManager::CNGSContentManager()
    : CSingleton(ClassId)
{
    Clear();
}

void CNGSContentManager::Clear()
{
    m_pendingRequest = NULL;
    m_transport = NULL;
    m_downloadBuffer = NULL;
    m_fileName = CStrWChar(L"");
    m_profileData = NULL;
    m_field20 = 0;
    m_field24 = 0;
    m_isProfileContent = 0;
    m_isFriendContent = 0;
    m_locked = 0;
}

CNGSContentManager::~CNGSContentManager()
{
}

eContentManagerStatus CNGSContentManager::getContentManagerStatus()
{
    eContentManagerStatus status = CONTENT_MANAGER_DOWNLOADING;
    if (m_transport == NULL) {
        status = CONTENT_MANAGER_UPLOADING;
        if (m_writeRequestsOutstanding == 0) {
            if (isReady()) {
                status = CONTENT_MANAGER_READY;
            } else {
                status = CONTENT_MANAGER_BUSY;
            }
        }
    }
    if (m_locked) {
        status = CONTENT_MANAGER_LOCKED;
    }
    return status;
}

void CNGSContentManager::HandleUpdate(int elapsedMs)
{
    if (m_transport != NULL) {
        m_transport->HandleUpdate(elapsedMs);
        int progress;
        if (m_transport->GetStatus(&progress) == 6) {
            unsigned char* data = NULL;
            int length = m_transport->GetResponseLen();
            bool hasData = length > 0;
            bool noError = m_transport->GetError() == 0;
            int statusCode = m_transport->GetResponseCode();
            bool success = hasData && noError;
            if (success) {
                data = new unsigned char[length + 1];
                memcpy(data, m_transport->GetResponsePtr(), length);
                if (m_pendingRequest != NULL) {
                    delete m_pendingRequest;
                    m_pendingRequest = NULL;
                }
            }
            m_transport->ClearResponse();
            m_transport->Cancel();
            if (m_downloadBuffer != NULL) {
                delete[] m_downloadBuffer;
            }
            m_downloadBuffer = NULL;
            if (m_transport != NULL) {
                delete m_transport;
            }
            m_transport = NULL;
            CNGSHandleContentDownloadResponse(success, data, length, statusCode);
            if (data != NULL) {
                delete[] data;
            }
        }
    }
}

void CNGSContentManager::CNGSHandleContentDownloadResponse(bool success, const unsigned char* data, int size, int statusCode)
{
    CNGSUtil::DebugLog("CNGSHandleContentDownloadResponse");
    CNGSLocalUser* localUser = CNGS::GetInstance()->GetLocalUser();
    if (!success && statusCode == 204) {
        success = true;
    }
    if (!success && statusCode == 404) {
        success = true;
        size = 0;
    }
    if (m_targetUserId == localUser->GetClientID()) {
        // Our own file: a profile collection or a plain content file.
        if (m_isProfileContent) {
            CNGS::GetInstance()->GetProfileManager()->HandleContentFileDownloadedSelf(success, data, size, m_profileData);
        } else {
            CNGS::GetInstance()->GetLocalUser()->m_notificationHandler->HandleContentFileDownloadedSelf(success, data, size);
        }
    } else if (m_isFriendContent) {
        CNGSRemoteUserList* friends = localUser->m_remoteUserLists[0];
        if (friends->isValid()) {
            CNGSRemoteUser* remoteUser = friends->findRemoteUserByID(m_targetUserId);
            if (remoteUser != NULL) {
                remoteUser->HandleContentFileDownloadedFriend(success, m_targetUserId, data, size);
            } else {
                // Not a listed friend: look the user up through the factory.
                CNGSUserCredentials credentials;
                credentials.m_userId = m_targetUserId;
                remoteUser = CNGS::GetInstance()->GetFactory()->getRemoteUserByCredentials(credentials);
                if (remoteUser != NULL) {
                    remoteUser->HandleContentFileDownloadedFriend(success, m_targetUserId, data, size);
                }
            }
        } else {
            CNGSUserCredentials credentials;
            credentials.m_userId = m_targetUserId;
            CNGSRemoteUser* remoteUser = CNGS::GetInstance()->GetFactory()->getRemoteUserByCredentials(credentials);
            if (remoteUser != NULL) {
                remoteUser->HandleContentFileDownloadedFriend(success, m_targetUserId, data, size);
            }
        }
    } else {
        CNGS::GetInstance()->GetLocalUser()->m_notificationHandler->HandleContentFileDownloadedFriend(success, m_targetUserId,
                                                                                                    data, size);
    }
    m_profileData = NULL;
    m_isProfileContent = 0;
    m_isFriendContent = 0;
}

unsigned char CNGSContentManager::GetContent(CObjectMap* message, CNGSContentRequestFunctor* functor)
{
    if (!isReady() || m_writeRequestsOutstanding != 0 || m_pendingRequest != NULL) {
        return 0;
    }

    m_field24 = 0;
    np_free(m_field20);
    m_field20 = NULL;

    CStrChar url(CNGS::GetInstance()->GetServerURL());
    url += "resources/content/get";
    TCVector<CObjectMap> messages;
    messages.Add(*message);
    CStrWChar json = CNGSJSONParser::encodeMessages(messages);
    unsigned int bufferSize = CUtf::GetUtfLen(json.GetWCharPtr(), json.GetLength()) + 1;
    m_downloadBuffer = new unsigned char[bufferSize];
    unsigned int length = CUtf::WcsToUtf8(json.GetWCharPtr(), json.GetLength(), m_downloadBuffer, bufferSize);

    SwpDeviceInfo info;
    info.m_timeoutMillis = 60000;
    info.m_connectTimeoutMillis = 60000;
    m_transport = new CHttpTransport_gServe();
    if (m_transport->Send(info, url.GetCharPtr(), m_downloadBuffer, length, 0, NULL,
                          "application/json", "PUT ", NULL) != 0) {
        delete m_transport;
        m_transport = NULL;
        delete[] m_downloadBuffer;
        m_downloadBuffer = NULL;
        return 0;
    }
    m_pendingRequest = functor;
    return 1;
}

unsigned char CNGSContentManager::GetContentSelf(const char* fileName)
{
    return GetContentFriend(CNGS::GetInstance()->GetLocalUser()->GetClientID(), fileName);
}

unsigned char CNGSContentManager::GetContentFriend(int userId, const char* fileName)
{
    CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
    m_targetUserId = userId;
    m_fileName = fileName;
    CNGSHeader header;
    CStrWChar serverFileName = generateFilename(userId, fileName);

    CObjectMapObject root;
    CObjectMapObject* userObject = new CObjectMapObject();
    userObject->addEntry(CStrWChar("id"), new CObjectMapInt(user->GetClientID()));
    root.addEntry(CStrWChar("user"), userObject);

    CObjectMapObject* content = new CObjectMapObject();
    content->addEntry(CStrWChar("gameid"), new CObjectMapInt(header.m_gameId));
    content->addEntry(CStrWChar("filename"), new CObjectMapString(serverFileName));
    content->addEntry(CStrWChar("targetId"), new CObjectMapInt(userId));
    root.addEntry(CStrWChar("content"), content);

    CObjectMap message(&root);
    InsertHeaderIntoMessage(&message, "get content");
    return GetContent(&message, NULL);
}

CObjectMapObject* CNGSContentManager::createContentUploadMessageObject(const char* fileName, const unsigned char* data, int size,
                                                                       CNGSContentRequestFunctor* functor)
{
    CStrWChar serverFileName = generateFilename(CNGS::GetInstance()->GetLocalUser()->GetClientID(), fileName);
    CNGSHeader header;
    // The encoded buffer is never freed (as in the original).
    char* encoded = CNGSUtil::Base64Encode(data, size);
    CObjectMapObject* content = new CObjectMapObject();
    content->addEntry(CStrWChar(NGS_GAME_ID_TOKEN), new CObjectMapInt(header.m_gameId));
    content->addEntry(CStrWChar(NGS_FILENAME_TOKEN), new CObjectMapString(serverFileName));
    content->addEntry(CStrWChar(NGS_DATA_TOKEN), new CObjectMapString(CStrWChar(encoded)));
    return content;
}

CStrWChar CNGSContentManager::generateFilename(int userId, const char* fileName)
{
    char userIdText[16] = { 0 };
    ICStdUtil::SPrintF(userIdText, "%i", userId);
    CStrWChar name;
    name += L"GLUID";
    name += userIdText;
    name += L"_";
    name += CStrWChar(fileName);
    return name;
}

unsigned int CNGSContentManager::UploadContent(const char* fileName, const unsigned char* data, int size)
{
    if (isReady() && m_writeRequestsOutstanding == 0) {
        m_fileName = fileName;
        CObjectMapObject* message = new CObjectMapObject();
        InsertHeaderIntoMessageObject(message, "upload content", 0);
        CObjectMapObject* user = CNGS::GetInstance()->GetLocalUser()->m_credentials.GetUserObject();
        CNGSContentRequestFunctor* functor =
            new CNGSContentRequestFunctor(this, &CNGSContentManager::CNGSHandleContentUploadResponse);
        message->addEntry(CStrWChar(NGS_USER_TOKEN), user);
        message->addEntry(CStrWChar(L"content"), createContentUploadMessageObject(fileName, data, size, functor));

        CObjectMap* map = new CObjectMap(message);
        delete message;

        CStrChar url(CNGS::GetInstance()->GetServerURL());
        url += "resources/content/upload";
        CNGSServerRequest* request = new CNGSServerRequest(url.GetCharPtr(), map, functor, 0, 1);
        AddWriteRequestOutstanding();
        return request->m_messageId;
    }
    return 0;
}

void CNGSContentManager::CNGSHandleContentUploadResponse(CObjectMap* response, CNGSContentRequestFunctor* functor)
{
    int error = CNGSServerObject::WasErrorInResponse(response, functor, "CNGSHandleContentUploadResponse");
    if (m_isProfileContent) {
        CNGS::GetInstance()->GetProfileManager()->HandleContentFileUploaded(error == 0, m_profileData);
    } else {
        CNGS::GetInstance()->GetLocalUser()->m_notificationHandler->HandleContentFileUploaded(error == 0);
    }
    m_profileData = NULL;
    m_isProfileContent = 0;
    m_isFriendContent = 0;
    CompleteWriteRequestOutstanding();
}

void CNGSContentManager::UploadProfileContent(const char* fileName, const unsigned char* data, int size,
                                              CSaveRestoreInterface* profileData)
{
    m_isProfileContent = 1;
    m_profileData = profileData;
    UploadContent(fileName, data, size);
}

void CNGSContentManager::Lock()
{
    m_locked = 1;
}

void CNGSContentManager::Unlock()
{
    m_locked = 0;
}

void CNGSContentManager::GetProfileContentSelf(const char* fileName, CSaveRestoreInterface* profileData)
{
    m_isProfileContent = 1;
    m_profileData = profileData;
    GetContentSelf(fileName);
}

void CNGSContentManager::GetRemoteUserContentFriend(int userId, const char* fileName)
{
    m_isFriendContent = 1;
    GetContentFriend(userId, fileName);
}

void CNGSContentManager::Reset()
{
    if (getContentManagerStatus() == CONTENT_MANAGER_DOWNLOADING && m_transport != NULL) {
        m_transport->Cancel();
        delete m_transport;
        delete[] m_downloadBuffer;
        np_free(m_field20);
    }
    CNGSServerObject::Reset();
    Clear();
}
