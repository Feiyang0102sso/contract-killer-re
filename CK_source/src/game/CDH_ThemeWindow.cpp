#include "CDH_ThemeWindow.h"

CDH_ThemeWindow::CDH_ThemeWindow()
{
    init(true);
}

CDH_ThemeWindow::~CDH_ThemeWindow()
{
}

void CDH_ThemeWindow::init(bool showClose)
{
    m_sideLeft.Create("SUR_DLG_SIDE_LEFT");
    m_sideRight.Create("SUR_DLG_SIDE_RIGHT");
    m_sideTop.Create("SUR_DLG_SIDE_TOP");
    m_sideBottom.Create("SUR_DLG_SIDE_BOTTOM");
    m_cornerLeftTop.Create("SUR_DLG_CORNER_LEFT_TOP");
    m_cornerRightTop.Create("SUR_DLG_CORNER_RIGHT_TOP");
    m_cornerLeftBottom.Create("SUR_DLG_CORNER_LEFT_BOTTOM");
    m_cornerRightBottom.Create("SUR_DLG_CORNER_RIGHT_BOTTOM");
    m_cornerTitleInnerRight.Create("SUR_DLG_CORNER_TITLE_INNER_RIGHT");
    m_filler.Create("SUR_DLG_FILLER");
    m_titleLeft.Create("SUR_DLG_TITLE_LEFT");
    m_titleRight.Create("SUR_DLG_TITLE_RIGHT");
    m_titleTop.Create("SUR_DLG_TITLE_TOP");
    m_closeButton.Create("SUR_BUTTON_CLOSE");
    GetImageSize(m_cornerLeftTop.GetSurface(), m_cornerWidth, m_cornerHeight);
    SetInsetSpacing(6, ImageWidth(m_cornerLeftTop.GetSurface()));
    m_captionWidth = 0;
    m_showClose = showClose;
}

void CDH_ThemeWindow::Paint(ICGraphics2d& graphics)
{
    if (m_captionWidth == 0 && !m_showClose) {
        DrawBorderFillRect(graphics, m_sideLeft.GetSurface(), m_cornerLeftTop.GetSurface(),
                           m_sideTop.GetSurface(), m_cornerRightTop.GetSurface(),
                           m_sideRight.GetSurface(), m_cornerRightBottom.GetSurface(),
                           m_sideBottom.GetSurface(), m_cornerLeftBottom.GetSurface(),
                           m_filler.GetSurface(), 0, 0, m_width, m_height);
    } else {
        int titleHeight = ImageHeight(m_titleLeft.GetSurface());
        int leftX;
        if (m_captionWidth != 0) {
            DrawImage(graphics, m_titleLeft.GetSurface(), 0, 0, ALIGN_LEFT | ALIGN_TOP, 0);
            leftX = ImageWidth(m_titleLeft.GetSurface());
            DrawTileImageHoriz(graphics, m_titleTop.GetSurface(), leftX, 0, m_captionWidth);
            leftX += m_captionWidth;
            DrawImage(graphics, m_titleRight.GetSurface(), leftX, 0, ALIGN_LEFT | ALIGN_TOP, 0);
            leftX += ImageWidth(m_titleRight.GetSurface());
        } else {
            DrawImage(graphics, m_cornerLeftTop.GetSurface(), 0, titleHeight, ALIGN_LEFT | ALIGN_BOTTOM, 0);
            leftX = ImageWidth(m_cornerLeftTop.GetSurface());
        }

        int rightX;
        if (m_showClose) {
            // Mirrored title cap, close button slot, then the mirrored title end.
            rightX = m_width - ImageWidth(m_titleLeft.GetSurface());
            DrawImage(graphics, m_titleLeft.GetSurface(), rightX, 0, ALIGN_LEFT | ALIGN_TOP, 2);
            rightX -= ImageWidth(m_closeButton.GetSurface());
            DrawTileImageHoriz(graphics, m_titleTop.GetSurface(), rightX, 0, ImageWidth(m_closeButton.GetSurface()));
            rightX -= ImageWidth(m_titleLeft.GetSurface());
            DrawImage(graphics, m_titleRight.GetSurface(), rightX, 0, ALIGN_LEFT | ALIGN_TOP, 2);
        } else {
            rightX = m_width - ImageWidth(m_cornerRightTop.GetSurface());
            DrawImage(graphics, m_cornerRightTop.GetSurface(), rightX,
                      titleHeight - ImageHeight(m_sideTop.GetSurface()), ALIGN_LEFT | ALIGN_TOP, 0);
        }
        DrawTileImageHoriz(graphics, m_sideTop.GetSurface(), leftX,
                           titleHeight - ImageHeight(m_sideTop.GetSurface()), rightX - leftX);

        // The top edge is already drawn by the title bar.
        DrawBorderFillRect(graphics, m_sideLeft.GetSurface(), NULL, NULL, NULL,
                           m_sideRight.GetSurface(), m_cornerRightBottom.GetSurface(),
                           m_sideBottom.GetSurface(), m_cornerLeftBottom.GetSurface(),
                           m_filler.GetSurface(), 0, titleHeight, m_width, m_height - titleHeight);
    }
}

typedef char AssertThemeWindowSize[sizeof(CDH_ThemeWindow) == 0x124 ? 1 : -1];
