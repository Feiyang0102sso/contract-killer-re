// CResourceMedia.cpp - resource wrapping a CMedia.
#include "CResourceMedia.h"

#include <stdlib.h>
#include <string.h>
#include <wchar.h>

                                                                        

CResourceMedia::CResourceMedia()
{
    m_classId = ClassId;
    m_media = NULL;
    CResource::ResetMembers();
}

CResourceMedia::~CResourceMedia()
{
    CResourceMedia::Destroy();
}

void CResourceMedia::Destroy()
{
    if (m_media != NULL) {
        delete m_media;
        m_media = NULL;
    }
}

void CResourceMedia::ResetMembers()
{
    CResource::ResetMembers();
}

unsigned int CResourceMedia::GetSize() const
{
    if (m_media == NULL) {
        return 0;
    }
    return m_media->m_size + sizeof(CMedia);
}

void* CResourceMedia::GetData() const
{
    return m_media;
}

unsigned int CResourceMedia::CreateInternal(unsigned int handle, const char* name, CInputStream* stream,
                                            unsigned int mimeType, CIdToObjectRouter* router,
                                            unsigned char copyName)
{
    m_handle = handle;
    m_media = new CMedia();
    unsigned int result = CreateResult_Created;
    if (stream != NULL) {
        // ?: kept on purpose: the original selects the result instead of branching
        result = m_media->Load(*stream, mimeType) ? CreateResult_Loaded : CreateResult_Failed;
    }
    if (result != CreateResult_Failed) {
        SetName(name, copyName);
        m_isCreated = 1;
    } else {
        Destroy();
        result = CreateResult_Failed;
    }
    return result;
}

       
