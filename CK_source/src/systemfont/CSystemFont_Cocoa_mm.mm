// CSystemFont_Cocoa_mm.mm - UIKit side of the system font: UIFont, measuring and text to PNG.
#import <UIKit/UIKit.h>

#include "CSystemFont_Cocoa.h"
#include "CMath.h"

#include <wchar.h>

static NSString* Unicode2NSString(const wchar_t* text, int length);
static CGColorRef Color_MakeCGColor(unsigned int argb);

unsigned char CSystemFont_Cocoa::Initialize(int size, const wchar_t* name, unsigned char antialias)
{
    if (name != NULL) {
        NSString* fontName = Unicode2NSString(name, -1);
        m_font = [UIFont fontWithName:fontName size:(float)size];
        [fontName release];
    } else {
        m_font = [UIFont systemFontOfSize:(float)size];
    }
    m_antialias = antialias != 0;
    return 1;
}

static NSString* Unicode2NSString(const wchar_t* text, int length)
{
    int textLength = wcslen(text);
    if (length >= 0) {
        if (length > textLength) {
            length = textLength;
        }
        textLength = length;
    }
    return [[NSString alloc] initWithBytes:text length:textLength * sizeof(wchar_t)
                                  encoding:NSUTF32LittleEndianStringEncoding];
}

int CSystemFont_Cocoa::GetFontHeight() const
{
    int height = 0;
    if (m_font != nil) {
        height = (int)[m_font leading];
    }
    return height;
}

int CSystemFont_Cocoa::GetMinLineSpacing() const
{
    int spacing = 0;
    if (m_font != nil) {
        spacing = (int)([m_font leading] - (float)GetFontHeight());
    }
    return spacing;
}

int CSystemFont_Cocoa::MeasureTextWidth(const wchar_t* text, int length, int maxWidth, int* fitCount) const
{
    int width = 0;
    int count = 0;
    if (m_font != nil && text != NULL) {
        if (length < 0) {
            length = wcslen(text);
        }
        // grow the measured prefix one character at a time until it no longer fits
        int measured = 0;
        int next = 0;
        while (true) {
            count = next;
            width = measured;
            if (count >= length) {
                break;
            }
            NSString* prefix = Unicode2NSString(text, count + 1);
            measured = MeasureTextWidth(prefix);
            [prefix release];
            next = count + 1;
            if (maxWidth >= 0 && measured > maxWidth) {
                break;
            }
        }
    }
    if (fitCount != NULL) {
        *fitCount = count;
    }
    return width;
}

int CSystemFont_Cocoa::MeasureTextWidth(const NSString* text) const
{
    int width = 0;
    if (m_font != nil && text != nil) {
        CGSize size = [text sizeWithFont:m_font];
        width = (int)size.width;
    }
    return width;
}

int CSystemFont_Cocoa::GetWidth(wchar_t character) const
{
    wchar_t text[2];
    text[0] = character;
    text[1] = 0;
    return MeasureTextWidth(text, -1, -1, NULL);
}

unsigned char CSystemFont_Cocoa::CreatePngWithText(const NSString* text, int width, int height, ICRenderSurface& surface)
{
    UIGraphicsBeginImageContext(CGSizeMake((float)width, (float)height));
    CGContextRef context = UIGraphicsGetCurrentContext();
    if (context != NULL) {
        CGContextSetAllowsAntialiasing(context, m_antialias);
        CGContextSetFillColorWithColor(context, Color_MakeCGColor(m_backgroundColor));
        CGContextFillRect(context, CGRectMake(0.0f, 0.0f, (float)width, (float)height));
        CGContextSetFillColorWithColor(context, Color_MakeCGColor(m_fontColor));
        [text drawAtPoint:CGPointMake(0.0f, 0.0f) withFont:m_font];
        UIImage* image = UIGraphicsGetImageFromCurrentImageContext();
        if (image != nil) {
            NSData* png = UIImagePNGRepresentation(image);
            InitializeSurfaceWithPng((const unsigned char*)[png bytes], [png length], surface);
        }
    }
    UIGraphicsEndImageContext();
    return 1;
}

static CGColorRef Color_MakeCGColor(unsigned int argb)
{
    float red = (float)((argb & 0xffffff) >> 16) / 255.0f;
    float green = (float)((argb & 0xffff) >> 8) / 255.0f;
    float blue = (float)(argb & 0xff) / 255.0f;
    float alpha = (float)(argb >> 24) / 255.0f;
    return [[UIColor colorWithRed:red green:green blue:blue alpha:alpha] CGColor];
}

unsigned char CSystemFont_Cocoa::InitializeSurfaceWithText(ICRenderSurface& surface, const wchar_t* text,
                                                           unsigned int length, int width, int height)
{
    unsigned char result = 0;
    if (m_font != nil && text != NULL) {
        NSString* string = Unicode2NSString(text, length);
        int textWidth = MeasureTextWidth(string);
        // a negative size means "measured"; the original clamps with CMath::Min
        if (width >= 0) {
            textWidth = CMath::Min(width, textWidth);
        }
        int textHeight = GetFontHeight() + GetMinLineSpacing();
        if (height >= 0) {
            textHeight = CMath::Min(textHeight, height);
        }
        // the original always renders and reports success
        CreatePngWithText(string, textWidth, textHeight, surface);
        [string release];
        result = 1;
    }
    return result;
}
