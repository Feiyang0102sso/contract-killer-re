// CProfileManager.cpp - player profile load / save / server sync (b025x).
#include "CProfileManager.h"
#include "CProfileDataDescriptor.h"
#include "CNGSAttribute.h"
#include "CNGSAttributeManager.h"
#include "CNGSContentManager.h"
#include "CNGS.h"
#include "CNGSLocalUser.h"
#include "CNGSSession.h"
#include "CNGSUtil.h"
#include "CNGSJSONParser.h"
#include "CNGSFunctor.h"
#include "CNGSFileMgr.h"
#include "ICFileMgr.h"
#include "CFileUtil_gServe.h"
#include "CArrayStream_gServe.h"
#include "CNotificationHandler.h"
#include "CObjectMapObject.h"
#include "CHash.h"
#include "CCrc32.h"
#include "CUtf.h"
#include <stdlib.h>
#include <string.h>


/** Key of the obfuscated profile files. */
static const wchar_t* const PROFILE_FILE_KEY = L"GgLlUu";

typedef char ProfileManagerSizeCheck[sizeof(CProfileManager) == 0xcc ? 1 : -1];

CProfileManager::CProfileManager(const CNGSUserCredentials* credentials)
{
    CNGSUtil::DebugLog("CProfileManager::CProfileManager");
    m_obfuscateFiles = false;
}

CProfileManager::~CProfileManager()
{
    CNGSUtil::DebugLog("CProfileManager::~CProfileManager");
    for (int index = 0; index < m_registeredData.Size(); index++) {
        if (m_registeredData[index] != NULL) {
            delete static_cast<CProfileDataDescriptor*>(m_registeredData[index]);
            m_registeredData[index] = NULL;
        }
    }
    m_registeredData.Clear();
}

void CProfileManager::Reset()
{
    CNGSServerObject::Reset();
}

void CProfileManager::setCollectionRange(int first, int last)
{
    m_firstCollectionType = first;
    m_lastCollectionType = last;
    // one slot per collection type of the range in every buffer
    int count = m_lastCollectionType + 1 - m_firstCollectionType;
    m_currentData.EnsureCapacity(count);
    m_currentData.m_size = count;
    count = m_lastCollectionType + 1 - m_firstCollectionType;
    m_serverData.EnsureCapacity(count);
    m_serverData.m_size = count;
    count = m_lastCollectionType + 1 - m_firstCollectionType;
    m_localData.EnsureCapacity(count);
    m_localData.m_size = count;
}

void CProfileManager::registerSaveRestoreInterface(CSaveRestoreInterface* current, CSaveRestoreInterface* server,
                                                   CSaveRestoreInterface* local)
{
    int type = current->getCollectionType();
    int index = type - m_firstCollectionType;
    m_currentData[index] = current;
    m_serverData[index] = server;
    m_localData[index] = local;
    if (current->getBackingStoreType() == BACKING_STORE_CONTENT_FILE) {
        CSaveRestoreInterface* copies[] = { current, local, server };
        int descriptorType = CProfileDataDescriptor::generateCollectionType(type);
        for (int copy = 0; copy < 3; copy++) {
            CProfileDataDescriptor* descriptor = new CProfileDataDescriptor();
            descriptor->setCollectionType(descriptorType);
            descriptor->setFile(copies[copy]);
            copies[copy]->setDescriptor(descriptor);
            m_registeredData.Add(descriptor);
        }
    }
}

int CProfileManager::getBaseCollection()
{
    return m_firstCollectionType;
}

void CProfileManager::getAllRegisteredCollections(TCVector<int>& collectionTypes)
{
    for (int collectionType = m_firstCollectionType; collectionType <= m_lastCollectionType; collectionType++) {
        collectionTypes.Add(collectionType);
    }
}

CSaveRestoreInterface* CProfileManager::getGameData(int collectionType, GameDataSource source)
{
    // Descriptor types live below the registered range and map onto their file collection.
    int index = collectionType;
    if (collectionType < m_firstCollectionType) {
        index = CProfileDataDescriptor::targetedCollectionType(collectionType);
    }
    CSaveRestoreInterface* data = NULL;
    switch (source) {
    case GAME_DATA_LOCAL:
        data = m_localData[index - m_firstCollectionType];
        break;
    case GAME_DATA_SERVER:
        data = m_serverData[index - m_firstCollectionType];
        break;
    case GAME_DATA_CURRENT:
        data = m_currentData[index - m_firstCollectionType];
        break;
    }
    if (collectionType < m_firstCollectionType) {
        data = data->getDescriptor();
    }
    return data;
}

void CProfileManager::loadSaveStart()
{
    CNGSContentManager::GetInstance()->Lock();
}

bool CProfileManager::dataExistsForUser(int userId)
{
    CStrWChar subDirectory = CNGSUserCredentials::getSubDirectoryPath(userId);
    CStrWChar directory = CNGSUserCredentials::getFullDirectoryPathForSubdir(subDirectory);
    CNGSUtil::DebugLog("checking for existance of %s", CNGSUtil::WStrToCStr(directory).GetCharPtr());
    bool exists = false;
    if (CFileUtil_gServe::ExistsDir(directory.GetWCharPtr()) && contentExistsWithinDir(directory.GetWCharPtr())) {
        exists = true;
    }
    return exists;
}

bool CProfileManager::anonymousLocalDataPromote()
{
    TCVector<int> types;
    for (int type = m_firstCollectionType; type <= m_lastCollectionType; type++) {
        if (loadFromFileSystem(type, -1)) {
            types.Add(type);
        }
    }
    if (types.Size() == 0) {
        return false;
    }
    useDataFromLocal(types);
    // Keep the anonymous files available if account migration is interrupted.
    return invalidateLocalData(types, false) != 0;
}

bool CProfileManager::anonymousLocalDataDelete()
{
    CStrWChar subDirectory = CNGSUserCredentials::getSubDirectoryPath(-1);
    CStrWChar directory = CNGSUserCredentials::getFullDirectoryPathForSubdir(subDirectory);
    CNGSUtil::DebugLog("deleting all files from %s", CNGSUtil::WStrToCStr(directory).GetCharPtr());
    unsigned char removed = CNGSFileMgr::RmDir(directory.GetWCharPtr());
    return removed != 0;
}

