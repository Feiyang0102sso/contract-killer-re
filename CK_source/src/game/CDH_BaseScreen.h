#ifndef CDH_BASESCREEN_H
#define CDH_BASESCREEN_H

#include "CDH_WidgetScreen.h"

class CDH_ButtonWidget;
class HorizImageWindow;

class CDH_BaseScreen : public CDH_WidgetScreen {
public:
    CDH_BaseScreen();
    virtual ~CDH_BaseScreen();
    virtual void Init();
    virtual void OnCommand(Event& event);
    virtual void OnLostFocus();
    virtual void OnGetFocus();

    void setButtons(const char* left, const char* right);
    HorizImageWindow* createLineSelector(int column, int row);

protected:
    XString m_field13c;
    XString m_leftText;
    XString m_rightText;
    ImageRes m_selector[3];
    ImageRes m_splitter[3];
    CDH_ButtonWidget* m_leftButton;
    CDH_ButtonWidget* m_rightButton;
    Window* m_buttons;
    int m_field184;
};

#endif
