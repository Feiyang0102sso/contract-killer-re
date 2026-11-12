// CCheckBox.h - check box window: background, check mark and pressed highlight images.
// Size 0xb8 from the np_malloc in CFacebookLoginDialog::CFacebookLoginDialog() (0xf9e7c).
#ifndef CCHECKBOX_H
#define CCHECKBOX_H

#include "WindowBase.h"

class ImageWindow;

class CCheckBox : public Window {
public:
    /** `commandParam` goes with the checked / unchecked tunnel command. */
    explicit CCheckBox(int commandParam);
    virtual ~CCheckBox();
    virtual void OnPointerEvent(Event& event);

    void HandlePointerReleased();
    bool IsCheched();
    void UpdateCheckStatus();
    void SetChecked(bool checked);

    ImageWindow* m_check;          // 0xa4
    ImageWindow* m_pressed;        // 0xa8
    bool m_checked;                // 0xac
    int m_commandParam;            // 0xb0
    bool m_enabled;                // 0xb4
};

#endif
