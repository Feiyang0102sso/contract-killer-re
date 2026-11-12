// CKeysetResource.h - a list of resource keys, optionally with their names resolved.
#ifndef CKEYSETRESOURCE_H
#define CKEYSETRESOURCE_H

#include <stddef.h>

                                                                           

class CInputStream;
class CIdToObjectRouter;

class CKeysetResource {
public:
    CKeysetResource()
    {
        m_count = 0;
        m_keys = NULL;
        m_names = NULL;
    }

    virtual ~CKeysetResource();

    void Destroy();
    /** uint16 count + count uint32 keys; names are looked up when `router` is given. */
    unsigned char Load(CInputStream& stream, unsigned int type, CIdToObjectRouter* router);

    /** Key `index`, NULL when out of range. */
    const char* GetKey(unsigned int index) const
    {
        if (m_names != NULL && index < m_count) {
            return m_names[index];
        }
        return NULL;
    }

    unsigned int m_count;           // 0x04
    unsigned int* m_keys;           // 0x08
    char** m_names;                 // 0x0c NULL when loaded without a router
};

inline CKeysetResource::~CKeysetResource()
{
    Destroy();
}

       

#endif
