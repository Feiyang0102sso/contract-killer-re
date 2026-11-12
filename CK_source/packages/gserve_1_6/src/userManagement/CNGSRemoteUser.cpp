// CNGSRemoteUser.cpp - another player: lookup by nickname, social network flags and the
// delivery of its downloaded profile content file.
#include "CNGSRemoteUser.h"
#include "CNGS.h"
#include "CNGSLocalUser.h"
#include "CObjectMap.h"
#include "CObjectMapObject.h"
#include "CObjectMapArray.h"
#include "CNGSContentManager.h"
#include "CProfileManager.h"
#include "CProfileDataDescriptor.h"
#include "CSaveRestoreInterface.h"
#include "CNotificationHandler.h"
#include "CNGSAttributeManager.h"
#include "CNGSTokens.h"
#include "CNGSUtil.h"


CNGSRemoteUser::CNGSRemoteUser(const CNGSUserCredentials* credentials)
    : CNGSUser(credentials)
    , m_isFriendOfLocalUser(0)
    , m_friendMsgTimestamp(0)
    , m_socialNetworkTypes(0)
    , m_profileDataLimit(0)
{
    if (credentials != NULL) {
        SetIsValid(credentials->isValid());
        // only a nickname known: ask the server for the rest
        if (!credentials->isValid() && credentials->m_nickName.GetLength() != 0) {
            PopulateViaNickname();
        }
    }
    m_deepPopulateStatus = 0;
}

unsigned int CNGSRemoteUser::PopulateViaNickname()
{
    unsigned int requestId = 0;
    if (m_credentials.m_nickName.GetLength() != 0) {
        AddReadRequestOutstanding();
        CObjectMapObject* message = new CObjectMapObject();

        CObjectMapObject* query = new CObjectMapObject();
        query->addEntry(CStrWChar(NGS_GAMERTAG_TOKEN), new CObjectMapString(m_credentials.m_nickName));

        CObjectMapObject* request = new CObjectMapObject();
        request->addEntry(CStrWChar("brother"), query);
        request->addEntry(CStrWChar("items"), new CObjectMapInt(1));
        request->addEntry(CStrWChar(NGS_STATUS_TOKEN), new CObjectMapInt(0));
        request->addEntry(CStrWChar("batch"), new CObjectMapBool(0));

        InsertHeaderIntoMessageObject(message, "", 0);
        message->addEntry(CStrWChar(NGS_USER_TOKEN), CNGS::GetInstance()->GetLocalUser()->GetUserObject());
        message->addEntry(CStrWChar("brother"), request);

        CObjectMap* map = new CObjectMap(message);
        CNGSRemoteUserFunctor* functor = new CNGSRemoteUserFunctor(this, &CNGSRemoteUser::HandlePopulateViaNicknameResponse);
        requestId = SendMessageToServer(map, "resources/friend/getOther", functor);
    }
    return requestId;
}

CNGSRemoteUser::~CNGSRemoteUser()
{
    // attribute collections own the content file collection they describe
    for (int index = 0; index < m_profileData.Size(); index++) {
        if (m_profileData[index]->getBackingStoreType() == BACKING_STORE_ATTRIBUTES) {
            CSaveRestoreInterface* collection = m_profileData[index];
            CSaveRestoreInterface* file = collection->getFile();
            if (file != NULL) {
                file->setDescriptor(NULL);
                delete collection;
            }
        }
    }
    m_profileData.Clear();
}

unsigned char CNGSRemoteUser::IsLocalUser()
{
    return 0;
}

void CNGSRemoteUser::SetIsFriendOfLocalUser(unsigned char isFriend)
{
    m_isFriendOfLocalUser = isFriend;
}

void CNGSRemoteUser::registerSaveRestoreInterface(CSaveRestoreInterface* collection)
{
    m_profileData.Add(collection);
    if (collection->getBackingStoreType() == BACKING_STORE_CONTENT_FILE) {
        CProfileDataDescriptor* descriptor = new CProfileDataDescriptor();
        descriptor->setCollectionType(CProfileDataDescriptor::generateCollectionType(collection->getCollectionType()));
        m_profileData.Add(descriptor);
        descriptor->setFile(collection);
        collection->setDescriptor(descriptor);
    }
    m_profileDataLimit = m_profileData.Size();
}

