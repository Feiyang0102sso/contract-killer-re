// CK MenuButton ABI. CKZ b074 identified the corresponding four derived slots.
#ifndef CBH_MENUBUTTON_H
#define CBH_MENUBUTTON_H
#include "CBH_Button.h"

class CBH_MenuButton : public CBH_Button {
public:
    CBH_MenuButton();
    virtual ~CBH_MenuButton() { ReleaseResources(); }
    virtual void Init();
    virtual void Paint(ICGraphics2d& graphics);
    virtual void ReleaseResources();
    virtual void SetCorner(int x, int y);
    virtual int Left() { return m_x + m_padding; }
    virtual int Top() { return m_y + m_padding; }
    void setImage(const char* name);
    void UpdateSize();
    void setPressedImage(const char* name);
    void SetCaption(const char* name);
    void SetBlinking(bool enabled);
    void SetBlinkingImage(const char* name);

    ICRenderSurface* m_image;         // d4
    ICRenderSurface* m_pressedImage;  // d8
    ICRenderSurface* m_blinkImage;    // dc
    XString m_caption;                // e0
    CFont* m_font;                     // e4
    int m_padding;                    // e8
    bool m_blinking;                  // ec
    int m_buttonType;                 // f0, set by CDialogWindow
};
#endif
