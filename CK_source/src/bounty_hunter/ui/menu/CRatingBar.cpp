// CRatingBar.cpp - draw the rating from the center toward either limit.
#include "CRatingBar.h"

#include <stdlib.h>

CRatingBar::CRatingBar()
    : Window(false)
{
    m_meter.Create("SUR_METER");
    m_center.Create("SUR_METER_CENTER");
    m_side.Create("SUR_METER_SIDE");
    m_green.Create("SUR_METER_GREEN");
    m_red.Create("SUR_METER_RED");
    SetSize(m_width, ImageHeight(m_center.GetSurface()));
}

void CRatingBar::SetLimitValues(int negativeLimit, int positiveLimit)
{
    m_negativeLimit = negativeLimit;
    m_positiveLimit = positiveLimit;
}

void CRatingBar::Paint(ICGraphics2d& graphics)
{
    ICRenderSurface* left = m_side.GetSurface();
    ICRenderSurface* middle = m_meter.GetSurface();
    ICRenderSurface* right = m_side.GetSurface();
    DrawLineImageHoriz(graphics, left, middle, right, true, 0, 0, m_width);
    DrawImage(graphics, m_center.GetSurface(), m_width / 2, 0, 0, 0);

    if (m_value > 0) {
        int percent = m_value * 100 / m_positiveLimit;
        int center = m_width / 2;
        int fillWidth = center * percent / 100;
        DrawTileImageHoriz(graphics, m_green.GetSurface(), center, 0, fillWidth);
    } else if (m_value < 0) {
        int percent = abs(m_value) * 100 / abs(m_negativeLimit);
        int center = m_width / 2;
        int fillWidth = center * percent / 100;
        DrawTileImageHoriz(graphics, m_red.GetSurface(), center - fillWidth, 0, fillWidth);
    }
}
