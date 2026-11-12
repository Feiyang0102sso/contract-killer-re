// CResourcePalette.h - resource wrapping a Color_Palette. (created by b011x)
#ifndef CRESOURCEPALETTE_H
#define CRESOURCEPALETTE_H

#include "CResource.h"
#include "CColor.h"

                                                                        

class CResourcePalette : public CResource {
public:
    /** Runtime class id: CStringToKey("CResourcePalette"). */
    enum { ClassId = 0x625fa174 };

    CResourcePalette();
    virtual ~CResourcePalette();

    virtual void Destroy();
    virtual unsigned int GetSize() const;
    virtual void* GetData() const;
    virtual unsigned int CreateInternal(unsigned int handle, const char* name, CInputStream* stream,
                                        unsigned int mimeType, CIdToObjectRouter* router,
                                        unsigned char copyName);
    virtual void ResetMembers();

    Color_Palette* m_palette; // 0x24
};

       

#endif
