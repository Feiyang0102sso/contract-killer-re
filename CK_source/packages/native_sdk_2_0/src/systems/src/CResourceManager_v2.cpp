// CResourceManager_v2.cpp - resource manager over CBigFile_v2 archives and loose files.
#include "CResourceManager_v2.h"

#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "CApplet.h"
#include "CApp.h"
#include "CEvent.h"
#include "CMessage.h"
#include "CEventListener.h"
#include "CHandle.h"
#include "CRegistryItr.h"
#include "CResource.h"
#include "CResourceBigFile.h"
#include "CResourceFactory.h"
#include "CResourceDataBlockFileAccess.h"
#include "CIdToObjectRouter.h"
#include "CArrayInputStream.h"
#include "ICFileMgr.h"
#include "ICStdUtil.h"

                                                                        

/** Mime types of string resources (UTF-8 / raw wide characters). */
static const unsigned int kMimeTypeStringUtf8 = 0xf686aadc;
static const unsigned int kMimeTypeStringWide = 0x43735eb4;

/** Text a string resource holds instead of its data when resource info is substituted. */
static const wchar_t kwszId[] = L"0x%8.8x";

unsigned char CResourceManager_HasBigFileExtension(const char* path)
{
    static const wchar_t ext_big[] = L".big";
    static const wchar_t ext_bar[] = L".bar";

    CStrWChar fileName(path);
    fileName.Trim();
    int dot = fileName.GetCharIndex(0, L'.');
    unsigned char isBigFile;
    if (dot >= 1) {
        isBigFile = wcscmp(fileName.m_string + dot, ext_big) == 0 || wcscmp(fileName.m_string + dot, ext_bar) == 0;
    } else {
        isBigFile = 0;
    }
    return isBigFile;
}

unsigned char CResourceManager_Handler_KeyToName(void* context, unsigned int key, char** outName)
{
    CResourceManager_v2* manager = (CResourceManager_v2*)context;
    char buffer[0x100];
    const char* stored = manager->m_currentBigFile->m_strings + key;
    const char* name = buffer;
    if (!manager->m_currentBigFile->DecompressIntoDestinationBufferIfNeeded(buffer, stored)) {
        name = stored;
    }
    unsigned int length = strlen(name);
    if (length != 0) {
        *outName = (char*)malloc(length + 1);
        strcpy(*outName, name);
        return 1;
    }
    *outName = NULL;
    return 0;
}

unsigned char CResourceManager_Handler_KeyToPointer(void* context, unsigned int key, void** outPointer,
                                                    unsigned int& outType)
{
    CResourceManager_v2* manager = (CResourceManager_v2*)context;
    char buffer[0x100];
    const char* stored = manager->m_currentBigFile->m_strings + key;
    const char* name = buffer;
    if (!manager->m_currentBigFile->DecompressIntoDestinationBufferIfNeeded(buffer, stored)) {
        name = stored;
    }
    CResource* resource;
    if (!manager->GetResource(name, &resource)) {
        return 0;
    }
    *outPointer = resource->GetData();
    outType = ((CClass*)resource->GetData())->m_classId;
    return 1;
}

unsigned char CResourceManager_Handler_KeyToInputStream(void* context, unsigned int key,
                                                        CInputStream& outStream, unsigned int& outType)
{
    CResourceManager_v2* manager = (CResourceManager_v2*)context;
    char buffer[0x100];
    const char* stored = manager->m_currentBigFile->m_strings + key;
    const char* name = buffer;
    if (!manager->m_currentBigFile->DecompressIntoDestinationBufferIfNeeded(buffer, stored)) {
        name = stored;
    }
    CSystemElement* bigFileElement = NULL;
    const char* storedName = NULL;
    unsigned char nameDecompressed = 0;
    return manager->GetStream(name, outStream, outType, &bigFileElement, &storedName, nameDecompressed);
}

