// CResourceRenderSurface.cpp - resource wrapping a ICRenderSurface.
#include "CResourceRenderSurface.h"
#include "ICRenderSurface.h"
#include "ICGraphics.h"

                                                                        

CResourceRenderSurface::CResourceRenderSurface()
{
    m_classId = ClassId;
    m_surface = NULL;
    CResource::ResetMembers();
}

CResourceRenderSurface::~CResourceRenderSurface()
{
    CResourceRenderSurface::Destroy();
}

void CResourceRenderSurface::Destroy()
{
    if (m_surface != NULL) {
        delete m_surface;
        m_surface = NULL;
    }
}

void CResourceRenderSurface::ResetMembers()
{
    CResource::ResetMembers();
}

unsigned int CResourceRenderSurface::GetSize() const
{
    if (m_surface == NULL) {
        return 0;
    }
    return 8;
}

void* CResourceRenderSurface::GetData() const
{
    return m_surface;
}

unsigned int CResourceRenderSurface::CreateInternal(unsigned int handle, const char* name, CInputStream* stream,
                                          unsigned int mimeType, CIdToObjectRouter* router,
                                          unsigned char copyName)
{
    m_handle = handle;
    ICGraphics::Abstraction abstraction;
    ICRenderSurface::Targetability targetability;
    unsigned int surfaceType = mimeType;
    unsigned char known = 1;
    // generic types: pick the format of the running renderer
    if (mimeType == 0xfc31810d) {
        ICGraphics::Abstraction current = ICGraphics::GetInstance()->GetDefaultActiveAbstractionLayer();
        if (current == 1) {
            surfaceType = 0xe179f4be;
        } else if (current == 2) {
            surfaceType = 0xe179efbe;
        }
    } else if (mimeType == 0x593f8140) {
        ICGraphics::Abstraction current = ICGraphics::GetInstance()->GetDefaultActiveAbstractionLayer();
        if (current == 1) {
            surfaceType = 0x59f4bc59;
        } else if (current == 2) {
            surfaceType = 0x59efbc42;
        }
    }
    switch (surfaceType) {
    case 0x9f4bc472:
        abstraction = ICGraphics::Abstraction_Hardware;
        targetability = ICRenderSurface::Targetability_3;
        break;
    case 0xe179efbe:
        targetability = ICRenderSurface::Targetability_0;
        abstraction = ICGraphics::Abstraction_Software;
        break;
    case 0xe179f4be:
        abstraction = ICGraphics::Abstraction_Hardware;
        targetability = ICRenderSurface::Targetability_0;
        break;
    case 0x59efbc42:
        abstraction = ICGraphics::Abstraction_Software;
        targetability = ICRenderSurface::Targetability_1;
        break;
    case 0x59f4bc42:
        targetability = ICRenderSurface::Targetability_1;
        abstraction = ICGraphics::Abstraction_Hardware;
        break;
    case 0x59f4bc59:
        abstraction = ICGraphics::Abstraction_Hardware;
        targetability = ICRenderSurface::Targetability_2;
        break;
    default:
        known = 0;
        break;
    }
    if (!known) {
        Destroy();
        return CreateResult_Failed;
    }

    unsigned int result = CreateResult_Failed;
    unsigned char success;
    m_surface = ICRenderSurface::CreateInstance(abstraction, ICRenderSurface::Type_Default, targetability);
    if (m_surface == NULL) {
        success = 1;
    } else if (stream == NULL) {
        success = 1;
        result = CreateResult_Created;
    } else {
        success = 0;
        result = CreateResult_Failed;
        if (m_surface->Load(*stream, surfaceType, router)) {
            result = CreateResult_Loaded;
            success = 1;
        }
    }
    if (name != NULL && success) {
        SetName(name, copyName);
    }
    if (success) {
        m_isCreated = 1;
    } else {
        Destroy();
        result = CreateResult_Failed;
    }
    return result;
}

       