unsigned char CNGSRemoteUser::deepPopulate(unsigned char includeFiles)
{
    CAttributeManager* attributes = CAttributeManager::GetInstance();
    if (!isAvailable()) {
        return 0;
    }
    if (m_credentials.m_avatarId < 0) {
        GetAvatarIdAndContinueDeepPopulate(includeFiles != 0);
        return 0;
    }

    m_deepPopulateStatus = 2;
    m_includeFiles = includeFiles;
    if (includeFiles) {
        CNGSContentManager::GetInstance()->Lock();
    }
    for (int index = 0; index < m_profileDataLimit; index++) {
        m_profileData[index]->resetAllData();
    }
    TCVector<int> types;
    getFriendCollectionTypes(types);
    AddReadRequestOutstanding();
    CNGSRemoteUserFunctor* functor = new CNGSRemoteUserFunctor(this, &CNGSRemoteUser::HandleDeepPopulateResponse);
    attributes->getDataFromServer(types, functor, &m_credentials);
    return 1;
}

void CNGSRemoteUser::getFriendCollectionTypes(TCVector<int>& types)
{
    for (int index = 0; index < m_profileDataLimit; index++) {
        CSaveRestoreInterface* collection = m_profileData[index];
        if (collection->getBackingStoreType() != BACKING_STORE_CONTENT_FILE &&
            (collection->getFile() == NULL || m_includeFiles)) {
            types.Add(collection->getCollectionType());
        }
    }
}

void CNGSRemoteUser::HandleDeepPopulateResponse(CObjectMap* response, CNGSRemoteUserFunctor* functor)
{
    CAttributeManager* attributes = CAttributeManager::GetInstance();
    unsigned char success = 0;
    bool pendingFile = false;
    if (WasErrorInResponse(response, functor, "CNGSRemoteUser::HandleDeepPopulateResponse")) {
        m_deepPopulateStatus = 3;
    } else {
        CHash values;
        values.Init(20, 20, 1);
        if (attributes->extractAttributeValuesAsHashMap(response, functor, values) < 1) {
            m_deepPopulateStatus = 5;
        } else {
            success = 1;
            for (int index = 0; index < m_profileDataLimit; index++) {
                CSaveRestoreInterface* collection = m_profileData[index];
                if (collection->getBackingStoreType() != BACKING_STORE_CONTENT_FILE) {
                    collection->setAllData(&values);
                }
            }
            if (m_includeFiles) {
                for (int index = 0; index < m_profileDataLimit; index++) {
                    if (m_profileData[index]->getBackingStoreType() == BACKING_STORE_CONTENT_FILE) {
                        success = downloadFileBackedDataFriend(index);
                        pendingFile = success != 0;
                        if (!success) {
                            m_deepPopulateStatus = 4;
                        }
                        break;
                    }
                }
            }
            for (int bucket = 0; bucket < values.m_bucketCount; bucket++) {
                for (CHash::Entry* entry = values.m_buckets[bucket]; entry != NULL; entry = entry->m_next) {
                    delete static_cast<CNGSAttribute*>(entry->m_value);
                }
            }
        }
        if (pendingFile) {
            return;
        }
        if (m_deepPopulateStatus == 2) {
            m_deepPopulateStatus = 1;
        }
    }

    CompleteReadRequestOutstanding();
    CNotificationHandler* handler = CNGS::GetInstance()->GetLocalUser()->m_notificationHandler;
    if (m_isFriendOfLocalUser) {
        handler->HandleFriendDeepPopulated(this, success);
    } else {
        handler->HandleGuestDeepPopulated(this, success);
    }
    if (m_includeFiles) {
        CNGSContentManager::GetInstance()->Unlock();
    }
}

unsigned char CNGSRemoteUser::downloadFileBackedDataFriend(unsigned int index)
{
    CNGSContentManager* contentManager = CNGSContentManager::GetInstance();
    CSaveRestoreInterface* collection = m_profileData[index];
    CStrChar fileName = CNGSUtil::WStrToCStr(collection->getFilename());
    CProfileDataDescriptor* descriptor = static_cast<CProfileDataDescriptor*>(collection->getDescriptor());
    if (descriptor != NULL) {
        char suffix[] = {'.', (char)descriptor->getAB(), '\0'};
        fileName += suffix;
    }
    contentManager->Unlock();
    if (contentManager->getContentManagerStatus() != CONTENT_MANAGER_READY) {
        return 0;
    }
    contentManager->GetRemoteUserContentFriend(GetClientID(), fileName.GetCharPtr());
    return 1;
}

