// CDH_RadioButtonWidget.h - an image and label for one choice.
#ifndef CDH_RADIOBUTTONWIDGET_H
#define CDH_RADIOBUTTONWIDGET_H

#include "WindowBase.h"

class ImageWindow;

class CDH_RadioButtonWidget : public Window {
public:
    enum { SelectCommand = 0x2074d009 };

    CDH_RadioButtonWidget();
    virtual ~CDH_RadioButtonWidget();
    virtual void OnPointerEvent(Event& event);
    virtual void Init(bool selected, XString label, int font);
    virtual void SetValue(bool selected);

private:
    bool m_fieldA4;        // 0xa4, initialized by the original constructor
    int m_fieldA8;         // 0xa8, initialized by the original constructor
    ImageWindow* m_image; // 0xac
    ImageRes m_off;        // 0xb0
    ImageRes m_on;         // 0xb8
};

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char RadioButtonSizeCheck[sizeof(CDH_RadioButtonWidget) == 0xc0 ? 1 : -1];
#endif

#endif
