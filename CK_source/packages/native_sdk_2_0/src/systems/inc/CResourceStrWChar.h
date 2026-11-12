// CResourceStrWChar.h - resource wrapping a CStrWChar. (created by b011x)
#ifndef CRESOURCESTRWCHAR_H
#define CRESOURCESTRWCHAR_H

#include "CResource.h"
#include "CStrWChar.h"

                                                                        

class CResourceStrWChar : public CResource {
public:
    /** Runtime class id: CStringToKey("CResourceStrWChar"). */
    enum { ClassId = 0x012b7b64 };

    CResourceStrWChar();
    virtual ~CResourceStrWChar();

    virtual void Destroy();
    virtual unsigned int GetSize() const;
    virtual void* GetData() const;
    virtual unsigned int CreateInternal(unsigned int handle, const char* name, CInputStream* stream,
                                        unsigned int mimeType, CIdToObjectRouter* router,
                                        unsigned char copyName);
    virtual void ResetMembers();

    CStrWChar* m_string; // 0x24
};

       

#endif
