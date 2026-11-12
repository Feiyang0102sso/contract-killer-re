// CBloodWindow.h - framed menu window with a title (base of CImportantDialog and the friends menu prompts).
#ifndef CBLOODWINDOW_H
#define CBLOODWINDOW_H

#include "WindowBase.h"

class CFont;
class TextWindow;

class CBloodWindow : public Window {
public:
    CBloodWindow();
    virtual void OnStartupEasy();

    static Window* CreateBg();
    void SetTitle(const XString& title);
    void CreateBgEffects();

protected:
    ImageRes m_bgTile;              // 0xa4
    ImageRes m_frameDown;           // 0xac
    ImageRes m_frameDownLeft;       // 0xb4
    ImageRes m_frameDownRight;      // 0xbc
    ImageRes m_frameLeft;           // 0xc4
    ImageRes m_frameRight;          // 0xcc
    ImageRes m_frameTop;            // 0xd4
    ImageRes m_frameTopLeft;        // 0xdc
    ImageRes m_frameTopRight;       // 0xe4
    ImageRes m_bgDownLeft;          // 0xec
    ImageRes m_bgDownRight;         // 0xf4
    ImageRes m_bgTopRight;          // 0xfc
    int m_frameTopHeight;           // 0x104
    int m_titleWidth;               // 0x108
    XString m_title;                // 0x10c
    CFont* m_titleFont;             // 0x110
    Window* m_content;              // 0x114 area below the title
    TextWindow* m_titleText;        // 0x118
    Window* m_background;           // 0x11c
};

#endif