unsigned char CResourceManager_v2::GetStream(const char* name, CInputStream& stream, unsigned int& mimeType,
                                             CSystemElement** bigFileElement, const char** storedName,
                                             unsigned char& nameDecompressed)
{
    unsigned char found = SearchBigFileRegistryForStream(name, stream, mimeType, bigFileElement, storedName,
                                                         nameDecompressed);
    if (name == NULL) {
        return found;
    }
    if (!found) {
        if (!CResourceManager_HasBigFileExtension(name)) {
            return 0;
        }
        mimeType = MimeType_BigFile;
    }

    // a loose file in the application folder wins over the archives
    wchar_t path[0x100];
    wcscpy(path, m_appPath.m_string);
    mbstowcs(path + m_appPath.m_length, name, strlen(name) + 1);
    if (!ICFileMgr::GetInstance()->Exists(path)) {
        return found;
    }

    if (m_nextFileStream >= FileStreamCount) {
        m_nextFileStream = 0;
    }
    CFileInputStream* fileStream = &m_fileStreams[m_nextFileStream++];
    if (!fileStream->Open(path)) {
        return found;
    }
    stream.Open(fileStream, fileStream->Available());
    nameDecompressed = 0;
    if (mimeType != MimeType_BigFile && mimeType != MimeType_BigFileAlt) {
        return 1;
    }
    // an archive file: remember where it comes from
    mimeType = MimeType_BigFile;
    *bigFileElement = NULL;
    CResourceDataBlockFileAccess::GetInstance()->m_path = path;
    return 1;
}

unsigned char CResourceManager_Handler_NameToPointer(void* context, const char* name, void** outPointer,
                                                     unsigned int& outType)
{
    CResourceManager_v2* manager = (CResourceManager_v2*)context;
    CResource* resource;
    if (!manager->GetResource(name, &resource)) {
        return 0;
    }
    *outPointer = resource->GetData();
    outType = ((CClass*)resource->GetData())->m_classId;
    return 1;
}

unsigned char CResourceManager_Handler_NameToInputStream(void* context, const char* name,
                                                         CInputStream& outStream, unsigned int& outType)
{
    CResourceManager_v2* manager = (CResourceManager_v2*)context;
    CSystemElement* bigFileElement = NULL;
    const char* storedName = NULL;
    unsigned char nameDecompressed = 0;
    return manager->SearchBigFileRegistryForStream(name, outStream, outType, &bigFileElement, &storedName,
                                                   nameDecompressed);
}

CResourceManager_v2::CResourceManager_v2()
{
    m_nextFileStream = 0;
    m_classId = ClassId;
}

CResourceManager_v2::~CResourceManager_v2()
{
}

unsigned char CResourceManager_v2::Initialize(unsigned long tableSize, unsigned long initialAllocCount,
                                              unsigned char growable)
{
    Destroy();
    unsigned char initialized = m_resources.Init(tableSize, initialAllocCount, growable);

    // folder searched for loose files
    ICFileMgr* fileMgr = ICFileMgr::GetInstance();
    if (fileMgr->GetApplicationPath() != NULL) {
        m_appPath = fileMgr->GetApplicationPath();
    } else {
        m_appPath = "";
    }
    if (m_appPath.m_length >= 1) {
        m_appPath.Concatenate(fileMgr->GetPathSeparator());
    }

    if (!initialized) {
        Destroy();
        return 0;
    }

    // resource references inside resources are resolved through this manager
    CIdToObjectRouter* router = new CIdToObjectRouter();
    m_router = router;
    router->m_keyToNameContext = this;
    router->m_keyToName = CResourceManager_Handler_KeyToName;
    router = m_router;
    router->m_keyToPointerContext = this;
    router->m_keyToPointer = CResourceManager_Handler_KeyToPointer;
    router = m_router;
    router->m_keyToInputStreamContext = this;
    router->m_keyToInputStream = CResourceManager_Handler_KeyToInputStream;
    router = m_router;
    router->m_nameToPointerContext = this;
    router->m_nameToPointer = CResourceManager_Handler_NameToPointer;
    router = m_router;
    router->m_nameToInputStreamContext = this;
    router->m_nameToInputStream = CResourceManager_Handler_NameToInputStream;

    m_reserved88 = 0;
    m_substituteResourceInfo = 0;
    return initialized;
}

