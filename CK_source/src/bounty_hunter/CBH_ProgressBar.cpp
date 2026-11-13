#include "CBH_ProgressBar.h"

#include "App.h"
#include "CBH_SurfaceManager.h"
#include "CFontMgr.h"
#include "CStdUtil_Cocoa.h"
#include "DGHelper.h"
#include "SScreenAdaptingConstant.h"

// file statics (original symbols __ZL12COLOR_IMAGES / __ZL12TEXT_Y_SHIFT, not an anonymous namespace)
// The trailing entries are written as 0: clang-211 treats NULL (__null) as a side effect and would emit the
// array eagerly; the original emits it lazily, after the CBH_ProgressBar vtable.
static const char* const COLOR_IMAGES[] = {
    "IDB_HUD_PROGRESSBAR_BLUE", "IDB_HUD_PROGRESSBAR_BROWN", "IDB_HUD_PROGRESSBAR_DARK_RED",
    "IDB_HUD_PROGRESSBAR_GREEN", "IDB_HUD_PROGRESSBAR_ORANGE", "IDB_HUD_PROGRESSBAR_SWAMP",
    "IDB_HUD_PROGRESSBAR_ORANGE_RED", 0, 0, 0
};
static const SScreenAdaptingConstant TEXT_Y_SHIFT(2);

static CBH_SurfaceManager* Surfaces()
{
    return static_cast<App*>(WindowApp::m_instance)->m_surfaceManager;
}

void CBH_ProgressBar::Init()
{
}

void CBH_ProgressBar::Update()
{
    Window::Update();
}

void CBH_ProgressBar::Paint(ICGraphics2d& graphics)
{
    int y = 0;
    unsigned int inset = 0;
    if (m_desc != NULL) {
        inset = Surfaces()->GetSurfaceWidth(m_desc) / 2;
        unsigned int iconHeight = Surfaces()->GetSurfaceHeight(m_desc);
        unsigned int capHeight = Surfaces()->GetSurfaceHeight(m_left);
        if (iconHeight > capHeight) {
            y = iconHeight / 2 - capHeight / 2;
        }
    }

    DrawImage(graphics, m_left, inset, y, 0, 0);
    int barX = inset + Surfaces()->GetSurfaceWidth(m_right);
    DrawTileImageHoriz(graphics, m_background, barX, y, GetFillWidth());
    if ((m_displayFlags & HIDE_FILL) == 0) {
        float percent = m_percent;
        float first = percent;
        float second = percent + m_deltaPercent;
        if (m_deltaPercent < 0.0f) {
            first = second;
        }
        if (percent > second) {
            second = percent;
        }
        float animatedWidth = GetFillWidth();
        if (first < 0.0f) {
            first = 0.0f;
        }
        float fillWidth = GetFillWidth();
        float animated = second * animatedWidth / 100.0f;
        int filled = static_cast<int>(first * fillWidth / 100.0f);
        if (filled > GetFillWidth()) {
            filled = GetFillWidth();
        }
        DrawTileImageRect(graphics, m_fill, barX, y, filled, ImageHeight(m_fill));
        if (m_secondaryName != NULL && m_deltaPercent != 0.0f) {
            int secondaryWidth = static_cast<int>(animated) - filled;
            DrawTileImageRect(graphics, m_secondary, barX + filled, y, secondaryWidth, ImageHeight(m_secondary));
        }
    }
    DrawImage(graphics, m_right, barX + GetFillWidth(), y, 0, 0);
    if (m_drawText) {
        CFont* font = CFontMgr::GetInstance()->GetFont(8);
        int x = m_textX;
        int y = m_textY;
        const wchar_t* text = m_text.c_str();
        DrawFontText(graphics, font, text, x, y + TEXT_Y_SHIFT(), m_drawAlign);
    }
    if (m_desc != NULL) {
        DrawImage(graphics, m_desc, 0, 0, 0, 0);
    }
    Window::Paint(graphics);
}

int CBH_ProgressBar::GetFillWidth()
{
    return m_width - Surfaces()->GetSurfaceWidth(m_desc) - Surfaces()->GetSurfaceWidth(m_left)
        - Surfaces()->GetSurfaceWidth(m_right);
}

CBH_ProgressBar::CBH_ProgressBar(int color, int width, int secondaryColor)
    : m_deltaPercent(0.0f), m_percent(0.0f),
      m_leftName("IDB_HUD_PROGRESSBAR_LEFT"), m_rightName("IDB_HUD_PROGRESSBAR_RIGHT"),
      m_backgroundName("IDB_HUD_PROGRESSBAR"), m_desc(NULL), m_style(STYLE_VALUE), m_drawText(true)
{
    m_fillName = COLOR_IMAGES[color];
    const char* secondaryName = NULL;
    if (secondaryColor >= 0) {
        secondaryName = COLOR_IMAGES[secondaryColor];
    }
    m_secondaryName = secondaryName;
    CreateImpl(width);
}

