// CSystemFont_Cocoa.cpp - system font object, render text cache and drawing.
// The UIKit parts (font creation, measuring, rendering to PNG) are in CSystemFont_Cocoa_mm.mm.
#include "CSystemFont_Cocoa.h"
#include "CArrayInputStream.h"
#include "CHash.h"
#include "ICGraphics2d.h"
#include "ICRenderSurface.h"
#include "ICRenderText2d.h"
#include "ICStdUtil.h"

#include <string.h>
#include <wchar.h>

CHash* g_text2d;

static int HashJava(const wchar_t* text);

ICSystemFont* ICSystemFont::CreateInstance()
{
    return new CSystemFont_Cocoa();
}

CSystemFont_Cocoa::CSystemFont_Cocoa()
{
    m_classId = 0x4231dfda;
    m_antialias = true;
    m_fontColor = 0xffffffff;
    m_backgroundColor = 0x00ff00ff;
    g_text2d = new CHash();
    g_text2d->Init(11, 20, 1);
}

CSystemFont_Cocoa::~CSystemFont_Cocoa()
{
    if (g_text2d != NULL) {
        // release every cached render text before dropping the table
        for (int bucket = 0; bucket < (int)g_text2d->m_bucketCount; bucket++) {
            CHash::Entry* entry = g_text2d->m_buckets[bucket];
            while (entry != NULL) {
                ICRenderText2d* text = (ICRenderText2d*)entry->m_value;
                if (text != NULL) {
                    delete text;
                }
                entry = entry->m_next;
            }
        }
        g_text2d->RemoveAll();
        if (g_text2d != NULL) {
            delete g_text2d;
            g_text2d = NULL;
        }
    }
}

void CSystemFont_Cocoa::Destroy()
{
}

void CSystemFont_Cocoa::SetFontColor(unsigned int color)
{
    m_fontColor = color;
}

void CSystemFont_Cocoa::SetBackgroundColor(unsigned int color)
{
    m_backgroundColor = color;
}

unsigned char CSystemFont_Cocoa::InitializeSurfaceWithPng(const unsigned char* data, unsigned int size,
                                                          ICRenderSurface& surface)
{
    CArrayInputStream stream;
    stream.Open(data, size);
    ICRenderSurface::InitParam params[4];
    params[0].m_key = ICRenderSurface::InitParam_ColorFormat;
    params[0].m_value = 0;
    params[1].m_key = ICRenderSurface::InitParam_SourceStream;
    params[1].m_value = (unsigned int)&stream;
    params[2].m_key = ICRenderSurface::InitParam_SourceType;
    params[2].m_value = 0xb7178678;  // png
    params[3].m_key = ICRenderSurface::InitParam_End;
    params[3].m_value = 0;
    return surface.Initialize(params);
}

int CSystemFont_Cocoa::GetMinCharSpacing() const
{
    return 0;
}

void CSystemFont_Cocoa::PaintText(const wchar_t* text, int length, int x, int y, int width, int height) const
{
    ICRenderText2d* text2d = GetText2d(this, text, length);
    if (text2d == NULL) {
        return;
    }
    ICGraphics2d* graphics = ICGraphics2d::GetInstance();
    graphics->PushTransform();
    graphics->Translate(x << 16, y << 16);
    graphics->Draw(*text2d);
    graphics->PopTransform();
}

ICRenderText2d* CSystemFont_Cocoa::GetText2d(const ICSystemFont* font, const wchar_t* text, int length) const
{
    int textLength = wcslen(text);
    wchar_t key[128];
    memset(key, 0, sizeof(key));
    if (length < 0) {
        length = textLength;
    }
    if (length > textLength) {
        length = textLength;
    }
    wcsncpy(key, text, length + 1);
    key[length] = 0;
    wcslen(key);
    ICStdUtil::SWPrintF(key + length, L"%x", font);

    ICRenderText2d* text2d = NULL;
    if (g_text2d->Find(HashJava(key), (void**)&text2d) == 0) {
        ICRenderText2d::InitParam params[5];
        params[0].m_key = ICRenderText2d::InitParam_Font;
        params[0].m_value = (unsigned int)font;
        params[1].m_key = ICRenderText2d::InitParam_Text;
        params[1].m_value = (unsigned int)text;
        params[2].m_key = ICRenderText2d::InitParam_TextType;
        params[2].m_value = 0x02515e62;  // wide text of the CK build
        params[3].m_key = ICRenderText2d::InitParam_Length;
        params[3].m_value = length;
        params[4].m_key = ICRenderText2d::InitParam_End;
        params[4].m_value = 0;
        text2d = ICRenderText2d::CreateInstance(ICGraphics2d::GetInstance()->GetActiveAbstractionLayer());
        text2d->Initialize(params);
        g_text2d->Insert(HashJava(key), text2d);
    }
    return text2d;
}

static int HashJava(const wchar_t* text)
{
    int length = wcslen(text);
    int hash = 0;
    int factor = 1;
    // Java String.hashCode, accumulated from the last character
    for (int index = length - 1; index >= 0; index--) {
        hash += text[index] * factor;
        factor *= 31;
    }
    return hash;
}
