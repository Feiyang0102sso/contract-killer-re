// CTabWindow.cpp - tab window of the CK menus (ported from the CKZ draft, rewritten after the CK original).
#include "CTabWindow.h"

#include "App.h"
#include "BaseDialog.h"
#include "CBH_Player.h"
#include "CBH_ProgressBar.h"
#include "CCountNotificationWindow.h"
#include "CDH_SoundManager.h"
#include "CFont.h"
#include "CFontMgr.h"
#include "SScreenAdaptingConstant.h"
#include "WindowApp.h"

CTabWindow::TTab::~TTab()
{
    if (m_notification != NULL) {
        delete m_notification;
        m_notification = NULL;
    }
}

CTabWindow::CTabWindow(bool showInfo)
    : Window(false), m_infoTab(NULL), m_showInfo(false), m_currentTab(-1)
{
    SetLayoutType(LAYOUT_NONE);
    m_background.Create("SUR_MENU_HONEYCOMD_BOTTOM");
    m_leftOff.Create("SUR_TABCONTROL_LEFT_OFF");
    m_middleOff.Create("SUR_TABCONTROL_OFF");
    m_rightOff.Create("SUR_TABCONTROL_RIGHT_OFF");
    m_leftOn.Create("SUR_TABCONTROL_LEFT_ON");
    m_middleOn.Create("SUR_TABCONTROL_ON");
    m_rightOn.Create("SUR_TABCONTROL_RIGHT_ON");
    m_border.Create("SUR_BG_BORDER");
    m_middleOnHigh.Create("SUR_TABCONTROL_ON_HIGH");
    m_rightOnHigh.Create("SUR_TABCONTROL_RIGHT_ON_HIGH");
    m_leftOnHigh.Create("SUR_TABCONTROL_LEFT_ON_HIGH");
    m_font = CFontMgr::GetInstance()->GetFont(15);
    m_showInfo = showInfo;
    if (showInfo == true) {
        CreateInfoTab();
    }
}

void CTabWindow::CreateInfoTab()
{
    m_infoTab = new TTab();
    m_infoTab->m_bounds.m_x = SScreenAdaptingConstant(10, 20)();
    m_infoTab->m_bounds.m_width = SScreenAdaptingConstant(150, 300)();
    m_infoTab->m_bounds.m_y = m_y;
    m_infoTab->m_bounds.m_height = ImageHeight(m_middleOnHigh.GetSurface());
    m_infoWindow = new Window(false);
    m_infoWindow->SetLayoutType(LAYOUT_NONE);
    m_infoWindow->SetRect(m_infoTab->m_bounds.m_x, m_infoTab->m_bounds.m_y, m_infoTab->m_bounds.m_width,
                          m_infoTab->m_bounds.m_height, false);

    // Player level in the scope icon.
    Window* levelGroup = new Window(false);
    levelGroup->SetPercentWidth(27, 0, 0);
    ImageWindow* scope = new ImageWindow();
    scope->SetFillType(ImageWindow::FILL_NONE);
    scope->SetImage(ImageRes("SUR_SCOPE_LEVEL"));
    scope->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    levelGroup->AddToFront(scope);
    XString level(CBH_Player::GetInstance()->m_level);
    TextWindow* levelText = new TextWindow(level, CFontMgr::GetInstance()->GetFont(9));
    levelText->SetLayoutType(LAYOUT_SIMPLE);
    levelText->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    levelText->SetOutsetSpacing(App::Scale() * -3, 0, 0, 0);
    levelGroup->AddToFront(levelText);
    levelGroup->SetLayoutType(LAYOUT_TABLE);
    levelGroup->SetCellPos(0, 0, 1, 1);
    int top = App::Scale() * 3;
    int bottom = App::Scale() * 3;
    int left = App::Scale() * 3;
    levelGroup->SetOutsetSpacing(top, 0, bottom, left);
    m_infoWindow->AddToFront(levelGroup);

    // Money and credits.
    Window* currencies = new Window(false);
    currencies->SetLayoutType(LAYOUT_TABLE);
    currencies->SetCellPos(1, 0, 1, 1);
    currencies->SetPercentWidth(75, 0, 0);
    CBH_ProgressBar* moneyBar = new CBH_ProgressBar(3, SScreenAdaptingConstant(100, 200)(), -1);
    m_moneyBar = moneyBar;
    moneyBar->SetCorner(SScreenAdaptingConstant(8, 8, 8, 6)() * App::Scale(), App::Scale());
    m_moneyBar->SetDescImage("IDB_HUD_ICON_MONEY");
    m_moneyBar->m_displayFlags = 5;
    m_moneyBar->SetTextAlign(TEXT_ALIGN_RIGHT);
    m_moneyBar->SetCurrValue(CBH_Player::GetInstance()->m_cash);
    currencies->AddToFront(m_moneyBar);
    CBH_ProgressBar* creditsBar = new CBH_ProgressBar(3, SScreenAdaptingConstant(100, 200)(), -1);
    m_creditsBar = creditsBar;
    creditsBar->SetCorner(SScreenAdaptingConstant(8, 8, 8, 6)() * App::Scale(),
                          SScreenAdaptingConstant(21, 21, 21, 32)() * App::Scale());
    m_creditsBar->SetDescImage("IDB_HUD_ICON_CREDITS");
    m_creditsBar->m_displayFlags = 5;
    m_creditsBar->SetTextAlign(TEXT_ALIGN_RIGHT);
    m_creditsBar->SetCurrValue(CBH_Player::GetInstance()->m_credits);
    currencies->AddToFront(m_creditsBar);
    m_infoWindow->AddToFront(currencies);
    m_infoWindow->SetInsetSpacing(0, SScreenAdaptingConstant(5, 10, 10, -1)(), SScreenAdaptingConstant(5, 10, 10, -1)(),
                                  SScreenAdaptingConstant(5, 10, 10, -1)());
    AddToFront(m_infoWindow);
}

