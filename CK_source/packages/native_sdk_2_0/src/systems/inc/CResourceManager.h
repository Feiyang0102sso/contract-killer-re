// CResourceManager.h - common part of the resource managers: the handle -> CResource hash and
// the id router. (first drafted by b000x as PROVISIONAL with the iterator interface; completed by
// b011x, the owner)
// plat 2026-11-10: CK 1.5.2 has a single base class CResourceManager (no ICResourceManager
// interface, no CSingleton, no version, no debug listener); layout and vtable from the original
// CResourceManager.o / CResourceManager_v2.o (vtable 0x33bb00, 43 used slots).
#ifndef CRESOURCEMANAGER_H
#define CRESOURCEMANAGER_H

#include <stddef.h>
#include <wchar.h>
#include "CPriorityClass.h"
#include "CApplet.h"
#include "CApp.h"
#include "CHash.h"




class CIdToObjectRouter;
class CInputStream;




class CResource;

class CResourceManager : public CPriorityClass {
public:
    /** Runtime class id stored by the constructor (derived managers overwrite it). */
    enum { ClassId = 0x7f5df373 };

    /** Default Initialize() arguments: hash buckets and initially allocated hash nodes. */
    static const unsigned long DefaultTableSize;
    static const unsigned long DefaultNumOfElementsInInitialAlloc;

    /** Walks resources whose ids follow each other in the archive name table. */
    class CConsecutiveResourceIdItr {
    public:
        /** Seek() origin; only a non-zero origin moves (meaning of the values inferred). */
        enum SeekFrom {
            SeekFrom_Start = 0,
            SeekFrom_Current = 1
        };

        /** Handle (name key) of the current resource. */
        virtual unsigned int GetHandle() = 0;
        /** Name of the current resource, NULL past the end. */
        virtual const char* GetName() = 0;
        virtual CConsecutiveResourceIdItr& Next() = 0;
        virtual CConsecutiveResourceIdItr& Prev() = 0;
        virtual CConsecutiveResourceIdItr& Seek(SeekFrom from, int count) = 0;
        virtual CConsecutiveResourceIdItr& operator++() = 0;
        virtual CConsecutiveResourceIdItr& operator++(int) = 0;
        virtual CConsecutiveResourceIdItr& operator--() = 0;
        virtual CConsecutiveResourceIdItr& operator--(int) = 0;
    };

    /** CreateResources() list entries; a list ends with a zero handle / NULL name. */
    struct CreationInfoHnd {
        unsigned int m_mimeType;                // 0x00
        CInputStream* m_stream;     // 0x04
        unsigned int m_handle;                  // 0x08
    };

    struct CreationInfoStr {
        unsigned int m_mimeType;                // 0x00
        CInputStream* m_stream;     // 0x04
        const char* m_name;                     // 0x08
    };

    struct CreationInfoWStr {
        unsigned int m_mimeType;                // 0x00
        CInputStream* m_stream;     // 0x04
        const wchar_t* m_name;                  // 0x08
    };

    /** The application's resource manager (created by CGameApp::OnInit, cached at CApp+0x10 in CK). */
    static inline CResourceManager* GetInstance()
    {
        return (CResourceManager*)CApplet::m_pApp->m_resourceManager;
    }

    inline CResourceManager()
    {
        m_classId = ClassId;
        m_router = NULL;
    }

    virtual ~CResourceManager();

    virtual unsigned char Initialize(unsigned long tableSize, unsigned long initialAllocCount,
                                     unsigned char growable) = 0;
    virtual void Destroy();
    virtual CConsecutiveResourceIdItr* CreateAndInitConsecutiveResourceIdItr(unsigned int handle,
                                                                             unsigned int count) = 0;
    virtual CConsecutiveResourceIdItr* CreateAndInitConsecutiveResourceIdItr(const char* name,
                                                                             unsigned int count) = 0;
    virtual CConsecutiveResourceIdItr* CreateAndInitConsecutiveResourceIdItr(const wchar_t* name,
                                                                             unsigned int count) = 0;
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
    /** Find a created resource; *resource is NULL when there is none. */
    virtual unsigned char GetResource(unsigned int handle, CResource** resource);
    virtual unsigned char GetResource(const char* name, CResource** resource);
    virtual unsigned char GetResource(const wchar_t* name, CResource** resource);
    virtual unsigned char ReleaseResource(unsigned int handle);
    virtual unsigned char ReleaseResources(const unsigned int* handles);
    virtual unsigned char ReleaseResource(const char* name);
    virtual unsigned char ReleaseResources(const char** names);
    virtual unsigned char ReleaseResource(const wchar_t* name);
    virtual unsigned char ReleaseResources(const wchar_t** names);
    /** Drop one reference; the resource is deleted and *resource cleared at zero. */
    virtual unsigned char ReleaseResource(CResource** resource);
    virtual unsigned char DestroyResource(unsigned int handle);
    virtual unsigned char DestroyResources(const unsigned int* handles);
    virtual unsigned char DestroyResource(const char* name);
    virtual unsigned char DestroyResources(const char** names);
    virtual unsigned char DestroyResource(const wchar_t* name);
    virtual unsigned char DestroyResources(const wchar_t** names);
    virtual unsigned char DestroyResources();
    virtual void SubstituteResourceInfoInPlaceOfStringData(unsigned char substitute) = 0;
    /** Create (or reference) the resource `handle` / `name` of `mimeType`, loading it from `stream`. */
    virtual unsigned int CreateResource(CResource** resource, unsigned int handle, const char* name,
                                        unsigned int mimeType, CInputStream* stream) = 0;

    /**
     * PORT COMPAT (not in CK 1.5.2): the CKZ interface reported an archive version; only the
     * ported CGraphics_OGLES.cpp still calls it. Remove once that caller is brought to CK.
     */
    inline void GetVersion(unsigned int& major, unsigned int& minor) const
    {
        major = 0;
        minor = 0;
    }

    CHash m_resources;              // 0x0c handle -> first CResource of the handle
    CIdToObjectRouter* m_router;    // 0x20 resolves resource references while loading
};



#endif
