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
    const char* resources[] = {
        "SUR_DLG_SIDE_LEFT", "SUR_DLG_SIDE_RIGHT", "SUR_DLG_SIDE_TOP", "SUR_DLG_SIDE_BOTTOM",
        "SUR_DLG_CORNER_LEFT_TOP", "SUR_DLG_CORNER_RIGHT_TOP", "SUR_DLG_CORNER_LEFT_BOTTOM",
        "SUR_DLG_CORNER_RIGHT_BOTTOM", "SUR_DLG_CORNER_TITLE_INNER_RIGHT", "SUR_DLG_FILLER",
        "SUR_DLG_TITLE_LEFT", "SUR_DLG_TITLE_RIGHT", "SUR_DLG_TITLE_TOP", "SUR_BUTTON_CLOSE"
    };
    for (int index = 0; index < ImageCount; ++index) {
        m_images[index].Create(resources[index]);
    }
    ICRenderSurface* corner = m_images[CornerLeftTop].GetSurface();
    GetImageSize(corner, m_cornerWidth, m_cornerHeight);
    SetInsetSpacing(6, ImageWidth(corner));
    m_captionWidth = 0;
    m_showClose = showClose;
}

void CDH_ThemeWindow::Paint(ICGraphics2d& graphics)
{
    ICRenderSurface* top = m_images[SideTop].GetSurface();
    ICRenderSurface* topLeft = m_images[CornerLeftTop].GetSurface();
    ICRenderSurface* topRight = m_images[CornerRightTop].GetSurface();
    int frameY = 0;
    if (m_captionWidth != 0 || m_showClose) {
        ICRenderSurface* titleLeft = m_images[TitleLeft].GetSurface();
        ICRenderSurface* titleRight = m_images[TitleRight].GetSurface();
        ICRenderSurface* titleTop = m_images[TitleTop].GetSurface();
        frameY = ImageHeight(titleLeft);
        int titleLeftWidth = ImageWidth(titleLeft);
        int endX;
        if (m_captionWidth == 0) {
            DrawImage(graphics, topLeft, 0, frameY, ALIGN_LEFT | ALIGN_BOTTOM, 0);
            endX = ImageWidth(topLeft);
        } else {
            DrawImage(graphics, titleLeft, 0, 0, ALIGN_LEFT | ALIGN_TOP, 0);
            DrawTileImageHoriz(graphics, titleTop, titleLeftWidth, 0, m_captionWidth);
            endX = titleLeftWidth + m_captionWidth;
            DrawImage(graphics, titleRight, endX, 0, ALIGN_LEFT | ALIGN_TOP, 0);
            endX += ImageWidth(titleRight);
        }

        int topY = frameY - ImageHeight(top);
        int rightX = m_width - ImageWidth(topRight);
        int transform = 0;
        if (m_showClose) {
            rightX = m_width - titleLeftWidth;
            DrawImage(graphics, titleLeft, rightX, 0, ALIGN_LEFT | ALIGN_TOP, 2);
            int closeWidth = ImageWidth(m_images[CloseButton].GetSurface());
            rightX -= closeWidth;
            DrawTileImageHoriz(graphics, titleTop, rightX, 0, closeWidth);
            rightX -= titleLeftWidth;
            topRight = titleRight;
            topY = 0;
            transform = 2;
        }
        DrawImage(graphics, topRight, rightX, topY, ALIGN_LEFT | ALIGN_TOP, transform);
        DrawTileImageHoriz(graphics, top, endX, frameY - ImageHeight(top), rightX - endX);
        topLeft = NULL;
        top = NULL;
        topRight = NULL;
    }

    DrawBorderFillRect(graphics, m_images[SideLeft].GetSurface(), topLeft, top, topRight,
                       m_images[SideRight].GetSurface(), m_images[CornerRightBottom].GetSurface(),
                       m_images[SideBottom].GetSurface(), m_images[CornerLeftBottom].GetSurface(),
                       m_images[Filler].GetSurface(), 0, frameY, m_width, m_height - frameY);
}

typedef char AssertThemeWindowSize[sizeof(CDH_ThemeWindow) == 0x124 ? 1 : -1];
