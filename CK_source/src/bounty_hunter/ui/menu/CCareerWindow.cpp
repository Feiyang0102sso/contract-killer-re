#include "CCareerWindow.h"

#include "CBH_Player.h"
#include "CDrawUtil.h"
#include "CFontMgr.h"
#include "MathLib.h"
#include "SScreenAdaptingConstant.h"
#include "ServiceWindows.h"

namespace {
const SScreenAdaptingConstant ScrollWidth(3, 6);
const SScreenAdaptingConstant ScrollBottom(10, 20);
const SScreenAdaptingConstant RowRight(6, 12);
const SScreenAdaptingConstant TitleRight(5, 10);
const SScreenAdaptingConstant ContentRight(10, 20);
const SScreenAdaptingConstant RowInsetX(4, 8);
const SScreenAdaptingConstant RowInsetY(5, 10);

int Percent(int count, int total)
{
    int percent = static_cast<int>(static_cast<float>(count) / static_cast<float>(total) * 100.0f);
    int result = 0;
    if (percent >= 0) {
        if (percent > 100) {
            percent = 100;
        }
        result = percent;
    }
    return result;
}
}

CCareerWindow::CCareerWindow()
    : CBaseScreen(false, true, true, false, true, false), m_elapsedMS(0)
{
    SetTitleText(ResString("IDS_TITLE_CAREER"));
    FillStats();

    int titleHeight = m_titleHeight;
    SetInsetSpacing(titleHeight, 0, ContentRight(), 0);
    CFont* headingFont = CFontMgr::GetInstance()->GetFont(10);
    Window* headingRow = new Window(false);
    headingRow->SetPercentHeight(12, 0, 0);
    TextWindow* heading = new TextWindow(XString("GAME STATISTIC"), headingFont);
    heading->SetLayoutType(LAYOUT_SIMPLE);
    heading->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    headingRow->AddToFront(createTD(heading, 3), 0, 0);
    AddToFront(headingRow, 0, 0);
    headingRow->SetOutsetSpacing(0, 0, TitleRight(), 0);

    SeparateLineRed* line = new SeparateLineRed();
    line->SetPercentHeight(2, 0, 0);
    AddToFront(line, 0, 1);

    m_careerContent = new Window(false);
    m_careerContent->SetLayoutType(LAYOUT_SIMPLE);
    m_careerContent->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    m_careerContent->SetPercentHeight(100, 0, 0);
    m_careerContent->SetWidthByContent(0, 0);
    ScrollingContainer* scroll = new ScrollingContainer();
    scroll->SetCellPos(0, 0, 1, 1);
    scroll->SetDesiredWidth(ImageWidth(ImageRes("SUR_FRAME_RED_CAREER").GetSurface()));

    WindowTransparent* rows = new WindowTransparent(false);
    rows->SetLayoutType(LAYOUT_NONE);
    rows->SetHeightByContent(0, 0);
    rows->SetWidthByContent(0, 0);
    for (int index = 0; index < m_stats.size(); ++index) {
        StatItem* row = new StatItem(m_stats[index], index);
        row->SetOutsetSpacing(0, 0, RowRight(), 0);
        rows->AddToFront(row, 0, index);
    }
    scroll->AddToFront(rows);
    m_careerContent->AddToFront(scroll);

    VertScrollerSimpleRed* scroller = new VertScrollerSimpleRed(scroll);
    scroller->SetOutsetSpacing(0, 0, 0, ScrollBottom());
    scroller->SetCellPos(1, 0, 1, 1);
    m_careerContent->AddToFront(scroller);
    Window* contentCell = createTD(m_careerContent, 3);
    AddToFront(contentCell, 0, 2);
    m_careerContent->ClearFlags(FLAG_EVENTS_TO_CHILDREN);
    contentCell->SetPercentHeight(92, 0, 0);
}