bool CProfileManager::checkAnonymousLocalData()
{
    CNGSUserCredentials& credentials = CNGS::GetInstance()->GetLocalUser()->m_credentials;
    if (!credentials.isValid() || credentials.m_userId == -1 || dataExistsForUser(credentials.m_userId)) {
        return false;
    }
    return dataExistsForUser(-1) && anonymousLocalDataPromote();
}

int CProfileManager::load(const TCVector<int>& collectionTypes)
{
    if (!isAvailable()) {
        return 0;
    }
    checkAnonymousLocalData();
    CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
    const bool online = user->isValid() && user->m_credentials.hasValidAvatar();
    loadSaveStart();
    TCVector<int> missing;
    TCVector<int> invalidated;
    m_savingCollections.Copy(collectionTypes);
    m_pendingCollections.Clear();
    for (int index = 0; index < collectionTypes.Size(); index++) {
        int type = collectionTypes[index];
        CSaveRestoreInterface* current = getGameData(type, GAME_DATA_CURRENT);
        CSaveRestoreInterface* local = getGameData(type, GAME_DATA_LOCAL);
        if (current == NULL || local == NULL) {
            continue;
        }
        if (!loadFromFileSystem(type, user->m_credentials.m_userId) ||
            local->getDataSaveStatus() == PROFILE_DATA_NOT_LOADED) {
            if (online) {
                missing.Add(type);
            } else {
                current->setDataSaveStatus(PROFILE_DATA_SAVED_LOCAL);
            }
        } else if (local->getDataSaveStatus() == PROFILE_DATA_INVALIDATED && online) {
            invalidated.Add(type);
        } else {
            current->copyAllData(*local);
            if (current->getDescriptor() != NULL && local->getDescriptor() != NULL) {
                current->getDescriptor()->copyAllData(*local->getDescriptor());
            }
        }
    }
    m_pendingCollections.AddAll(invalidated);
    m_pendingCollections.AddAll(missing);
    int result = 1;
    if (missing.Size() != 0) {
        result = loadFromServer(missing);
    }
    if (invalidated.Size() != 0) {
        resolveLocalServerDataConflicts(invalidated);
        return 1;
    }
    if (missing.Size() == 0) {
        loadSaveEnd();
        if (user->m_notificationHandler != NULL) {
            user->m_notificationHandler->HandleProfileLoaded(1, m_savingCollections);
        }
    }
    return result;
}

void CProfileManager::ReplaceFileWithDescriptor(TCVector<int>& collectionTypes)
{
    for (int index = 0; index < collectionTypes.Size(); index++) {
        CSaveRestoreInterface* data = getGameData(collectionTypes[index], GAME_DATA_CURRENT);
        if (data->getBackingStoreType() == 2) {
            CSaveRestoreInterface* descriptor = data->getDescriptor();
            if (descriptor != NULL) {
                collectionTypes.RemoveAt(index);
                index--;
                collectionTypes.Add(descriptor->getCollectionType());
            }
        }
    }
}

void CProfileManager::ReplaceDescriptorWithFile(TCVector<int>& collectionTypes)
{
    for (int index = 0; index < collectionTypes.Size(); index++) {
        CSaveRestoreInterface* data = getGameData(collectionTypes[index], GAME_DATA_CURRENT);
        if (data->getBackingStoreType() == 1) {
            CSaveRestoreInterface* file = data->getFile();
            if (file != NULL) {
                collectionTypes.RemoveAt(index);
                index--;
                collectionTypes.Add(file->getCollectionType());
            }
        }
    }
}

void CProfileManager::resetDataBuffer(const TCVector<int>& collectionTypes, ProfileDataSaveStatus status)
{
    TCVector<CSaveRestoreInterface*>* buffer;
    if (status == PROFILE_DATA_SYNCED) {
        buffer = &m_serverData;
    } else if (status == PROFILE_DATA_SAVED_LOCAL) {
        buffer = &m_localData;
    } else {
        buffer = &m_currentData;
    }
    for (int index = 0; index < collectionTypes.Size(); index++) {
        // a descriptor type resets the collection it describes and the descriptor itself
        int collectionType = collectionTypes[index];
        if (collectionType < m_firstCollectionType) {
            collectionType = CProfileDataDescriptor::targetedCollectionType(collectionType);
        }
        CSaveRestoreInterface* data = (*buffer)[collectionType - m_firstCollectionType];
        if (collectionTypes[index] < m_firstCollectionType) {
            data->resetAllData();
            data = data->getDescriptor();
        }
        if (data != NULL) {
            data->resetAllData();
        }
    }
}

int CProfileManager::loadFromServer(const TCVector<int>& collectionTypes)
{
    CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
    TCVector<int> query(collectionTypes);
    ReplaceFileWithDescriptor(query);
    if (!isReady() || m_writeRequestsOutstanding != 0) {
        m_pendingCollections.RemoveAll(collectionTypes);
        if (m_pendingCollections.Size() == 0) {
            loadSaveEnd();
            user->m_notificationHandler->HandleProfileLoaded(0, m_savingCollections);
        }
        return 0;
    }
    resetDataBuffer(query, PROFILE_DATA_SYNCED);
    AddReadRequestOutstanding();
    CAttributeManager::GetInstance()->getDataFromServer(query,
        new ProfileManagerFunctor(this, &CProfileManager::handleResponseLoadFromServer, query), &user->m_credentials);
    return 1;
}

void CProfileManager::resolveLocalServerDataConflicts(const TCVector<int>& collectionTypes)
{
    CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
    TCVector<int> query(collectionTypes);
    ReplaceFileWithDescriptor(query);
    AddReadRequestOutstanding();
    CAttributeManager::GetInstance()->getDataFromServer(query,
        new ProfileManagerFunctor(this, &CProfileManager::handleResponseResolveLocalServerDataConflicts, query),
        &user->m_credentials);
}

