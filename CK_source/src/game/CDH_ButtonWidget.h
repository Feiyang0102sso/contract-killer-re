#ifndef CDH_BUTTONWIDGET_H
#define CDH_BUTTONWIDGET_H

#include "WindowBase.h"

class HorizImageWindow;
class TextWindow;

class CDH_ButtonWidget : public Window {
public:
    enum button_type_t { Normal, Add, Info };

    CDH_ButtonWidget();
    CDH_ButtonWidget(XString text, int font);
    virtual ~CDH_ButtonWidget();
    virtual void Init();
    virtual void OnPointerEvent(Event& event);
    virtual void SetRect(int x, int y, int width, int height, bool force);
    void init(XString text, int font, const char* tile);
    void SetType(button_type_t type);
    void setText(const char* text);
    void setContainerAlign(int alignment);

    Window* m_content;
    TextWindow* m_label;
    int m_font;
    XString m_text;
    const char* m_tile;
    int m_command;
    int m_textAlign;
    int m_textInset;
    int m_textWidth;
    bool m_disabledBackgroundAttached;
    HorizImageWindow* m_disabledBackground;
};

#endif
