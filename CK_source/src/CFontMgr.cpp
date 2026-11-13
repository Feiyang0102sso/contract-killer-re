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
    CFont*& cached = m_fonts[index];
    CFont* font = cached;
    if (font != NULL) {
        const GameFontInfo* info = kTableGameFontInfo + index;
        if (info != NULL) {
            CApplet::m_pApp->m_resourceManager->ReleaseResource(info->m_image);
            font = cached;
            if (font == NULL) {
                return;
            }
        }
        delete font;
        cached = NULL;
    }
}

CFont* CFontMgr::GetFont(int index)
{
    CFont*& cached = m_fonts[index];
    CFont* result = cached;
    if (result == NULL) {
        const GameFontInfo* info = kTableGameFontInfo + index;
        if (info != NULL) {
            CGraphicFont* font = new CGraphicFont;
            if (font != NULL) {
                CResourceManager* resources = CApplet::m_pApp->m_resourceManager;
                CResource* resource = NULL;
                resources->CreateResource(info->m_image, &resource);
                ICRenderSurface* atlas = static_cast<ICRenderSurface*>(resource->GetData());
                resources->CreateResource(kTableGameFontInfo[index].m_metrics, &resource);
                CBinary* metrics = static_cast<CBinary*>(resource->GetData());
                font->SetFontImage(atlas);
                font->ParseFontMetrics(metrics->m_data, metrics->m_size);
                cached = font;
                resources->ReleaseResource(kTableGameFontInfo[index].m_metrics);
                result = font;
            }
        }
    }
    return result;
}
