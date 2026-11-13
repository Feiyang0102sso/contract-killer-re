#include "CBaseScreen.h"

#include "App.h"
#include "CBH_MenuButton.h"
#include "CBH_Player.h"
#include "CBH_ProgressBar.h"
#include "CDH_SoundManager.h"
#include "CFontMgr.h"
#include "SScreenAdaptingConstant.h"

namespace {
const char* const RedFrameResources[] = {
    "SUR_TITLE_LINE02_LEFT", "SUR_TITLE_LINE02_RIGHT", "SUR_TITLE_LINE01_TOP", "SUR_TITLE_LINE01_DOWN",
    "SUR_TITLE_CORNER02_LEFT_TOP", "SUR_TITLE_CORNER02_RIGHT_TOP", "SUR_TITLE_CORNER02_LEFT_DOWN",
    "SUR_TITLE_CORNER02_RIGHT_DOWN", "SUR_TAB_BACKGROUND"
};
const char* const GPSFrameResources[] = {
    "SUR_FRAME_MAP_LEFT", "SUR_FRAME_MAP_RIGHT", "SUR_FRAME_MAP_TOP", "SUR_FRAME_MAP_BOTTOM",
    "SUR_FRAME_MAP_LEFT_TOP", "SUR_FRAME_MAP_RIGHT_TOP", "SUR_FRAME_MAP_LEFT_BOTTOM",
    "SUR_FRAME_MAP_RIGHT_BOTTOM", NULL
};

CBH_ProgressBar* CreateBalance(const char* icon, int width, int x, int y)
{
    CBH_ProgressBar* bar = new CBH_ProgressBar(3, width, -1);
    bar->SetDescImage(icon);
    bar->m_displayFlags = CBH_ProgressBar::DISPLAY_CURRENT | CBH_ProgressBar::HIDE_FILL;
    bar->SetTextAlign(TEXT_ALIGN_RIGHT);
    bar->SetCorner(x, y);
    return bar;
}
}

