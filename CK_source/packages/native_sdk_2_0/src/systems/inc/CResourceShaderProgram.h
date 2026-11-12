// CResourceShaderProgram.h - resource wrapping a ICShaderProgram. (created by b011x)
#ifndef CRESOURCESHADERPROGRAM_H
#define CRESOURCESHADERPROGRAM_H

#include "CResource.h"
#include "ICShaderProgram.h"

                                                                        

class CResourceShaderProgram : public CResource {
public:
    /** Runtime class id: CStringToKey("CResourceShaderProgram"). */
    enum { ClassId = 0x2028c6dd };

    CResourceShaderProgram();
    virtual ~CResourceShaderProgram();

    virtual void Destroy();
    virtual unsigned int GetSize() const;
    virtual void* GetData() const;
    virtual unsigned int CreateInternal(unsigned int handle, const char* name, CInputStream* stream,
                                        unsigned int mimeType, CIdToObjectRouter* router,
                                        unsigned char copyName);
    virtual void ResetMembers();

    ICShaderProgram* m_program; // 0x24
};

       

#endif
