// b074: complete surface-cache owner, original 0x119534..0x119a50.
#include "CBH_SurfaceManager.h"
#include "ICRenderSurface.h"
#include "ICGraphics.h"
#include "ICGraphics2d.h"
#include "CArrayInputStream.h"
#include "CInputStream.h"
#include "CResourceManager_v2.h"
#include "CApplet.h"
#include "CApp.h"





ICRenderSurface* CBH_SurfaceManager::CreateSurface(const char* name, bool keep, bool rgb565)
{
    if (ImageInCache(name) == 1) {
        CachedSurface* cached = m_surfaces.Get(XString(name));
        ++cached->m_references;
        return cached->m_surface;
    }
    CInputStream stream;
    unsigned int mimeType;
    CSystemElement* element;
    unsigned char nameDecompressed;
    CResourceManager_v2* resources = static_cast<CResourceManager_v2*>(CApplet::m_pApp->m_resourceManager);
    if (!resources->GetStream(name, stream, mimeType, &element, NULL, nameDecompressed)) {
        return NULL;
    }
    ICGraphics::Abstraction abstraction = ICGraphics::GetInstance()->GetDefaultActiveAbstractionLayer();
    ICRenderSurface* surface = ICRenderSurface::CreateInstance(abstraction, ICRenderSurface::Type_Default,
                                                             ICRenderSurface::Targetability_0);
    if (surface == NULL) {
        return NULL;
    }
    ICRenderSurface::InitParam normalParams[] = {
        {15, mimeType}, {14, (unsigned int)&stream}, {16, 1}, {0, 0}, {0, 0}
    };
    ICRenderSurface::InitParam rgb565Params[] = {
        {6, 0x21302}, {15, mimeType}, {14, (unsigned int)&stream}, {0, 0}, {0, 0}
    };
    ICRenderSurface::InitParam* params = normalParams;
    if (rgb565) {
        params = rgb565Params;
    }
    if (abstraction == ICGraphics::Abstraction_Hardware) {
        params[3].m_key = 9;
        params[3].m_value = 0x7c1f;
    }
    if (!surface->Initialize(params)) {
        delete surface;
        return NULL;
    }
    if (keep == 1) {
        CachedSurface cached;
        cached.m_surface = surface;
        cached.m_name = name;
        cached.m_references = 1;
        m_surfaces.Add(XString(name), cached);
    }
    return surface;
}

ICRenderSurface* CBH_SurfaceManager::CreateSurface(const char* name, bool keep)
{
    return CreateSurface(name, keep, false);
}

ICRenderSurface* CBH_SurfaceManager::CreateSurface565(const char* name, bool keep)
{
    return CreateSurface(name, keep, true);
}

ICRenderSurface* CBH_SurfaceManager::CreateSurface(const void* data, unsigned int size, unsigned int mimeType)
{
    CArrayInputStream stream;
    stream.Open(static_cast<const unsigned char*>(data), size);
    ICGraphics::Abstraction abstraction = ICGraphics2d::GetInstance()->GetActiveAbstractionLayer();
    ICRenderSurface* surface = ICRenderSurface::CreateInstance(abstraction, ICRenderSurface::Type_Default,
                                                             ICRenderSurface::Targetability_0);
    if (surface == NULL) {
        return NULL;
    }
    ICRenderSurface::InitParam params[] = {
        {6, 0}, {4, 0}, {10, 0}, {15, mimeType}, {14, (unsigned int)&stream}, {0, 0}, {0, 0}
    };
    if (abstraction == ICGraphics::Abstraction_Hardware) {
        params[5].m_key = 9;
        params[5].m_value = 0xffff00ff;
    }
    if (!surface->Initialize(params)) {
        delete surface;
        return NULL;
    }
    return surface;
}

bool CBH_SurfaceManager::ImageInCache(const char* name)
{
    if (name == NULL) {
        return false;
    }
    CachedSurface* cached = m_surfaces.Get(XString(name));
    return cached != NULL;
}

unsigned char CBH_SurfaceManager::IsImageExists(const char* name)
{
    if (ImageInCache(name)) {
        return 1;
    }
    CInputStream stream;
    unsigned int mimeType;
    const char* storedName;
    unsigned char nameDecompressed;
    return ((CResourceManager_v2*)CApplet::m_pApp->m_resourceManager)->GetStream(name, stream, mimeType, NULL, &storedName, nameDecompressed);
}

bool CBH_SurfaceManager::ReleaseSurface(const char* name)
{
    if (ImageInCache(name) != 1) {
        return false;
    }
    CachedSurface* cached = m_surfaces.Get(XString(name));
    --cached->m_references;
    if (cached->m_references != 0) {
        return false;
    }
    ICRenderSurface* surface = m_surfaces.Get(XString(name))->m_surface;
    delete surface;
    m_surfaces.Remove(XString(name));
    return true;
}

bool CBH_SurfaceManager::ReleaseSurface(ICRenderSurface* surface)
{
    for (int index = 0; index < m_surfaces.m_count; ++index) {
        XString* key = NULL;
        CachedSurface* cached = m_surfaces.GetAt(index, &key);
        if (cached->m_surface == surface) {
            XString::AnsiString name(key->m_data);
            return ReleaseSurface(name.m_text);
        }
    }
    return false;
}

unsigned int CBH_SurfaceManager::GetSurfaceWidth(ICRenderSurface* surface)
{
    if (surface == NULL) {
        return 0;
    }
    unsigned int width;
    unsigned int height;
    surface->GetWidthAndHeight(width, height);
    return width;
}

unsigned int CBH_SurfaceManager::GetSurfaceHeight(ICRenderSurface* surface)
{
    unsigned int width;
    unsigned int height;
    surface->GetWidthAndHeight(width, height);
    return height;
}

void CBH_SurfaceManager::Clear()
{
    // The shipped owner clears entries only; surface lifetime is not changed here.
    m_surfaces.Clear();
}

ICRenderSurface* CBH_SurfaceManager::GetFromCache(const char* name)
{
    if (ImageInCache(name) != 1) {
        return NULL;
    }
    CachedSurface* cached = m_surfaces.Get(XString(name));
    return cached->m_surface;
}
