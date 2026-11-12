// CResourceBigFile.h - resource wrapping a CBigFile. (created by b011x)
#ifndef CRESOURCEBIGFILE_H
#define CRESOURCEBIGFILE_H

#include "CResource.h"
#include "CBigFile_v2.h"

                                                                        

class CResourceBigFile : public CResource {
public:
    /** Runtime class id: CStringToKey("CResourceBigFile"). */
    enum { ClassId = 0x70d68df4 };

    CResourceBigFile();
    virtual ~CResourceBigFile();

    virtual void Destroy();
    virtual unsigned int GetSize() const;
    virtual void* GetData() const;
    virtual unsigned int CreateInternal(unsigned int handle, const char* name, CInputStream* stream,
                                        unsigned int mimeType, CIdToObjectRouter* router,
                                        unsigned char copyName);
    virtual void ResetMembers();

    CBigFile* m_bigFile; // 0x24
};

       

#endif
