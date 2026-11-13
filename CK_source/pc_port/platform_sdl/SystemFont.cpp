// The UIKit part of the system font (replaces CSystemFont_Cocoa_mm.mm): a Windows GDI font measures
// and draws the dynamic text. The caching and drawing of the texts stay in CSystemFont_Cocoa.cpp.
#include "CSystemFont_Cocoa.h"
#include "CRenderSurfaceBuffer.h"
#include "ICRenderSurface.h"
#include <windows.h>
#include <stdio.h>
#include <wchar.h>
#include <vector>

namespace {

// Font used when the game asks for the system font (no name).
const wchar_t* const DefaultFontName = L"Arial";

/** The GDI font kept in the UIFont* member. */
HFONT NativeFont(UIFont* font) { return reinterpret_cast<HFONT>(font); }

int TextLength(const wchar_t* text, int length)
{
    const int available = static_cast<int>(wcslen(text));
    if (length < 0 || length > available) {
        return available;
    }
    return length;
}

/** A memory DC with the font selected (released by the destructor). */
class FontDc {
public:
    explicit FontDc(UIFont* font)
    {
        m_dc = CreateCompatibleDC(nullptr);
        m_oldFont = SelectObject(m_dc, NativeFont(font));
    }
    ~FontDc()
    {
        SelectObject(m_dc, m_oldFont);
        DeleteDC(m_dc);
    }
    HDC Get() const { return m_dc; }

private:
    HDC m_dc;
    HGDIOBJ m_oldFont;
};

/** Width in pixels of the first `length` characters. */
int TextWidth(HDC dc, const wchar_t* text, int length)
{
    SIZE size = {};
    GetTextExtentPoint32W(dc, text, length, &size);
    return size.cx;
}

/** One color channel of `argb` (shift 24: alpha, 16: red, 8: green, 0: blue). */
int Channel(unsigned int argb, int shift)
{
    return static_cast<int>((argb >> shift) & 0xff);
}

/** Mix a background and a text channel by the text coverage (0..255). */
unsigned char Mix(int background, int text, int coverage)
{
    return static_cast<unsigned char>(background + (text - background) * coverage / 255);
}

} // namespace

unsigned char CSystemFont_Cocoa::Initialize(int size, const wchar_t* name, unsigned char antialias)
{
    if (name == nullptr) {
        name = DefaultFontName;
    }
    m_antialias = antialias != 0;
    DWORD quality = NONANTIALIASED_QUALITY;
    if (m_antialias) {
        quality = ANTIALIASED_QUALITY;
    }
    // The UIFont size is the em height in points, which is pixels on the 1024 x 768 screen.
    HFONT font = CreateFontW(-size, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                             CLIP_DEFAULT_PRECIS, quality, DEFAULT_PITCH, name);
    if (font == nullptr) {
        fwprintf(stderr, L"System font %ls (%d) could not be created\n", name, size);
        return 1;
    }
    m_font = reinterpret_cast<UIFont*>(font);
    return 1;
}

int CSystemFont_Cocoa::GetFontHeight() const
{
    if (m_font == nullptr) {
        return 0;
    }
    // UIFont leading: the height of one line.
    FontDc dc(m_font);
    TEXTMETRICW metrics = {};
    GetTextMetricsW(dc.Get(), &metrics);
    return metrics.tmHeight + metrics.tmExternalLeading;
}

int CSystemFont_Cocoa::GetMinLineSpacing() const
{
    // leading - GetFontHeight() in the original: always 0
    return 0;
}

int CSystemFont_Cocoa::MeasureTextWidth(const wchar_t* text, int length, int maxWidth, int* fitCount) const
{
    int width = 0;
    int count = 0;
    if (m_font != nullptr && text != nullptr) {
        if (length < 0) {
            length = static_cast<int>(wcslen(text));
        }
        FontDc dc(m_font);
        // As the original: grow the measured prefix one character at a time until it no longer fits.
        while (count < length) {
            const int measured = TextWidth(dc.Get(), text, count + 1);
            if (maxWidth >= 0 && measured > maxWidth) {
                break;
            }
            width = measured;
            count++;
        }
    }
    if (fitCount != nullptr) {
        *fitCount = count;
    }
    return width;
}

