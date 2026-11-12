// CImageButton.h - button made of shared images plus an optional text label; can blink.
// Owner: src/bounty_hunter/CImageButton.cpp (game2). CK layout from CImageButton::CImageButton()
// 0xfb400 and its destructor 0xfb5d8 (CBH_Button ends at 0xd4).
#ifndef CIMAGEBUTTON_H
#define CIMAGEBUTTON_H
#include "CBH_Button.h"
#include "Vector_ss.h"
#include "xString.h"

class TextWindow;
class ImageWindow;
class CFont;

class CImageButton : public CBH_Button {
public:
    CImageButton();
    virtual ~CImageButton();
    virtual void Update();

    /** Add the shared surface `name` (released again by the destructor); x / y are unused. */
    void AddImage(const char* name, int x, int y);
    /** Add an image window showing `surface`; the button grows to the image size. */
    void AddImage(ICRenderSurface* surface, int x, int y);
    /** Replace the text label (font NULL: font 15). */
    void SetText(XString text, int align, CFont* font, int leftSpacing, int percentHeight);
    void UpdateText(XString text);
    void SetFont(CFont* font);
    /** Blink the image for `time` milliseconds. */
    void Blink(int time);
    void Disable();
    void Enable();

    int m_extraHeight;              // 0xd4 added to the image height when the button grows
    int m_fieldD8;                  // 0xd8 cleared by the constructor, otherwise unused here
    Vector<XString> m_imageNames;   // 0xdc surfaces taken from the surface manager
    ImageWindow* m_imageWindow;     // 0xec last added image (owned by the window tree)
    bool m_blinking;                // 0xf0
    int m_blinkTime;                // 0xf4 milliseconds left
    TextWindow* m_textWindow;       // 0xf8 owned by the window tree
};
#endif
