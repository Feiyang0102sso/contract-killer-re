#include "CBH_ProgressBar.h"

#include "App.h"
#include "CBH_SurfaceManager.h"
#include "CFontMgr.h"
#include "CStdUtil_Cocoa.h"
#include "DGHelper.h"
#include "SScreenAdaptingConstant.h"

namespace {
const char* const COLOR_IMAGES[] = {
    "IDB_HUD_PROGRESSBAR_BLUE", "IDB_HUD_PROGRESSBAR_BROWN", "IDB_HUD_PROGRESSBAR_DARK_RED",
    "IDB_HUD_PROGRESSBAR_GREEN", "IDB_HUD_PROGRESSBAR_ORANGE", "IDB_HUD_PROGRESSBAR_SWAMP",
    "IDB_HUD_PROGRESSBAR_ORANGE_RED", NULL, NULL, NULL
};
const SScreenAdaptingConstant TEXT_Y_SHIFT(2);

CBH_SurfaceManager* Surfaces()
{
    return static_cast<App*>(WindowApp::m_instance)->m_surfaceManager;
}

float ClampPercent(float value)
{
    if (value < 0.0f) {
        return 0.0f;
    }
    if (value > 100.0f) {
        return 100.0f;
    }
    return value;
}
}

CBH_ProgressBar::CBH_ProgressBar(int color, int width, int secondaryColor)
    : m_deltaPercent(0.0f), m_percent(0.0f), m_descName(NULL),
      m_leftName("IDB_HUD_PROGRESSBAR_LEFT"), m_rightName("IDB_HUD_PROGRESSBAR_RIGHT"),
      m_backgroundName("IDB_HUD_PROGRESSBAR"), m_fillName(COLOR_IMAGES[color]), m_secondaryName(NULL),
      m_secondary(NULL), m_desc(NULL), m_style(STYLE_VALUE), m_drawText(true)
{
    if (secondaryColor >= 0) {
        m_secondaryName = COLOR_IMAGES[secondaryColor];
    }
    CreateImpl(width);
}

CBH_ProgressBar::CBH_ProgressBar(int width, const char* fill, const char* left, const char* right,
                               const char* background, const char* secondary)
    : m_deltaPercent(0.0f), m_percent(0.0f), m_descName(NULL),
      m_leftName(left), m_rightName(right), m_backgroundName(background), m_fillName(fill),
      m_secondaryName(secondary), m_secondary(NULL), m_desc(NULL), m_style(STYLE_VALUE), m_drawText(true)
{
    CreateImpl(width);
}

CBH_ProgressBar::~CBH_ProgressBar()
{
    ReleaseResources();
}

void CBH_ProgressBar::Init()
{
}

void CBH_ProgressBar::Update()
{
    Window::Update();
}

void CBH_ProgressBar::CreateImpl(int width)
{
    CBH_SurfaceManager* surfaces = Surfaces();
    m_left = surfaces->CreateSurface(m_leftName, true);
    m_right = surfaces->CreateSurface(m_rightName, true);
    m_background = surfaces->CreateSurface(m_backgroundName, true);
    m_fill = surfaces->CreateSurface(m_fillName, true);
    if (m_secondaryName != NULL) {
        m_secondary = surfaces->CreateSurface(m_secondaryName, true);
    }
    SetLayoutType(LAYOUT_NONE);
    int height = ImageHeight(m_background);
    SetDesiredWidth(width);
    SetDesiredHeight(height);
    SetSize(width, height);
    m_maximum = -1;
    m_current = -1;
    SetTextAlign(TEXT_ALIGN_CENTER);
    m_displayFlags = DISPLAY_CURRENT | DISPLAY_MAX;
}

int CBH_ProgressBar::GetFillWidth()
{
    CBH_SurfaceManager* surfaces = Surfaces();
    return m_width - surfaces->GetSurfaceWidth(m_desc) - surfaces->GetSurfaceWidth(m_left)
        - surfaces->GetSurfaceWidth(m_right);
}

void CBH_ProgressBar::Paint(ICGraphics2d& graphics)
{
    int y = 0;
    int inset = 0;
    if (m_desc != NULL) {
        inset = ImageWidth(m_desc) / 2;
        int iconHeight = ImageHeight(m_desc);
        int capHeight = ImageHeight(m_left);
        if (iconHeight > capHeight) {
            y = iconHeight / 2 - capHeight / 2;
        }
    }
    DrawImage(graphics, m_left, inset, y, 0, 0);
    int barX = inset + ImageWidth(m_right);
    int fillWidth = GetFillWidth();
    if (fillWidth > 0) {
        DrawTileImageHoriz(graphics, m_background, barX, y, fillWidth);
        if ((m_displayFlags & HIDE_FILL) == 0) {
            float first = m_percent;
            float second = m_percent;
            if (m_deltaPercent < 0.0f) {
                first += m_deltaPercent;
            } else {
                second += m_deltaPercent;
            }
            int filled = static_cast<int>(fillWidth * ClampPercent(first) / 100.0f);
            int animated = static_cast<int>(fillWidth * ClampPercent(second) / 100.0f);
            DrawTileImageRect(graphics, m_fill, barX, y, filled, ImageHeight(m_fill));
            if (m_secondary != NULL && m_deltaPercent != 0.0f) {
                DrawTileImageRect(graphics, m_secondary, barX + filled, y, animated - filled, ImageHeight(m_secondary));
            }
        }
    }
    DrawImage(graphics, m_right, barX + fillWidth, y, 0, 0);
    if (m_drawText) {
        CFont* font = CFontMgr::GetInstance()->GetFont(8);
        DrawFontText(graphics, font, m_text.c_str(), m_textX, m_textY + TEXT_Y_SHIFT(), m_drawAlign);
    }
    DrawImage(graphics, m_desc, 0, 0, 0, 0);
    Window::Paint(graphics);
}

