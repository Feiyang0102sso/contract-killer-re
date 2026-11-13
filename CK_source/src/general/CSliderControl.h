// Originally declared by b057x for CSliderControl.cpp and the inline HUD access.
// CSliderControl.h - slider window of the HUD.
#ifndef CSLIDERCONTROL_H
#define CSLIDERCONTROL_H

#include "WindowBase.h"

class CSliderControl : public Window {
public:
    enum ESliderType {
        Vertical,
        Horizontal
    };

    CSliderControl(const char* knob, const char* track, ESliderType type);
    virtual ~CSliderControl() {}

    /** Enable or disable the slider (input and drawing flags). */
    void Enable(bool enable);
    void SetProgress(float progress);
    /** GamePlayHUD at 0x14fda8 reads the progress at +0xf8. */
    float GetValue() const { return m_progress; }

    virtual void OnPointerPressed(Event& event);
    virtual void OnPointerDragged(Event& event);
    virtual void OnPointerReleased(Event& event);
    virtual void Paint(ICGraphics2d& graphics);

protected:
    void CalcPositionRange();
    void SetProgressByTouch(int x, int y);
    void CorrectProgress(int dx, int dy);
    bool IsPointerOnSlider(int x, int y);

    ImageRes m_knobImage;             // 0xa4
    ImageRes m_trackImage;            // 0xac
    ICRenderSurface* m_knob;          // 0xb4
    ICRenderSurface* m_track;         // 0xb8
    int m_trackWidth;                 // 0xbc
    int m_trackHeight;                // 0xc0
    int m_knobWidth;                  // 0xc4
    int m_knobHeight;                 // 0xc8
    int m_knobX;                      // 0xcc
    int m_knobY;                      // 0xd0
    int m_knobOffsetX;                // 0xd4
    int m_knobOffsetY;                // 0xd8
    ESliderType m_type;               // 0xdc
    int m_startInset;                 // 0xe0
    int m_endInset;                   // 0xe4
    int m_crossAxisCenter;            // 0xe8
    int m_minPosition;                // 0xec
    int m_maxPosition;                // 0xf0
    int m_positionRange;              // 0xf4
    float m_progress;                 // 0xf8
    int m_lastTouchX;                 // 0xfc
    int m_lastTouchY;                 // 0x100
    bool m_dragging;                  // 0x104
};

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char CSliderControlSizeCheck[sizeof(CSliderControl) == 0x108 ? 1 : -1];
#endif

#endif
