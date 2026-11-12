#ifndef CBH_PROGRESSBAR_H
#define CBH_PROGRESSBAR_H

#include "CBH_Button.h"

enum TextAlign { TEXT_ALIGN_LEFT, TEXT_ALIGN_RIGHT, TEXT_ALIGN_CENTER };

class CBH_ProgressBar : public CBH_Button {
public:
    enum EStyle { STYLE_VALUE, STYLE_CLOCK };
    enum DisplayFlag { DISPLAY_CURRENT = 1, DISPLAY_MAX = 2, HIDE_FILL = 4, CLEAR_TEXT = 8, MANUAL_PERCENT = 0x20 };

    CBH_ProgressBar(int color, int width, int secondaryColor);
    CBH_ProgressBar(int width, const char* fill, const char* left, const char* right,
                    const char* background, const char* secondary);
    virtual ~CBH_ProgressBar();
    virtual void Init();
    virtual void Update();
    virtual void Paint(ICGraphics2d& graphics);
    virtual void SetCurrValue(int value);
    virtual void ReleaseResources();

    void CreateImpl(int width);
    int GetFillWidth();
    void SetTextAlign(TextAlign align);
    void SetDescImage(const char* name);
    void SetStyle(EStyle style);
    void SetText(XString text);
    void UpdateBarParams();
    void SetMaxValue(int maximum);
    void SetSpecialPercents(int percent);

    float m_deltaPercent;                    // 0xd4
    int m_maximum;
    int m_current;
    float m_percent;
    int m_fieldsE4[2];
    const char* m_descName;
    int m_fieldF0;
    int m_displayFlags;
    TextAlign m_textAlign;
    int m_drawAlign;
    int m_textX;
    int m_textY;
    XString m_text;
    const char* m_leftName;
    const char* m_rightName;
    const char* m_backgroundName;
    const char* m_fillName;
    const char* m_secondaryName;
    ICRenderSurface* m_fill;
    ICRenderSurface* m_secondary;
    ICRenderSurface* m_desc;
    ICRenderSurface* m_left;
    ICRenderSurface* m_right;
    ICRenderSurface* m_background;
    EStyle m_style;
    bool m_drawText;
};

class CEnergyProgressBar : public CBH_ProgressBar {
public:
    CEnergyProgressBar(int color, int width);
    virtual void Update();
    void SetValues(int current, int maximum, int seconds);

private:
    int m_energy;
    int m_energyMaximum;
    int m_cooldownSeconds;
};

class CAnimatedProgressBar : public CBH_ProgressBar {
public:
    CAnimatedProgressBar(int color, int width, int secondaryColor);
    CAnimatedProgressBar(int width, const char* fill, const char* left, const char* right,
                         const char* background, const char* secondary);
    virtual void Update();
    virtual void SetCurrValueWithAnimation(int value);
    virtual void SetSpecialPercents(int percent);

private:
    unsigned int m_started;
    float m_changePerMs;
};

#endif
