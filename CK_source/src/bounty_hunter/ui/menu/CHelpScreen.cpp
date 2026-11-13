// CHelpScreen.cpp - localized help pages, with a scrollbar for each tab.
#include "CHelpScreen.h"

#include "CFontMgr.h"
#include "CNGS.h"
#include "CNGSLocalUser.h"
#include "CTabWindow.h"
#include "SScreenAdaptingConstant.h"
#include "IOSInterface.h"
#include "ICStdUtil.h"

CHelpScreen::CHelpScreen()
    : CBaseScreen(false, false, true, false, true, false),
      m_aboutText(NULL), m_unlock(NULL), m_aboutPending(true)
{
    SetTitleText(ResString("IDS_MENU_HELP"));
    m_tabs = new CTabWindow(false);
    m_tabs->SetPercentHeight(100, 0, 0);
    m_tabs->SetPercentWidth(100, 0, 0);
    m_content->AddToFront(m_tabs);

    char build[100];
    ICStdUtil::SPrintF(build, "\n\n\n\n\n\n$Name: BOUNTY_20120817-131937 $\n%s %s", "Aug 17 2012", "13:59:08");
    int major = XString("1").ToInt();
    int minor = XString("5").ToInt();
    int patch = XString("2").ToInt();
    XString about = XString::Format(ResString("IDS_HELP_TEXT_ABOUT").c_str(), major, minor, patch);
    about.Append(XString(build));
    about.Append(L"\n" + ResString("IDS_GLU_ID"));
    if (CNGS::GetInstance()->GetLocalUser()->GetClientID() != -1) {
        about.Append(XString::Format(L"%d", CNGS::GetInstance()->GetLocalUser()->GetClientID()));
    } else {
        about.Append(ResString("IDS_GLU_ID_UNAVAILABLE"));
    }
    // The unlocked about page also lists the service ids and urls of this build.
    m_about = about + L"\n";
    m_about.Append(XString("NGServ url:") + XString("http://ng-bounty.glu.com/bounty/"));
    m_about.Append(XString("\ngOffer url:") + XString("http://goffer.glu.com/gOffer/"));
    m_about.Append(XString::Format(L"\ngOffer id: %u", 8));
    m_about.Append(L"\nFlurry ID:" + XString("1GQE9DWB9DP459DYZHE1"));
    m_about.Append(L"\nPlayheaven public: " + XString("f51d2107e4174736b33bcbd699da7648"));
    m_about.Append(L"\nPlayheaven secret: " + XString("82cf3f2f8c2443d3bbdb649cee5987a1"));
    m_about.Append(L"\n", wcslen(L"\n") * sizeof(wchar_t));
    m_about.Append(XString("Tapjoy ID:") + XString("9b0a75e0-3a11-432a-8c24-414e30f69b35"));
    m_about.Append(L"\n", wcslen(L"\n") * sizeof(wchar_t));
    if (IOSInterface::isDeviceTablet() == true) {
        m_about.Append(XString("AC_APPID:") + XString("app4ddb6c2ec6a37"));
    } else {
        m_about.Append(XString("AC_APPID:") + XString("app4ddb6b6d7e9df"));
    }

    AddTab(ResString("IDS_HELP_TITLE_INTRO"), ResString("IDS_HELP_TEXT_INTRO"), NULL);
    AddTab(ResString("IDS_TITLE_ABOUT"), about, &m_aboutText);
    AddTab(ResString("IDS_HELP_CONTROLS"), ResString("IDS_HELP_TEXT_CONTROLS"), NULL);
    AddTab(ResString("IDS_MENU_EQUIPMENT"), ResString("IDS_HELP_TEXT_EQUIPMENT"), NULL);
    AddTab(ResString("IDS_HELP_TITLE_MISSIONS"), ResString("IDS_HELP_TEXT_MISSIONS"), NULL);
    m_tabs->SetCurrTab(0, false);
    m_background.Create("SUR_MENU_HONEYCOMD_UP");
    m_unlock = new CUnlockWindow();
    AddToFront(m_unlock);
}