int CSystemFont_Cocoa::GetWidth(wchar_t character) const
{
    const wchar_t text[2] = { character, 0 };
    return MeasureTextWidth(text, -1, -1, nullptr);
}

unsigned char CSystemFont_Cocoa::InitializeSurfaceWithText(ICRenderSurface& surface, const wchar_t* text,
                                                           unsigned int length, int width, int height)
{
    if (m_font == nullptr || text == nullptr) {
        return 0;
    }
    const int characters = TextLength(text, static_cast<int>(length));
    int textWidth = MeasureTextWidth(text, characters, -1, nullptr);
    // A negative size means "measured"; otherwise the smaller one, as CMath::Min in the original.
    if (width >= 0 && width < textWidth) {
        textWidth = width;
    }
    int textHeight = GetFontHeight() + GetMinLineSpacing();
    if (height >= 0 && height < textHeight) {
        textHeight = height;
    }
    if (textWidth <= 0 || textHeight <= 0) {
        // The original still reports success for an empty picture.
        return 1;
    }

    // Draw white text on black: the red channel is then the text coverage of each pixel.
    FontDc dc(m_font);
    BITMAPINFO info = {};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = textWidth;
    info.bmiHeader.biHeight = -textHeight;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    void* bits = nullptr;
    HBITMAP bitmap = CreateDIBSection(dc.Get(), &info, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (bitmap == nullptr) {
        return 0;
    }
    HGDIOBJ oldBitmap = SelectObject(dc.Get(), bitmap);
    SetTextColor(dc.Get(), RGB(255, 255, 255));
    SetBkColor(dc.Get(), RGB(0, 0, 0));
    SetBkMode(dc.Get(), OPAQUE);
    TextOutW(dc.Get(), 0, 0, text, characters);
    GdiFlush();

    // The text color over the background color, as the original's PNG (RGBA bytes).
    std::vector<unsigned char> pixels(static_cast<size_t>(textWidth) * textHeight * 4);
    const auto* source = static_cast<const unsigned char*>(bits);
    for (size_t pixel = 0; pixel < pixels.size() / 4; pixel++) {
        const int coverage = source[pixel * 4 + 2];
        pixels[pixel * 4 + 0] = Mix(Channel(m_backgroundColor, 16), Channel(m_fontColor, 16), coverage);
        pixels[pixel * 4 + 1] = Mix(Channel(m_backgroundColor, 8), Channel(m_fontColor, 8), coverage);
        pixels[pixel * 4 + 2] = Mix(Channel(m_backgroundColor, 0), Channel(m_fontColor, 0), coverage);
        pixels[pixel * 4 + 3] = Mix(Channel(m_backgroundColor, 24), Channel(m_fontColor, 24), coverage);
    }
    SelectObject(dc.Get(), oldBitmap);
    DeleteObject(bitmap);

    const ICRenderSurface::InitParam params[] = {
        { ICRenderSurface::InitParam_Width, static_cast<unsigned int>(textWidth) },
        { ICRenderSurface::InitParam_Height, static_cast<unsigned int>(textHeight) },
        { ICRenderSurface::InitParam_ColorFormat, RSBFormat_A8B8G8R8 },
        { ICRenderSurface::InitParam_End, 0 }
    };
    if (!surface.Initialize(params)) {
        fprintf(stderr, "System font: text surface %dx%d could not be created\n", textWidth, textHeight);
        return 0;
    }
    unsigned char* destination = nullptr;
    int pitch = 0;
    surface.Lock(&destination, pitch, ICRenderSurface::LockType_ReadWrite, nullptr);
    if (destination == nullptr) {
        fprintf(stderr, "System font: text surface could not be locked\n");
        return 0;
    }
    for (int row = 0; row < textHeight; row++) {
        memcpy(destination + row * pitch, &pixels[static_cast<size_t>(row) * textWidth * 4],
               static_cast<size_t>(textWidth) * 4);
    }
    surface.Unlock();
    return 1;
}
