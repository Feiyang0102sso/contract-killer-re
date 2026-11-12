#ifndef CRESOURCEAGGREGATE_H
#define CRESOURCEAGGREGATE_H

#include "CResource.h"

class CAggregateResource;

/** Managed wrapper for an aggregate resource. */
class CResourceAggregate : public CResource {
public:
    enum { ClassId = 0x35c14556 };

    CResourceAggregate();
    virtual ~CResourceAggregate();
    virtual void Destroy();
    virtual unsigned int GetSize() const;
    virtual void* GetData() const;
    virtual unsigned int CreateInternal(unsigned int handle, const char* name, CInputStream* stream,
                                        unsigned int mimeType, CIdToObjectRouter* router,
                                        unsigned char copyName);
    virtual void ResetMembers();

private:
    CAggregateResource* m_aggregate;  // 0x24
};

#endif
