#include "CBaseScreen.h"

#include "App.h"
#include "CBH_MenuButton.h"
#include "CBH_Player.h"
#include "CBH_ProgressBar.h"
#include "CDH_SoundManager.h"
#include "CFontMgr.h"
#include "SScreenAdaptingConstant.h"

CBaseScreen::CBaseScreen(bool redFrame, bool drawBody, bool showBack, bool showOptions, bool drawTitle, bool gpsFrame)
    : Window(false), m_drawBody(drawBody), m_drawTitle(drawTitle), m_back(NULL)
{
    m_titleImage.Create("SUR_TITLE_BACKGROUND");
    if (m_drawBody) {
        m_bodyImage.Create("SUR_MENU_HONEYCOMD_BOTTOM");
    }
    m_backImage.Create("SUR_BUTTON_BACK");
    m_titleHeight = ImageHeight(m_titleImage.GetSurface());
    m_content = new Window(false);
    m_content->SetLayoutType(LAYOUT_NONE);
    m_content->SetRect(0, m_titleHeight, m_width - m_x, m_height - m_titleHeight, false);
    AddToFront(m_content);
    m_cash = NULL;
    m_credits = NULL;

    if (redFrame) {
        FrameRed* frame = new FrameRed();
        const int frameX = -App::Scale();
        const int panelY = SScreenAdaptingConstant(40, 40, 40, 60)();
        const int scaledPanelY = App::Scale() * panelY;
        const int panelWidth = SScreenAdaptingConstant(100, 100, 100, 170)();
        const int panelBadgeSize = SScreenAdaptingConstant(50, 50, 50, 80)();
        const int frameWidth = (panelWidth + panelBadgeSize) * App::Scale();
        const int panelHeight = SScreenAdaptingConstant(52, 52, 52, 80)();
        const int frameHeight = panelHeight * App::Scale();
        frame->SetRect(frameX, scaledPanelY, frameWidth, frameHeight, false);
        frame->DisableTop();
        AddToFront(frame);

        Window* badge = new Window(false);
        badge->SetLayoutType(LAYOUT_NONE);
        ImageWindow* badgeImage = new ImageWindow();
        badgeImage->SetFillType(ImageWindow::FILL_NONE);
        badgeImage->SetImage(ImageRes("SUR_SCOPE_LEVEL"));
        badge->AddToFront(badgeImage);
        XString levelText(CBH_Player::GetInstance()->GetLevel());
        TextWindow* level = new TextWindow(levelText, CFontMgr::GetInstance()->GetFont(9));
        level->SetLayoutType(LAYOUT_SIMPLE);
        level->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
        badge->AddToFront(level);
        const int badgeX = App::Scale();
        const int badgeY = SScreenAdaptingConstant(37, 37, 37, 52)();
        const int scaledBadgeY = App::Scale() * badgeY;
        const int badgeWidth = SScreenAdaptingConstant(50, 50, 50, 80)();
        const int scaledBadgeWidth = App::Scale() * badgeWidth;
        const int badgeHeight = SScreenAdaptingConstant(50, 50, 50, 80)();
        const int scaledBadgeHeight = App::Scale() * badgeHeight;
        badge->SetRect(badgeX, scaledBadgeY, scaledBadgeWidth, scaledBadgeHeight, false);
        AddToFront(badge);

        CBH_ProgressBar* cash = new CBH_ProgressBar(
            3, SScreenAdaptingConstant(100, 100, 100, 160)() * App::Scale(), -1);
        m_cash = cash;
        const int cashLeft = SScreenAdaptingConstant(50, 50, 50, 80)();
        const int cashX = cashLeft * App::Scale();
        const int cashTop = SScreenAdaptingConstant(42, 42, 42, 65)();
        const int cashY = cashTop * App::Scale();
        cash->SetCorner(cashX, cashY);
        m_cash->SetDescImage("IDB_HUD_ICON_MONEY");
        m_cash->m_displayFlags = CBH_ProgressBar::DISPLAY_CURRENT | CBH_ProgressBar::HIDE_FILL;
        m_cash->SetTextAlign(TEXT_ALIGN_RIGHT);
        m_cash->SetCurrValue(CBH_Player::GetInstance()->m_cash);
        AddToFront(m_cash);

        CBH_ProgressBar* credits = new CBH_ProgressBar(
            3, SScreenAdaptingConstant(100, 100, 100, 160)() * App::Scale(), -1);
        m_credits = credits;
        const int creditsLeft = SScreenAdaptingConstant(50, 50, 50, 80)();
        const int creditsX = creditsLeft * App::Scale();
        const int creditsTop = SScreenAdaptingConstant(62, 62, 62, 97)();
        const int creditsY = creditsTop * App::Scale();
        credits->SetCorner(creditsX, creditsY);
        m_credits->SetDescImage("IDB_HUD_ICON_CREDITS");
        m_credits->m_displayFlags = CBH_ProgressBar::DISPLAY_CURRENT | CBH_ProgressBar::HIDE_FILL;
        m_credits->SetTextAlign(TEXT_ALIGN_RIGHT);
        m_credits->SetCurrValue(CBH_Player::GetInstance()->m_credits);
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
        const int screenWidth = static_cast<App*>(WindowApp::m_instance)->m_screenWidth;
        options->SetCorner(screenWidth - App::Scale() * 10 - options->m_width + options->m_padding * 2,
                           m_titleHeight / 2 - (options->m_height - options->m_padding * 2) / 2);
        options->SetCommandParameters(0x121683a7, 0, 0, 0);
        AddToFront(options);
    }
    m_title = new TextWindow(CFontMgr::GetInstance()->GetFont(7));
    m_title->SetLayoutType(LAYOUT_NONE);
    TextWindow* title = m_title;
    int titleX;
    if (showBack) {
        const int backLeft = m_back->Left();
        titleX = backLeft + m_back->m_width + 10 - m_back->m_padding * 2;
    } else {
        titleX = 10;
    }
    title->SetCorner(titleX, 0);
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
        const int backLeft = m_back->Left();
        x = backLeft + m_back->m_width + 10 - m_back->m_padding * 2;
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
    ImageRes left;
    left.Create("SUR_DLG_ITEM_SPLITTER_LEFT");
    ImageRes middle;
    middle.Create("SUR_DLG_ITEM_SPLITTER_MIDDLE");
    ImageRes right;
    right.Create("SUR_DLG_ITEM_SPLITTER_RIGHT");
    HorizImageWindow* line = new HorizImageWindow(left, middle, right, false);
    if ((column | row) >= 0) {
        line->SetCellPos(column, row, 1, 1);
    }
    line->SetDesiredHeight(20);
    return line;
}