CBaseScreen::CBaseScreen(bool redFrame, bool drawBody, bool showBack, bool showOptions, bool drawTitle, bool gpsFrame)
    : Window(false), m_titleImage("SUR_TITLE_BACKGROUND"), m_backImage("SUR_BUTTON_BACK"),
      m_drawBody(drawBody), m_drawTitle(drawTitle), m_cash(NULL), m_credits(NULL), m_back(NULL)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    int scale = App::Scale();
    SetSize(app->m_screenWidth, app->m_screenHeight);
    if (m_drawBody) {
        m_bodyImage.Create("SUR_MENU_HONEYCOMD_BOTTOM");
    }
    m_titleHeight = ImageHeight(m_titleImage.GetSurface());
    m_content = new Window(false);
    m_content->SetLayoutType(LAYOUT_NONE);
    m_content->SetRect(0, m_titleHeight, m_width, m_height - m_titleHeight, false);
    AddToFront(m_content);

    if (redFrame) {
        const SScreenAdaptingConstant panelY(40, 40, 40, 60);
        const SScreenAdaptingConstant panelWidth(100, 100, 100, 170);
        const SScreenAdaptingConstant panelHeight(52, 52, 52, 80);
        const SScreenAdaptingConstant badgeY(37, 37, 37, 52);
        const SScreenAdaptingConstant badgeSize(50, 50, 50, 80);
        const SScreenAdaptingConstant balanceWidth(100, 100, 100, 160);
        const SScreenAdaptingConstant cashY(42, 42, 42, 65);
        const SScreenAdaptingConstant creditsY(62, 62, 62, 97);
        const int scaledBadgeSize = scale * badgeSize();
        FrameRed* frame = new FrameRed();
        frame->SetRect(-scale, scale * panelY(), scale * panelWidth() + scaledBadgeSize,
                       scale * panelHeight(), false);
        frame->DisableTop();
        AddToFront(frame);

        CBH_Player* player = CBH_Player::GetInstance();
        Window* badge = new Window(false);
        badge->SetLayoutType(LAYOUT_NONE);
        badge->AddToFront(new ImageWindow(ImageWindow::FILL_NONE, ImageRes("SUR_SCOPE_LEVEL")));
        TextWindow* level = new TextWindow(XString(player->GetLevel()), CFontMgr::GetInstance()->GetFont(9));
        level->SetLayoutType(LAYOUT_SIMPLE);
        level->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
        badge->AddToFront(level);
        badge->SetRect(scale, scale * badgeY(), scaledBadgeSize, scaledBadgeSize, false);
        AddToFront(badge);

        const int width = scale * balanceWidth();
        m_cash = CreateBalance("IDB_HUD_ICON_MONEY", width, scaledBadgeSize, scale * cashY());
        m_credits = CreateBalance("IDB_HUD_ICON_CREDITS", width, scaledBadgeSize, scale * creditsY());
        m_cash->SetCurrValue(player->m_cash);
        m_credits->SetCurrValue(player->m_credits);
        AddToFront(m_cash);
        AddToFront(m_credits);
    }
    if (showBack) {
        m_back = new CBH_MenuButton();
        m_back->m_padding = App::Scale() * 25;
        AddToFront(m_back);
        m_back->setImage("IDB_BUTTON_BACK");
        // Centered on the title bar; the button height includes the padding on both sides.
        m_back->SetCorner(10, ImageHeight(m_titleImage.GetSurface()) / 2 - (m_back->m_height - m_back->m_padding * 2) / 2);
        m_back->SetCommandParameters(0x5f523cd9, 0, 0, 0);
        m_back->SetBlinkingImage("IDB_BUTTON_BACK_ACTIVE_TUTORIAL");
    }
    if (gpsFrame) {
        AddToFront(new GPSFrame());
    }
    if (showOptions) {
        CBH_MenuButton* options = new CBH_MenuButton();
        options->m_padding = App::Scale() * 15;
        options->setImage("IDB_BUTTON_OPTIONS");
        options->SetCorner(app->m_screenWidth - App::Scale() * 10 - options->m_width + options->m_padding * 2,
                           m_titleHeight / 2 - (options->m_height - options->m_padding * 2) / 2);
        options->SetCommandParameters(0x121683a7, 0, 0, 0);
        AddToFront(options);
    }
    m_title = new TextWindow(CFontMgr::GetInstance()->GetFont(7));
    m_title->SetLayoutType(LAYOUT_NONE);
    int titleX;
    if (showBack) {
        titleX = m_back->Left() + m_back->m_width + 10 - m_back->m_padding * 2;
    } else {
        titleX = 10;
    }
    m_title->SetCorner(titleX, 0);
    AddToFront(m_title);
}

CBaseScreen::~CBaseScreen()
{
}

CBH_MenuButton* CBaseScreen::CreateMenuButton(const char* image, int command)
{
    CBH_MenuButton* button = new CBH_MenuButton();
    button->setImage(image);
    button->SetLayoutType(LAYOUT_NONE);
    button->m_command = command;
    button->m_commandParameter1 = 0;
    button->m_commandParameter2 = 0;
    button->m_commandParameter3 = 0;
    return button;
}

int CBaseScreen::GetTitleBarHeight()
{
    return m_titleHeight;
}

void CBaseScreen::SetTitleText(XString title)
{
    // Original 0xf6be4: only moves the title (it keeps its own size).
    m_title->SetText(title);
    TextWindow* titleWindow = m_title;
    int x;
    int y;
    if (m_back == NULL) {
        x = App::Scale() * 20;
        y = App::Scale();
    } else {
        x = m_back->Left() + m_back->m_width + 10 - m_back->m_padding * 2;
        y = App::Scale();
    }
    titleWindow->SetCorner(x, y * 5);
}