bool CProfileManager::downloadFileBackedData(const TCVector<int>& collectionTypes, CSaveRestoreInterface* collection)
{
    if (collectionTypes.Size() == 0) {
        return false;
    }
    CStrChar name = CNGSUtil::WStrToCStr(collection->getFilename());
    CProfileDataDescriptor* descriptor = static_cast<CProfileDataDescriptor*>(collection->getDescriptor());
    if (descriptor != NULL) {
        char extension[] = { '.', (char)descriptor->getAB(), 0 };
        name += extension;
    }
    CNGSContentManager* manager = CNGSContentManager::GetInstance();
    manager->Unlock();
    if (manager->getContentManagerStatus() != CONTENT_MANAGER_READY) {
        return false;
    }
    manager->GetProfileContentSelf(name.GetCharPtr(), collection);
    return true;
}

unsigned char CProfileManager::loadFromFileSystem(int collectionType, int userId)
{
    CSaveRestoreInterface* data = getGameData(collectionType, GAME_DATA_LOCAL);
    if (data != NULL) {
        data->resetAllData();
    }
    unsigned int size = CFileUtil_gServe::GetFilesize(CNGSUserCredentials::getSubDirectoryPath(userId).GetWCharPtr(),
                                                      data->getFilename().GetWCharPtr());
    if (size == 0) {
        return 0;
    }
    unsigned char loaded = 0;
    unsigned char* buffer = new unsigned char[size];
    memset(buffer, 0, size);
    unsigned char read = CFileUtil_gServe::ReadApplicationDataFile(data->getFilename().GetWCharPtr(), buffer, size,
                                                                   CNGSUserCredentials::getSubDirectoryPath(userId).GetWCharPtr());
    if (read && !data->isValidJsonData(buffer, size)) {
        // Let the game convert an old file format.
        unsigned char* convertedData = NULL;
        size = 0;
        CNotificationHandler* handler = CNGS::GetInstance()->GetLocalUser()->m_notificationHandler;
        if (!handler->HandleProfileDataFormatIncorrect(data->getFilename(), CNGSUserCredentials::getSubDirectoryPath(userId),
                                                       convertedData, size)) {
            read = 0;
        } else {
            if (buffer != NULL) {
                delete[] buffer;
            }
            buffer = convertedData;
        }
    }
    if (read) {
        if (ReadRawFileDataToAttributes(buffer, size, data)) {
            loaded = 1;
            if (data->getBackingStoreType() == BACKING_STORE_CONTENT_FILE) {
                CSaveRestoreInterface* descriptor = data->getDescriptor();
                if (descriptor != NULL) {
                    size = CFileUtil_gServe::GetFilesize(CNGSUserCredentials::getSubDirectoryPath(userId).GetWCharPtr(),
                                                         descriptor->getFilename().GetWCharPtr());
                    if (size == 0) {
                        loaded = 0;
                        descriptor->resetAllData();
                    } else {
                        if (buffer != NULL) {
                            delete[] buffer;
                        }
                        buffer = new unsigned char[size];
                        loaded = 0;
                        memset(buffer, 0, size);
                        unsigned char descriptorRead =
                            CFileUtil_gServe::ReadApplicationDataFile(descriptor->getFilename().GetWCharPtr(), buffer, size,
                                                                      CNGSUserCredentials::getSubDirectoryPath(userId).GetWCharPtr());
                        descriptor->resetAllData();
                        if (descriptorRead && ReadRawFileDataToAttributes(buffer, size, descriptor)) {
                            loaded = 1;
                        }
                    }
                }
            }
        }
    }
    if (buffer != NULL) {
        delete[] buffer;
    }
    return loaded;
}

void CProfileManager::copyDataBuffer(const TCVector<int>& collectionTypes, TCVector<CSaveRestoreInterface*>* destination,
                                     TCVector<CSaveRestoreInterface*>* source)
{
    for (int index = 0; index < collectionTypes.Size(); index++) {
        int slot = collectionTypes[index];
        if (collectionTypes[index] < m_firstCollectionType) {
            slot = CProfileDataDescriptor::targetedCollectionType(collectionTypes[index]);
        }
        CSaveRestoreInterface* from = (*source)[slot - m_firstCollectionType];
        CSaveRestoreInterface* to = (*destination)[slot - m_firstCollectionType];
        if (collectionTypes[index] < m_firstCollectionType) {
            // A descriptor: copy its file collection and continue with the descriptors.
            to->copyAllData(*from);
            from = from->getDescriptor();
            to = to->getDescriptor();
        }
        if (from->getDescriptor() != NULL && to->getDescriptor() != NULL) {
            to->getDescriptor()->copyAllData(*from->getDescriptor());
        }
        to->copyAllData(*from);
    }
}

void CProfileManager::loadSaveEnd()
{
    CNGSContentManager::GetInstance()->Unlock();
}

/** Delete the attributes of a CHash filled by CAttributeManager::extractAttributeValuesAsHashMap. */
static inline void DeleteAttributes(CHash& attributes)
{
    for (int bucket = 0; bucket < attributes.m_bucketCount; bucket++) {
        for (CHash::Entry* entry = attributes.m_buckets[bucket]; entry != NULL; entry = entry->m_next) {
            CNGSAttribute* attribute = (CNGSAttribute*)entry->m_value;
            if (attribute != NULL) {
                delete attribute;
            }
        }
    }
}

void CProfileManager::handleResponseLoadFromServer(CObjectMap* response, ProfileManagerFunctor* functor)
{
    ApplyProfileResponse(response, functor, false);
}

void CProfileManager::handleResponseResolveLocalServerDataConflicts(CObjectMap* response,
                                                                   ProfileManagerFunctor* functor)
{
    ApplyProfileResponse(response, functor, true);
}

