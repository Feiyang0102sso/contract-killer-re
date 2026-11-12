// CResourceAggregate.cpp - lifetime and stream loading of an aggregate resource.
#include "CResourceAggregate.h"
#include "CAggregateResource.h"

CResourceAggregate::CResourceAggregate()
    : m_aggregate(NULL)
{
    m_classId = ClassId;
    CResource::ResetMembers();
}

CResourceAggregate::~CResourceAggregate()
{
    CResourceAggregate::Destroy();
}

void CResourceAggregate::Destroy()
{
    if (m_aggregate != NULL) {
        delete m_aggregate;
        m_aggregate = NULL;
    }
}

void CResourceAggregate::ResetMembers()
{
    CResource::ResetMembers();
}

unsigned int CResourceAggregate::GetSize() const
{
    if (m_aggregate == NULL) {
        return 0;
    }
    return sizeof(CAggregateResource);
}

void* CResourceAggregate::GetData() const
{
    return m_aggregate;
}

unsigned int CResourceAggregate::CreateInternal(unsigned int handle, const char* name,
                                                CInputStream* stream, unsigned int mimeType,
                                                CIdToObjectRouter* router, unsigned char copyName)
{
    m_handle = handle;
    m_aggregate = new CAggregateResource();
    unsigned int result = CreateResult_Created;
    if (stream != NULL) {
        if (!m_aggregate->Load(*stream, mimeType)) {
            Destroy();
            return CreateResult_Failed;
        }
        result = CreateResult_Loaded;
    }
    SetName(name, copyName);
    m_isCreated = 1;
    return result;
}