void CTabWindow::AddTab(XString name, Window* window, int padding)
{
    AddTab(name, window, NULL, false, padding);
}

void CTabWindow::AddTab(XString name, Window* window, const char* image, bool locked, int padding)
{
    TTab* tab = new TTab();
    tab->m_name = name;
    tab->m_window = window;
    tab->m_locked = locked;
    if (image != NULL) {
        tab->m_image.Create(image);
    }
    int leftWidth = ImageWidth(m_leftOn.GetSurface());
    int middleWidth = ImageWidth(m_middleOn.GetSurface());
    int rightWidth = ImageWidth(m_rightOn.GetSurface());
    int textWidth = m_font->MeasureTextWidth(name.c_str(), -1, -1, 0);
    int width = leftWidth + middleWidth + rightWidth;
    int imageWidth = ImageWidth(tab->m_image.GetSurface());
    if (imageWidth > width) {
        width = imageWidth;
    }
    if (width < textWidth) {
        width = SScreenAdaptingConstant(3, 5)() + textWidth;
    }
    if (padding > 0) {
        width += padding;
    }
    int extra = SScreenAdaptingConstant(7, 20)();
    int height = GetCaptionHeight();
    short y = m_y;
    int infoWidth = 0;
    if (m_showInfo) {
        int margin = SScreenAdaptingConstant(10, 20)();
        infoWidth = SScreenAdaptingConstant(150, 300)() + margin;
    }
    short tabWidth = width + extra;
    if (m_tabs.size() == 0) {
        tab->m_bounds.m_x = infoWidth + (m_width - infoWidth) / 2 - tabWidth / 2;
    } else {
        TTab* first = m_tabs[0];
        first->m_bounds.m_x = infoWidth + (m_width - infoWidth) / 2 - (GetTabWidth() + tabWidth - 1) / 2;
        tab->m_bounds.m_x = 0;
    }
    tab->m_bounds.m_y = y;
    tab->m_bounds.m_width = tabWidth;
    tab->m_bounds.m_height = height;
    AddToFront(window);
    m_tabs.addElement(tab);
    for (int index = 1; index < m_tabs.size(); index++) {
        m_tabs[index]->m_bounds.m_x = m_tabs[index - 1]->m_bounds.m_x + m_tabs[index - 1]->m_bounds.m_width - 1;
    }
    window->ClearFlags(FLAG_VISIBLE | FLAG_ENABLED);
    window->SetLayoutType(LAYOUT_NONE);
    int windowWidth = m_width;
    int windowHeight = m_height;
    int windowY = m_y;
    int captionHeight = GetCaptionHeight();
    window->SetSize(windowWidth, windowHeight - captionHeight - windowY);
    int windowX = m_x;
    windowY = m_y;
    captionHeight = GetCaptionHeight();
    window->SetCorner(windowX, windowY + captionHeight);
    if (m_showInfo) {
        m_infoWindow->SetCorner(m_infoWindow->m_x, m_y);
    }
}

int CTabWindow::GetTabCount() const
{
    return m_tabs.size();
}

int CTabWindow::GetCaptionHeight()
{
    return ImageHeight(m_middleOff.GetSurface());
}

int CTabWindow::GetTabWidth()
{
    int result = 0;
    for (int index = 0; index < m_tabs.size(); index++) {
        result += m_tabs[index]->m_bounds.m_width - 1;
    }
    return result;
}

