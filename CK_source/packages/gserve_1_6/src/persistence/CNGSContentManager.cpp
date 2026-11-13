// CNGSContentManager.cpp - content file upload / download of the gServe user (b025x).
#include "CNGSContentManager.h"
#include "CArrayOutputStream.h"
#include "CNetMessageServer.h"
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
#include "NPMem.h"
#undef memcpy
#include <string.h>

extern "C" void* memcpy(void* destination, const void* source, size_t size);

static const char CONTENT_TYPE_JSON[] = "application/json";
static const char cszPut[] = "PUT ";
static const char NGS_CONTENT_TOKEN[] = "content";
static const char NGS_TARGET_ID[] = "targetId";


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
            int success = hasData && noError;
            int statusCode = m_transport->GetResponseCode();
            if (success) {
                data = new unsigned char[length + 1];
                np_memcpy(data, m_transport->GetResponsePtr(), length);
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
            CNGSHandleContentDownloadResponse(success == 1, data, length, statusCode);
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
    if (!success && (statusCode == 204 || statusCode == 404)) {
        size = 0;
        success = true;
    }

    int targetUserId = m_targetUserId;
    if (targetUserId == localUser->GetClientID()) {
        // Our own file: a profile collection or a plain content file.
        unsigned char profileContent = m_isProfileContent;
        CNGS* gserve = CNGS::GetInstance();
        if (profileContent) {
            gserve->GetProfileManager()->HandleContentFileDownloadedSelf(success, data, size, m_profileData);
        } else {
            gserve->GetLocalUser()->m_notificationHandler->HandleContentFileDownloadedSelf(success, data, size);
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
        CNGS::GetInstance()->GetLocalUser()->m_notificationHandler->HandleContentFileDownloadedFriend(
            success, m_targetUserId, data, size);
    }

    // The original clears these fields after delivering the completion callback.
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
    if (m_field20 != NULL) {
        np_free(m_field20);
        m_field20 = NULL;
    }

    char* url = static_cast<char*>(np_malloc(
        strlen(CNGS::GetInstance()->GetServerURL()) + sizeof("resources/content/get")));
    strcpy(url, CNGS::GetInstance()->GetServerURL());
    memcpy(url + strlen(url), "resources/content/get", sizeof("resources/content/get"));
    CNetMessageServer server((CStrChar(url)));
    server.m_value10 = 0;
    server.m_value14 = 0;
    server.m_method = CNetMessageServer::METHOD_PUT;

    TCVector<CObjectMap> messages;
    messages.Add(*message);
    CStrWChar json = CNGSJSONParser::encodeMessages(messages);
    SwpDeviceInfo info;
    info.m_timeoutMillis = 60000;
    info.m_connectTimeoutMillis = 60000;
    m_downloadBuffer = new unsigned char[json.GetLength() + 1];
    m_transport = new CHttpTransport_gServe();
    CUtf::WcsToUtf8(json.GetWCharPtr(), json.GetLength(), m_downloadBuffer, json.GetLength() + 1);

    int length = json.GetLength();
    unsigned char* utf = static_cast<unsigned char*>(np_malloc(length + 1));
    CUtf::WcsToUtf8(json.GetWCharPtr(), length, utf, length + 1);
    CArrayOutputStream output;
    output.Open(m_downloadBuffer, length);
    output.m_isBigEndian = 1;
    output.Write(utf, length);
    unsigned char started = 0;
    if (output.m_error == 0) {
        CHttpTransport_gServe* transport = m_transport;
        int result;
        {
            CStrChar requestUrl(server.m_url);
            result = transport->Send(info, requestUrl.GetCharPtr(), m_downloadBuffer, length,
                                     0, NULL, CONTENT_TYPE_JSON, cszPut, NULL);
        }
        if (result == 0) {
            m_pendingRequest = functor;
            started = 1;
        }
    }
    output.Close();
    if (utf != NULL) {
        np_free(utf);
    }
    if (url != NULL) {
        np_free(url);
    }
    return started;
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
    CNGSUtil::DebugLog("CNGSHandleContentUploadResponse");
    int error = CNGSServerObject::WasErrorInResponse(response, functor, "CNGSHandleContentUploadResponse");
    unsigned char profileContent = m_isProfileContent;
    CNGS* gserve = CNGS::GetInstance();
    if (profileContent) {
        gserve->GetProfileManager()->HandleContentFileUploaded(error == 0, m_profileData);
    } else {
        gserve->GetLocalUser()->m_notificationHandler->HandleContentFileUploaded(error == 0);
    }
    m_profileData = NULL;
    m_isProfileContent = 0;
    m_isFriendContent = 0;
    CompleteWriteRequestOutstanding();
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

    CObjectMap* message = new CObjectMap();
    message->beginObject();
    message->beginObject(CStrWChar(NGS_USER_TOKEN));
    message->addEntry(CStrWChar(NGS_ID_TOKEN), CObjectMapInt(user->GetClientID()));
    message->endObject();
    message->beginObject(CStrWChar(NGS_CONTENT_TOKEN));
    message->addEntry(CStrWChar(NGS_GAME_ID_TOKEN), CObjectMapInt(header.m_gameId));
    message->addEntry(CStrWChar(NGS_FILENAME_TOKEN), CObjectMapString(serverFileName));
    message->addEntry(CStrWChar(NGS_TARGET_ID), CObjectMapInt(userId));
    message->endObject();
    InsertHeaderIntoMessage(message, "get content");
    message->endObject();
    unsigned char started = GetContent(message, NULL);
    if (message != NULL) {
        delete message;
    }
    return started;
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

void CNGSContentManager::UploadProfileContent(const char* fileName, const unsigned char* data, int size,
                                              CSaveRestoreInterface* profileData)
{
    m_isProfileContent = 1;
    m_profileData = profileData;
    UploadContent(fileName, data, size);
}

unsigned char CNGSContentManager::GetProfileContentSelf(const char* fileName, CSaveRestoreInterface* profileData)
{
    m_isProfileContent = 1;
    m_profileData = profileData;
    return GetContentSelf(fileName);
}

void CNGSContentManager::GetRemoteUserContentFriend(int userId, const char* fileName)
{
    m_isFriendContent = 1;
    GetContentFriend(userId, fileName);
}

void CNGSContentManager::Lock()
{
    m_locked = 1;
}

void CNGSContentManager::Unlock()
{
    m_locked = 0;
}

void CNGSContentManager::Reset()
{
    if (getContentManagerStatus() == CONTENT_MANAGER_DOWNLOADING && m_transport != NULL) {
        m_transport->Cancel();
        if (m_transport != NULL) {
            delete m_transport;
            m_transport = NULL;
        }
        if (m_downloadBuffer != NULL) {
            delete[] m_downloadBuffer;
            m_downloadBuffer = NULL;
        }
        if (m_field20 != NULL) {
            np_free(m_field20);
            m_field20 = NULL;
        }
    }
    CNGSServerObject::Reset();
    Clear();
}