void CProfileManager::ApplyProfileResponse(CObjectMap* response, ProfileManagerFunctor* functor, bool conflict)
{
    const TCVector<int>& query = functor->m_collectionTypes;
    unsigned char success = WasErrorInResponse(response, functor, "CProfileManager::ApplyProfileResponse") == 0;
    if (!success) {
        TCVector<int> files(query);
        ReplaceDescriptorWithFile(files);
        if (conflict) {
            useDataFromLocal(files);
        }
        m_pendingCollections.RemoveAll(files);
    } else {
        if (conflict) {
            resetDataBuffer(query, PROFILE_DATA_SYNCED);
        }
        CHash attributes;
        attributes.Init(20, 20, 1);
        CAttributeManager::GetInstance()->extractAttributeValuesAsHashMap(response, functor, attributes);
        for (int index = 0; index < query.Size(); index++) {
            int type = query[index];
            CSaveRestoreInterface* server = getGameData(type, GAME_DATA_SERVER);
            CSaveRestoreInterface* current = getGameData(type, GAME_DATA_CURRENT);
            server->setDataSaveStatus(PROFILE_DATA_SYNCED);
            server->setData(&attributes);
            if (server->getDataSaveStatus() == PROFILE_DATA_NOT_LOADED) {
                if (!conflict && current->getDataSaveStatus() == PROFILE_DATA_NOT_LOADED) {
                    current->setDataSaveStatus(PROFILE_DATA_SAVED_LOCAL);
                }
            } else if (!conflict) {
                current->copyAllData(*server);
                current->setDataSaveStatus(PROFILE_DATA_SYNCED);
            }

            CSaveRestoreInterface* file = server->getFile();
            if (file == NULL) {
                m_pendingCollections.Remove(type);
                continue;
            }
            int fileType = file->getCollectionType();
            if (server->getDataSaveStatus() == PROFILE_DATA_SYNCED) {
                TCVector<int> files;
                files.Add(fileType);
                if (!conflict) {
                    file = current->getFile();
                }
                if (downloadFileBackedData(files, file)) {
                    continue;
                }
            }
            m_pendingCollections.Remove(fileType);
        }
        DeleteAttributes(attributes);
    }
    CompleteReadRequestOutstanding();
    if (m_pendingCollections.Size() == 0 && isReady()) {
        FinishProfileLoad(success);
    }
}

void CProfileManager::FinishProfileLoad(unsigned char success)
{
    loadSaveEnd();
    CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
    if (user->m_notificationHandler == NULL) {
        return;
    }
    TCVector<int> conflicts;
    for (int index = 0; index < m_savingCollections.Size(); index++) {
        int type = m_savingCollections[index];
        CSaveRestoreInterface* local = getGameData(type, GAME_DATA_LOCAL);
        CSaveRestoreInterface* server = getGameData(type, GAME_DATA_SERVER);
        if (local != NULL && server != NULL && local->getDataSaveStatus() == PROFILE_DATA_INVALIDATED &&
            server->getDataSaveStatus() == PROFILE_DATA_SYNCED && !local->compareData(server)) {
            conflicts.Add(type);
        }
    }
    if (conflicts.Size() > 0) {
        user->m_notificationHandler->HandleProfileDataConflict(conflicts);
    } else {
        user->m_notificationHandler->HandleProfileLoaded(success, m_savingCollections);
    }
}

/**
 * Sort the collections into "use server data", "use local data" and real conflicts; apply the
 * first two and hand the conflicts to the notification handler (or keep them while requests are pending).
 */
void CProfileManager::CNGSHandleProfileDataConflict(const TCVector<int>& collectionTypes)
{
    CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
    TCVector<int> conflicts;
    TCVector<int> useServer;
    TCVector<int> useLocal;
    TCVector<int> fileBacked;
    for (int index = 0; index < collectionTypes.Size(); index++) {
        CSaveRestoreInterface* local = getGameData(collectionTypes[index], GAME_DATA_LOCAL);
        CSaveRestoreInterface* server = getGameData(collectionTypes[index], GAME_DATA_SERVER);
        CSaveRestoreInterface* file = server->getFile();
        ProfileDataSaveStatus status = server->getDataSaveStatus();
        if (file != NULL) {
            // Content files: no comparison here.
            if (status == PROFILE_DATA_NOT_LOADED) {
                useLocal.Add(collectionTypes[index]);
            } else {
                fileBacked.Add(collectionTypes[index]);
            }
        } else {
            if (status == PROFILE_DATA_NOT_LOADED) {
                useLocal.Add(collectionTypes[index]);
                CNGSUtil::DebugLog("Data Conflict: FALSE (no server data exists so conflict not possible). Use local data");
            } else if (local->compareAllData(*server)) {
                useServer.Add(collectionTypes[index]);
                CNGSUtil::DebugLog("Data Conflict: FALSE (local data and server data match). Use server data");
            } else {
                conflicts.Add(collectionTypes[index]);
                CNGSUtil::DebugLog("Data Conflict: TRUE (local data and server data do not match). User must resolve");
            }
        }
        local->debugPrint();
        server->debugPrint();
    }
    // Content files follow the attribute collections: conflicts when there are any, local otherwise.
    if (fileBacked.Size() > 0) {
        if (conflicts.Size() > 0) {
            conflicts.AddAll(fileBacked);
        } else {
            useLocal.AddAll(fileBacked);
        }
    }
    if (useServer.Size() > 0) {
        useDataFromServer(useServer);
    }
    if (useLocal.Size() > 0) {
        useDataFromLocal(useLocal);
        save(useLocal, false);
    }
    if (m_pendingCollections.Size() == 0) {
        loadSaveEnd();
        user->m_notificationHandler->HandleProfileDataConflict(conflicts);
    } else {
        m_savingCollections.Copy(conflicts);
    }
}

