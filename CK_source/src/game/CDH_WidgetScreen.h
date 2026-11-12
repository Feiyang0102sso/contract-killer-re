#ifndef CDH_WIDGETSCREEN_H
#define CDH_WIDGETSCREEN_H

#include "CDH_ThemeWindow.h"

class CDH_WidgetScreen : public CDH_ThemeWindow {
public:
    enum eLayoutPosition { Caption, Body, Footer, LayoutPositionCount };

    CDH_WidgetScreen();
    virtual ~CDH_WidgetScreen();
    virtual void Init();
    void setLayoutWidget(Window* widget, eLayoutPosition position);
    void setCaptionText(const char* text, bool showClose);
    void setCaptionText(const XString& text, bool showClose);

protected:
    Window* m_sections[LayoutPositionCount];
    XString m_caption;
    XString m_subtitle;
    Window* m_captionRow;
};

#endif
