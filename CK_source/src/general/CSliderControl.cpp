// CSliderControl.cpp - slider window of the HUD.
// Constructor, progress handling, pointer events and Paint: original 0x178bc4..0x17908c.
#include "CSliderControl.h"

#include "ICRenderSurface.h"

CSliderControl::CSliderControl(const char* knob, const char* track, ESliderType type)
    : Window(false), m_knobImage(knob), m_trackImage(track),
      m_knob(m_knobImage.GetSurface()), m_track(m_trackImage.GetSurface()),
      m_trackWidth(0), m_trackHeight(0), m_knobWidth(0), m_knobHeight(0),
      m_knobOffsetX(0), m_knobOffsetY(0), m_type(type),
      m_startInset(0), m_endInset(0), m_crossAxisCenter(0), m_minPosition(0),
      m_maxPosition(0), m_positionRange(0), m_progress(0.0f),
      m_lastTouchX(0), m_lastTouchY(0), m_dragging(false)
{
    unsigned int width;
    unsigned int height;
    m_track->GetWidthAndHeight(width, height);
    m_trackWidth = width;
    m_trackHeight = height;
    SetSize(width, height);
    m_knob->GetWidthAndHeight(width, height);
    m_knobWidth = width;
    m_knobHeight = height;
    CalcPositionRange();
}

void CSliderControl::CalcPositionRange()
{
    if (m_type == Vertical) {
        m_crossAxisCenter = m_trackWidth / 2;
        m_minPosition = m_startInset;
        m_maxPosition = m_trackHeight - m_endInset;
        m_positionRange = m_trackHeight + 1 - (m_startInset + m_endInset);
    } else if (m_type == Horizontal) {
        m_crossAxisCenter = m_trackHeight / 2;
        m_minPosition = m_startInset;
        m_maxPosition = m_trackWidth - m_endInset;
        m_positionRange = m_trackWidth + 1 - (m_startInset + m_endInset);
    }
    if (m_positionRange < 1) {
        m_positionRange = 1;
    }
}

void CSliderControl::Enable(bool enable)
{
    if (enable) {
        SetFlags(FLAG_VISIBLE);
        SetFlags(FLAG_ENABLED);
        SetFlags(FLAG_UPDATE);
    } else {
        ClearFlags(FLAG_VISIBLE);
        ClearFlags(FLAG_ENABLED);
        ClearFlags(FLAG_UPDATE);
    }
}

void CSliderControl::SetProgress(float progress)
{
    m_progress = progress;
    if (m_progress >= 0.0f) {
        if (m_progress > 1.0f) {
            m_progress = 1.0f;
        }
    } else {
        m_progress = 0.0f;
    }
}

void CSliderControl::SetProgressByTouch(int x, int y)
{
    if (m_type == Horizontal) {
        y = x;
    }
    if (m_positionRange < 1) {
        m_positionRange = 1;
    }
    SetProgress(static_cast<float>(y - m_minPosition) / m_positionRange);
}

void CSliderControl::CorrectProgress(int dx, int dy)
{
    int position = static_cast<int>(m_minPosition + m_progress * m_positionRange);
    if (m_type == Horizontal) {
        position += dx;
    } else if (m_type == Vertical) {
        position += dy;
    }
    if (m_positionRange < 1) {
        m_positionRange = 1;
    }
    SetProgress(static_cast<float>(position - m_minPosition) / m_positionRange);
}

bool CSliderControl::IsPointerOnSlider(int x, int y)
{
    return m_knobX - m_knobWidth / 2 <= x && x <= m_knobX + m_knobWidth / 2
        && m_knobY - m_knobHeight / 2 <= y && y <= m_knobY + m_knobHeight / 2;
}

void CSliderControl::OnPointerPressed(Event& event)
{
    int x = event.m_param1;
    int y = event.m_param2;
    if (IsPointerOnSlider(x, y)) {
        m_dragging = true;
        m_lastTouchX = x;
        m_lastTouchY = y;
    } else {
        SetProgressByTouch(x, y);
    }
    event.Clear();
}

void CSliderControl::OnPointerDragged(Event& event)
{
    if (m_dragging) {
        int x = event.m_param1;
        int y = event.m_param2;
        if (x >= 0 && x <= m_width && y >= 0 && y <= m_height) {
            CorrectProgress(x - m_lastTouchX, y - m_lastTouchY);
            m_lastTouchX = x;
            m_lastTouchY = y;
        }
    }
    event.Clear();
}

void CSliderControl::OnPointerReleased(Event& event)
{
    m_dragging = false;
    event.Clear();
}

void CSliderControl::Paint(ICGraphics2d& graphics)
{
    DrawImage(graphics, m_track, 0, 0, 9, 0);
    SetProgress(m_progress);
    int position = static_cast<int>(m_minPosition + m_progress * m_positionRange);
    if (position < m_minPosition) {
        position = m_minPosition;
    } else if (position > m_maxPosition) {
        position = m_maxPosition;
    }
    if (m_type == Vertical) {
        m_knobX = m_crossAxisCenter;
        m_knobY = position;
    } else if (m_type == Horizontal) {
        m_knobX = position;
        m_knobY = m_crossAxisCenter;
    }
    m_knobX += m_knobOffsetX;
    m_knobY += m_knobOffsetY;
    DrawImage(graphics, m_knob, m_knobX - m_knobWidth / 2, m_knobY - m_knobHeight / 2, 9, 0);
}
