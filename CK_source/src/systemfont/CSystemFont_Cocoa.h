// CSystemFont_Cocoa.h - system font drawn with UIKit (UIFont), cached as render texts.
#ifndef CSYSTEMFONT_COCOA_H
#define CSYSTEMFONT_COCOA_H

#include "ICSystemFont.h"

class CHash;
class ICRenderText2d;
#ifdef __OBJC__
@class NSString;
@class UIFont;
#else
class NSString;
class UIFont;
#endif

class CSystemFont_Cocoa : public ICSystemFont {
public:
    CSystemFont_Cocoa();

    // GetFontHeight (CSystemFont_Cocoa_mm.mm) is the key function: the vtable belongs to that unit, as in the original
    virtual int GetFontHeight() const;
    virtual ~CSystemFont_Cocoa();
    virtual int GetMinLineSpacing() const;
    virtual int GetMinCharSpacing() const;
    virtual int MeasureTextWidth(const wchar_t* text, int length, int maxWidth, int* fitCount) const;
    virtual int GetWidth(wchar_t character) const;
    virtual void PaintText(const wchar_t* text, int length, int x, int y, int width, int height) const;
    virtual unsigned char Initialize(int size, const wchar_t* name, unsigned char antialias);
    virtual void SetFontColor(unsigned int color);
    virtual void SetBackgroundColor(unsigned int color);
    virtual unsigned char InitializeSurfaceWithText(ICRenderSurface& surface, const wchar_t* text,
                                                    unsigned int length, int width, int height);
    virtual void Destroy();

    unsigned char InitializeSurfaceWithPng(const unsigned char* data, unsigned int size, ICRenderSurface& surface);
    ICRenderText2d* GetText2d(const ICSystemFont* font, const wchar_t* text, int length) const;
    int MeasureTextWidth(const NSString* text) const;
    unsigned char CreatePngWithText(const NSString* text, int width, int height, ICRenderSurface& surface);

    unsigned int m_fontColor;          // 0x08
    unsigned int m_backgroundColor;    // 0x0c
    UIFont* m_font;                    // 0x10
    bool m_antialias;                  // 0x14
    unsigned char m_reserved15[7];     // up to the 0x1c allocation
};

#endif
