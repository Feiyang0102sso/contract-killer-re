#ifndef CDH_CURRENCYWIDGET_H
#define CDH_CURRENCYWIDGET_H

#include "WindowBase.h"

class ImageWindow;
class TextWindow;

class CDH_CurrencyWidget : public Window {
public:
    enum eValueType { Credits, SkillPoints, HunterPoints };

    explicit CDH_CurrencyWidget(eValueType type);
    virtual ~CDH_CurrencyWidget() {}
    void init();
    void updateData();
    void setText(XString format);
    void setValue(int value);

private:
    eValueType m_type;
    int m_small;
    int m_value;
    ImageWindow* m_image;
    TextWindow* m_label;
    XString m_format;
    CFont* m_font;
};

#endif
