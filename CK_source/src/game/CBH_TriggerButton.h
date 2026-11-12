// CBH_TriggerButton.h - two-state image button for entering and leaving cover.
#ifndef CBH_TRIGGERBUTTON_H
#define CBH_TRIGGERBUTTON_H

#include "ButtonWindow.h"

class CBH_TriggerButton : public ButtonWindow {
public:
    CBH_TriggerButton(const char* active, const char* inactive);
    virtual ~CBH_TriggerButton() {}
    void Enable(bool enabled);
    void ChangeState();

    virtual void Paint(ICGraphics2d& graphics);
    virtual void OnPointerPressed(Event& event);
    virtual void OnPointerReleased(Event& event);

private:
    void SetButtonSize();
    ImageRes m_activeImage;             // 0x124
    ImageRes m_inactiveImage;           // 0x12c
    ICRenderSurface* m_activeSurface;   // 0x134
    ICRenderSurface* m_inactiveSurface; // 0x138
    unsigned int m_activeWidth;         // 0x13c
    unsigned int m_activeHeight;        // 0x140
    unsigned int m_inactiveWidth;       // 0x144
    unsigned int m_inactiveHeight;      // 0x148
    bool m_active;                      // 0x14c
    bool m_acceptsInput;                // 0x14d
};

typedef char TriggerButtonSizeCheck[sizeof(CBH_TriggerButton) == 0x150 ? 1 : -1];

#endif