void CBH_ProgressBar::SetTextAlign(TextAlign align)
{
    m_textAlign = align;
    if (align == TEXT_ALIGN_LEFT) {
        m_textX = ImageWidth(m_desc);
        m_drawAlign = ALIGN_LEFT;
    } else if (align == TEXT_ALIGN_RIGHT) {
        m_textX = GetFillWidth() + ImageWidth(m_right) + (App::IsHD() ? 20 : 10);
        m_drawAlign = ALIGN_RIGHT | ALIGN_VCENTER;
    } else if (align == TEXT_ALIGN_CENTER) {
        m_textX = m_width / 2;
        m_drawAlign = ALIGN_HCENTER | ALIGN_VCENTER;
    } else {
        return;
    }
    m_textY = m_height / 2;
}

void CBH_ProgressBar::SetDescImage(const char* name)
{
    CBH_SurfaceManager* surfaces = Surfaces();
    if (m_desc != NULL && m_descName != NULL) {
        surfaces->ReleaseSurface(m_descName);
    }
    m_desc = surfaces->CreateSurface(name, true);
    m_descName = name;
    int height = ImageHeight(m_desc);
    SetSize(m_width, height);
    SetDesiredHeight(height);
    SetTextAlign(m_textAlign);
}

void CBH_ProgressBar::ReleaseResources()
{
    CBH_SurfaceManager* surfaces = Surfaces();
    const char* names[] = {m_leftName, m_rightName, m_backgroundName, m_fillName, m_descName, m_secondaryName};
    for (unsigned int index = 0; index < sizeof(names) / sizeof(names[0]); ++index) {
        if (names[index] != NULL) {
            surfaces->ReleaseSurface(names[index]);
        }
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

void CBH_ProgressBar::UpdateBarParams()
{
    if ((m_displayFlags & (DISPLAY_CURRENT | DISPLAY_MAX)) == (DISPLAY_CURRENT | DISPLAY_MAX)) {
        m_text = XString();
        if (m_style == STYLE_CLOCK) {
            m_text.Append(DGHelper::ClockStringFromSeconds(m_current));
        } else if (m_style == STYLE_VALUE) {
            m_text.Append(XString(m_current));
            if (m_maximum > 0) {
                m_text.Append(XString(L"/"));
                m_text.Append(XString(m_maximum));
            }
        }
    } else if (m_displayFlags & DISPLAY_CURRENT) {
        m_text = XString(m_current);
    } else if (m_displayFlags & CLEAR_TEXT) {
        m_text = XString();
    }
    if (m_maximum > 0 && (m_displayFlags & MANUAL_PERCENT) == 0) {
        m_percent = m_current * 100 / m_maximum;
    }
}

CEnergyProgressBar::CEnergyProgressBar(int color, int width)
    : CBH_ProgressBar(color, width, -1), m_energy(0), m_energyMaximum(0), m_cooldownSeconds(0)
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
    if ((CStdUtil_Cocoa::GetUpTimeMS() / 1000 & 4) == 0 && m_energy < m_energyMaximum) {
        m_text = DGHelper::ClockStringFromSeconds(m_cooldownSeconds);
    }
}

CAnimatedProgressBar::CAnimatedProgressBar(int color, int width, int secondaryColor)
    : CBH_ProgressBar(color, width, secondaryColor), m_started(0), m_changePerMs(0.0f)
{
}

CAnimatedProgressBar::CAnimatedProgressBar(int width, const char* fill, const char* left, const char* right,
                                           const char* background, const char* secondary)
    : CBH_ProgressBar(width, fill, left, right, background, secondary), m_started(0), m_changePerMs(0.0f)
{
}

void CAnimatedProgressBar::SetCurrValueWithAnimation(int value)
{
    m_started = CStdUtil_Cocoa::GetUpTimeMS();
    m_deltaPercent = 0.0f;
    if (m_maximum > 0) {
        m_deltaPercent = (m_current - value) * 100 / m_maximum;
    }
    if (m_deltaPercent < -100.0f) {
        m_deltaPercent = -100.0f;
    } else if (m_deltaPercent > 100.0f) {
        m_deltaPercent = 100.0f;
    }
    m_changePerMs = m_deltaPercent * 0.0025f;
    SetCurrValue(value);
}

void CAnimatedProgressBar::SetSpecialPercents(int percent)
{
    m_started = CStdUtil_Cocoa::GetUpTimeMS();
    float delta = m_percent - percent;
    if (delta < -100.0f) {
        delta = -100.0f;
    } else if (delta > 0.0f) {
        delta = 0.0f;
    }
    m_deltaPercent = delta;
    m_changePerMs = delta * 0.0025f;
    m_percent = percent;
}

void CAnimatedProgressBar::Update()
{
    if (m_deltaPercent == 0.0f || CStdUtil_Cocoa::GetUpTimeMS() - m_started <= 200) {
        return;
    }
    float next = m_deltaPercent - WindowApp::m_instance->m_updateFrameTime * m_changePerMs;
    if (next * m_deltaPercent < 0.0f) {
        next = 0.0f;
    }
    m_deltaPercent = next;
}

typedef char AssertProgressBarSize[sizeof(CBH_ProgressBar) == 0x140 ? 1 : -1];
typedef char AssertEnergyProgressBarSize[sizeof(CEnergyProgressBar) == 0x14c ? 1 : -1];
typedef char AssertAnimatedProgressBarSize[sizeof(CAnimatedProgressBar) == 0x148 ? 1 : -1];
