// CResourceManager_v2.h - resource manager over CBigFile_v2 archives and loose files.
// (first drafted by b000x as PROVISIONAL with the iterator class; completed by b011x, the owner)
// The iterator members are defined inline after the class: GetHandle() is the key function,
// so the vtable is emitted in every file that includes this header.
#ifndef CRESOURCEMANAGER_V2_H
#define CRESOURCEMANAGER_V2_H

#include <stddef.h>
#include <stdlib.h>
#include <wchar.h>
#include "CResourceManager.h"
#include "CStrWChar.h"
#include "CRegistry.h"
#include "CBigFile_v2.h"
#include "CFileInputStream.h"
#include "CStringToKey.h"

                                                    

                
template <class T> class CSharedPtr;
 

                   

class CEvent;

/** True for names ending in ".big" or ".bar". */
unsigned char CResourceManager_HasBigFileExtension(const char* path);
/** CIdToObjectRouter callbacks; the context is the CResourceManager_v2. */
unsigned char CResourceManager_Handler_KeyToName(void* context, unsigned int key, char** outName);
unsigned char CResourceManager_Handler_KeyToPointer(void* context, unsigned int key, void** outPointer,
                                                    unsigned int& outType);
unsigned char CResourceManager_Handler_KeyToInputStream(void* context, unsigned int key,
                                                        CInputStream& outStream, unsigned int& outType);
unsigned char CResourceManager_Handler_NameToPointer(void* context, const char* name, void** outPointer,
                                                     unsigned int& outType);
unsigned char CResourceManager_Handler_NameToInputStream(void* context, const char* name,
                                                         CInputStream& outStream, unsigned int& outType);

class CResourceManager_v2 : public CResourceManager {
public:
    /** Mime types of archives. */
    enum {
        MimeType_BigFile = 0x5ed4e9cb,
        MimeType_BigFileAlt = 0x69e4c505
    };

    /** Runtime class id stored at the end of the constructor. */
    enum { ClassId = 0xdf375fa7 };

    /** Stream slots for loose files (used round robin). */
    enum { FileStreamCount = 5 };

    /** Iterator over the name table of one archive (created by CreateAndInitConsecutiveResourceIdItr). */
    class CConsecutiveResourceIdItr : public CResourceManager::CConsecutiveResourceIdItr {
    public:
        CConsecutiveResourceIdItr()
        {
            m_names.m_bigFile = NULL;
            m_names.m_index = 0;
            m_names.m_count = 0;
            m_names.m_current = NULL;
        }

        virtual unsigned int GetHandle();
        virtual const char* GetName();
        virtual CConsecutiveResourceIdItr& Next();
        virtual CConsecutiveResourceIdItr& Prev();
        virtual CConsecutiveResourceIdItr& Seek(SeekFrom from, int count);
        virtual CConsecutiveResourceIdItr& operator++();
        virtual CConsecutiveResourceIdItr& operator++(int);
        virtual CConsecutiveResourceIdItr& operator--();
        virtual CConsecutiveResourceIdItr& operator--(int);

        CBigFile_v2::CStringBlockItr m_names;   // 0x004
        char m_nameBuffer[0x100];                           // 0x014 decompressed name
    };

    /** An open archive, registered in m_bigFiles. */
    struct BigFileEntry {
        BigFileEntry(CBigFile_v2* bigFile)
        {
            m_bigFile = bigFile;
        }

        CBigFile_v2* m_bigFile;     // 0x00
        CStrWChar m_name;           // 0x04 resource name of the archive
        CStrWChar m_path;           // 0x14 file the archive is read from
    };

    CResourceManager_v2();
    virtual ~CResourceManager_v2();

    virtual unsigned char Initialize(unsigned long tableSize, unsigned long initialAllocCount, unsigned char growable);
    virtual void Destroy();
    virtual CResourceManager::CConsecutiveResourceIdItr* CreateAndInitConsecutiveResourceIdItr(unsigned int handle,
                                                                                                unsigned int count);
    virtual CResourceManager::CConsecutiveResourceIdItr* CreateAndInitConsecutiveResourceIdItr(const char* name,
                                                                                                unsigned int count);
    virtual CResourceManager::CConsecutiveResourceIdItr* CreateAndInitConsecutiveResourceIdItr(const wchar_t* name,
                                                                                                unsigned int count);
    virtual unsigned char AddResource(CResource& resource);
    virtual unsigned char RemoveResource(unsigned int handle);
    virtual unsigned char RemoveResource(const char* name);
    virtual unsigned char RemoveResource(const wchar_t* name);
    virtual unsigned char RemoveResource(CResource& resource);
    virtual unsigned int CreateResource(unsigned int handle, CResource** resource);
    virtual unsigned char CreateResources(const unsigned int* handles);
    virtual unsigned int CreateResource(const CreationInfoHnd& info, CResource** resource);
    virtual unsigned char CreateResources(const CreationInfoHnd* infos);
    virtual unsigned int CreateResource(const char* name, CResource** resource);
    virtual unsigned char CreateResources(const char** names);
    virtual unsigned int CreateResource(const CreationInfoStr& info, CResource** resource);
    virtual unsigned char CreateResources(const CreationInfoStr* infos);
    virtual unsigned int CreateResource(const wchar_t* name, CResource** resource);
    virtual unsigned char CreateResources(const wchar_t** names);
    virtual unsigned int CreateResource(const CreationInfoWStr& info, CResource** resource);
    virtual unsigned char CreateResources(const CreationInfoWStr* infos);
    virtual unsigned char GetResource(unsigned int handle, CResource** resource);
    virtual unsigned char GetResource(const char* name, CResource** resource);
    virtual unsigned char GetResource(const wchar_t* name, CResource** resource);
    virtual unsigned char ReleaseResource(unsigned int handle);
    virtual unsigned char ReleaseResources(const unsigned int* handles);
    virtual unsigned char ReleaseResource(const char* name);
    virtual unsigned char ReleaseResources(const char** names);
    virtual unsigned char ReleaseResource(const wchar_t* name);
    virtual unsigned char ReleaseResources(const wchar_t** names);
    virtual unsigned char ReleaseResource(CResource** resource);
    virtual unsigned char DestroyResource(unsigned int handle);
    virtual unsigned char DestroyResources(const unsigned int* handles);
    virtual unsigned char DestroyResource(const char* name);
    virtual unsigned char DestroyResources(const char** names);
    virtual unsigned char DestroyResource(const wchar_t* name);
    virtual unsigned char DestroyResources(const wchar_t** names);
    virtual unsigned char DestroyResources();
    virtual void SubstituteResourceInfoInPlaceOfStringData(unsigned char substitute);
    virtual unsigned int CreateResource(CResource** resource, unsigned int handle, const char* name,
                                        unsigned int mimeType, CInputStream* stream);

