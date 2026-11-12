// CResourceRenderSurface.h - resource wrapping a ICRenderSurface. (created by b011x)
#ifndef CRESOURCERENDERSURFACE_H
#define CRESOURCERENDERSURFACE_H

#include "CResource.h"
#include "ICRenderSurface.h"

                                                                        

class CResourceRenderSurface : public CResource {
public:
    /** Runtime class id: CStringToKey("CResourceRenderSurface"). */
    enum { ClassId = 0x23441af5 };

    CResourceRenderSurface();
    virtual ~CResourceRenderSurface();

    virtual void Destroy();
    virtual unsigned int GetSize() const;
    virtual void* GetData() const;
    virtual unsigned int CreateInternal(unsigned int handle, const char* name, CInputStream* stream,
                                        unsigned int mimeType, CIdToObjectRouter* router,
                                        unsigned char copyName);
    virtual void ResetMembers();

    ICRenderSurface* m_surface; // 0x24
};

       

#endif