void CResourceManager_v2::Destroy()
{
    DestroyResources();
    m_substituteResourceInfo = 0;
    m_currentBigFile = NULL;
    m_reserved88 = 0;
    if (m_router != NULL) {
        delete m_router;
        m_router = NULL;
    }
    m_appPath = "";
    CResourceManager::Destroy();
}

CResourceManager::CConsecutiveResourceIdItr* CResourceManager_v2::CreateAndInitConsecutiveResourceIdItr(
    unsigned int handle, unsigned int count)
{
    CConsecutiveResourceIdItr* names = new CConsecutiveResourceIdItr();
    for (CRegistryItr itr(m_bigFiles); !itr.IsDone(); itr++) {
        BigFileEntry* entry = (BigFileEntry*)(*itr).m_object;
        if (names->m_names.Initialize(*entry->m_bigFile, handle, count)) {
            return names;
        }
    }
    if (names != NULL) {
        delete names;
    }
    return NULL;
}

CResourceManager::CConsecutiveResourceIdItr* CResourceManager_v2::CreateAndInitConsecutiveResourceIdItr(
    const char* name, unsigned int count)
{
    CConsecutiveResourceIdItr* names = new CConsecutiveResourceIdItr();
    for (CRegistryItr itr(m_bigFiles); !itr.IsDone(); itr++) {
        BigFileEntry* entry = (BigFileEntry*)(*itr).m_object;
        if (names->m_names.Initialize(*entry->m_bigFile, name, count)) {
            return names;
        }
    }
    if (names != NULL) {
        delete names;
    }
    return NULL;
}

CResourceManager::CConsecutiveResourceIdItr* CResourceManager_v2::CreateAndInitConsecutiveResourceIdItr(
    const wchar_t* name, unsigned int count)
{
    return CreateAndInitConsecutiveResourceIdItr(ToNarrowName(name), count);
}

unsigned char CResourceManager_v2::AddResource(CResource& resource)
{
    if (resource.m_manager == this) {
        return 0;
    }
    resource.m_manager = this;
    resource.m_refCount++;
    resource.m_nextSameHandle = NULL;
    CResource* last;
    if (m_resources.Find(resource.m_handle, (void**)&last)) {
        // append to the chain of resources with this handle
        while (last->m_nextSameHandle != NULL) {
            last = last->m_nextSameHandle;
        }
        last->m_nextSameHandle = &resource;
    } else {
        m_resources.Insert(resource.m_handle, &resource);
    }
    return 1;
}

unsigned char CResourceManager_v2::RemoveResource(unsigned int handle)
{
    CResource* resource;
    if (!m_resources.Find(handle, (void**)&resource)) {
        return 0;
    }
    return RemoveResource(*resource);
}

unsigned char CResourceManager_v2::RemoveResource(const char* name)
{
    if (name == NULL) {
        return 0;
    }
    CResource* resource;
    if (!m_resources.Find(CStringToKey(name, 0) & 0x7fffffff, (void**)&resource)) {
        return 0;
    }

    // find the resource called `name` in the chain of its handle
    int index = 0;
    CResource* previous;
    for (;;) {
        if (resource == NULL) {
            return 0;
        }
        if (resource->m_name == NULL) {
            continue;
        }
        if (strcmp(name, resource->m_name) == 0) {
            break;
        }
        index++;
        previous = resource;
        resource = resource->m_nextSameHandle;
    }

    if (resource->m_classId == CResourceBigFile::ClassId) {
        // an archive: resources named from it need their own name copies, then unregister it
        CRegistry& bigFiles = m_bigFiles;
        for (CRegistryItr itr(bigFiles); !itr.IsDone(); itr++) {
            CSystemElement* element = &*itr;
            BigFileEntry* entry = (BigFileEntry*)element->m_object;
            wchar_t wideName[0x100];
            mbstowcs(wideName, resource->m_name, strlen(resource->m_name) + 1);
            if (wcscmp(wideName, entry->m_name.m_string) == 0) {
                int bucketCount = m_resources.m_bucketCount;
                for (int bucket = 0; bucket < bucketCount; bucket++) {
                    for (CHash::Entry* node = m_resources.m_buckets[bucket]; node != NULL;) {
                        CResource* chain = (CResource*)node->m_value;
                        node = node->m_next;
                        RemoveSourceFromAllResourcesOfSameHandleAndOfSpecificSource(chain, entry->m_bigFile);
                    }
                }
                if (entry != NULL) {
                    delete entry;
                }
                bigFiles.Remove(*element);
                if (element != NULL) {
                    delete element;
                }
                break;
            }
        }
    }

    if (index == 0) {
        // first of its chain: the hash now points to the next one
        m_resources.Remove(resource->m_handle);
        if (resource->m_nextSameHandle != NULL) {
            m_resources.Insert(resource->m_handle, resource->m_nextSameHandle);
        }
    } else {
        previous->m_nextSameHandle = resource->m_nextSameHandle;
    }
    resource->m_manager = NULL;
    resource->m_refCount = 0;
    resource->m_nextSameHandle = NULL;
    return 1;
}