unsigned char CProfileManager::ReadRawFileDataToAttributes(const unsigned char* data, unsigned int size,
                                                           CSaveRestoreInterface* collection)
{
    CAttributeManager* attributeManager = CAttributeManager::GetInstance();
    unsigned int capacity = size + 1;
    CArrayInputStream_gServe stream;
    wchar_t* text = new wchar_t[capacity];
    memset(text, 0, capacity * sizeof(wchar_t));
    unsigned char loaded = 0;

    // Plain format: the JSON as JMUtf (JMUtf32 for big files).
    if (stream.Open(data, size)) {
        int length;
        if (size < 0x10002) {
            length = stream.ReadJMUtf(text, capacity);
        } else {
            length = stream.ReadJMUtf32(text, capacity);
        }
        text[length] = 0;
        loaded = 0;
        if (length > 0) {
            CHash attributes;
            attributes.Init(10, 10, 1);
            CStrWChar json(text);
            CObjectMapObject* root = (CObjectMapObject*)CNGSJSONParser::decodeValue(json);
            attributeManager->extractAttributeValuesAsHashMap(root, attributes);
            collection->setAllData(&attributes);
            if (root != NULL) {
                delete root;
            }
            DeleteAttributes(attributes);
            loaded = 1;
        }
        stream.Close();
        if (loaded) {
            if (text != NULL) {
                delete[] text;
            }
            return loaded;
        }
    }

    // Obfuscated format: CRC, text length, key length, key + text.
    loaded = 0;
    if (stream.Open(data, size)) {
        unsigned int crc = stream.ReadUInt32();
        unsigned int textLength = stream.ReadUInt32();
        unsigned int keyLength = stream.ReadUInt32();
        loaded = 0;
        unsigned int bytes = (keyLength + textLength) * 4;
        if (bytes <= size - 12) {
            stream.Read((unsigned char*)text, bytes);
            text[bytes + 1] = 0;
            sDataBlock block;
            if (CNGSUtil::DeObfuscateString(block, text, PROFILE_FILE_KEY, textLength, keyLength) == 0) {
                char* plain = (char*)malloc(block.m_size + (1 - keyLength));
                wchar_t* plainText = (wchar_t*)block.m_data + keyLength;
                wcstombs(plain, plainText, wcslen(plainText) + 1);
                loaded = 0;
                if (CCrc32::GetInstance()->Crc32((unsigned char*)plain, strlen(plain)) == crc) {
                    CStrWChar json(plain);
                    CObjectMapObject* root = (CObjectMapObject*)CNGSJSONParser::decodeValue(json);
                    CHash attributes;
                    attributes.Init(10, 10, 1);
                    attributeManager->extractAttributeValuesAsHashMap(root, attributes);
                    collection->setAllData(&attributes);
                    if (root != NULL) {
                        delete root;
                    }
                    DeleteAttributes(attributes);
                    loaded = 1;
                }
                free(block.m_data);
                free(plain);
            }
        }
    }
    stream.Close();
    if (text != NULL) {
        delete[] text;
    }
    return loaded;
}

void CProfileManager::useDataFromLocal(const TCVector<int>& collectionTypes)
{
    copyDataBuffer(collectionTypes, &m_currentData, &m_localData);
    for (int index = 0; index < collectionTypes.Size(); index++) {
        CSaveRestoreInterface* data = getGameData(collectionTypes[index], GAME_DATA_CURRENT);
        data->setDataSaveStatus(PROFILE_DATA_SAVED_LOCAL);
        if (data->getFile() != NULL) {
            data->getFile()->setDataSaveStatus(PROFILE_DATA_SAVED_LOCAL);
        }
    }
}

unsigned char CProfileManager::invalidateLocalData(const TCVector<int>& collectionTypes, unsigned char reset)
{
    CNGSLocalUser* localUser = CNGS::GetInstance()->GetLocalUser();
    unsigned char success = 1;
    for (int index = 0; index < collectionTypes.Size(); index++) {
        CSaveRestoreInterface* data = getGameData(collectionTypes[index], GAME_DATA_CURRENT);
        if (localUser->m_credentials.m_userId != -1) {
            if (data->getDataSaveStatus() == PROFILE_DATA_NOT_LOADED
                && loadFromFileSystem(collectionTypes[index], localUser->m_credentials.m_userId)) {
                data->copyAllData(*getGameData(collectionTypes[index], GAME_DATA_LOCAL));
            }
            data->setDataSaveStatus(PROFILE_DATA_INVALIDATED);
            if (!saveToFileSystem(data, localUser->m_credentials.getSubDirectoryPath())) {
                success = 0;
            }
        } else {
            success = 0;
        }
        if (reset && data != NULL) {
            data->resetAllData();
            if (data->getBackingStoreType() == BACKING_STORE_CONTENT_FILE) {
                CSaveRestoreInterface* descriptor = data->getDescriptor();
                if (descriptor != NULL) {
                    descriptor->resetAllData();
                }
            }
        }
    }
    return success;
}

void CProfileManager::useDataFromServer(const TCVector<int>& collectionTypes)
{
    CNGSLocalUser* localUser = CNGS::GetInstance()->GetLocalUser();
    copyDataBuffer(collectionTypes, &m_currentData, &m_serverData);
    for (int index = 0; index < collectionTypes.Size(); index++) {
        CSaveRestoreInterface* data = getGameData(collectionTypes[index], GAME_DATA_CURRENT);
        data->setDataSaveStatus(PROFILE_DATA_SYNCED);
        saveToFileSystem(data, localUser->m_credentials.getSubDirectoryPath());
        if (data->getFile() != NULL) {
            data->getFile()->setDataSaveStatus(PROFILE_DATA_SYNCED);
            saveToFileSystem(data->getFile(), localUser->m_credentials.getSubDirectoryPath());
        }
    }
}

bool CProfileManager::save(const TCVector<int>& collectionTypes, bool syncToServer)
{
    CNGSLocalUser* localUser = CNGS::GetInstance()->GetLocalUser();
    bool saved = false;
    if (isAvailable()) {
        // Nothing is saved while an attribute collection was never loaded.
        for (int index = 0; index < collectionTypes.Size(); index++) {
            CSaveRestoreInterface* data = getGameData(collectionTypes[index], GAME_DATA_CURRENT);
            if (data->getBackingStoreType() != BACKING_STORE_CONTENT_FILE && data->getDataSaveStatus() == PROFILE_DATA_NOT_LOADED) {
                return false;
            }
        }
        loadSaveStart();
        saved = true;
        for (int index = 0; index < collectionTypes.Size(); index++) {
            CSaveRestoreInterface* data = getGameData(collectionTypes[index], GAME_DATA_CURRENT);
            switch (data->getDataSaveStatus()) {
            case PROFILE_DATA_NOT_LOADED:
                data->setDataSaveStatus(PROFILE_DATA_SAVED_LOCAL);
                // fall through
            case PROFILE_DATA_SAVED_LOCAL:
                if (!saveToFileSystem(data, localUser->m_credentials.getSubDirectoryPath())) {
                    saved = false;
                }
                break;
            case PROFILE_DATA_SYNCED:
                data->setDataSaveStatus(PROFILE_DATA_SAVED_LOCAL);
                if (!saveToFileSystem(data, localUser->m_credentials.getSubDirectoryPath())) {
                    saved = false;
                }
                break;
            case PROFILE_DATA_INVALIDATED:
                if (!saveToFileSystem(data, localUser->m_credentials.getSubDirectoryPath())) {
                    saved = false;
                }
                break;
            default:
                saved = false;
                break;
            }
        }
        if (saved && syncToServer && localUser->isValid()) {
            m_pendingCollections.Clear();
            m_savingCollections = collectionTypes;
            syncWithServer(collectionTypes);
        } else {
            loadSaveEnd();
            if (saved) {
                localUser->m_notificationHandler->HandleProfileSaved(saved, collectionTypes);
            }
        }
    }
    return saved;
}

