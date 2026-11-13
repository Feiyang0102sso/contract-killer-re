// CGraphicFont.cpp - load CK font metrics and draw glyphs from an atlas.
#include "CGraphicFont.h"
#include "CArrayInputStream.h"
#include "ICGraphics2d.h"
#include "CRectangle.h"
#include <stdlib.h>
#include <string.h>

CGraphicFont::CGraphicFont()
{
    m_classId = ClassId;
    m_image = NULL;
    m_metrics = NULL;
    m_glyphs = NULL;
    m_controls = NULL;
}

CGraphicFont::~CGraphicFont()
{
    Destroy();
}

void CGraphicFont::Destroy()
{
    if (m_metrics != NULL) {
        delete m_metrics;
        m_metrics = NULL;
    }
    if (m_glyphs != NULL) {
        delete[] m_glyphs;
        m_glyphs = NULL;
    }
    if (m_controls != NULL) {
        delete[] m_controls;
        m_controls = NULL;
    }
}

void CGraphicFont::SetFontImage(ICRenderSurface* surface)
{
    m_image = surface;
}

void CGraphicFont::ParseFontMetrics(const unsigned char* data, unsigned int size)
{
    Destroy();
    if (data == NULL || size < sizeof(Metrics)) {
        return;
    }

    m_metrics = new Metrics;
    if (m_metrics == NULL) {
        return;
    }
    memcpy(m_metrics, data, sizeof(Metrics));
    int glyphCount = m_metrics->m_glyphCount;
    int controlCount = m_metrics->m_controlCount;
    if (glyphCount < 0 || controlCount < 0) {
        Destroy();
        return;
    }
    if (glyphCount > 0) {
        m_glyphs = new Glyph[glyphCount];
    }
    if (controlCount > 0) {
        m_controls = new ControlChar[controlCount];
    }

    unsigned int capacity = glyphCount + 1;
    if (controlCount > glyphCount) {
        capacity = controlCount + 1;
    }
    wchar_t* characters = new wchar_t[capacity];
    if (characters == NULL || (glyphCount > 0 && m_glyphs == NULL) ||
        (controlCount > 0 && m_controls == NULL)) {
        delete[] characters;
        Destroy();
        return;
    }
    CArrayInputStream stream;
    stream.Open(data + sizeof(Metrics), size - sizeof(Metrics));

    // Format 2 stores the character codes in a Java modified UTF-8 string ahead of each array.
    bool useUtf = m_metrics->m_version == 2;
    if (m_glyphs != NULL) {
        if (useUtf && stream.ReadJMUtf(characters, capacity) != (unsigned int)glyphCount) {
            delete[] characters;
            Destroy();
            return;
        }
        for (int index = 0; index < glyphCount; ++index) {
            Glyph& glyph = m_glyphs[index];
            if (useUtf) {
                glyph.m_character = characters[index];
            } else {
                glyph.m_character = stream.ReadInt16();
            }
            glyph.m_x = stream.ReadInt16();
            glyph.m_y = stream.ReadInt16();
            glyph.m_width = stream.ReadInt8();
            glyph.m_height = stream.ReadInt8();
            glyph.m_offsetX = stream.ReadInt8();
            glyph.m_offsetY = stream.ReadInt8();
            glyph.m_advance = stream.ReadInt8();
            glyph.m_flags = stream.ReadInt8();
        }
    }
    if (m_controls != NULL) {
        if (useUtf && stream.ReadJMUtf(characters, capacity) != (unsigned int)controlCount) {
            delete[] characters;
            Destroy();
            return;
        }
        for (int index = 0; index < controlCount; ++index) {
            ControlChar& control = m_controls[index];
            if (useUtf) {
                control.m_character = characters[index];
            } else {
                control.m_character = stream.ReadInt16();
            }
            control.m_flags = stream.ReadInt8();
            control.m_advance = stream.ReadInt8();
        }
    }
    bool failed = stream.m_error;
    stream.Close();
    delete[] characters;
    if (failed && (glyphCount != 0 || controlCount != 0)) {
        Destroy();
    }
}