void CCareerWindow::FillStats()
{
    m_stats.removeAllElements();
    const CPlayerGameStats& stats = CBH_Player::GetInstance()->m_gameStats;
    m_stats.addElement(StatItemInfo("IDS_CAREER_STAT_TOTAL_KILLS", stats.m_targetsKilled + stats.m_enemyKilled, false, true));
    m_stats.addElement(StatItemInfo("IDS_CARRER_STAT_HEADSHOTS_NUMBER", stats.m_headshotsNumber, false, true));
    m_stats.addElement(StatItemInfo("IDS_CAREER_STAT_CONTRACTS_COMPLETED", stats.m_successBounties, false, true));
    m_stats.addElement(StatItemInfo("IDS_CARRER_STAT_HUMANS_CAPTURED", stats.m_enemyTranquillized, false, true));
    m_stats.addElement(StatItemInfo("IDS_CAREER_STAT_ENCOUNTERS_NUMBER", stats.m_totalBounties, false, false));
    m_stats.addElement(StatItemInfo("IDS_CAREER_STAT_SUCCESS_CONTRACTS", Percent(stats.m_successBounties, stats.m_totalBounties), true, false));
    m_stats.addElement(StatItemInfo("IDS_CAREER_STAT_TARGETS_KILLED", stats.m_targetsKilled, false, false));
    m_stats.addElement(StatItemInfo("IDS_CARRER_STAT_FOES_KILLED", stats.m_enemyKilled, false, false));
    m_stats.addElement(StatItemInfo("IDS_CARRER_STAT_CIVILIANS_KILLED", stats.m_civiliansKilled, false, false));
    m_stats.addElement(StatItemInfo("IDS_CARRER_STAT_SHOTS_NUMBER", stats.m_shotsNumber, false, false));
    m_stats.addElement(StatItemInfo("IDS_CARRER_STAT_HITS_NUMBER", stats.m_hitsNumber, false, false));
    m_stats.addElement(StatItemInfo("IDS_CARRER_STAT_ACCURACY", Percent(stats.m_hitsNumber, stats.m_shotsNumber), true, false));
    m_stats.addElement(StatItemInfo("IDS_CARRER_STAT_PRECISION", Percent(stats.m_headshotsNumber, stats.m_hitsNumber), true, false));
    m_stats.addElement(StatItemInfo("IDS_CARRER_STAT_MONEY_EARNED", stats.m_moneyEarned, false, false));
    m_stats.addElement(StatItemInfo("IDS_CARRER_STAT_WEAPONS_OWNED", stats.m_weaponsEarned, false, false));
    m_stats.addElement(StatItemInfo("IDS_CARRER_STAT_X3_MULTIPLY_KILLS", stats.m_x3MultiplierKills, false, false));
    m_stats.addElement(StatItemInfo("IDS_CARRER_STAT_MEDKITS_USED", stats.m_medkitsUsed, false, false));
}

void CCareerWindow::OnCommand(Event& event)
{
    CBaseScreen::OnCommand(event);
}

void CCareerWindow::Update()
{
    m_elapsedMS += WindowApp::m_instance->m_updateFrameTime;
    float halfDuration = static_cast<float>(m_stats.size()) * 0.05f;
    float duration = (halfDuration + halfDuration + 0.3f) * 1000.0f;
    if (static_cast<float>(m_elapsedMS) >= duration) {
        m_careerContent->SetFlags(FLAG_EVENTS_TO_CHILDREN);
    }
    CBaseScreen::Update();
}

void CCareerWindow::OnClosed()
{
    WindowApp::HandleTunnelCommand(0x703ba476, 0, 0, 0);
}

VertScrollerSimpleRed::VertScrollerSimpleRed(ScrollingContainer* container)
{
    m_container = container;
    SetDesiredWidth(ScrollWidth());
    SetPercentHeight(100, 0, 0);
    SetOutsetSpacing(0, 0, 0, 1);
}

void VertScrollerSimpleRed::Paint(ICGraphics2d&)
{
    if (m_container->IsFullVisible()) {
        return;
    }
    float position = m_container->ScrollPos();
    float visibleFraction = static_cast<float>(m_container->m_height) / static_cast<float>(m_container->m_contentHeight);
    float fraction = 0.15f;
    if (visibleFraction > fraction) {
        fraction = visibleFraction;
    }
    float height = m_height;
    int top = MathLib::Round(height * ((1.0f - fraction) * position));
    int barHeight = MathLib::Round(height * fraction);
    int middle = m_width / 2;
    int leftWidth = ScrollWidth();
    int rightWidth = ScrollWidth();
    CDrawUtil::Fill(middle - leftWidth, 0, middle + rightWidth, m_height,
                    0xff820b0b, ICGraphics2d::Primitive_Rectangles);
    leftWidth = ScrollWidth();
    rightWidth = ScrollWidth();
    CDrawUtil::Fill(middle - leftWidth, top, middle + rightWidth, top + barHeight,
                    0xffff0000, ICGraphics2d::Primitive_Rectangles);
}