void CResourceManager_v2::RemoveSourceFromAllResourcesOfSameHandleAndOfSpecificSource(CResource* resource, void* source)
{
    if (resource->m_nextSameHandle != NULL) {
        RemoveSourceFromAllResourcesOfSameHandleAndOfSpecificSource(resource->m_nextSameHandle, source);
    }
    if (resource->m_source != source) {
        return;
    }
    // the name pointed into the archive: keep a private copy
    if (!resource->m_ownsName && resource->m_name != NULL) {
        resource->SetName(resource->m_name, 1);
    }
    resource->m_source = NULL;
}

unsigned char CResourceManager_v2::RemoveResource(const wchar_t* name)
{
    return RemoveResource(ToNarrowName(name));
}

unsigned char CResourceManager_v2::RemoveResource(CResource& resource)
{
    if (resource.m_manager != this) {
        return 0;
    }
    CResource* current;
    if (!m_resources.Find(resource.m_handle, (void**)&current)) {
        return 0;
    }

    // position of `resource` in the chain of its handle
    int index = 0;
    CResource* previous;
    while (current != &resource) {
        index--;
        previous = current;
        current = current->m_nextSameHandle;
        if (current == NULL) {
            return 0;
        }
    }

    if (resource.m_classId == CResourceBigFile::ClassId) {
        // an archive: resources named from it need their own name copies, then unregister it
        CRegistry& bigFiles = m_bigFiles;
        for (CRegistryItr itr(bigFiles); !itr.IsDone(); itr++) {
            CSystemElement* element = &*itr;
            BigFileEntry* entry = (BigFileEntry*)element->m_object;
            wchar_t wideName[0x100];
            mbstowcs(wideName, resource.m_name, strlen(resource.m_name) + 1);
            if (wcscmp(wideName, entry->m_name.m_string) == 0) {
                int bucketCount = m_resources.m_bucketCount;
                for (int bucket = 0; bucket < bucketCount; bucket++) {
                    for (CHash::Entry* node = m_resources.m_buckets[bucket]; node != NULL;) {
                        CResource* chain = (CResource*)node->m_value;
                        node = node->m_next;
                        RemoveSourceFromAllResourcesOfSameHandleAndOfSpecificSource(chain, entry->m_bigFile);
                    }
                }
                if (entry != NULL) {
                    delete entry;
                }
                bigFiles.Remove(*element);
                if (element != NULL) {
                    delete element;
                }
                break;
            }
        }
    }

    if (index == 0) {
        // first of its chain: the hash now points to the next one
        m_resources.Remove(current->m_handle);
        current = current->m_nextSameHandle;
        if (current != NULL) {
            m_resources.Insert(current->m_handle, current);
        }
    } else {
        previous->m_nextSameHandle = current->m_nextSameHandle;
    }
    resource.m_manager = NULL;
    resource.m_refCount = 0;
    resource.m_nextSameHandle = NULL;
    return 1;
}