void CHelpScreen::AddTab(XString title, XString text, TextWindow** output)
{
    Window* page = new Window(false);
    page->SetCorner(m_tabs->m_x, m_tabs->m_y);
    page->SetLayoutType(LAYOUT_NONE);

    ScrollingContainer* contents = new ScrollingContainer();
    contents->SetCellPos(0, 0, 1, 1);
    contents->SetOutsetSpacing(10, 10, 10, 10);
    contents->SetPercentWidth(100, 0, 0);
    contents->SetPercentHeight(100, 0, 0);
    TextWindow* body = new TextWindow(text, CFontMgr::GetInstance()->GetFont(15));
    body->SetPercentWidth(100, 0, 0);
    body->SetLayoutType(LAYOUT_NONE);
    contents->AddToFront(body);
    page->AddToFront(contents);

    ScrollingContainer::VertScroller* scrollBar = new ScrollingContainer::VertScroller(contents);
    scrollBar->SetCellPos(1, 0, 1, 1);
    scrollBar->SetOutsetSpacing(3, 5, 3, 4);
    page->AddToFront(scrollBar);
    m_tabs->AddTab(title, page, SScreenAdaptingConstant(15, 20, -1, -1)());
    if (output != NULL) {
        *output = body;
    }
}

CHelpScreen::~CHelpScreen()
{
    WindowApp::HandleTunnelCommand(0x703ba476, 0, 0, 0);
}

void CHelpScreen::Paint(ICGraphics2d& graphics)
{
    DrawTileImageHoriz(graphics, m_background.GetSurface(), 0, 0, m_width);
}

void CHelpScreen::Update()
{
    if (m_unlock->m_unlocked && m_aboutPending) {
        m_aboutPending = false;
        m_aboutText->SetText(m_about);
    }
}

bool CUnlockWindow::CFingerZone::ContainsPoint(int x, int y)
{
    SRect* rect = m_rect;
    // Edges are exclusive on all four sides (as in the original).
    if (rect->m_x + rect->m_width > x && rect->m_x < x && rect->m_y < y) {
        return rect->m_y + rect->m_height > y;
    }
    return false;
}

void CUnlockWindow::OnPointerPressed(Event& event)
{
    m_pressed = true;
    if (m_zones[0]->ContainsPoint(event.m_param1, event.m_param2) == true) {
        m_nextZone = 1;
    }
}

void CUnlockWindow::OnPointerReleased(Event& event)
{
    m_pressed = false;
    for (int index = 0; index < ZoneCount; ++index) {
        m_zones[index]->m_passed = false;
    }
    m_nextZone = 0;
}

void CUnlockWindow::OnPointerDragged(Event& event)
{
    if (!m_pressed) {
        return;
    }
    int zone = m_nextZone;
    if (m_zones[zone]->ContainsPoint(event.m_param1, event.m_param2) == true) {
        m_nextZone = zone + 1;
        if (m_nextZone == ZoneCount) {
            for (int index = 0; index < ZoneCount; ++index) {
                m_zones[index]->m_passed = false;
            }
            m_nextZone = 0;
            m_unlocked = true;
        }
    }
}

CUnlockWindow::CUnlockWindow()
    : Window(false)
{
    m_pressed = false;
    m_unlocked = false;
    m_nextZone = 0;
    InitFingerZones();
    ClearFlags(FLAG_POINTER_EXCLUSIVE);
}

void CUnlockWindow::InitFingerZones()
{
    short halfWidth = m_width / 2;
    short halfHeight = m_height / 2;
    m_zones[0] = new CFingerZone(0, 0, halfWidth, halfHeight);
    m_zones[1] = new CFingerZone(halfWidth, 0, halfWidth, halfHeight);
    m_zones[2] = new CFingerZone(0, halfHeight, halfWidth, halfHeight);
    m_zones[3] = new CFingerZone(halfWidth, halfHeight, halfWidth, halfHeight);
}

CUnlockWindow::~CUnlockWindow()
{
    SAFE_DELETE_ARRAY_OBJS(m_zones, ZoneCount);
}

typedef char UnlockWindowSizeCheck[sizeof(CUnlockWindow) == 0xbc ? 1 : -1];
