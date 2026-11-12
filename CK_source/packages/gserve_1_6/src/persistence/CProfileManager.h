// CProfileManager.h - loads / saves the player profile: the game registers its data collections
// (CSaveRestoreInterface) and the manager keeps three copies of each (in use, as read from the device,
// as read from the server), writes them to the device and syncs them with the server either as
// avatar attributes or as content files (b025x; replaces the b016x PROVISIONAL). sizeof 0xcc.
#ifndef CPROFILEMANAGER_H
#define CPROFILEMANAGER_H

#include "CNGSServerObject.h"
#include "CSaveRestoreInterface.h"
#include "CStrWChar.h"
#include "TCVector.h"

class CNGSUserCredentials;
class CObjectMap;
class ProfileManagerFunctor;

class CProfileManager : public CNGSServerObject {
public:
    /** Which copy of a collection getGameData() returns. */
    enum GameDataSource {
        GAME_DATA_CURRENT = 0,      // the copy the game works on
        GAME_DATA_LOCAL = 1,        // as loaded from the device
        GAME_DATA_SERVER = 2        // as loaded from the server
    };

    CProfileManager(const CNGSUserCredentials* credentials);
    virtual ~CProfileManager();
    void Reset();
    int getBaseCollection();
    void setCollectionRange(int first, int last);
    void registerSaveRestoreInterface(CSaveRestoreInterface* current, CSaveRestoreInterface* server,
                                      CSaveRestoreInterface* local);

    /** Append every collection type from m_firstCollectionType to m_lastCollectionType. */
    void getAllRegisteredCollections(TCVector<int>& collectionTypes);
    /** Collection `collectionType` of `source`; descriptor types (below the first type) map to their descriptor. */
    CSaveRestoreInterface* getGameData(int collectionType, GameDataSource source);
    int load(const TCVector<int>& collectionTypes);
    bool checkAnonymousLocalData();
    bool dataExistsForUser(int userId);
    bool anonymousLocalDataPromote();
    /** Remove the directory of the anonymous user (-1). */
    bool anonymousLocalDataDelete();
    /** One of the current collection files exists in `directory`. */
    unsigned char contentExistsWithinDir(const wchar_t* directory);
    int loadFromServer(const TCVector<int>& collectionTypes);
    void resolveLocalServerDataConflicts(const TCVector<int>& collectionTypes);
    void ReplaceFileWithDescriptor(TCVector<int>& collectionTypes);
    void ReplaceDescriptorWithFile(TCVector<int>& collectionTypes);
    void resetDataBuffer(const TCVector<int>& collectionTypes, ProfileDataSaveStatus status);
    void handleResponseLoadFromServer(CObjectMap* response, ProfileManagerFunctor* functor);
    void handleResponseResolveLocalServerDataConflicts(CObjectMap* response, ProfileManagerFunctor* functor);
    bool downloadFileBackedData(const TCVector<int>& collectionTypes, CSaveRestoreInterface* collection);
    /** Lock the content manager while loading / saving. */
    void loadSaveStart();
    /** Read collection `collectionType` of user `userId` from the device into the local copy. */
    unsigned char loadFromFileSystem(int collectionType, int userId);
    /** destination[type] = source[type] (and their descriptors) for every type in `collectionTypes`. */
    void copyDataBuffer(const TCVector<int>& collectionTypes,
                        TCVector<CSaveRestoreInterface*>* destination,
                        TCVector<CSaveRestoreInterface*>* source);
    void loadSaveEnd();
    /** Split the loaded collections into local / server / conflicting and report the conflicts. */
    void CNGSHandleProfileDataConflict(const TCVector<int>& collectionTypes);
    /** Parse a saved file (plain JMUtf JSON or obfuscated JSON with CRC) into `collection`. */
    unsigned char ReadRawFileDataToAttributes(const unsigned char* data, unsigned int size, CSaveRestoreInterface* collection);
    void useDataFromLocal(const TCVector<int>& collectionTypes);
    /** Mark the collections invalidated on the device (and reset them when `reset`). */
    unsigned char invalidateLocalData(const TCVector<int>& collectionTypes, unsigned char reset);
    void useDataFromServer(const TCVector<int>& collectionTypes);
    /** Save the collections to the device and, when `syncToServer`, to the server. */
    bool save(const TCVector<int>& collectionTypes, bool syncToServer);
    /** Upload the content file collections; true when the last file could be read. */
    bool uploadFileBackedData(const TCVector<int>& collectionTypes);
    bool syncWithServer(const TCVector<int>& collectionTypes);
    /** False when one of the collections is invalidated on the device. */
    unsigned char isLocalDataValid(const TCVector<int>& collectionTypes);
    void handleResponseSyncWithServer(CObjectMap* response, ProfileManagerFunctor* functor);
    /** Write `collection` to the user directory `subDirectory` (and its descriptor for content files). */
    unsigned char saveToFileSystem(CSaveRestoreInterface* collection, const CStrWChar& subDirectory);
    /** File image of `collection` (malloc'd, the caller frees it). */
    unsigned char WriteAttributesToRawFileData(CSaveRestoreInterface* collection, unsigned char*& data, unsigned int& size);
    /** Local copy for SAVED_LOCAL, otherwise the server copy. */
    CSaveRestoreInterface* getManagedObjectFromBuffer(ProfileDataSaveStatus status, int collectionType);
    virtual unsigned char HandleContentFileUploaded(unsigned char success, CSaveRestoreInterface* collection);
    void handleResponseSyncDataDescWithServer(CObjectMap* response, ProfileManagerFunctor* functor);
    virtual unsigned char HandleContentFileDownloadedSelf(unsigned char success, const unsigned char* data, int size,
                                                          CSaveRestoreInterface* collection);
    /** Ready, idle and with registered collections, and the content manager is ready. */
    virtual unsigned char isAvailable();

    TCVector<CSaveRestoreInterface*> m_currentData;     // +0x18 GAME_DATA_CURRENT
    TCVector<CSaveRestoreInterface*> m_serverData;      // +0x30 GAME_DATA_SERVER
    TCVector<CSaveRestoreInterface*> m_localData;       // +0x48 GAME_DATA_LOCAL
    bool m_obfuscateFiles;                                                       // +0x60 write obfuscated files
    int m_firstCollectionType;                                                   // +0x64 set by the game
    int m_lastCollectionType;                                                    // +0x68 set by the game
    TCVector<CSaveRestoreInterface*> m_registeredData;  // +0x6c owned content-file descriptors
    TCVector<int> m_pendingCollections;                 // +0x84 requests in flight
    TCVector<int> m_savingCollections;                  // +0x9c reported to HandleProfileSaved
    TCVector<int> m_fileBackedCollections;              // +0xb4 attribute collections still to sync

private:
    void ApplyProfileResponse(CObjectMap* response, ProfileManagerFunctor* functor, bool conflict);
    void FinishProfileLoad(unsigned char success);
};

#endif