unsigned int CResourceManager_v2::CreateResource(unsigned int handle, CResource** resource)
{
    return CreateResource(resource, handle, NULL, 0, NULL);
}

unsigned char CResourceManager_v2::CreateResources(const unsigned int* handles)
{
    unsigned char allCreated = 1;
    CResource* resource;
    // note: the original walks until the pointer (not the value) becomes NULL
    if (handles != NULL) {
        do {
            if (!CreateResource(*handles++, &resource)) {
                allCreated = 0;
            }
        } while (handles != NULL);
    }
    return allCreated;
}

unsigned int CResourceManager_v2::CreateResource(const CreationInfoHnd& info, CResource** resource)
{
    return CreateResource(resource, info.m_handle, NULL, info.m_mimeType, info.m_stream);
}

unsigned char CResourceManager_v2::CreateResources(const CreationInfoHnd* infos)
{
    unsigned char allCreated = 1;
    while (infos->m_handle != 0) {
        CResource* resource;
        if (!CreateResource(*infos, &resource)) {
            allCreated = 0;
        }
        infos++;
    }
    return allCreated;
}

unsigned int CResourceManager_v2::CreateResource(const char* name, CResource** resource)
{
    return CreateResource(resource, CStringToKey(name, 0) & 0x7fffffff, name, 0, NULL);
}

unsigned char CResourceManager_v2::CreateResources(const char** names)
{
    unsigned char allCreated = 1;
    CResource* resource;
    // note: the original walks until the pointer (not the value) becomes NULL
    if (names != NULL) {
        do {
            if (!CreateResource(*names++, &resource)) {
                allCreated = 0;
            }
        } while (names != NULL);
    }
    return allCreated;
}

unsigned int CResourceManager_v2::CreateResource(const CreationInfoStr& info, CResource** resource)
{
    return CreateResource(resource, CStringToKey(info.m_name, 0) & 0x7fffffff, info.m_name, info.m_mimeType,
                          info.m_stream);
}

unsigned char CResourceManager_v2::CreateResources(const CreationInfoStr* infos)
{
    unsigned char allCreated = 1;
    while (infos->m_name != NULL) {
        CResource* resource;
        if (!CreateResource(*infos, &resource)) {
            allCreated = 0;
        }
        infos++;
    }
    return allCreated;
}

unsigned int CResourceManager_v2::CreateResource(const wchar_t* name, CResource** resource)
{
    return CreateResource(ToNarrowName(name), resource);
}

unsigned char CResourceManager_v2::CreateResources(const wchar_t** names)
{
    unsigned char allCreated = 1;
    CResource* resource;
    // note: the original walks until the pointer (not the value) becomes NULL
    if (names != NULL) {
        do {
            if (!CreateResource(ToNarrowName(*names++), &resource)) {
                allCreated = 0;
            }
        } while (names != NULL);
    }
    return allCreated;
}

unsigned int CResourceManager_v2::CreateResource(const CreationInfoWStr& info, CResource** resource)
{
    return CreateResource(resource, CStringToKey(info.m_name, 0) & 0x7fffffff, ToNarrowName(info.m_name),
                          info.m_mimeType, info.m_stream);
}

unsigned char CResourceManager_v2::CreateResources(const CreationInfoWStr* infos)
{
    unsigned char allCreated = 1;
    while (infos->m_name != NULL) {
        CResource* resource;
        if (!CreateResource(*infos, &resource)) {
            allCreated = 0;
        }
        infos++;
    }
    return allCreated;
}

unsigned char CResourceManager_v2::GetResource(unsigned int handle, CResource** resource)
{
    *resource = NULL;
    if (handle == 0) {
        return 0;
    }
    CResource* found;
    if (!m_resources.Find(handle, (void**)&found)) {
        return 0;
    }
    if (!found->m_isCreated) {
        return 0;
    }
    *resource = found;
    return 1;
}

