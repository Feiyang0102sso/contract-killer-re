// CResourceBinary.h - resource wrapping a CBinary. (created by b011x)
#ifndef CRESOURCEBINARY_H
#define CRESOURCEBINARY_H

#include "CResource.h"
#include "CBinary.h"

                                                                        

class CResourceBinary : public CResource {
public:
    /** Runtime class id: CStringToKey("CResourceBinary"). */
    enum { ClassId = 0xe70dde7d };

    CResourceBinary();
    virtual ~CResourceBinary();

    virtual void Destroy();
    virtual unsigned int GetSize() const;
    virtual void* GetData() const;
    virtual unsigned int CreateInternal(unsigned int handle, const char* name, CInputStream* stream,
                                        unsigned int mimeType, CIdToObjectRouter* router,
                                        unsigned char copyName);
    virtual void ResetMembers();

    CBinary* m_binary; // 0x24
};

       

#endif
