// CResourceDIB.h - resource wrapping a CDIB. (created by b011x)
#ifndef CRESOURCEDIB_H
#define CRESOURCEDIB_H

#include "CResource.h"
#include "CDIB.h"

                                                                        

class CResourceDIB : public CResource {
public:
    /** Runtime class id: CStringToKey("CResourceDIB"). */
    enum { ClassId = 0x824d74f7 };

    CResourceDIB();
    virtual ~CResourceDIB();

    virtual void Destroy();
    virtual unsigned int GetSize() const;
    virtual void* GetData() const;
    virtual unsigned int CreateInternal(unsigned int handle, const char* name, CInputStream* stream,
                                        unsigned int mimeType, CIdToObjectRouter* router,
                                        unsigned char copyName);
    virtual void ResetMembers();

    CDIB* m_dib;       // 0x24
};

       

#endif
