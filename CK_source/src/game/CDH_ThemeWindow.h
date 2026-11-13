#ifndef CDH_THEMEWINDOW_H
#define CDH_THEMEWINDOW_H

#include "CDH_SoftkeysScreen.h"

/** Frame shared by the CK modal screens. */
class CDH_ThemeWindow : public CDH_SoftkeysScreen {
public:
    CDH_ThemeWindow();
    virtual ~CDH_ThemeWindow();
    virtual void Paint(ICGraphics2d& graphics);
    void init(bool showClose);

protected:
    // Original keeps 14 separate members (ctor/dtor are unrolled per member).
    ImageRes m_sideLeft;          // 0xa4
    ImageRes m_sideRight;         // 0xac
    ImageRes m_sideTop;           // 0xb4
    ImageRes m_sideBottom;        // 0xbc
    ImageRes m_cornerLeftTop;     // 0xc4
    ImageRes m_cornerRightTop;    // 0xcc
    ImageRes m_cornerLeftBottom;  // 0xd4
    ImageRes m_cornerRightBottom; // 0xdc
    ImageRes m_cornerTitleInnerRight; // 0xe4
    ImageRes m_filler;            // 0xec
    ImageRes m_titleLeft;         // 0xf4
    ImageRes m_titleRight;        // 0xfc
    ImageRes m_titleTop;          // 0x104
    ImageRes m_closeButton;       // 0x10c
    int m_captionWidth;           // 0x114
    bool m_showClose;             // 0x118
    int m_cornerWidth;
    int m_cornerHeight;
};

#endif