CBaseScreen::FrameRed::FrameRed()
{
    m_topLeftFrame.Create("SUR_TITLE_CORNER02_LEFT_TOP");
    m_bottomLeftFrame.Create("SUR_TITLE_CORNER02_LEFT_DOWN");
    m_topRightFrame.Create("SUR_TITLE_CORNER02_RIGHT_TOP");
    m_bottomRightFrame.Create("SUR_TITLE_CORNER02_RIGHT_DOWN");
    m_leftFrame.Create("SUR_TITLE_LINE02_LEFT");
    m_rightFrame.Create("SUR_TITLE_LINE02_RIGHT");
    m_topFrame.Create("SUR_TITLE_LINE01_TOP");
    m_bottomFrame.Create("SUR_TITLE_LINE01_DOWN");
    m_centerFrame.Create("SUR_TAB_BACKGROUND");
    SetLayoutType(LAYOUT_NONE);
    m_inset = 2 * App::Scale();
}

void CBaseScreen::FrameRed::Paint(ICGraphics2d& graphics)
{
    DrawTileImageRect(graphics, m_centerFrame.GetSurface(), m_inset, 2,
                      m_width - m_inset * 2, m_height - m_inset * 2);
    DrawBorderFillRect(graphics, m_leftFrame.GetSurface(), m_topLeftFrame.GetSurface(),
                       m_topFrame.GetSurface(), m_topRightFrame.GetSurface(), m_rightFrame.GetSurface(),
                       m_bottomRightFrame.GetSurface(), m_bottomFrame.GetSurface(), m_bottomLeftFrame.GetSurface(),
                       NULL, 0, 0, m_width, m_height);
}

CBaseScreen::GPSFrame::GPSFrame()
{
    m_topLeftFrame.Create("SUR_FRAME_MAP_LEFT_TOP");
    m_bottomLeftFrame.Create("SUR_FRAME_MAP_LEFT_BOTTOM");
    m_topRightFrame.Create("SUR_FRAME_MAP_RIGHT_TOP");
    m_bottomRightFrame.Create("SUR_FRAME_MAP_RIGHT_BOTTOM");
    m_leftFrame.Create("SUR_FRAME_MAP_LEFT");
    m_rightFrame.Create("SUR_FRAME_MAP_RIGHT");
    m_topFrame.Create("SUR_FRAME_MAP_TOP");
    m_bottomFrame.Create("SUR_FRAME_MAP_BOTTOM");
    SetLayoutType(LAYOUT_NONE);
}

void CBaseScreen::GPSFrame::Paint(ICGraphics2d& graphics)
{
    // Like Filler::Paint, but without the center tile.
    DrawBorderFillRect(graphics, m_leftFrame.GetSurface(), m_topLeftFrame.GetSurface(),
                       m_topFrame.GetSurface(), m_topRightFrame.GetSurface(), m_rightFrame.GetSurface(),
                       m_bottomRightFrame.GetSurface(), m_bottomFrame.GetSurface(), m_bottomLeftFrame.GetSurface(),
                       NULL, 0, 0, m_width, m_height);
}

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AssertBaseScreenSize[sizeof(CBaseScreen) == 0xd8 ? 1 : -1];
#endif
