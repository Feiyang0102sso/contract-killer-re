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

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char ProfileManagerSizeCheck[sizeof(CProfileManager) == 0xcc ? 1 : -1];
#endif

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
    m_currentData[current->getCollectionType() - m_firstCollectionType] = current;
    m_serverData[server->getCollectionType() - m_firstCollectionType] = server;
    m_localData[local->getCollectionType() - m_firstCollectionType] = local;
    if (current->getBackingStoreType() == BACKING_STORE_CONTENT_FILE) {
        CProfileDataDescriptor* currentDescriptor = new CProfileDataDescriptor;
        CProfileDataDescriptor* serverDescriptor = new CProfileDataDescriptor;
        CProfileDataDescriptor* localDescriptor = new CProfileDataDescriptor;
        int descriptorType = CProfileDataDescriptor::generateCollectionType(current->getCollectionType());
        currentDescriptor->setCollectionType(descriptorType);
        serverDescriptor->setCollectionType(descriptorType);
        localDescriptor->setCollectionType(descriptorType);
        current->setDescriptor(currentDescriptor);
        local->setDescriptor(localDescriptor);
        server->setDescriptor(serverDescriptor);
        currentDescriptor->setFile(current);
        localDescriptor->setFile(local);
        serverDescriptor->setFile(server);
        m_registeredData.Add(currentDescriptor);
        m_registeredData.Add(localDescriptor);
        m_registeredData.Add(serverDescriptor);
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
    return CFileUtil_gServe::ExistsDir(directory.GetWCharPtr()) && contentExistsWithinDir(directory.GetWCharPtr());
}

bool CProfileManager::anonymousLocalDataPromote()
{
    TCVector<int> types;
    getAllRegisteredCollections(types);
    int count = types.Size();
    for (int index = 0; index < count; ++index) {
        loadFromFileSystem(types[index], -1);
    }
    useDataFromLocal(types);
    // CK deletes the anonymous files even if invalidation failed to write the new copy.
    invalidateLocalData(types, false);
    anonymousLocalDataDelete();
    return false;
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
    if (!credentials.isValid() || credentials.m_userId == -1 || !dataExistsForUser(-1)) {
        return false;
    }
    if (dataExistsForUser(credentials.m_userId)) {
        anonymousLocalDataDelete();
        return false;
    }
    anonymousLocalDataPromote();
    return true;
}

