// CResource.h - base of the managed resources: a named, reference-counted wrapper around one
// loaded object (CBinary, CDIB, CMedia ...). (created by b011x)
// Resources with the same handle form a chain (m_nextSameHandle) in the resource manager.
#ifndef CRESOURCE_H
#define CRESOURCE_H

#include <stddef.h>
#include "CPriorityClass.h"

                                                    

                      
class CInputStream;
class CIdToObjectRouter;
 

                   

class CResourceManager;

class CResource : public CPriorityClass {
public:
    /** Runtime class id: CStringToKey("CResource"). */
    enum { ClassId = 0x42582483 };

    /** CreateInternal() results. */
    enum CreateResult {
        CreateResult_Failed = 0,    // nothing usable, the resource destroyed its object
        CreateResult_Created = 1,   // object created, nothing loaded (no stream)
        CreateResult_Loaded = 3     // object created and loaded from the stream
    };

    CResource();
    virtual ~CResource();

    /** Release the wrapped object. */
    virtual void Destroy() = 0;
    /** Bytes used by the wrapped object (0 when there is none). */
    virtual unsigned int GetSize() const = 0;
    /** The wrapped object. */
    virtual void* GetData() const = 0;
    /**
     * Create the wrapped object for `mimeType`, load it from `stream` when there is one and
     * take `name` (copied when `copyName`). Returns a CreateResult.
     */
    virtual unsigned int CreateInternal(unsigned int handle, const char* name, CInputStream* stream,
                                        unsigned int mimeType, CIdToObjectRouter* router,
                                        unsigned char copyName) = 0;

    /** Forget the name, the handle, the chain and the reference count. */
    virtual void ResetMembers()
    {
        DestroyName();
        m_source = NULL;
        m_nextSameHandle = NULL;
        m_handle = 0;
        m_refCount = 0;
        m_priority = DefaultPriority;
    }

    /** Free the name when it is owned. */
    void DestroyName();
    /**
     * Create the wrapped object (no name, no router) and register with the resource manager.
     * Returns the CreateResult, or 0 after Destroy() when the manager refuses it.
     */
    unsigned int Create(unsigned int handle, CInputStream* stream, unsigned int mimeType);
    /** Use `name` (a copy of it when `copy`; the caller's pointer when copying fails). */
    void SetName(const char* name, unsigned char copy);

    void* m_source;                 // 0x0c archive registry entry the resource came from
    char* m_name;                   // 0x10
    CResource* m_nextSameHandle;    // 0x14 next resource with the same handle
    unsigned int m_handle;          // 0x18
    unsigned short m_refCount;      // 0x1c
    unsigned char m_isCreated;      // 0x1e CreateInternal() succeeded
    unsigned char m_ownsName;       // 0x1f m_name is a private copy
    CResourceManager* m_manager;    // 0x20 manager the resource is registered with
};

       

#endif