unsigned char CResourceManager_v2::GetResource(const char* name, CResource** resource)
{
    *resource = NULL;
    if (name == NULL) {
        return 0;
    }
    CResource* found;
    if (!m_resources.Find(CStringToKey(name, 0) & 0x7fffffff, (void**)&found)) {
        return 0;
    }
    for (; found != NULL; found = found->m_nextSameHandle) {
        if (found->m_name != NULL && strcmp(name, found->m_name) == 0) {
            if (!found->m_isCreated) {
                return 0;
            }
            *resource = found;
            return 1;
        }
    }
    return 0;
}

unsigned char CResourceManager_v2::GetResource(const wchar_t* name, CResource** resource)
{
    return GetResource(ToNarrowName(name), resource);
}

unsigned char CResourceManager_v2::ReleaseResource(unsigned int handle)
{
    CResource* resource;
    if (!m_resources.Find(handle, (void**)&resource)) {
        return 0;
    }
    return ReleaseResource(&resource);
}

// returns the status of the last element (r0 is left from the last call in the original)
unsigned char CResourceManager_v2::ReleaseResources(const unsigned int* handles)
{
    unsigned char result;
    do {
        result = ReleaseResource(*handles++);
    } while (*handles != 0);
    return result;
}

unsigned char CResourceManager_v2::ReleaseResource(const char* name)
{
    if (name == NULL) {
        return 0;
    }
    CResource* resource;
    if (!m_resources.Find(CStringToKey(name, 0) & 0x7fffffff, (void**)&resource)) {
        return 0;
    }
    for (; resource != NULL; resource = resource->m_nextSameHandle) {
        if (resource->m_name != NULL && strcmp(name, resource->m_name) == 0) {
            return ReleaseResource(&resource);
        }
    }
    return 0;
}

// returns the status of the last element (r0 is left from the last call in the original)
unsigned char CResourceManager_v2::ReleaseResources(const char** names)
{
    // note: the original walks until the pointer (not the name) becomes NULL
    unsigned char result;
    do {
        result = ReleaseResource(*names++);
    } while (names != NULL);
    return result;
}

unsigned char CResourceManager_v2::ReleaseResource(const wchar_t* name)
{
    return ReleaseResource(ToNarrowName(name));
}

// returns the status of the last element (r0 is left from the last call in the original)
unsigned char CResourceManager_v2::ReleaseResources(const wchar_t** names)
{
    // note: the original walks until the pointer (not the name) becomes NULL
    unsigned char result;
    do {
        result = ReleaseResource(*names++);
    } while (names != NULL);
    return result;
}

unsigned char CResourceManager_v2::ReleaseResource(CResource** resource)
{
    if ((*resource)->m_manager != this) {
        return 0;
    }
    if ((*resource)->m_handle == 0) {
        return 0;
    }
    if ((*resource)->m_refCount != 0) {
        (*resource)->m_refCount--;
        if ((*resource)->m_refCount != 0) {
            return 1;
        }
    }
    // last reference
    if (*resource != NULL) {
        delete *resource;
    }
    *resource = NULL;
    return 1;
}

unsigned char CResourceManager_v2::DestroyResource(unsigned int handle)
{
    CResource* resource;
    if (!m_resources.Find(handle, (void**)&resource)) {
        return 0;
    }
    if (resource->m_manager != this) {
        return 0;
    }
    if (resource != NULL) {
        delete resource;
    }
    return 1;
}

// returns the status of the last element (r0 is left from the last call in the original)
unsigned char CResourceManager_v2::DestroyResources(const unsigned int* handles)
{
    unsigned char result;
    do {
        result = DestroyResource(*handles++);
    } while (*handles != 0);
    return result;
}

unsigned char CResourceManager_v2::DestroyResource(const char* name)
{
    if (name == NULL) {
        return 0;
    }
    CResource* resource;
    if (!m_resources.Find(CStringToKey(name, 0) & 0x7fffffff, (void**)&resource)) {
        return 0;
    }
    if (resource->m_manager != this) {
        return 0;
    }
    for (; resource != NULL; resource = resource->m_nextSameHandle) {
        if (resource->m_name != NULL && strcmp(name, resource->m_name) == 0) {
            delete resource;
            return 1;
        }
    }
    return 0;
}

