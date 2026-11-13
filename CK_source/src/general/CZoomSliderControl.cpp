// CZoomSliderControl.cpp - arc rendering and pointer tracking.
#include "CZoomSliderControl.h"

#include <math.h>
#include "App.h"
#include "DGHelper.h"

namespace {
const double SliderPi = 3.141590118408203125;

// The original rounds the angle to float before passing it to sin and cos.
inline float SliderAngle(float progress, int maxAngle)
{
    double distance = fabs(static_cast<double>(0.5f - progress));
    return static_cast<float>((distance + distance) * maxAngle * SliderPi / 180.0);
}
}

CZoomSliderControl::CZoomSliderControl(const char* knob, const char* track)
    : CSliderControl(knob, track, Vertical), m_maxAngle(0), m_centerOffset(0), m_radius(0),
      m_locked(false)
{
    SetProgress(1.0f);
    SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    ImageRes knobImage(knob);
    GetImageSize(knobImage.GetSurface(), m_touchWidth, m_touchHeight);
}

void CZoomSliderControl::SetParams(int maxAngle, int centerOffset, int radius)
{
    m_maxAngle = maxAngle;
    m_centerOffset = centerOffset;
    m_radius = radius;
    App* app = static_cast<App*>(WindowApp::m_instance);
    SetOutsetSpacing(0, centerOffset + (app->m_helper->m_screenWidth >> 1));
}

void CZoomSliderControl::Paint(ICGraphics2d& graphics)
{
    DrawImage(graphics, m_track, 0, 0, 9, 0);
    float progress = m_progress;
    double angle = SliderAngle(progress, m_maxAngle);
    float vertical = m_radius * static_cast<float>(sin(angle));
    int x = static_cast<int>(m_radius * static_cast<float>(cos(angle)) - m_centerOffset);
    int y = static_cast<int>(vertical);
    if (progress > 0.5f) {
        y = -y;
    }
    DrawImage(graphics, m_knob, x - (m_knobWidth >> 1),
              (m_trackHeight >> 1) - y - (m_knobHeight >> 1), 9, 0);
}

void CZoomSliderControl::HandleLock(Event& event)
{
    float progress = m_progress;
    double angle = SliderAngle(progress, m_maxAngle);
    int radius = m_radius;
    double sine = sin(angle);
    double cosine = cos(angle);
    int trackHeight = m_trackHeight;
    float vertical = radius * static_cast<float>(sine);
    if (progress > 0.5f) {
        vertical = -vertical;
    }
    int x = static_cast<int>(radius * static_cast<float>(cosine) - m_centerOffset);
    bool locked = false;
    if (fabsf(static_cast<float>(x - event.m_param1)) < m_touchWidth / 2) {
        int y = (trackHeight >> 1) - static_cast<int>(vertical);
        locked = fabsf(static_cast<float>(y - event.m_param2)) < m_touchHeight / 2;
    }
    m_locked = locked;
    event.Clear();
}

void CZoomSliderControl::HandleTouch(Event& event)
{
    if (m_locked) {
        int x = m_centerOffset + event.m_param1;
        if (x == 0) {
            return;
        }
        int y = (m_trackHeight >> 1) - event.m_param2;
        int angle = static_cast<int>(atan(static_cast<float>(y) / x) * 180.0 / SliderPi);
        if (m_maxAngle < angle) {
            angle = m_maxAngle;
        }
        if (angle < -m_maxAngle) {
            angle = -m_maxAngle;
        }
        SetProgress(1.0f - static_cast<float>(angle + m_maxAngle) / (m_maxAngle * 2));
    }
    event.Clear();
}