CBH_ProgressBar::CBH_ProgressBar(int width, const char* fill, const char* left, const char* right,
                               const char* background, const char* secondary)
    : m_deltaPercent(0.0f), m_percent(0.0f),
      m_leftName(left), m_rightName(right), m_backgroundName(background), m_fillName(fill),
      m_secondaryName(secondary), m_desc(NULL), m_style(STYLE_VALUE), m_drawText(true)
{
    CreateImpl(width);
}

CBH_ProgressBar::~CBH_ProgressBar()
{
    ReleaseResources();
}

void CBH_ProgressBar::CreateImpl(int width)
{
    m_left = Surfaces()->CreateSurface(m_leftName, true);
    m_right = Surfaces()->CreateSurface(m_rightName, true);
    m_background = Surfaces()->CreateSurface(m_backgroundName, true);
    m_fill = Surfaces()->CreateSurface(m_fillName, true);
    if (m_secondaryName != NULL) {
        m_secondary = Surfaces()->CreateSurface(m_secondaryName, true);
    }
    SetLayoutType(LAYOUT_NONE);
    int height = Surfaces()->GetSurfaceHeight(m_background);
    SetDesiredWidth(width);
    SetDesiredHeight(height);
    SetSize(width, height);
    m_descName = NULL;
    m_maximum = -1;
    m_current = -1;
    SetTextAlign(TEXT_ALIGN_CENTER);
    m_displayFlags = DISPLAY_CURRENT | DISPLAY_MAX;
}

void CBH_ProgressBar::SetTextAlign(TextAlign align)
{
    m_textAlign = align;
    switch (align) {
    case TEXT_ALIGN_CENTER:
        m_textX = m_width / 2;
        m_textY = m_height / 2;
        m_drawAlign = ALIGN_HCENTER | ALIGN_VCENTER;
        break;
    case TEXT_ALIGN_RIGHT: {
        int fillWidth = GetFillWidth();
        m_textX = fillWidth + ImageWidth(m_right);
        int inset = 10;
        if (App::IsHD()) {
            inset = 20;
        }
        m_textX += inset;
        m_textY = m_height / 2;
        m_drawAlign = ALIGN_RIGHT | ALIGN_VCENTER;
        break;
    }
    case TEXT_ALIGN_LEFT:
        m_textX = ImageWidth(m_desc);
        m_textY = m_height / 2;
        m_drawAlign = ALIGN_LEFT;
        break;
    }
}

void CBH_ProgressBar::SetDescImage(const char* name)
{
    if (m_desc != NULL && m_descName != NULL) {
        Surfaces()->ReleaseSurface(m_descName);
    }
    m_desc = Surfaces()->CreateSurface(name, true);
    m_descName = name;
    m_height = ImageHeight(m_desc);
    SetSize(m_width, m_height);
    SetDesiredHeight(m_height);
    SetTextAlign(m_textAlign);
}

void CBH_ProgressBar::ReleaseResources()
{
    Surfaces()->ReleaseSurface(m_leftName);
    Surfaces()->ReleaseSurface(m_rightName);
    Surfaces()->ReleaseSurface(m_backgroundName);
    Surfaces()->ReleaseSurface(m_fillName);
    if (m_descName != NULL) {
        Surfaces()->ReleaseSurface(m_descName);
    }
    if (m_secondaryName != NULL) {
        Surfaces()->ReleaseSurface(m_secondaryName);
    }
}

void CBH_ProgressBar::SetStyle(EStyle style)
{
    m_style = style;
}

void CBH_ProgressBar::SetText(XString text)
{
    m_text = text;
}

void CBH_ProgressBar::SetCurrValue(int value)
{
    if (m_current != value) {
        m_current = value;
        UpdateBarParams();
    }
}

void CBH_ProgressBar::UpdateBarParams()
{
    if ((m_displayFlags & (DISPLAY_CURRENT | DISPLAY_MAX)) == (DISPLAY_CURRENT | DISPLAY_MAX)) {
        m_text = *XString::pEmpty;
        if (m_style == STYLE_VALUE) {
            m_text.Append(XString::ToString(m_current));
            if (m_maximum > 0) {
                const wchar_t* separator = L"/";
                m_text.Append(separator, wcslen(separator) * sizeof(wchar_t));
                m_text.Append(XString::ToString(m_maximum));
            }
        } else if (m_style == STYLE_CLOCK) {
            m_text.Append(DGHelper::ClockStringFromSeconds(m_current));
        }
    } else if (m_displayFlags & DISPLAY_CURRENT) {
        m_text = *XString::pEmpty;
        m_text = XString::ToString(m_current);
    } else if (m_displayFlags & CLEAR_TEXT) {
        m_text = *XString::pEmpty;
    }
    if (m_maximum > 0 && (m_displayFlags & MANUAL_PERCENT) == 0) {
        m_percent = m_current * 100 / m_maximum;
    }
}