bool CProfileManager::uploadFileBackedData(const TCVector<int>& collectionTypes)
{
    CNGSLocalUser* localUser = CNGS::GetInstance()->GetLocalUser();
    CNGSContentManager* contentManager = CNGSContentManager::GetInstance();
    bool uploaded = false;
    for (int index = 0; index < collectionTypes.Size(); index++) {
        CSaveRestoreInterface* data = getGameData(collectionTypes[index], GAME_DATA_CURRENT);
        char fileName[128];
        {
            CStrWChar wideName = data->getFilename();
            wcstombs(fileName, wideName.GetWCharPtr(), wcslen(wideName.GetWCharPtr()) + 1);
        }
        // Upload to the copy that is not current: "<file>.A" / "<file>.B".
        CProfileDataDescriptor* descriptor = (CProfileDataDescriptor*)data->getDescriptor();
        if (descriptor != NULL) {
            descriptor->switchAB();
            char suffix[3];
            suffix[0] = '.';
            suffix[1] = (char)descriptor->getAB();
            suffix[2] = 0;
            descriptor->switchAB();
            strcat(fileName, suffix);
        }
        contentManager->Unlock();
        if (contentManager->getContentManagerStatus() == CONTENT_MANAGER_READY) {
            unsigned int size = CFileUtil_gServe::GetFilesize(
                CNGSUserCredentials::getSubDirectoryPath(localUser->m_credentials.m_userId).GetWCharPtr(),
                data->getFilename().GetWCharPtr());
            unsigned char* buffer = new unsigned char[size];
            memset(buffer, 0, size);
            uploaded = CFileUtil_gServe::ReadApplicationDataFile(
                           data->getFilename().GetWCharPtr(), buffer, size,
                           CNGSUserCredentials::getSubDirectoryPath(localUser->m_credentials.m_userId).GetWCharPtr()) != 0;
            if (uploaded) {
                contentManager->UploadProfileContent(fileName, buffer, size, data);
            }
            if (buffer != NULL) {
                delete[] buffer;
            }
        }
    }
    return uploaded;
}

bool CProfileManager::syncWithServer(const TCVector<int>& collectionTypes)
{
    CNGSLocalUser* localUser = CNGS::GetInstance()->GetLocalUser();
    CAttributeManager* attributeManager = CAttributeManager::GetInstance();
    TCVector<CNGSAttribute*> attributes;
    TCVector<int> collections(collectionTypes);
    if (!isLocalDataValid(collections)) {
        loadSaveEnd();
        localUser->m_notificationHandler->HandleProfileSaved(0, collectionTypes);
    } else {
        m_fileBackedCollections.Clear();
        bool uploading = false;
        bool hasContentFiles = false;
        for (int index = 0; index < collections.Size(); index++) {
            CSaveRestoreInterface* data = getGameData(collections[index], GAME_DATA_CURRENT);
            if (data->getBackingStoreType() == BACKING_STORE_ATTRIBUTES) {
                m_fileBackedCollections.Add(collections[index]);
            } else if (data->getBackingStoreType() == BACKING_STORE_CONTENT_FILE) {
                hasContentFiles = true;
                if (localUser->m_session->isSessionKeyValid() == 1) {
                    uploading = true;
                    TCVector<int> single;
                    single.Add(collections[index]);
                    uploadFileBackedData(single);
                    m_pendingCollections.Add(collections[index]);
                }
            }
        }
        if (!uploading) {
            if (hasContentFiles) {
                loadSaveEnd();
                localUser->m_notificationHandler->HandleProfileSaved(0, collectionTypes);
            } else {
                for (int index = 0; index < collections.Size(); index++) {
                    CSaveRestoreInterface* data = getGameData(collections[index], GAME_DATA_CURRENT);
                    if (data->getBackingStoreType() == BACKING_STORE_ATTRIBUTES) {
                        data->getAllData(attributes);
                        m_pendingCollections.Add(collections[index]);
                    }
                }
            }
        }
        if (attributes.Size() < 1) {
            return uploading;
        }
        AddWriteRequestOutstanding();
        attributeManager->saveDataToServer(
            attributes, new ProfileManagerFunctor(this, &CProfileManager::handleResponseSyncWithServer, collections),
            &localUser->m_credentials, 0);
    }
    return true;
}

unsigned char CProfileManager::isLocalDataValid(const TCVector<int>& collectionTypes)
{
    for (int index = 0; index < collectionTypes.Size(); index++) {
        CSaveRestoreInterface* data = getGameData(collectionTypes[index], GAME_DATA_CURRENT);
        if (data != NULL && data->getBackingStoreType() != BACKING_STORE_CONTENT_FILE
            && data->getDataSaveStatus() == PROFILE_DATA_INVALIDATED) {
            return 0;
        }
    }
    return 1;
}