void CTabWindow::OnPointerEvent(Event& event)
{
    if (event.m_type != Event::POINTER_PRESSED) {
        return;
    }
    int x = event.m_param1;
    int y = event.m_param2;
    for (int index = 0; index < m_tabs.size(); index++) {
        TTab* tab = m_tabs[index];
        if (x > tab->m_bounds.m_x && x < tab->m_bounds.m_x + tab->m_bounds.m_width && y > tab->m_bounds.m_y &&
            y < tab->m_bounds.m_y + tab->m_bounds.m_height) {
            SetCurrTab(index, true);
        }
    }
}

void CTabWindow::SetCurrTab(int index, bool fromPointer)
{
    if (m_currentTab == index) {
        return;
    }
    TTab* tab = m_tabs[index];
    if (tab->m_locked && fromPointer == true) {
        WindowApp::HandleTunnelCommand((int)0xde1f7407, 0, 0, 0);
        return;
    }
    if (m_currentTab >= 0) {
        m_tabs[m_currentTab]->m_window->ClearFlags(FLAG_VISIBLE | FLAG_ENABLED);
        tab = m_tabs[index];
    }
    Window* window = tab->m_window;
    window->SetFlags(FLAG_VISIBLE | FLAG_ENABLED);
    m_currentTab = index;
    static_cast<App*>(WindowApp::m_instance)->m_soundManager->PlaySound("IDM_MENU_WINDOW_BUTTON", false, false);
    EasyParams easy(window, EasyLib::OUT_CUBIC);
    easy.SetFullTime(0.5f);
    easy.SetOpacity(0.0f, 1.0f);
    window->StartEasy(easy, 0x100);
    WindowApp::HandleTunnelCommand(0x5ce88d33, index, 0, 0);
}

void CTabWindow::Paint(ICGraphics2d& graphics)
{
    if (m_currentTab == -1) {
        return;
    }
    DrawTileImageRect(graphics, m_background.GetSurface(), m_x, m_y + GetCaptionHeight(), m_width, m_height);
    for (int index = 0; index < m_tabs.size(); index++) {
        if (index != m_currentTab) {
            PaintInactiveTab(graphics, m_tabs[index]);
        }
    }
    DrawTileImageRect(graphics, m_border.GetSurface(), m_x, m_y + GetCaptionHeight() - ImageHeight(m_border.GetSurface()),
                      m_width, ImageHeight(m_border.GetSurface()));
    PaintActiveTab(graphics, m_tabs[m_currentTab]);
    if (m_showInfo) {
        DrawInfoTab(graphics);
    }
}

void CTabWindow::PaintInactiveTab(ICGraphics2d& graphics, TTab* tab)
{
    if (tab == NULL) {
        return;
    }
    DrawLineImageHoriz(graphics, m_leftOff.GetSurface(), m_middleOff.GetSurface(), m_rightOff.GetSurface(),
                       tab->m_bounds.m_x, tab->m_bounds.m_y, tab->m_bounds.m_width);
    if (tab->m_name.ByteSize() == 0 || tab->m_image.m_resource == NULL) {
        DrawFontText(graphics, m_font, tab->m_name.c_str(), tab->m_bounds.m_x + tab->m_bounds.m_width / 2,
                     tab->m_bounds.m_y + tab->m_bounds.m_height / 2, ALIGN_HCENTER | ALIGN_VCENTER);
        if (tab->m_image.m_resource == NULL) {
            goto notification;
        }
    }
    DrawImage(graphics, tab->m_image.GetSurface(), tab->m_bounds.m_x + tab->m_bounds.m_width / 2,
              tab->m_bounds.m_y + tab->m_bounds.m_height, ALIGN_HCENTER | ALIGN_BOTTOM, 0);
notification:
    if (tab->m_notification != NULL) {
        tab->m_notification->SetPos(tab->m_bounds.m_x, tab->m_bounds.m_y);
        tab->m_notification->Paint(graphics);
    }
}

