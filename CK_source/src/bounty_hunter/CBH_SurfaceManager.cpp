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





ICRenderSurface* CBH_SurfaceManager::CreateSurface(const char* name, bool keep)
{
    if (ImageInCache(name) == 1) {
        CachedSurface* cached = m_surfaces.Get(XString(name));
        ++cached->m_references;
        return cached->m_surface;
    }
    // Original: no failure checks; a missing resource still yields (and caches) a surface.
    ICGraphics::Abstraction abstraction = ICGraphics::GetInstance()->GetDefaultActiveAbstractionLayer();
    ICRenderSurface* surface = ICRenderSurface::CreateInstance(abstraction, ICRenderSurface::Type_Default,
                                                             ICRenderSurface::Targetability_0);
    CSystemElement* element;
    CInputStream stream;
    unsigned int mimeType;
    unsigned char nameDecompressed;
    static_cast<CResourceManager_v2*>(CApplet::m_pApp->m_resourceManager)
        ->GetStream(name, stream, mimeType, &element, NULL, nameDecompressed);
    ICRenderSurface::InitParam params[] = {
        {15, mimeType}, {14, (unsigned int)&stream}, {16, 1}, {0, 0}, {0, 0}, {0, 0}
    };
    if (abstraction == ICGraphics::Abstraction_Hardware) {
        params[3].m_key = 9;
        params[3].m_value = 0x7c1f;
    }
    surface->Initialize(params);
    if (keep) {
        CachedSurface cached;
        cached.m_name = name;
        cached.m_surface = surface;
        cached.m_references = 1;
        m_surfaces.Add(XString(name), cached);
    }
    return surface;
}

ICRenderSurface* CBH_SurfaceManager::CreateSurface565(const char* name, bool keep)
{
    if (ImageInCache(name) == 1) {
        CachedSurface* cached = m_surfaces.Get(XString(name));
        ++cached->m_references;
        return cached->m_surface;
    }
    ICGraphics::Abstraction abstraction = ICGraphics::GetInstance()->GetDefaultActiveAbstractionLayer();
    ICRenderSurface* surface = ICRenderSurface::CreateInstance(abstraction, ICRenderSurface::Type_Default,
                                                             ICRenderSurface::Targetability_0);
    CSystemElement* element;
    CInputStream stream;
    unsigned int mimeType;
    unsigned char nameDecompressed;
    static_cast<CResourceManager_v2*>(CApplet::m_pApp->m_resourceManager)
        ->GetStream(name, stream, mimeType, &element, NULL, nameDecompressed);
    ICRenderSurface::InitParam params[] = {
        {6, 0x21302}, {15, mimeType}, {14, (unsigned int)&stream}, {0, 0}, {0, 0}, {0, 0}
    };
    if (abstraction == ICGraphics::Abstraction_Hardware) {
        params[3].m_key = 9;
        params[3].m_value = 0x7c1f;
    }
    surface->Initialize(params);
    if (keep) {
        CachedSurface cached;
        cached.m_name = name;
        cached.m_surface = surface;
        cached.m_references = 1;
        m_surfaces.Add(XString(name), cached);
    }
    return surface;
}

ICRenderSurface* CBH_SurfaceManager::CreateSurface(const void* data, unsigned int size, unsigned int mimeType)
{
    CArrayInputStream stream;
    stream.Open(static_cast<const unsigned char*>(data), size);
    ICGraphics::Abstraction abstraction = ICGraphics2d::GetInstance()->GetActiveAbstractionLayer();
    ICRenderSurface* surface = ICRenderSurface::CreateInstance(abstraction, ICRenderSurface::Type_Default,
                                                             ICRenderSurface::Targetability_0);
    // Original: two parameter lists (hardware adds the color key), no failure checks.
    ICRenderSurface::InitParam softwareParams[] = {
        {6, 0}, {4, 0}, {10, 0}, {15, mimeType}, {14, (unsigned int)&stream}, {0, 0}
    };
    ICRenderSurface::InitParam hardwareParams[] = {
        {6, 0}, {4, 0}, {10, 0}, {15, mimeType}, {14, (unsigned int)&stream}, {9, 0xffff00ff}, {0, 0}
    };
    ICRenderSurface::InitParam* params = softwareParams;
    if (abstraction == ICGraphics::Abstraction_Hardware) {
        params = hardwareParams;
    }
    surface->Initialize(params);
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
    CSystemElement* source;
    CInputStream stream;
    unsigned int mimeType;
    unsigned char nameDecompressed;
    return ((CResourceManager_v2*)CApplet::m_pApp->m_resourceManager)->GetStream(
        name, stream, mimeType, &source, NULL, nameDecompressed);
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
