// CGraphicFont.h - bitmap glyph metrics and atlas rendering.
#ifndef CGRAPHICFONT_H
#define CGRAPHICFONT_H

#include "CFont.h"

class ICRenderSurface;

class CGraphicFont : public CFont {
public:
    enum { ClassId = 0x6eff79d2 };

    CGraphicFont();
    virtual ~CGraphicFont();
    void Destroy();
    void SetFontImage(ICRenderSurface* surface);
    void ParseFontMetrics(const unsigned char* data, unsigned int size);

    virtual int GetFontHeight() const;
    virtual int GetMinLineSpacing() const;
    virtual int GetMinCharSpacing() const;
    virtual int MeasureTextWidth(const wchar_t* text, int length, int maxWidth, int* fitCount) const;
    virtual int GetWidth(wchar_t character) const;
    virtual void PaintText(const wchar_t* text, int length, int x, int y, int width, int height) const;

private:
    friend class CFontMgr;
    friend class CGraphics2dAbstract;

    struct Metrics {
        unsigned char m_version;
        unsigned char m_reserved[4];
        signed char m_height;
        signed char m_charSpacing;
        signed char m_lineSpacing;
        short m_glyphCount;
        short m_controlCount;
    };

    struct Glyph {
        unsigned short m_character;
        short m_x;
        short m_y;
        signed char m_width;
        signed char m_height;
        signed char m_offsetX;
        signed char m_offsetY;
        signed char m_advance;
        signed char m_flags;
    };

    struct ControlChar {
        unsigned short m_character;
        signed char m_flags;
        signed char m_advance;
    };

    const ControlChar* GetControlChar(wchar_t character) const;
    const Glyph* GetChar(wchar_t character) const;

    ICRenderSurface* m_image;
    Metrics* m_metrics;
    Glyph* m_glyphs;
    ControlChar* m_controls;
};

#endif
