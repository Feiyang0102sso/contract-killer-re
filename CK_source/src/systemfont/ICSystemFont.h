// ICSystemFont.h - platform system font (a CFont drawn by the OS).
// Slots after CFont from the CSystemFont_Cocoa vtable (CSystemFont_Cocoa_mm.mm).
#ifndef ICSYSTEMFONT_H
#define ICSYSTEMFONT_H

#include "CFont.h"

class ICRenderSurface;

class ICSystemFont : public CFont {
public:
    static ICSystemFont* CreateInstance();

    /** Font of `size` pixels; `name` NULL selects the system font. */
    virtual unsigned char Initialize(int size, const wchar_t* name, unsigned char antialias) = 0;  // 0x20
    virtual void SetFontColor(unsigned int color) = 0;                                              // 0x24
    virtual void SetBackgroundColor(unsigned int color) = 0;                                        // 0x28
    /** Render `text` into `surface` (width / height < 0: measured). */
    virtual unsigned char InitializeSurfaceWithText(ICRenderSurface& surface, const wchar_t* text,
                                                    unsigned int length, int width, int height) = 0;  // 0x2c
    virtual void Destroy() = 0;                                                                     // 0x30
};

#endif