int CGraphicFont::GetFontHeight() const
{
    return m_metrics->m_height + m_metrics->m_lineSpacing;
}

int CGraphicFont::GetMinLineSpacing() const
{
    return m_metrics->m_lineSpacing;
}

int CGraphicFont::GetMinCharSpacing() const
{
    return m_metrics->m_charSpacing;
}

int CGraphicFont::MeasureTextWidth(const wchar_t* text, int length, int maxWidth, int* fitCount) const
{
    if (length < 0) {
        length = wcslen(text);
    }
    int width = 0;
    int count = 0;
    for (; count < length; ++count) {
        int glyphWidth = GetWidth(text[count]);
        int advance = glyphWidth + m_metrics->m_charSpacing;
        if (maxWidth >= 0) {
            int needed = advance;
            if (needed < glyphWidth) {
                needed = glyphWidth;
            }
            if (maxWidth - width < needed) {
                break;
            }
        }
        width += advance;
    }
    if (count > 0) {
        width -= m_metrics->m_charSpacing;
    }
    if (fitCount != NULL) {
        *fitCount = count;
    }
    return width;
}

void CGraphicFont::PaintText(const wchar_t* text, int length, int x, int y, int width, int height) const
{
    ICGraphics2d* graphics = ICGraphics2d::GetInstance();
    if (length < 0) {
        length = wcslen(text);
    }
    int cursor = 0;
    for (; length > 0; ++text, --length) {
        const ControlChar* control = GetControlChar(*text);
        if (control != NULL) {
            cursor += control->m_advance + m_metrics->m_charSpacing;
            continue;
        }
        const Glyph* glyph = GetChar(*text);
        if (glyph == NULL) {
            continue;
        }
        int glyphWidth = glyph->m_width;
        int glyphHeight = glyph->m_height;
        if (width >= 0) {
            int remaining = width - cursor - glyph->m_offsetX;
            if (glyphWidth > remaining) {
                glyphWidth = remaining;
            }
            if (glyphWidth < 0) {
                glyphWidth = 0;
            }
        }
        if (height >= 0) {
            int remaining = height - glyph->m_offsetY;
            if (glyphHeight > remaining) {
                glyphHeight = remaining;
            }
            if (glyphHeight < 0) {
                glyphHeight = 0;
            }
        }

        CRectangle source(glyph->m_x, glyph->m_y, glyphWidth, glyphHeight);
        graphics->PushTransform();
        graphics->Translate((x + cursor + glyph->m_offsetX) * 0x10000, (y + glyph->m_offsetY) * 0x10000);
        graphics->Draw(*m_image, ICGraphics2d::Flip_None, &source);
        graphics->PopTransform();
        cursor += glyph->m_advance + m_metrics->m_charSpacing;
    }
}

__attribute__((noinline)) const CGraphicFont::ControlChar* CGraphicFont::GetControlChar(wchar_t character) const
{
    if (m_metrics->m_controlCount <= 0) {
        return NULL;
    }
    const ControlChar* control = m_controls;
    for (int index = 0; index < m_metrics->m_controlCount; ++index) {
        if (control->m_character == character) {
            return control;
        }
        ++control;
    }
    return NULL;
}

__attribute__((noinline)) const CGraphicFont::Glyph* CGraphicFont::GetChar(wchar_t character) const
{
    if (m_metrics->m_glyphCount <= 0) {
        return NULL;
    }
    int first = 0;
    int last = m_metrics->m_glyphCount - 1;
    while (first <= last) {
        int middle = (first + last) >> 1;
        const Glyph& glyph = m_glyphs[middle];
        if (character == glyph.m_character) {
            return &glyph;
        }
        if (character < glyph.m_character) {
            last = middle - 1;
        } else {
            first = middle + 1;
        }
    }
    return NULL;
}

int CGraphicFont::GetWidth(wchar_t character) const
{
    const ControlChar* control = GetControlChar(character);
    if (control != NULL) {
        return control->m_advance;
    }
    const Glyph* glyph = GetChar(character);
    if (glyph != NULL) {
        return glyph->m_advance;
    }
    return 0;
}
