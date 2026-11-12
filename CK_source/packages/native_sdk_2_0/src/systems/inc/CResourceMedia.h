// CResourceMedia.h - resource wrapping a CMedia. (created by b011x)
#ifndef CRESOURCEMEDIA_H
#define CRESOURCEMEDIA_H

#include "CResource.h"
#include "CMedia.h"

                                                                        

class CResourceMedia : public CResource {
public:
    /** Runtime class id: CStringToKey("CResourceMedia"). */
    enum { ClassId = 0x4f7f1773 };

    CResourceMedia();
    virtual ~CResourceMedia();

    virtual void Destroy();
    virtual unsigned int GetSize() const;
    virtual void* GetData() const;
    virtual unsigned int CreateInternal(unsigned int handle, const char* name, CInputStream* stream,
                                        unsigned int mimeType, CIdToObjectRouter* router,
                                        unsigned char copyName);
    virtual void ResetMembers();

    CMedia* m_media; // 0x24
};

       

#endif
