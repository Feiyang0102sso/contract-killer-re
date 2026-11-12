// CFontMgr.cpp - load and retain the game's bitmap fonts.
#include "CFontMgr.h"
#include "CGraphicFont.h"
#include "CBinary.h"
#include "CResource.h"
#include "CResourceManager.h"
#include <string.h>

CFontMgr::CFontMgr()
    : CSingleton(SingletonId)
{
    memset(m_fonts, 0, sizeof(m_fonts));
}

CFontMgr::~CFontMgr()
{
    for (int index = 0; index < FontCount; ++index) {
        FreeFont(index);
    }
}

void CFontMgr::FreeFont(int index)
{
    if (index < 0 || index >= FontCount || m_fonts[index] == NULL) {
        return;
    }
    const GameFontInfo& info = kTableGameFontInfo[index];
    CApplet::m_pApp->m_resourceManager->ReleaseResource(info.m_image);
    delete m_fonts[index];
    m_fonts[index] = NULL;
}

CFont* CFontMgr::GetFont(int index)
{
    if (index < 0 || index >= FontCount) {
        return NULL;
    }
    CFont* cached = m_fonts[index];
    if (cached != NULL) {
        return cached;
    }

    CResourceManager* resources = CApplet::m_pApp->m_resourceManager;
    const GameFontInfo& info = kTableGameFontInfo[index];
    CResource* atlasResource = NULL;
    resources->CreateResource(info.m_image, &atlasResource);
    if (atlasResource == NULL) {
        return NULL;
    }
    CResource* metricsResource = NULL;
    resources->CreateResource(info.m_metrics, &metricsResource);
    if (metricsResource == NULL) {
        resources->ReleaseResource(info.m_image);
        return NULL;
    }

    ICRenderSurface* atlas = static_cast<ICRenderSurface*>(atlasResource->GetData());
    CBinary* metrics = static_cast<CBinary*>(metricsResource->GetData());
    CGraphicFont* font = NULL;
    if (atlas != NULL && metrics != NULL) {
        font = new CGraphicFont;
        if (font != NULL) {
            font->SetFontImage(atlas);
            font->ParseFontMetrics(metrics->m_data, metrics->m_size);
            if (font->m_metrics == NULL) {
                delete font;
                font = NULL;
            }
        }
    }
    resources->ReleaseResource(info.m_metrics);
    if (font == NULL) {
        resources->ReleaseResource(info.m_image);
    }
    m_fonts[index] = font;
    return font;
}