int CProfileManager::load(const TCVector<int>& collectionTypes)
{
    CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
    if (!isAvailable()) {
        return 0;
    }
    if (checkAnonymousLocalData()) {
        return load(collectionTypes);
    }

    loadSaveStart();
    TCVector<int> loaded;
    TCVector<int> invalidated;
    TCVector<int> savedLocally;
    TCVector<int> missing;
    for (int index = 0; index < collectionTypes.Size(); ++index) {
        CSaveRestoreInterface* local = getGameData(collectionTypes[index], GAME_DATA_LOCAL);
        CSaveRestoreInterface* current = getGameData(collectionTypes[index], GAME_DATA_CURRENT);
        if (!loadFromFileSystem(collectionTypes[index], user->m_credentials.m_userId)) {
            missing.Add(collectionTypes[index]);
            continue;
        }
        switch (local->getDataSaveStatus()) {
        case PROFILE_DATA_NOT_LOADED:
            CNGSUtil::DebugLog("Error: local datastore read with state PDSS_NO_DATA");
            missing.Add(collectionTypes[index]);
            break;
        case PROFILE_DATA_SAVED_LOCAL:
            current->copyAllData(*local);
            if (current->getDescriptor() != NULL) {
                current->getDescriptor()->copyAllData(*local->getDescriptor());
            }
            loaded.Add(collectionTypes[index]);
            savedLocally.Add(collectionTypes[index]);
            break;
        case PROFILE_DATA_SYNCED:
            current->copyAllData(*local);
            if (current->getDescriptor() != NULL) {
                current->getDescriptor()->copyAllData(*local->getDescriptor());
            }
            loaded.Add(collectionTypes[index]);
            break;
        case PROFILE_DATA_INVALIDATED:
            invalidated.Add(collectionTypes[index]);
            break;
        default:
            break;
        }
    }

    m_savingCollections.Clear();
    m_pendingCollections.Clear();
    if (loaded.Size() > 0) {
        m_savingCollections.AddAll(loaded);
    }
    if (invalidated.Size() > 0) {
        m_savingCollections.AddAll(invalidated);
        if (user->isValid()) {
            m_pendingCollections.AddAll(invalidated);
            resolveLocalServerDataConflicts(invalidated);
        } else {
            copyDataBuffer(collectionTypes, &m_currentData, &m_localData);
        }
    }

    int result = 1;
    if (missing.Size() > 0) {
        m_savingCollections.AddAll(missing);
        if (user->isValid()) {
            m_pendingCollections.AddAll(missing);
            result = loadFromServer(missing);
        } else {
            result = 0;
            for (int index = 0; index < missing.Size(); ++index) {
                getGameData(missing[index], GAME_DATA_CURRENT)->setDataSaveStatus(PROFILE_DATA_SAVED_LOCAL);
            }
        }
    }
    if (m_pendingCollections.Size() == 0) {
        loadSaveEnd();
        if (result != 0) {
            user->m_notificationHandler->HandleProfileLoaded(result, m_savingCollections);
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
    if (status == PROFILE_DATA_SAVED_LOCAL) {
        buffer = &m_localData;
    } else if (status == PROFILE_DATA_SYNCED) {
        buffer = &m_serverData;
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
    CNGSContentManager* manager = CNGSContentManager::GetInstance();
    bool ready = false;
    for (int index = 0; index < collectionTypes.Size(); index++) {
        char name[128];
        {
            CStrWChar filename = collection->getFilename();
            const wchar_t* text = filename.GetWCharPtr();
            wcstombs(name, text, wcslen(text) + 1);
        }
        CProfileDataDescriptor* descriptor = static_cast<CProfileDataDescriptor*>(collection->getDescriptor());
        if (descriptor != NULL) {
            char extension[] = { '.', (char)descriptor->getAB(), 0 };
            strcat(name, extension);
        }
        manager->Unlock();
        ready = manager->getContentManagerStatus() == CONTENT_MANAGER_READY;
        if (ready) {
            manager->GetProfileContentSelf(name, collection);
        }
    }
    return ready;
}

unsigned char CProfileManager::loadFromFileSystem(int collectionType, int userId)
{
    CSaveRestoreInterface* data = getGameData(collectionType, GAME_DATA_LOCAL);
    if (data != NULL) {
        data->resetAllData();
    }
    unsigned int size;
    {
        CStrWChar directory = CNGSUserCredentials::getSubDirectoryPath(userId);
        const wchar_t* path = directory.GetWCharPtr();
        CStrWChar filename = data->getFilename();
        size = CFileUtil_gServe::GetFilesize(path, filename.GetWCharPtr());
    }
    if (size == 0) {
        return 0;
    }
    unsigned char loaded = 0;
    unsigned char* buffer = new unsigned char[size];
    memset(buffer, 0, size);
    unsigned char read;
    {
        CStrWChar filename = data->getFilename();
        const wchar_t* name = filename.GetWCharPtr();
        unsigned int fileSize = size;
        CStrWChar directory = CNGSUserCredentials::getSubDirectoryPath(userId);
        read = CFileUtil_gServe::ReadApplicationDataFile(name, buffer, fileSize, directory.GetWCharPtr());
    }
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
                    {
                        CStrWChar directory = CNGSUserCredentials::getSubDirectoryPath(userId);
                        const wchar_t* path = directory.GetWCharPtr();
                        CStrWChar filename = descriptor->getFilename();
                        size = CFileUtil_gServe::GetFilesize(path, filename.GetWCharPtr());
                    }
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
                        unsigned char descriptorRead;
                        {
                            CStrWChar filename = descriptor->getFilename();
                            const wchar_t* name = filename.GetWCharPtr();
                            unsigned int fileSize = size;
                            CStrWChar directory = CNGSUserCredentials::getSubDirectoryPath(userId);
                            descriptorRead = CFileUtil_gServe::ReadApplicationDataFile(name, buffer, fileSize,
                                                                                      directory.GetWCharPtr());
                        }
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
    CNGSUtil::DebugLog("CProfileManager::handleResponseLoadFromServer");
    CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
    CAttributeManager* attributeManager = CAttributeManager::GetInstance();
    const TCVector<int>& collections = functor->m_collectionTypes;
    unsigned char success = 0;
    if (!WasErrorInResponse(response, functor, "CProfileManager::handleResponseLoadFromServer")) {
        CHash attributes;
        attributes.Init(20, 20, 1);
        attributeManager->extractAttributeValuesAsHashMap(response, functor, attributes);
        for (int index = 0; index < collections.Size(); ++index) {
            CSaveRestoreInterface* server = getGameData(collections[index], GAME_DATA_SERVER);
            server->setDataSaveStatus(PROFILE_DATA_SYNCED);
            server->setData(&attributes);
            if (server->getBackingStoreType() == BACKING_STORE_ATTRIBUTES) {
                if (server->getDataSaveStatus() == PROFILE_DATA_NOT_LOADED) {
                    CSaveRestoreInterface* local = getGameData(collections[index], GAME_DATA_LOCAL);
                    CSaveRestoreInterface* current = getGameData(collections[index], GAME_DATA_CURRENT);
                    if (local->getDataSaveStatus() == PROFILE_DATA_NOT_LOADED) {
                        current->resetAllData();
                        current->setDataSaveStatus(PROFILE_DATA_SAVED_LOCAL);
                        if (current->getFile() != NULL) {
                            current->getFile()->resetAllData();
                            current->getFile()->setDataSaveStatus(PROFILE_DATA_SAVED_LOCAL);
                        }
                    }
                    if (server->getFile() != NULL) {
                        int fileType = server->getFile()->getCollectionType();
                        m_pendingCollections.Remove(fileType);
                    }
                } else {
                    CSaveRestoreInterface* current = getGameData(collections[index], GAME_DATA_CURRENT);
                    if (current->getDataSaveStatus() != PROFILE_DATA_INVALIDATED) {
                        current->copyAllData(*server);
                        current->setDataSaveStatus(PROFILE_DATA_SYNCED);
                        if (current->getFile() != NULL) {
                            TCVector<int> files;
                            files.Add(current->getFile()->getCollectionType());
                            downloadFileBackedData(files, current->getFile());
                            continue;
                        }
                    }
                }
            }
            m_pendingCollections.Remove(collections[index]);
        }
        success = 1;
    } else {
        TCVector<int> files;
        files.Copy(collections);
        ReplaceDescriptorWithFile(files);
        invalidateLocalData(files, false);
        m_pendingCollections.RemoveAll(files);
    }
    if (m_pendingCollections.Size() == 0) {
        loadSaveEnd();
        user->m_notificationHandler->HandleProfileLoaded(success, m_savingCollections);
    }
    CompleteReadRequestOutstanding();
}

void CProfileManager::handleResponseResolveLocalServerDataConflicts(CObjectMap* response,
                                                                   ProfileManagerFunctor* functor)
{
    CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
    CAttributeManager* attributeManager = CAttributeManager::GetInstance();
    const TCVector<int>& collections = functor->m_collectionTypes;
    if (!WasErrorInResponse(response, functor, "CProfileManager::handleResponseResolveLocalServerDataConflicts")) {
        resetDataBuffer(collections, PROFILE_DATA_SYNCED);
        CHash attributes;
        attributes.Init(20, 20, 1);
        attributeManager->extractAttributeValuesAsHashMap(response, functor, attributes);
        bool requestedFile = false;
        for (int index = 0; index < collections.Size(); ++index) {
            CSaveRestoreInterface* server = getGameData(collections[index], GAME_DATA_SERVER);
            if (server->getBackingStoreType() == BACKING_STORE_ATTRIBUTES) {
                server->setDataSaveStatus(PROFILE_DATA_SYNCED);
                server->setData(&attributes);
                if (server->getFile() != NULL) {
                    if (server->getDataSaveStatus() != PROFILE_DATA_NOT_LOADED) {
                        TCVector<int> files;
                        files.Add(server->getFile()->getCollectionType());
                        downloadFileBackedData(files, server->getFile());
                        requestedFile = true;
                        continue;
                    }
                    int fileType = server->getFile()->getCollectionType();
                    m_pendingCollections.Remove(fileType);
                }
            }
            if (!requestedFile) {
                m_pendingCollections.Remove(collections[index]);
            }
        }
        CNGSHandleProfileDataConflict(collections);
    } else {
        TCVector<int> files;
        files.Copy(collections);
        ReplaceDescriptorWithFile(files);
        useDataFromLocal(files);
        invalidateLocalData(files, false);
        m_pendingCollections.RemoveAll(files);
        if (m_pendingCollections.Size() == 0) {
            loadSaveEnd();
            user->m_notificationHandler->HandleProfileLoaded(false, m_savingCollections);
        }
    }
    CompleteReadRequestOutstanding();
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
    if (stream.Open(data, size)) {
        unsigned int crc = stream.ReadUInt32();
        unsigned int textLength = stream.ReadUInt32();
        unsigned int keyLength = stream.ReadUInt32();
        loaded = 0;
        unsigned int bytes = (keyLength + textLength) * 4;
        if (bytes <= size - 12) {
            stream.Read((unsigned char*)text, bytes);
            text[bytes | 1] = 0;
            sDataBlock block;
            if (CNGSUtil::DeObfuscateString(block, text, PROFILE_FILE_KEY, textLength, keyLength) == 0) {
                char* plain = (char*)malloc(block.m_size + (1 - keyLength));
                wchar_t* plainText = (wchar_t*)block.m_data + keyLength;
                wcstombs(plain, plainText, wcslen(plainText) + 1);
                loaded = 0;
                if (CCrc32::GetInstance()->Crc32((unsigned char*)plain, strlen(plain)) == crc) {
                    CNGSUtil::DebugLog("WcsLen = %d and length = %d", strlen(plain), block.m_size);
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
                CNGSUtil::DebugLog("Error: Attempt to Save without loading");
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
                CNGSUtil::DebugLog("Error: How did we get here?");
                saved = false;
                break;
            }
        }
        unsigned char valid = localUser->isValid();
        CNGSUtil::DebugLog("CProfileManager::save: %d, %d, %d", syncToServer, saved, valid);
        if (saved && syncToServer && localUser->isValid()) {
            m_pendingCollections.Clear();
            m_savingCollections.Copy(collectionTypes);
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
    TCVector<int> collections;
    collections.Copy(collectionTypes);
    CNGSUtil::DebugLog("CProfileManager::syncWithServer %d", collections.Size());
    if (!isLocalDataValid(collections)) {
        loadSaveEnd();
        localUser->m_notificationHandler->HandleProfileSaved(0, collectionTypes);
    } else {
        CNGSUtil::DebugLog("CProfileManager::syncWithServer isLocalDataValid(cts)");
        m_fileBackedCollections.Clear();
        bool uploading = false;
        bool hasContentFiles = false;
        int count = collections.Size();
        const int* types = collections.m_data;
        for (int index = 0; index < count; index++) {
            CSaveRestoreInterface* data = getGameData(types[index], GAME_DATA_CURRENT);
            if (data->getBackingStoreType() == BACKING_STORE_ATTRIBUTES) {
                m_fileBackedCollections.Add(types[index]);
            } else if (data->getBackingStoreType() == BACKING_STORE_CONTENT_FILE) {
                hasContentFiles = true;
                if (localUser->m_session->isSessionKeyValid() == 1) {
                    uploading = true;
                    TCVector<int> single;
                    single.Add(types[index]);
                    uploadFileBackedData(single);
                    m_pendingCollections.Add(types[index]);
                }
            }
        }
        if (!uploading) {
            if (hasContentFiles) {
                loadSaveEnd();
                localUser->m_notificationHandler->HandleProfileSaved(0, collectionTypes);
            } else {
                // Resnapshot the storage for the attribute pass after the first pass has run.
                types = *static_cast<int* volatile*>(&collections.m_data);
                for (int index = 0; index < count; index++) {
                    CSaveRestoreInterface* data = getGameData(types[index], GAME_DATA_CURRENT);
                    if (data->getBackingStoreType() == BACKING_STORE_ATTRIBUTES) {
                        data->getAllData(attributes);
                        m_pendingCollections.Add(types[index]);
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
    CNGSUtil::DebugLog("CProfileManager::handleResponseSyncWithServer");
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

    unsigned int fileSize;
    sDataBlock block;
    if (!m_obfuscateFiles) {
        unsigned int utfLength = CUtf::GetUtfLen(json.GetWCharPtr(), json.GetLength());
        if (utfLength == 0) {
            return 0;
        }
        // JMUtf: 16-bit length prefix, JMUtf32 above 64 KB.
        fileSize = utfLength + 3;
        if (fileSize > 0xffff) {
            fileSize = utfLength + 5;
        }
    } else {
        if (CNGSUtil::ObfuscateString(block, json.GetWCharPtr(), PROFILE_FILE_KEY) != 0) {
            return 0;
        }
        fileSize = block.m_size + 4;
    }
    if (fileSize == 0) {
        return 0;
    }
    unsigned char* buffer = new unsigned char[fileSize];
    memset(buffer, 0, fileSize);
    CArrayOutputStream_gServe stream;
    if (!stream.Open(buffer, fileSize)) {
        if (buffer != NULL) {
            delete[] buffer;
        }
        return 0;
    }
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
        if (!success) {
            serverData->setDataSaveStatus(PROFILE_DATA_SAVED_LOCAL);
            TCVector<int> collectionTypes;
            getAllRegisteredCollections(collectionTypes);
            useDataFromLocal(collectionTypes);
            invalidateLocalData(collectionTypes, false);
        } else if (size < 1) {
            TCVector<int> collectionTypes;
            getAllRegisteredCollections(collectionTypes);
            useDataFromLocal(collectionTypes);
            save(collectionTypes, false);
        } else if (!collection->isValidJsonData(data, size)) {
            CSaveRestoreInterface* localData = getGameData(collection->getCollectionType(), GAME_DATA_LOCAL);
            success = 0;
            if (localData->getDataSaveStatus() == PROFILE_DATA_INVALIDATED && serverData == collection) {
                // The server copy is unusable: push the local data instead.
                TCVector<int> collectionTypes;
                getAllRegisteredCollections(collectionTypes);
                useDataFromLocal(collectionTypes);
                save(collectionTypes, false);
            }
        } else {
            serverData->setDataSaveStatus(PROFILE_DATA_SYNCED);
            if (!ReadRawFileDataToAttributes(data, size, serverData)) {
                serverData->setDataSaveStatus(PROFILE_DATA_SAVED_LOCAL);
                success = 0;
            } else if (currentData == collection) {
                currentData->copyAllData(*serverData);
                currentData->setDataSaveStatus(PROFILE_DATA_SYNCED);
                saveToFileSystem(currentData, localUser->m_credentials.getSubDirectoryPath());
            }
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