void CProfileManager::handleResponseSyncWithServer(CObjectMap* response, ProfileManagerFunctor* functor)
{
    CNGSLocalUser* localUser = CNGS::GetInstance()->GetLocalUser();
    TCVector<int> failed;
    if (CNGSServerObject::WasErrorInResponse(response, functor, "CProfileManager::handleResponseSyncWithServer") == 0) {
        for (int index = 0; index < functor->m_collectionTypes.Size(); index++) {
            CSaveRestoreInterface* data = getGameData(functor->m_collectionTypes[index], GAME_DATA_CURRENT);
            if (data->getBackingStoreType() == BACKING_STORE_ATTRIBUTES) {
                data->setDataSaveStatus(PROFILE_DATA_SYNCED);
                if (!saveToFileSystem(data, localUser->m_credentials.getSubDirectoryPath())) {
                    failed.Add(functor->m_collectionTypes[index]);
                }
                m_pendingCollections.Remove(functor->m_collectionTypes[index]);
            }
        }
        CompleteWriteRequestOutstanding();
        if (m_pendingCollections.Size() != 0) {
            return;
        }
    } else {
        CompleteWriteRequestOutstanding();
    }
    loadSaveEnd();
    localUser->m_notificationHandler->HandleProfileSaved(0, m_savingCollections);
}

unsigned char CProfileManager::saveToFileSystem(CSaveRestoreInterface* collection, const CStrWChar& subDirectory)
{
    unsigned char saved = 0;
    unsigned char* data = NULL;
    unsigned int size = 0;
    if (WriteAttributesToRawFileData(collection, data, size) && data != NULL) {
        saved = 0;
        if (size != 0) {
            CStrWChar directory = CNGSUserCredentials::getFullDirectoryPathForSubdir(subDirectory);
            saved = 0;
            if (CNGSFileMgr::MkDir(directory.GetWCharPtr())) {
                saved = CFileUtil_gServe::WriteApplicationDataFile(collection->getFilename().GetWCharPtr(), data, size,
                                                                   subDirectory.GetWCharPtr());
            }
        }
        if (data != NULL) {
            delete[] data;
            data = NULL;
        }
    }
    // A content file collection also writes its attribute backed descriptor.
    if (collection->getBackingStoreType() == BACKING_STORE_CONTENT_FILE) {
        CSaveRestoreInterface* descriptor = collection->getDescriptor();
        if (descriptor != NULL && descriptor->getBackingStoreType() == BACKING_STORE_ATTRIBUTES) {
            descriptor->setDataSaveStatus(collection->getDataSaveStatus());
            saveToFileSystem(descriptor, subDirectory);
        }
    }
    return saved;
}

unsigned char CProfileManager::WriteAttributesToRawFileData(CSaveRestoreInterface* collection, unsigned char*& data,
                                                            unsigned int& size)
{
    data = NULL;
    size = 0;
    TCVector<CNGSAttribute*> attributes;
    collection->getAllData(attributes);
    CObjectMapValue* message = CAttributeManager::GetInstance()->createAttributeMessageObject(attributes);
    CStrWChar json = CNGSJSONParser::encodeValue(message);
    if (message != NULL) {
        delete message;
    }

    unsigned char written = 0;
    unsigned int fileSize = 0;
    bool sized = false;
    sDataBlock block;
    if (!m_obfuscateFiles) {
        unsigned int utfLength = CUtf::GetUtfLen(json.GetWCharPtr(), json.GetLength());
        if (utfLength != 0) {
            // JMUtf: 16-bit length prefix, JMUtf32 above 64 KB.
            fileSize = utfLength + 3;
            if (fileSize > 0xffff) {
                fileSize = utfLength + 5;
            }
            sized = true;
        }
    } else {
        if (CNGSUtil::ObfuscateString(block, json.GetWCharPtr(), PROFILE_FILE_KEY) == 0) {
            fileSize = block.m_size + 4;
            sized = true;
        }
    }
    if (sized && fileSize != 0) {
        unsigned char* buffer = new unsigned char[fileSize];
        memset(buffer, 0, fileSize);
        CArrayOutputStream_gServe stream;
        if (stream.Open(buffer, fileSize)) {
            if (!m_obfuscateFiles) {
                if (fileSize >= 0x10000) {
                    stream.WriteJMUtf32(json.GetWCharPtr());
                } else {
                    stream.WriteJMUtf(json.GetWCharPtr());
                }
            } else {
                // CRC of the plain JSON, then the obfuscated block.
                char plain[json.GetLength() + 1];
                wcstombs(plain, json.GetWCharPtr(), wcslen(json.GetWCharPtr()) + 1);
                unsigned int crc = CCrc32::GetInstance()->Crc32((unsigned char*)plain, json.GetLength());
                stream.WriteUInt32(crc);
                stream.Write((unsigned char*)block.m_data, block.m_size);
                free(block.m_data);
            }
            stream.Close();
            data = buffer;
            size = fileSize;
            return 1;
        }
        if (buffer != NULL) {
            delete[] buffer;
        }
        return 0;
    }
    return written;
}

CSaveRestoreInterface* CProfileManager::getManagedObjectFromBuffer(ProfileDataSaveStatus status, int collectionType)
{
    if (status == PROFILE_DATA_SAVED_LOCAL) {
        return getGameData(collectionType, GAME_DATA_LOCAL);
    }
    return getGameData(collectionType, GAME_DATA_SERVER);
}

unsigned char CProfileManager::contentExistsWithinDir(const wchar_t* directory)
{
    int found = 0;
    CStrWChar path(directory);
    path += ICFileMgr::GetInstance()->GetPathSeparator();
    for (int index = 0; index < m_currentData.Size(); index++) {
        CStrWChar fileName = m_currentData[index]->getFilename();
        fileName = path + fileName;
        unsigned char exists = ICFileMgr::GetInstance()->Exists(fileName.GetWCharPtr());
        if (exists) {
            found = 1;
            break;
        }
    }
    return found;
}