void CTabWindow::PaintActiveTab(ICGraphics2d& graphics, TTab* tab)
{
    if (tab == NULL) {
        return;
    }
    if (tab->m_name.ByteSize() == 0 || tab->m_image.m_resource == NULL) {
        DrawLineImageHoriz(graphics, m_leftOn.GetSurface(), m_middleOn.GetSurface(), m_rightOn.GetSurface(),
                           tab->m_bounds.m_x, tab->m_bounds.m_y, tab->m_bounds.m_width);
        DrawFontText(graphics, m_font, tab->m_name.c_str(), tab->m_bounds.m_x + tab->m_bounds.m_width / 2,
                     tab->m_bounds.m_y + tab->m_bounds.m_height / 2, ALIGN_HCENTER | ALIGN_VCENTER);
    } else {
        int tabY = tab->m_bounds.m_y;
        int highHeight = ImageHeight(m_middleOnHigh.GetSurface());
        int normalHeight = ImageHeight(m_middleOff.GetSurface());
        DrawLineImageHoriz(graphics, m_leftOnHigh.GetSurface(), m_middleOnHigh.GetSurface(), m_rightOnHigh.GetSurface(),
                           tab->m_bounds.m_x, tabY - highHeight + normalHeight, tab->m_bounds.m_width);
        DrawFontText(graphics, m_font, tab->m_name.c_str(), tab->m_bounds.m_x + tab->m_bounds.m_width / 2,
                     tab->m_bounds.m_y + SScreenAdaptingConstant(5, 10)(), ALIGN_HCENTER | ALIGN_VCENTER);
    }
    if (tab->m_image.m_resource != NULL) {
        DrawImage(graphics, tab->m_image.GetSurface(), tab->m_bounds.m_x + tab->m_bounds.m_width / 2,
                  tab->m_bounds.m_y + tab->m_bounds.m_height, ALIGN_HCENTER | ALIGN_BOTTOM, 0);
    }
    if (tab->m_notification != NULL) {
        tab->m_notification->SetPos(tab->m_bounds.m_x, tab->m_bounds.m_y);
        tab->m_notification->Paint(graphics);
    }
}

void CTabWindow::DrawInfoTab(ICGraphics2d& graphics)
{
    ICRenderSurface* left = m_leftOnHigh.GetSurface();
    ICRenderSurface* middle = m_middleOnHigh.GetSurface();
    ICRenderSurface* right = m_rightOnHigh.GetSurface();
    int y = m_y;
    int x = m_infoTab->m_bounds.m_x;
    int highHeight = ImageHeight(m_middleOnHigh.GetSurface());
    int normalHeight = ImageHeight(m_middleOff.GetSurface());
    DrawLineImageHoriz(graphics, left, middle, right, x, y - highHeight + normalHeight, m_infoTab->m_bounds.m_width);
}

CTabWindow::~CTabWindow()
{
    SAFE_DELETE_VECTOR(m_tabs);
    m_tabs.removeAllElements();
    if (m_infoTab != NULL) {
        delete m_infoTab;
        m_infoTab = NULL;
    }
}

void CTabWindow::SetTabLock(int index, bool locked)
{
    TTab* tab = m_tabs[index];
    if (tab != NULL) {
        tab->m_locked = locked;
    }
}

void CTabWindow::OnGetFocus()
{
    UpdateInfoTab();
}

void CTabWindow::UpdateInfoTab()
{
    if (!m_showInfo) {
        return;
    }
    m_moneyBar->SetCurrValue(CBH_Player::GetInstance()->m_cash);
    m_creditsBar->SetCurrValue(CBH_Player::GetInstance()->m_credits);
}

void CTabWindow::SetTabsWidthByWindowWidth()
{
    HandleUpdate(false);
    int count = m_tabs.size();
    int width = m_width;
    int available = width - SScreenAdaptingConstant(7, 20)() * count;
    int maxCaptionWidth = GetMaxTabCaptionWidth();
    int tabWidth = available / count;
    if (maxCaptionWidth > tabWidth) {
        tabWidth = maxCaptionWidth;
    }
    TTab* first = m_tabs[0];
    first->m_bounds.m_x = m_width / 2 - (tabWidth * count) / 2;
    first->m_bounds.m_width = tabWidth;
    for (int index = 1; index < m_tabs.size(); index++) {
        TTab* tab = m_tabs[index];
        tab->m_bounds.m_x = m_tabs[index - 1]->m_bounds.m_x + tabWidth - 1;
        tab->m_bounds.m_width = tabWidth;
    }
}

int CTabWindow::GetMaxTabCaptionWidth()
{
    int result = 0;
    for (int index = 0; index < m_tabs.size(); index++) {
        if (m_tabs[index]->m_bounds.m_width > result) {
            result = m_tabs[index]->m_bounds.m_width;
        }
    }
    return result;
}

void CTabWindow::CreateCountNotification(int index)
{
    if (index < 0 || index >= m_tabs.size()) {
        return;
    }
    TTab* tab = m_tabs[index];
    if (tab == NULL) {
        return;
    }
    if (tab->m_notification != NULL) {
        return;
    }
    tab->m_notification = new CCountNotificationWindow();
}

void CTabWindow::SetCountToNotification(int index, int count)
{
    if (index < 0 || index >= m_tabs.size()) {
        return;
    }
    TTab* tab = m_tabs[index];
    if (tab == NULL) {
        return;
    }
    tab->m_notification->SetCount(count);
}
