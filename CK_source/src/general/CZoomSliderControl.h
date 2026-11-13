// CZoomSliderControl.h - curved zoom slider used by the scope HUD.
#ifndef CZOOMSLIDERCONTROL_H
#define CZOOMSLIDERCONTROL_H

#include "CSliderControl.h"

class CZoomSliderControl : public CSliderControl {
public:
    CZoomSliderControl(const char* knob, const char* track);
    virtual ~CZoomSliderControl() {}

    void SetParams(int maxAngle, int centerOffset, int radius);
    virtual void Paint(ICGraphics2d& graphics);

    virtual void OnPointerPressed(Event& event) { HandleLock(event); }
    virtual void OnPointerDragged(Event& event) { HandleTouch(event); }
    virtual void OnPointerReleased(Event& event) { event.Clear(); }

private:
    void HandleLock(Event& event);
    void HandleTouch(Event& event);

    int m_maxAngle;       // 0x108
    int m_centerOffset;   // 0x10c
    int m_radius;         // 0x110
    int m_touchWidth;     // 0x114
    int m_touchHeight;    // 0x118
    bool m_locked;        // 0x11c
};

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char CZoomSliderControlSizeCheck[sizeof(CZoomSliderControl) == 0x120 ? 1 : -1];
#endif

#endif