SimpleGreyProgressBar::SimpleGreyProgressBar(float percent) : Window(false), m_percent(percent)
{
    m_background.Create("SUR_PROGRESSBAR_CAREER");
    m_center.Create("SUR_PROGRESSBAR_CAREER_GREY");
    m_left.Create("SUR_PROGRESSBAR_CAREER_GREY_LEFT");
    m_right.Create("SUR_PROGRESSBAR_CAREER_GREY_RIGHT");
    int width = ImageWidth(m_background.GetSurface());
    int height = ImageHeight(m_background.GetSurface());
    SetSize(width, height);
}

void SimpleGreyProgressBar::Paint(ICGraphics2d& graphics)
{
    DrawImage(graphics, m_background.GetSurface(), 0, 0, 0, 0);
    if (m_percent > 0.0f) {
        DrawImage(graphics, m_left.GetSurface(), 0, 0, 0, 0);
    }
    int leftWidth = ImageWidth(m_left.GetSurface());
    int fillPixels = static_cast<int>(static_cast<float>(m_width - leftWidth * 2) * m_percent);
    DrawTileImageHoriz(graphics, m_center.GetSurface(), leftWidth, 0, fillPixels);
    if (m_percent > 0.0f) {
        DrawImage(graphics, m_right.GetSurface(), fillPixels + leftWidth, 0, 0, 0);
    }
    Window::Paint(graphics);
}

StatItem::StatItem(const CCareerWindow::StatItemInfo& info, int index)
    : Window(false), m_index(index)
{
    CFont* labelFont = CFontMgr::GetInstance()->GetFont(9);
    CFont* valueFont = CFontMgr::GetInstance()->GetFont(11);
    ImageWindow* frame = new ImageWindow(ImageWindow::FILL_NONE, ImageRes("SUR_FRAME_RED_CAREER"));
    int insetX = RowInsetX();
    int insetY = RowInsetY();
    frame->SetInsetSpacing(insetX, insetY);

    WindowTransparent* label = new WindowTransparent(false);
    label->SetLayoutType(LAYOUT_SIMPLE);
    label->SetAlign(ALIGN_LEFT);
    label->SetWidthByContent(0, 0);
    label->SetHeightByContent(0, 0);
    int labelColumn = 0;
    if (info.m_gameCenter) {
        ImageWindow* icon = new ImageWindow();
        icon->SetImage(ImageRes("SUR_GAMECENTER_ICON"));
        icon->SetOutsetSpacing(4 * App::Scale(), 0, 0, 0);
        label->AddToFront(icon, 0, 0);
        labelColumn = 1;
    }
    TextWindow* caption = new TextWindow(ResString(info.m_name).Upper(), labelFont);
    label->AddToFront(caption, labelColumn, 0);
    frame->AddToFront(label);

    Window* value = new Window(false);
    value->SetLayoutType(LAYOUT_SIMPLE);
    value->SetAlign(ALIGN_RIGHT | ALIGN_VCENTER);
    value->SetHeightByContent(0, 0);
    value->SetPercentWidth(42, 0, 0);
    int valueColumn = 0;
    if (info.m_percent) {
        SimpleGreyProgressBar* bar = new SimpleGreyProgressBar(static_cast<float>(info.m_value) * 0.01f);
        bar->SetLayoutType(LAYOUT_SIMPLE);
        bar->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
        value->AddToFront(createTD(bar, 3), 0, 0);
        valueColumn = 1;
    }
    {
        XString valueText(info.m_value);
        if (info.m_percent) {
            valueText.Append(XString(" %"));
        }
        TextWindow* number = new TextWindow(valueText, valueFont);
        number->SetAlign(ALIGN_RIGHT | ALIGN_VCENTER);
        number->SetPercentWidth(22, 0, 0);
        value->AddToFront(number, valueColumn, 0);
    }
    frame->AddToFront(value);
    AddToFront(frame);
    SetHeightByContent(0, 0);
    SetWidthByContent(0, 0);
    m_started = false;
}

void StatItem::OnStartupEasy()
{
    if (!m_started) {
        EasyParams params(this, static_cast<EasyLib::EMethod>(6));
        params.SetDelayTime(m_index * 0.05f);
        params.SetFullTime(m_index * 0.05f + 0.3f);
        params.SetMoving(m_x, WindowApp::m_instance->m_screenHeight, m_x, m_y);
        StartEasy(params, FLAG_SHOW_AFTER_EASY);
        m_started = true;
    }
}

typedef char CKCareerWindowSize[sizeof(CCareerWindow) == 0xf0 ? 1 : -1];
typedef char CKCareerStatItemSize[sizeof(StatItem) == 0xac ? 1 : -1];