void CBH_ProgressBar::SetMaxValue(int maximum)
{
    if (m_maximum != maximum) {
        m_maximum = maximum;
        UpdateBarParams();
    }
}

void CBH_ProgressBar::SetSpecialPercents(int percent)
{
    m_percent = percent;
}

CEnergyProgressBar::CEnergyProgressBar(int color, int width)
    : CBH_ProgressBar(color, width, -1)
{
}

void CEnergyProgressBar::SetValues(int current, int maximum, int seconds)
{
    m_energy = current;
    m_energyMaximum = maximum;
    m_cooldownSeconds = seconds;
}

void CEnergyProgressBar::Update()
{
    SetCurrValue(m_energy);
    SetMaxValue(m_energyMaximum);
    UpdateBarParams();
    unsigned int uptime = CStdUtil_Cocoa::GetUpTimeMS();
    if ((uptime / 1000 & 4) < 4 && m_energy < m_energyMaximum) {
        m_text = DGHelper::ClockStringFromSeconds(m_cooldownSeconds);
    }
}

CAnimatedProgressBar::CAnimatedProgressBar(int width, const char* fill, const char* left, const char* right,
                                           const char* background, const char* secondary)
    : CBH_ProgressBar(width, fill, left, right, background, secondary)
{
}

CAnimatedProgressBar::CAnimatedProgressBar(int color, int width, int secondaryColor)
    : CBH_ProgressBar(color, width, secondaryColor)
{
}

void CAnimatedProgressBar::SetCurrValueWithAnimation(int value)
{
    m_started = CStdUtil_Cocoa::GetUpTimeMS();
    int previous = m_current;
    m_deltaPercent = (previous - value) * 100 / m_maximum;
    float delta = -100.0f;
    if (!(m_deltaPercent < -100.0f)) {
        delta = 100.0f < m_deltaPercent ? 100.0f : m_deltaPercent;
    }
    m_deltaPercent = delta;
    m_changePerMs = delta * 0.0025f;
    if (previous != value) {
        m_current = value;
        UpdateBarParams();
    }
}

void CAnimatedProgressBar::SetSpecialPercents(int percent)
{
    m_started = CStdUtil_Cocoa::GetUpTimeMS();
    float nextPercent = percent;
    m_deltaPercent = m_percent - nextPercent;
    if (m_deltaPercent < -100.0f) {
        m_deltaPercent = -100.0f;
        m_changePerMs = -0.25f;
    } else {
        float delta = m_deltaPercent;
        if (100.0f < delta) {
            delta = 100.0f;
        }
        m_deltaPercent = delta;
        m_changePerMs = delta * 0.0025f;
        if (delta > 0.0f) {
            m_deltaPercent = 0.0f;
        }
    }
    m_percent = nextPercent;
}

void CAnimatedProgressBar::Update()
{
    if (m_deltaPercent > 0.0f) {
        float elapsed = static_cast<unsigned int>(CStdUtil_Cocoa::GetUpTimeMS()) - m_started;
        if (elapsed > 200.0f) {
            m_deltaPercent -= WindowApp::m_instance->m_updateFrameTime * m_changePerMs;
            if (m_deltaPercent < 0.0f) {
                m_deltaPercent = 0.0f;
            }
            return;
        }
    }
    if (m_deltaPercent < 0.0f) {
        float elapsed = static_cast<unsigned int>(CStdUtil_Cocoa::GetUpTimeMS()) - m_started;
        if (elapsed > 200.0f) {
            m_deltaPercent -= WindowApp::m_instance->m_updateFrameTime * m_changePerMs;
            if (m_deltaPercent > 0.0f) {
                m_deltaPercent = 0.0f;
            }
        }
    }
}

typedef char AssertProgressBarSize[sizeof(CBH_ProgressBar) == 0x140 ? 1 : -1];
typedef char AssertEnergyProgressBarSize[sizeof(CEnergyProgressBar) == 0x14c ? 1 : -1];
typedef char AssertAnimatedProgressBarSize[sizeof(CAnimatedProgressBar) == 0x148 ? 1 : -1];