// returns the status of the last element (r0 is left from the last call in the original)
unsigned char CResourceManager_v2::DestroyResources(const char** names)
{
    // note: the original walks until the pointer (not the name) becomes NULL
    unsigned char result;
    do {
        result = DestroyResource(*names++);
    } while (names != NULL);
    return result;
}

unsigned char CResourceManager_v2::DestroyResource(const wchar_t* name)
{
    return DestroyResource(ToNarrowName(name));
}

// returns the status of the last element (r0 is left from the last call in the original)
unsigned char CResourceManager_v2::DestroyResources(const wchar_t** names)
{
    // note: the original walks until the pointer (not the name) becomes NULL
    unsigned char result;
    do {
        result = DestroyResource(*names++);
    } while (names != NULL);
    return result;
}

unsigned char CResourceManager_v2::DestroyResources()
{
    // every resource chain of the hash
    int bucketCount = m_resources.m_bucketCount;
    for (int bucket = 0; bucket < bucketCount; bucket++) {
        for (CHash::Entry* node = m_resources.m_buckets[bucket]; node != NULL;) {
            CResource* chain = (CResource*)node->m_value;
            node = node->m_next;
            DestroyIncludingAllResourcesOfSameHandle(chain);
        }
    }
    return 1;
}

void CResourceManager_v2::DestroyIncludingAllResourcesOfSameHandle(CResource* resource)
{
    if (resource->m_nextSameHandle != NULL) {
        DestroyIncludingAllResourcesOfSameHandle(resource->m_nextSameHandle);
    }
    delete resource;
}

void CResourceManager_v2::SubstituteResourceInfoInPlaceOfStringData(unsigned char substitute)
{
    // the original ignores the argument and always switches substitution on
    m_substituteResourceInfo = 1;
}