void CBaseScreen::Init()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_soundManager->PlaySound("IDM_MENU_WINDOW_OPEN_CLOSE", false, false);
}

void CBaseScreen::Shutdown()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_soundManager->PlaySound("IDM_MENU_WINDOW_OPEN_CLOSE", false, false);
}

void CBaseScreen::OnStartupEasy()
{
    EasyParams params(this, (EasyLib::EMethod)6);
    params.SetFullTime(0.5f);
    params.SetOpacity(0.0f, 1.0f);
    StartEasy(params, FLAG_SHOW_AFTER_EASY);
}

void CBaseScreen::OnCommand(Event& event)
{
    if (event.m_id == -0x68dfb87c || event.m_id == 0x5f523cd9) {
        ReverseEasy(FLAG_CLOSE_AFTER_EASY);
        event.Clear();
    }
}

void CBaseScreen::Update()
{
    if (m_credits != NULL && m_cash != NULL) {
        m_cash->SetCurrValue(CBH_Player::GetInstance()->m_cash);
        m_credits->SetCurrValue(CBH_Player::GetInstance()->m_credits);
    }
}

void CBaseScreen::Paint(ICGraphics2d& graphics)
{
    if (m_drawTitle) {
        DrawTileImageHoriz(graphics, m_titleImage.GetSurface(), 0, 0, m_width);
    }
    if (m_drawBody) {
        DrawTileImageRect(graphics, m_bodyImage.GetSurface(), 0, m_titleHeight, m_width, m_height - m_titleHeight);
    }
}

HorizImageWindow* CBaseScreen::createLineSplitter(int column, int row)
{
    HorizImageWindow* line = new HorizImageWindow(ImageRes("SUR_DLG_ITEM_SPLITTER_LEFT"),
                                                 ImageRes("SUR_DLG_ITEM_SPLITTER_MIDDLE"),
                                                 ImageRes("SUR_DLG_ITEM_SPLITTER_RIGHT"), false);
    if ((column | row) >= 0) {
        line->SetCellPos(column, row, 1, 1);
    }
    line->SetDesiredHeight(20);
    return line;
}

CBaseScreen::FrameRed::FrameRed() : m_inset(2 * App::Scale())
{
    SetFrame(RedFrameResources);
    SetLayoutType(LAYOUT_NONE);
}

void CBaseScreen::FrameRed::Paint(ICGraphics2d& graphics)
{
    DrawTileImageRect(graphics, m_frame[Center].GetSurface(), m_inset, 2,
                      m_width - m_inset * 2, m_height - m_inset * 2);
    DrawBorderFillRect(graphics, m_frame[Left].GetSurface(), m_frame[TopLeft].GetSurface(),
                       m_frame[Top].GetSurface(), m_frame[TopRight].GetSurface(), m_frame[Right].GetSurface(),
                       m_frame[BottomRight].GetSurface(), m_frame[Bottom].GetSurface(), m_frame[BottomLeft].GetSurface(),
                       NULL, 0, 0, m_width, m_height);
}

CBaseScreen::GPSFrame::GPSFrame()
{
    SetFrame(GPSFrameResources);
    SetLayoutType(LAYOUT_NONE);
}

void CBaseScreen::GPSFrame::Paint(ICGraphics2d& graphics)
{
    // Like Filler::Paint, but without the center tile.
    DrawBorderFillRect(graphics, m_frame[Left].GetSurface(), m_frame[TopLeft].GetSurface(),
                       m_frame[Top].GetSurface(), m_frame[TopRight].GetSurface(), m_frame[Right].GetSurface(),
                       m_frame[BottomRight].GetSurface(), m_frame[Bottom].GetSurface(), m_frame[BottomLeft].GetSurface(),
                       NULL, 0, 0, m_width, m_height);
}

typedef char AssertBaseScreenSize[sizeof(CBaseScreen) == 0xd8 ? 1 : -1];