unsigned char CProfileManager::HandleContentFileUploaded(unsigned char success, CSaveRestoreInterface* collection)
{
    CNGSLocalUser* localUser = CNGS::GetInstance()->GetLocalUser();
    CNGSContentManager* contentManager = CNGSContentManager::GetInstance();
    if (collection != NULL) {
        CSaveRestoreInterface* descriptor = collection->getDescriptor();
        // The uploaded name without its ".A" / ".B" suffix.
        CStrWChar uploadedName(contentManager->m_fileName.GetWCharPtr());
        uploadedName = uploadedName.GetSubString(0, uploadedName.GetLength() - 3);
        bool sameFile = collection->getFilename() == uploadedName;
        if (success && sameFile) {
            collection->setDataSaveStatus(PROFILE_DATA_SYNCED);
            if (saveToFileSystem(collection, localUser->m_credentials.getSubDirectoryPath()) && descriptor != NULL) {
                TCVector<int> collectionTypes;
                int descriptorType = descriptor->getCollectionType();
                collectionTypes.Add(descriptorType);
                ((CProfileDataDescriptor*)descriptor)->switchAB();
                if (isLocalDataValid(collectionTypes)) {
                    TCVector<CNGSAttribute*> attributes;
                    CAttributeManager* attributeManager = CAttributeManager::GetInstance();
                    for (int index = 0; index < m_fileBackedCollections.Size(); index++) {
                        CSaveRestoreInterface* data = getGameData(m_fileBackedCollections[index], GAME_DATA_CURRENT);
                        if (data->getBackingStoreType() == BACKING_STORE_ATTRIBUTES) {
                            data->getAllData(attributes);
                            m_pendingCollections.Add(m_fileBackedCollections[index]);
                            collectionTypes.Add(m_fileBackedCollections[index]);
                        }
                    }
                    m_fileBackedCollections.Clear();
                    descriptor->getAllData(attributes);
                    int pendingType = descriptor->getCollectionType();
                    m_pendingCollections.Add(pendingType);
                    if (attributes.Size() > 0) {
                        AddWriteRequestOutstanding();
                        attributeManager->saveDataToServer(
                            attributes,
                            new ProfileManagerFunctor(this, &CProfileManager::handleResponseSyncDataDescWithServer, collectionTypes),
                            &localUser->m_credentials, 0);
                    }
                }
                ((CProfileDataDescriptor*)descriptor)->switchAB();
            }
        }
        int collectionType = collection->getCollectionType();
        m_pendingCollections.Remove(collectionType);
        if (m_pendingCollections.Size() == 0) {
            loadSaveEnd();
            localUser->m_notificationHandler->HandleProfileSaved(success, m_savingCollections);
        }
    }
    return 1;
}

void CProfileManager::handleResponseSyncDataDescWithServer(CObjectMap* response, ProfileManagerFunctor* functor)
{
    unsigned char success = 0;
    CNGSLocalUser* localUser = CNGS::GetInstance()->GetLocalUser();
    TCVector<int> failed;
    if (CNGSServerObject::WasErrorInResponse(response, functor, "CProfileManager::handleResponseSyncDataDescWithServer") == 0) {
        success = 1;
        for (int index = 0; index < functor->m_collectionTypes.Size(); index++) {
            CSaveRestoreInterface* data = getGameData(functor->m_collectionTypes[index], GAME_DATA_CURRENT);
            if (data->getFile() != NULL) {
                ((CProfileDataDescriptor*)data)->switchAB();
            }
            data->setDataSaveStatus(PROFILE_DATA_SYNCED);
            if (!saveToFileSystem(data, localUser->m_credentials.getSubDirectoryPath())) {
                failed.Add(functor->m_collectionTypes[index]);
            }
        }
    }
    m_pendingCollections.RemoveAll(functor->m_collectionTypes);
    if (m_pendingCollections.Size() == 0) {
        loadSaveEnd();
        localUser->m_notificationHandler->HandleProfileSaved(success, m_savingCollections);
    }
    CompleteWriteRequestOutstanding();
}

unsigned char CProfileManager::HandleContentFileDownloadedSelf(unsigned char success, const unsigned char* data, int size,
                                                               CSaveRestoreInterface* collection)
{
    CNGSLocalUser* localUser = CNGS::GetInstance()->GetLocalUser();
    CNGSContentManager* contentManager = CNGSContentManager::GetInstance();
    if (collection == NULL) {
        return 1;
    }
    CSaveRestoreInterface* serverData = getGameData(collection->getCollectionType(), GAME_DATA_SERVER);
    CSaveRestoreInterface* currentData = getGameData(collection->getCollectionType(), GAME_DATA_CURRENT);
    // The downloaded name without its ".A" / ".B" suffix.
    CStrWChar downloadedName(contentManager->m_fileName.GetWCharPtr());
    downloadedName = downloadedName.GetSubString(0, downloadedName.GetLength() - 3);
    if (downloadedName == collection->getFilename()) {
        if (success) {
            if (collection->isValidJsonData(data, size)) {
                serverData->setDataSaveStatus(PROFILE_DATA_SYNCED);
                if (ReadRawFileDataToAttributes(data, size, serverData)) {
                    if (currentData == collection) {
                        currentData->copyAllData(*serverData);
                        currentData->setDataSaveStatus(PROFILE_DATA_SYNCED);
                        saveToFileSystem(currentData, localUser->m_credentials.getSubDirectoryPath());
                    }
                } else {
                    serverData->setDataSaveStatus(PROFILE_DATA_SAVED_LOCAL);
                    success = 0;
                }
            } else {
                success = 0;
                CSaveRestoreInterface* localData = getGameData(collection->getCollectionType(), GAME_DATA_LOCAL);
                if (localData->getDataSaveStatus() == PROFILE_DATA_INVALIDATED && serverData == collection) {
                    // The server copy is unusable: push the local data instead.
                    TCVector<int> collectionTypes;
                    getAllRegisteredCollections(collectionTypes);
                    useDataFromLocal(collectionTypes);
                    save(collectionTypes, false);
                }
            }
        } else {
            serverData->setDataSaveStatus(PROFILE_DATA_SAVED_LOCAL);
            success = 0;
        }
    }
    int collectionType = collection->getCollectionType();
    m_pendingCollections.Remove(collectionType);
    if (m_pendingCollections.Size() == 0) {
        loadSaveEnd();
        if (currentData == collection || !success) {
            localUser->m_notificationHandler->HandleProfileLoaded(success, m_savingCollections);
        } else if (serverData == collection) {
            localUser->m_notificationHandler->HandleProfileDataConflict(m_savingCollections);
        }
    }
    return 1;
}

unsigned char CProfileManager::isAvailable()
{
    if (!isReady() || m_writeRequestsOutstanding != 0) {
        return 0;
    }
    if (m_registeredData.Size() < 1) {
        return 1;
    }
    return CNGSContentManager::GetInstance()->getContentManagerStatus() == CONTENT_MANAGER_READY;
}
