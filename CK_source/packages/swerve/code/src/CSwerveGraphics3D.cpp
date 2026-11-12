// CSwerveGraphics3D.cpp - binds Glu render surfaces as targets of the Swerve renderer and keeps
// the render surfaces made for mutable images. (CK version, written by ggn from the CK reference)
#include <string.h>
#include "CSwerveGraphics3D.h"
#include "CSwerve.h"
#include "CApplet.h"
#include "CApp.h"
#include "ICCore.h"
#include "ICGraphics.h"
#include "ICGraphics2d.h"
#include "ICRenderSurface.h"
#include "CResourceManager.h"
#include "CResource.h"
#include "CArrayInputStream.h"
#include "ICStdUtil.h"

// ---- constants ----
/** swvImage format whose surfaces keep an alpha channel. */
static const unsigned int IMAGE_FORMAT_RGBA = 0x64;
/** Render surface resource types (by the active 2d abstraction layer). */
static const unsigned int MIME_RENDER_SURFACE_LAYER1 = 0x59f4bc59;
static const unsigned int MIME_RENDER_SURFACE_OTHER = 0x59efbc42;
/** Words of the render surface description stream (tag / value pairs). */
static const int SURFACE_PARAMS_WORDS = 12;
/** Color format tag value of RGBA surfaces (other formats leave it 0). */
static const unsigned int SURFACE_COLOR_FORMAT_RGBA = 0xf4404;

CSwerveGraphics3D::CSwerveGraphics3D()
    : m_viewport()
{
    m_graphics3D = NULL;
    m_target = NULL;
    m_previousTarget = NULL;
    m_boundImage = NULL;
    Init();
}

unsigned char CSwerveGraphics3D::Init()
{
    int error = CSwerve::GetInstance()->m_swerve->m_functions->CreateObject(SwerveObjectType_Graphics3D,
                                                                             (void**)&m_graphics3D);
    return error == 0;
}

CSwerveGraphics3D::~CSwerveGraphics3D()
{
    Free();
}

void CSwerveGraphics3D::Free()
{
    int count = m_mutableSurfaces.Size();
    for (int index = 0; index < count; index++) {
        MutableSurface* entry = (MutableSurface*)m_mutableSurfaces.m_data[index];
        CApplet::m_pApp->m_resourceManager->DestroyResource(entry->m_name);
        delete entry;
    }
    if (m_graphics3D != NULL) {
        m_graphics3D->m_functions->Release(m_graphics3D);
    }
    m_graphics3D = NULL;
}

void CSwerveGraphics3D::BindTarget(ICRenderSurface* target, const CRectangle* viewport, unsigned char clear,
                                   int flags)
{
    ICRenderSurface* previousTarget = ICGraphics::GetInstance()->GetTargetSurface();
    m_target = target;
    m_previousTarget = previousTarget;
    target->Bind();

    unsigned int width;
    unsigned int height;
    target->GetWidthAndHeight(width, height);
    if (viewport != NULL) {
        m_viewport.m_x = viewport->m_x;
        m_viewport.m_y = viewport->m_y;
        m_viewport.m_width = viewport->m_width;
        m_viewport.m_height = viewport->m_height;
    } else {
        m_viewport.m_x = 0;
        m_viewport.m_y = 0;
        m_viewport.m_width = width;
        m_viewport.m_height = height;
    }
    m_graphics3D->m_functions->BindTarget(m_graphics3D, width, height, 0, 0, m_viewport.m_x, m_viewport.m_y,
                                          m_viewport.m_width, m_viewport.m_height);
    if (!clear || flags != 0) {
        m_graphics3D->m_functions->SetClearMode(m_graphics3D, clear, flags);
    }

    // Window targets follow the device orientation
    ICRenderSurface::Description description;
    target->GetDescription(description);
    if (description.m_usage == 1) {
        ICCore* core = CApplet::m_pCore;
        if (core->m_deviceOrientationSet) {
            switch (core->m_deviceOrientation) {
            case ICCore::DeviceOrientation_LandscapeRight:
                m_graphics3D->m_functions->SetDisplayRotation(m_graphics3D, 270);
                break;
            case ICCore::DeviceOrientation_PortraitUpsideDown:
                m_graphics3D->m_functions->SetDisplayRotation(m_graphics3D, 180);
                break;
            case ICCore::DeviceOrientation_LandscapeLeft:
                m_graphics3D->m_functions->SetDisplayRotation(m_graphics3D, 90);
                break;
            default:
                break;
            }
        }
    }
}

void CSwerveGraphics3D::ReleaseTarget()
{
    if (m_target != NULL) {
        m_target->UpdateTextureWithTarget();
        m_target = NULL;
        m_previousTarget->Bind();
        m_previousTarget = NULL;
    } else if (m_boundImage != NULL) {
        m_graphics3D->m_functions->BindImageTarget(m_graphics3D, NULL);
        m_boundImage->m_functions->Release(m_boundImage);
        m_boundImage = NULL;
    }
}

ICRenderSurface* CSwerveGraphics3D::GetMutableSurface(swvImage* image)
{
    // a surface at least as large as the image
    int count = m_mutableSurfaces.Size();
    for (int index = 0; index < count; index++) {
        MutableSurface* entry = (MutableSurface*)m_mutableSurfaces.m_data[index];
        unsigned int width;
        unsigned int height;
        entry->m_surface->GetWidthAndHeight(width, height);
        if (width >= (unsigned int)image->m_width && height >= (unsigned int)image->m_height) {
            if (entry->m_surface != NULL) {
                return entry->m_surface;
            }
            break;
        }
    }

    // none: create a render surface resource described by a tag / value stream
    ICRenderSurface* surface = NULL;
    unsigned int colorFormat = 0;
    if (image->m_format == IMAGE_FORMAT_RGBA) {
        colorFormat = SURFACE_COLOR_FORMAT_RGBA;
    }
    unsigned int params[SURFACE_PARAMS_WORDS];
    params[0] = 6;
    params[1] = colorFormat;
    params[2] = 7;
    params[3] = 0x005a0102;
    params[4] = 4;
    params[5] = 0;
    params[6] = 1;
    params[7] = image->m_width;
    params[8] = 2;
    params[9] = image->m_height;
    params[10] = 0;
    params[11] = 0;
    CArrayInputStream stream;
    if (stream.Open((const unsigned char*)params, sizeof(params))) {
        char name[0x10];
        ICStdUtil::SPrintF(name, "SWV_%08X", image->m_pixels);
        CResourceManager::CreationInfoStr info;
        info.m_name = name;
        info.m_stream = &stream;
        if (ICGraphics2d::GetInstance()->GetActiveAbstractionLayer() == 1) {
            info.m_mimeType = MIME_RENDER_SURFACE_LAYER1;
        } else {
            info.m_mimeType = MIME_RENDER_SURFACE_OTHER;
        }
        CResource* resource;
        if (CApplet::m_pApp->m_resourceManager->CreateResource(info, &resource) == CResource::CreateResult_Loaded) {
            ICRenderSurface* created = (ICRenderSurface*)resource->GetData();
            if (created != NULL) {
                surface = created;
                MutableSurface* entry = new MutableSurface();
                strcpy(entry->m_name, name);
                entry->m_surface = created;
                m_mutableSurfaces.Add((int)entry);
            }
        }
    }
    return surface;
}
