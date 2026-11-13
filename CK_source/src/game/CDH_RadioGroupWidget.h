// CDH_RadioGroupWidget.h - a group that keeps a single radio choice selected.
#ifndef CDH_RADIOGROUPWIDGET_H
#define CDH_RADIOGROUPWIDGET_H

#include "WindowBase.h"

class CDH_RadioButtonWidget;

class CDH_RadioGroupWidget : public Window {
public:
    CDH_RadioGroupWidget();
    virtual ~CDH_RadioGroupWidget();
    void AddButton(CDH_RadioButtonWidget* button);
    int GetValue();
    void SetValue(int selected);
    virtual void OnCommand(Event& event);

private:
    int m_selected;                          // 0xa4, assigned by SetValue
    Vector<CDH_RadioButtonWidget*> m_buttons; // 0xa8
};

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char RadioGroupSizeCheck[sizeof(CDH_RadioGroupWidget) == 0xb8 ? 1 : -1];
#endif

#endif