    /** Open a stream on a resource: archives first, then loose files in the application folders. */
    unsigned char GetStream(const char* name, CInputStream& stream, unsigned int& mimeType,
                            CSystemElement** bigFileElement, const char** storedName, unsigned char& nameDecompressed);
    /** Open a stream on a resource of the first registered archive holding it. */
    unsigned char SearchBigFileRegistryForStream(unsigned int handle, CInputStream& stream,
                                                 unsigned int& mimeType, CSystemElement** bigFileElement,
                                                 const char** storedName, unsigned char& nameDecompressed);
    unsigned char SearchBigFileRegistryForStream(const char* name, CInputStream& stream,
                                                 unsigned int& mimeType, CSystemElement** bigFileElement,
                                                 const char** storedName, unsigned char& nameDecompressed);
    /** `name` converted into m_nameBuffer. */
    inline const char* ToNarrowName(const wchar_t* name)
    {
        char* buffer = m_nameBuffer;
        wcstombs(buffer, name, wcslen(name) + 1);
        return buffer;
    }

    /** Give every resource of the chain that came from `source` a private name copy. */
    void RemoveSourceFromAllResourcesOfSameHandleAndOfSpecificSource(CResource* resource, void* source);
    /** Delete the resource and the rest of its chain (last first). */
    void DestroyIncludingAllResourcesOfSameHandle(CResource* resource);

    CStrWChar m_appPath;            // 0x024 folder searched for loose files, with separator
    CBigFile_v2* m_currentBigFile;  // 0x034 archive of the resource being created
    CRegistry m_bigFiles;                       // 0x038 BigFileEntry objects
    int m_reserved88;                           // 0x088 cleared by Initialize / Destroy, never read
    char m_nameBuffer[0x100];                   // 0x08c wide -> narrow name conversion
    wchar_t m_wideBuffer[0x100];                // 0x18c not used by this unit (size from the layout)
    unsigned char m_substituteResourceInfo;     // 0x58c string resources get their id instead of their data
    int m_nextFileStream;                       // 0x590 next slot of m_fileStreams
    CFileInputStream m_fileStreams[FileStreamCount];   // 0x594 (object size 0x65c)
};

inline unsigned int CResourceManager_v2::CConsecutiveResourceIdItr::GetHandle()
{
    // named handles have the top bit clear here (see CHandle)
    return CStringToKey(CConsecutiveResourceIdItr::GetName(), 0) & 0x7fffffff;
}

inline const char* CResourceManager_v2::CConsecutiveResourceIdItr::GetName()
{
    const char* name = NULL;
    if (m_names.m_current != NULL) {
        if (m_names.m_bigFile == NULL) {
            name = m_names.m_current;
        } else if (m_names.m_bigFile->DecompressIntoDestinationBufferIfNeeded(m_nameBuffer, m_names.m_current)) {
            name = m_nameBuffer;
        } else {
            name = m_names.m_current;
        }
    }
    return name;
}

inline CResourceManager_v2::CConsecutiveResourceIdItr& CResourceManager_v2::CConsecutiveResourceIdItr::Next()
{
    m_names++;
    return *this;
}

inline CResourceManager_v2::CConsecutiveResourceIdItr& CResourceManager_v2::CConsecutiveResourceIdItr::Prev()
{
    m_names--;
    return *this;
}

inline CResourceManager_v2::CConsecutiveResourceIdItr& CResourceManager_v2::CConsecutiveResourceIdItr::Seek(SeekFrom from,
                                                                                                          int count)
{
    if (from != SeekFrom_Start) {
        if (count > 0) {
            for (; count > 0; count--) {
                m_names++;
            }
        } else {
            for (; count != 0; count++) {
                m_names--;
            }
        }
    }
    return *this;
}

inline CResourceManager_v2::CConsecutiveResourceIdItr& CResourceManager_v2::CConsecutiveResourceIdItr::operator++()
{
    m_names++;
    return *this;
}

inline CResourceManager_v2::CConsecutiveResourceIdItr& CResourceManager_v2::CConsecutiveResourceIdItr::operator++(int)
{
    m_names++;
    return *this;
}

inline CResourceManager_v2::CConsecutiveResourceIdItr& CResourceManager_v2::CConsecutiveResourceIdItr::operator--()
{
    m_names--;
    return *this;
}

inline CResourceManager_v2::CConsecutiveResourceIdItr& CResourceManager_v2::CConsecutiveResourceIdItr::operator--(int)
{
    m_names--;
    return *this;
}

       

#endif
