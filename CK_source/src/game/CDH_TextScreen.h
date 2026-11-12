#ifndef CDH_TEXTSCREEN_H
#define CDH_TEXTSCREEN_H

#include "CDH_BaseScreen.h"

class CDH_TextScreen : public CDH_BaseScreen {
public:
    CDH_TextScreen();
    CDH_TextScreen(const char* caption, const char* text);
    virtual ~CDH_TextScreen();
    virtual void Init();
    virtual void OnCommand(Event& event);
    virtual Window* PopulateSofkeys();
    void setText(const char* text);
    void setText(const XString& text);

    XString m_text;
    int m_field18c;
    int m_experience;
    int m_maxWidth;
    int m_font;
    int m_textAlign;
};

#endif