void CNGSRemoteUser::HandlePopulateViaNicknameResponse(CObjectMap* response, CNGSRemoteUserFunctor* functor)
{
    if (WasErrorInResponse(response, functor, "HandlePopulateViaNicknameResponse") == 0) {
        CObjectMapArray* list = (CObjectMapArray*)response->getDataAt(CStrWChar(NGS_LIST_TOKEN), CStrWChar(L" "));
        int count;
        if (list->getType() == CObjectMapValue::TYPE_ARRAY && (count = list->m_values.Size()) > 0) {
            // exactly one user with our nickname: take its credentials
            int matches = 0;
            for (int index = 0; index < count; index++) {
                CNGSUserCredentials credentials((CObjectMapObject*)list->m_values[index], true);
                if (credentials.m_nickName == m_credentials.m_nickName) {
                    matches++;
                    m_credentials = credentials;
                }
            }
            if (matches == 1) {
                SetIsValid(1);
            }
        }
    }
    CompleteReadRequestOutstanding();
}

void CNGSRemoteUser::AddSocialNetworkType(int network)
{
    m_socialNetworkTypes |= 1 << network;
}

void CNGSRemoteUser::RemoveSocialNetworkType(int network)
{
    m_socialNetworkTypes &= ~(1 << network);
}

unsigned char CNGSRemoteUser::IsFromSocialNetwork(int network) const
{
    return (m_socialNetworkTypes & (1 << network)) != 0;
}

unsigned int CNGSRemoteUser::GetSocialNetworkType() const
{
    return m_socialNetworkTypes;
}

unsigned char CNGSRemoteUser::isAvailable()
{
    unsigned char available = 0;
    if (isReady() && m_writeRequestsOutstanding == 0) {
        available = 1;
        for (int index = 0; index < m_profileData.Size(); index++) {
            if (m_profileData[index]->getBackingStoreType() == BACKING_STORE_CONTENT_FILE) {
                if (CNGSContentManager::GetInstance()->getContentManagerStatus() == CONTENT_MANAGER_READY) {
                    return 1;
                }
                return 0;
            }
        }
    }
    return available;
}

unsigned char CNGSRemoteUser::HandleContentFileDownloadedFriend(unsigned char success, int userId, const unsigned char* data,
                                                                int size)
{
    CNGSContentManager* contentManager = CNGSContentManager::GetInstance();
    if (GetClientID() == userId) {
        for (int index = 0; index < m_profileDataLimit; index++) {
            if (m_profileData[index]->getBackingStoreType() == BACKING_STORE_CONTENT_FILE) {
                CSaveRestoreInterface* collection = m_profileData[index];
                if (collection != NULL) {
                    // downloaded file name without its 3 character suffix (the A/B name)
                    CStrWChar fileName(contentManager->m_fileName);
                    fileName = fileName.GetSubString(0, fileName.GetLength() - 3);
                    bool sameFile = fileName == collection->getFilename();
                    if (sameFile) {
                        if (!success) {
                            m_deepPopulateStatus = 4;
                        } else if (size > 0) {
                            CNGS::GetInstance()->GetProfileManager()->ReadRawFileDataToAttributes(data, size, collection);
                        } else {
                            m_deepPopulateStatus = 6;
                        }
                    }
                }
                break;
            }
        }
    }
    if (m_deepPopulateStatus == 2) {
        m_deepPopulateStatus = 1;
    }
    CompleteReadRequestOutstanding();

    unsigned char isFriend = m_isFriendOfLocalUser;
    CNotificationHandler* handler = CNGS::GetInstance()->GetLocalUser()->m_notificationHandler;
    if (isFriend) {
        handler->HandleFriendDeepPopulated(this, success);
    } else {
        handler->HandleGuestDeepPopulated(this, success);
    }
    CNGSContentManager::GetInstance()->Unlock();
    return 1;
}
