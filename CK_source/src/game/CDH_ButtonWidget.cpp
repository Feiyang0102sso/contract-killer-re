#include "CDH_ButtonWidget.h"

#include "BaseDialog.h"
#include "CFontMgr.h"
#include "CUtility.h"
#include "WindowApp.h"

namespace {
const char ButtonTile[] = "SUR_BUTTON_TILE";
const char GreenTile[] = "SUR_BUTTON_GREEN_TILE";
const char BlueTile[] = "SUR_BUTTON_BLUE_TILE";
}

CDH_ButtonWidget::CDH_ButtonWidget() : Window(false)
{
    init(XString(""), 5, ButtonTile);
}

CDH_ButtonWidget::CDH_ButtonWidget(XString text, int font) : Window(false)
{
    init(text, font, NULL);
}

CDH_ButtonWidget::~CDH_ButtonWidget()
{
    if (!m_disabledBackgroundAttached) {
        delete m_disabledBackground;
    }
}

void CDH_ButtonWidget::init(XString text, int font, const char* tile)
{
    m_text = text;
    m_font = font;
    m_tile = tile;
    SetType(Normal);
    m_content = new Window(false);
    AddToFront(m_content);
    m_textAlign = ALIGN_HCENTER | ALIGN_VCENTER;
    m_textInset = 0;
    m_content->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    m_textWidth = 0;
    m_label = NULL;
    m_command = 0;
    m_disabledBackground = new HorizImageWindow(ImageRes("SUR_BUTTON_DISABLED_LEFT"),
                                                 ImageRes("SUR_BUTTON_DISABLED_TILE"),
                                                 ImageRes("SUR_BUTTON_DISABLED_RIGHT"), false);
    m_disabledBackground->ClearFlags(FLAG_VISIBLE);
    m_disabledBackgroundAttached = false;
}

void CDH_ButtonWidget::SetType(button_type_t type)
{
    if (type == Info) {
        m_tile = BlueTile;
        m_text = ResString("IDS_BTN_INFO");
        m_command = 0x9720a3cb;
    } else if (type == Add) {
        m_tile = GreenTile;
        setText("+");
        m_command = 0x7214ff0d;
    }
}

void CDH_ButtonWidget::setContainerAlign(int alignment)
{
    m_content->SetAlign(alignment);
}

void CDH_ButtonWidget::setText(const char* text)
{
    CStrWChar localized = CUtility::GetString(text);
    m_text = XString(localized.GetWCharPtr());
}

void CDH_ButtonWidget::Init()
{
    const char* left = NULL;
    const char* right = NULL;
    if (m_tile == ButtonTile) {
        left = "SUR_BUTTON_LEFT";
        right = "SUR_BUTTON_RIGHT";
    } else if (m_tile == GreenTile) {
        left = "SUR_BUTTON_GREEN_LEFT";
        right = "SUR_BUTTON_GREEN_RIGHT";
    } else if (m_tile == BlueTile) {
        left = "SUR_BUTTON_BLUE_LEFT";
        right = "SUR_BUTTON_BLUE_RIGHT";
    }
    int inset = m_textInset;
    if (left != NULL) {
        ImageRes cap(left);
        m_content->AddToFront(new HorizImageWindow(cap, ImageRes(m_tile), ImageRes(right), false));
        inset += ImageWidth(cap.GetSurface());
    } else if (m_tile != NULL) {
        m_content->AddToFront(new ImageWindow(ImageWindow::FILL_NONE, ImageRes(m_tile)));
    }
    if (m_text.Length() != 0) {
        m_label = new TextWindow(m_text, CFontMgr::GetInstance()->GetFont(m_font));
        m_label->SetAlign(m_textAlign);
        if (m_textWidth == 0) {
            m_label->SetWidthByContent(0, 0);
        } else {
            m_label->SetDesiredWidth(m_textWidth);
        }
        m_label->SetHeightByContent(0, 0);
        m_label->SetOutsetSpacing(0, inset);
        m_content->AddToFront(m_label);
    }
    m_content->AddToFront(m_disabledBackground);
    m_disabledBackgroundAttached = true;
    m_content->SetWidthByContent(0, 0);
    m_content->SetHeightByContent(0, 0);
    SetWidthByContent(0, 0);
    SetHeightByContent(0, 0);
}

void CDH_ButtonWidget::OnPointerEvent(Event& event)
{
    if (event.m_type == Event::POINTER_PRESSED) {
        WindowApp::HandleEvent(Event::TUNNEL_COMMAND, m_command, m_userData, 0, 0);
        event.Clear();
    }
}

void CDH_ButtonWidget::SetRect(int x, int y, int width, int height, bool force)
{
    Window::SetRect(x, y, width, height, force);
}

typedef char AssertButtonWidgetSize[sizeof(CDH_ButtonWidget) == 0xd0 ? 1 : -1];
