// CSaveRestoreInterface.cpp - base fields of a profile data collection and their attribute form (b025x).
#include "CSaveRestoreInterface.h"
#include "CNGSAttribute.h"
#include "CNGS.h"
#include "CProfileManager.h"
#include "CHash.h"
#include "CStringToKey.h"


static const char* const ProfileDataCollectionTypeStrings[] = {
    "PUBLICDATA1", "PRIVATEDATA1", "FILEDATA1", "SAMPLEDATA2", "SAMPLEDATA3"
};

CSaveRestoreInterface::CSaveRestoreInterface(int collectionType, int version, BackingStoreType backingStore)
    : m_currentVersion(version)
    , m_descriptor(NULL)
    , m_file(NULL)
    , m_collectionType(collectionType)
    , m_dataSaveStatus(PROFILE_DATA_NOT_LOADED)
    , m_version(version)
    , m_backingStoreType(backingStore)
{
}

unsigned char CSaveRestoreInterface::compareAllData(const CSaveRestoreInterface& other)
{
    unsigned char same = 0;
    if (m_collectionType == other.m_collectionType) {
        same = m_version == other.m_version;
        if (same) {
            same = compareData(&other);
        }
    }
    return same;
}

unsigned char CSaveRestoreInterface::copyAllData(const CSaveRestoreInterface& other)
{
    if (this == &other) {
        return 1;
    }
    m_collectionType = other.m_collectionType;
    m_dataSaveStatus = other.m_dataSaveStatus;
    m_version = other.m_version;
    m_backingStoreType = other.m_backingStoreType;
    return copyData(&other);
}

int CSaveRestoreInterface::getCollectionType()
{
    return m_collectionType;
}

ProfileDataSaveStatus CSaveRestoreInterface::getDataSaveStatus()
{
    return m_dataSaveStatus;
}

void CSaveRestoreInterface::setDataSaveStatus(ProfileDataSaveStatus status)
{
    m_dataSaveStatus = status;
}

void CSaveRestoreInterface::resetAllData()
{
    m_dataSaveStatus = PROFILE_DATA_NOT_LOADED;
    m_version = m_currentVersion;
    resetData();
}

void CSaveRestoreInterface::getAllData(TCVector<CNGSAttribute*>& attributes)
{
    CStrWChar collectionTypeName = getwTokenNameFor("COLLECTIONTYPE");
    CStrWChar dataSaveStatusName = getwTokenNameFor("DATASAVESTATUS");
    CStrWChar versionName = getwTokenNameFor("VERSION");
    CStrWChar backingStoreName = getwTokenNameFor("BACKINGSTORE");
    CNGSAttribute* collectionType = new CNGSAttribute(collectionTypeName, m_collectionType, m_collectionType);
    CNGSAttribute* dataSaveStatus = new CNGSAttribute(dataSaveStatusName, (int)m_dataSaveStatus, m_collectionType);
    CNGSAttribute* version = new CNGSAttribute(versionName, m_version, m_collectionType);
    CNGSAttribute* backingStore = new CNGSAttribute(backingStoreName, m_backingStoreType, m_collectionType);
    attributes.Add(collectionType);
    attributes.Add(dataSaveStatus);
    attributes.Add(version);
    attributes.Add(backingStore);
    getData(attributes);
}

CStrWChar CSaveRestoreInterface::getwTokenNameFor(const char* name)
{
    return CStrWChar(getTokenNameFor(name).GetCharPtr());
}

void CSaveRestoreInterface::setAllData(const CHash* attributes)
{
    resetAllData();
    CNGSAttribute* collectionType = NULL;
    CNGSAttribute* dataSaveStatus = NULL;
    CNGSAttribute* version = NULL;
    CNGSAttribute* backingStore = NULL;
    CStrChar collectionTypeName = getTokenNameFor("COLLECTIONTYPE");
    CStrChar dataSaveStatusName = getTokenNameFor("DATASAVESTATUS");
    CStrChar versionName = getTokenNameFor("VERSION");
    CStrChar backingStoreName = getTokenNameFor("BACKINGSTORE");

    // A missing base field marks the collection as not loaded.
    if (attributes->Find(CStringToKey(collectionTypeName.GetCharPtr(), 0), (void**)&collectionType)) {
        m_collectionType = collectionType->getVal_int32();
    } else {
        m_dataSaveStatus = PROFILE_DATA_NOT_LOADED;
    }
    if (attributes->Find(CStringToKey(dataSaveStatusName.GetCharPtr(), 0), (void**)&dataSaveStatus)) {
        m_dataSaveStatus = (ProfileDataSaveStatus)dataSaveStatus->getVal_int32();
    } else {
        m_dataSaveStatus = PROFILE_DATA_NOT_LOADED;
    }
    if (attributes->Find(CStringToKey(versionName.GetCharPtr(), 0), (void**)&version)) {
        m_version = version->getVal_int32();
    } else {
        m_dataSaveStatus = PROFILE_DATA_NOT_LOADED;
    }
    if (attributes->Find(CStringToKey(backingStoreName.GetCharPtr(), 0), (void**)&backingStore)) {
        m_backingStoreType = backingStore->getVal_int32();
    } else {
        m_dataSaveStatus = PROFILE_DATA_NOT_LOADED;
    }
    setData(attributes);
}

CStrChar CSaveRestoreInterface::getTokenNameFor(const char* name)
{
    CStrChar token;
    token += getBaseName();
    token += "_";
    token += name;
    return token;
}

int CSaveRestoreInterface::getBackingStoreType()
{
    return m_backingStoreType;
}

CStrWChar CSaveRestoreInterface::getFilename()
{
    CStrWChar fileName(getBaseName().GetCharPtr());
    fileName += L".dat";
    return fileName;
}

void CSaveRestoreInterface::setDescriptor(CSaveRestoreInterface* descriptor)
{
    m_descriptor = descriptor;
}

CSaveRestoreInterface* CSaveRestoreInterface::getDescriptor()
{
    return m_descriptor;
}

CSaveRestoreInterface* CSaveRestoreInterface::getFile()
{
    return m_file;
}

void CSaveRestoreInterface::setFile(CSaveRestoreInterface* file)
{
    m_file = file;
}

void CSaveRestoreInterface::dataNotFound()
{
    m_dataSaveStatus = PROFILE_DATA_NOT_LOADED;
}

void CSaveRestoreInterface::debugPrintBase()
{
}

CStrChar CSaveRestoreInterface::getBaseName()
{
    CProfileManager* manager = CNGS::GetInstance()->GetProfileManager();
    int collectionType = m_collectionType;
    int baseCollection = manager->getBaseCollection();
    return CStrChar(ProfileDataCollectionTypeStrings[collectionType - baseCollection]);
}

unsigned char CSaveRestoreInterface::isValidJsonData(const unsigned char* data, unsigned int size)
{
    if (data == NULL || size < 3) {
        return 0;
    }
    const unsigned char* suffix = data + size - 3;
    if (suffix[0] == 0 || suffix[1] == 0 || suffix[2] == 0) {
        return 1;
    }
    return suffix[0] == '}' && suffix[1] == ']' && suffix[2] == '}';
}