unsigned int CResourceManager_v2::CreateResource(CResource** resource, unsigned int handle, const char* name,
                                                 unsigned int mimeType, CInputStream* stream)
{
    unsigned int result = CResource::CreateResult_Failed;
    *resource = NULL;
    CInputStream archiveStream;
    if (handle == 0) {
        return result;
    }

    // a resource of this handle (and name) may exist already
    CResource* existing;
    CResource* previous = NULL;
    unsigned char proceed = 0;
    unsigned char success = 0;
    if (m_resources.Find(handle, (void**)&existing)) {
        previous = existing;
        if (name != NULL) {
            while (existing != NULL) {
                if (existing->m_name != NULL && strcmp(name, existing->m_name) == 0) {
                    break;
                }
                previous = existing;
                existing = existing->m_nextSameHandle;
            }
        }
        if (existing != NULL) {
            existing->m_refCount++;
            if (existing->m_isCreated) {
                result = 4;
                success = 1;
            } else {
                proceed = 1;
            }
        }
    }

    if (!success) {
        // where the data comes from; the archive replaces `name` by its stored copy
        CSystemElement* bigFileElement = NULL;
        unsigned char nameDecompressed = 0;
        unsigned char copyName;
        if (stream == NULL) {
            unsigned int foundMimeType;
            if (name != NULL) {
                proceed = GetStream(name, archiveStream, foundMimeType, &bigFileElement, &name, nameDecompressed);
            } else {
                proceed = SearchBigFileRegistryForStream(handle, archiveStream, foundMimeType, &bigFileElement,
                                                         &name, nameDecompressed);
            }
            if (mimeType != 0 && foundMimeType != mimeType) {
                return result;
            }
            if (proceed) {
                mimeType = foundMimeType;
                // a name stored uncompressed in the archive is used in place
                copyName = nameDecompressed != 0;
                stream = &archiveStream;
            } else {
                // not in an archive: an empty resource of the given type
                copyName = 1;
                proceed = 1;
                if (mimeType == 0) {
                    return result;
                }
            }
        } else if (mimeType != 0) {
            copyName = 1;
            proceed = 1;
        } else if (!proceed) {
            return result;
        } else {
            copyName = 1;
        }

        CResourceFactory* factory = CResourceFactory::GetInstance();
        if (!factory->CreateResource(mimeType, &existing)) {
            return result;
        }

        CBigFile_v2* bigFile = NULL;
        if (bigFileElement != NULL) {
            BigFileEntry* entry = (BigFileEntry*)bigFileElement->m_object;
            if (entry != NULL) {
                bigFile = entry->m_bigFile;
                if (mimeType == MimeType_BigFile || mimeType == MimeType_BigFileAlt) {
                    // an archive inside an archive is read from the same file
                    CResourceDataBlockFileAccess* access = CResourceDataBlockFileAccess::GetInstance();
                    if (!(entry->m_path == access->m_path)) {
                        access->m_path = entry->m_path;
                    }
                }
            }
        }

        {
            CArrayInputStream infoStream;
            if (m_substituteResourceInfo) {
                // the string resource holds its id instead of its text
                wchar_t* info = (wchar_t*)np_malloc(12 * sizeof(wchar_t));
                stream = &infoStream;
                mimeType = kMimeTypeStringWide;
                ICStdUtil::SWPrintF(info, kwszId, handle);
                infoStream.Open((const unsigned char*)info, 12 * sizeof(wchar_t));
            }
            m_currentBigFile = bigFile;
            result = existing->CreateInternal(handle, name, stream, mimeType, m_router, copyName);
            if (result == CResource::CreateResult_Failed) {
                proceed = 0;
            }
            if (proceed) {
                if (existing->m_classId == CResourceBigFile::ClassId) {
                    // a new archive: register it for streaming
                    CResourceDataBlockFileAccess* access = CResourceDataBlockFileAccess::GetInstance();
                    BigFileEntry* entry = new BigFileEntry((CBigFile_v2*)existing->GetData());
                    entry->m_name = name;
                    entry->m_path = access->m_path;
                    CRegistry& bigFiles = m_bigFiles;
                    bigFiles.Add(*bigFiles.CreateSystemElement((unsigned int)entry, entry, 0,
                                                               CPriorityClass::DefaultPriority));
                }
                existing->m_source = bigFile;
                if (previous != NULL) {
                    // append to the chain of the handle
                    previous->m_nextSameHandle = existing;
                    existing->m_manager = this;
                    existing->m_refCount++;
                } else {
                    proceed = AddResource(*existing);
                }
            }
            success = 1;
            if (!proceed) {
                success = 0;
                existing->Destroy();
            }
        }
    }
    if (success) {
        *resource = existing;
    }
    return result;
}

unsigned char CResourceManager_v2::SearchBigFileRegistryForStream(unsigned int handle, CInputStream& stream,
                                                                  unsigned int& mimeType, CSystemElement** bigFileElement,
                                                                  const char** storedName,
                                                                  unsigned char& nameDecompressed)
{
    for (CRegistryItr itr(m_bigFiles); !itr.IsDone(); itr++) {
        BigFileEntry* entry = (BigFileEntry*)(*itr).m_object;
        if (entry->m_bigFile->GetStream(handle, stream, mimeType, storedName, 1, nameDecompressed)) {
            if (bigFileElement != NULL) {
                *bigFileElement = &*itr;
            }
            return 1;
        }
    }
    return 0;
}

unsigned char CResourceManager_v2::SearchBigFileRegistryForStream(const char* name, CInputStream& stream,
                                                                  unsigned int& mimeType, CSystemElement** bigFileElement,
                                                                  const char** storedName,
                                                                  unsigned char& nameDecompressed)
{
    for (CRegistryItr itr(m_bigFiles); !itr.IsDone(); itr++) {
        BigFileEntry* entry = (BigFileEntry*)(*itr).m_object;
        if (entry->m_bigFile->GetStream(name, stream, mimeType, storedName, 1, nameDecompressed)) {
            if (bigFileElement != NULL) {
                *bigFileElement = &*itr;
            }
            return 1;
        }
    }
    return 0;
}

       
