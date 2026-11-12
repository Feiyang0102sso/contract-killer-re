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
    enum Image {
        SideLeft, SideRight, SideTop, SideBottom,
        CornerLeftTop, CornerRightTop, CornerLeftBottom, CornerRightBottom,
        CornerTitleInnerRight, Filler, TitleLeft, TitleRight, TitleTop, CloseButton,
        ImageCount
    };

    ImageRes m_images[ImageCount];
    int m_captionWidth;           // 0x114
    bool m_showClose;             // 0x118
    int m_cornerWidth;
    int m_cornerHeight;
};

#endif
