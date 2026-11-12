// CResourceShader.h - resource wrapping a ICShader. (created by b011x)
#ifndef CRESOURCESHADER_H
#define CRESOURCESHADER_H

#include "CResource.h"
#include "ICShader.h"

                                                                        

class CResourceShader : public CResource {
public:
    /** Runtime class id: CStringToKey("CResourceShader"). */
    enum { ClassId = 0xe61c2a06 };

    CResourceShader();
    virtual ~CResourceShader();

    virtual void Destroy();
    virtual unsigned int GetSize() const;
    virtual void* GetData() const;
    virtual unsigned int CreateInternal(unsigned int handle, const char* name, CInputStream* stream,
                                        unsigned int mimeType, CIdToObjectRouter* router,
                                        unsigned char copyName);
    virtual void ResetMembers();

    ICShader* m_shader; // 0x24
};

       

#endif
