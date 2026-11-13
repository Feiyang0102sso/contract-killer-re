#include "CDH_ButtonWidget.h"

#include "BaseDialog.h"
#include "CFontMgr.h"
#include "CUtility.h"
#include "WindowApp.h"

namespace {
const char* const ButtonTile = "SUR_BUTTON_TILE";
const char* const GreenTile = "SUR_BUTTON_GREEN_TILE";
const char* const BlueTile = "SUR_BUTTON_BLUE_TILE";
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
    // Original never frees m_disabledBackground (leaks if Init was not called).
}

void CDH_ButtonWidget::init(XString text, int font, const char* tile)
{
    m_text = XString(text);
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
    m_disabledBackground = NULL;
    m_disabledBackground = new HorizImageWindow(ImageRes("SUR_BUTTON_DISABLED_LEFT"),
                                                 ImageRes("SUR_BUTTON_DISABLED_TILE"),
                                                 ImageRes("SUR_BUTTON_DISABLED_RIGHT"), false);
    m_disabledBackground->ClearFlags(FLAG_VISIBLE);
}

void CDH_ButtonWidget::SetType(button_type_t type)
{
    if (type == Add) {
        m_tile = GreenTile;
        setText("+");
        m_command = 0x7214ff0d;
    } else if (type == Info) {
        m_tile = BlueTile;
        m_text = ResString("IDS_BTN_INFO");
        m_command = 0x9720a3cb;
    }
}

void CDH_ButtonWidget::setContainerAlign(int alignment)
{
    m_content->SetAlign(alignment);
}

void CDH_ButtonWidget::setText(const char* text)
{
    m_text = XString(CUtility::GetString(text).GetWCharPtr());
}

void CDH_ButtonWidget::Init()
{
    int inset = m_textInset;
    if (m_tile == ButtonTile) {
        ImageRes left("SUR_BUTTON_LEFT");
        HorizImageWindow* background = new HorizImageWindow(left, ImageRes(ButtonTile), ImageRes("SUR_BUTTON_RIGHT"), false);
        int leftWidth = ImageWidth(left.GetSurface());
        m_content->AddToFront(background);
        inset += leftWidth;
    } else if (m_tile == GreenTile) {
        ImageRes left("SUR_BUTTON_GREEN_LEFT");
        HorizImageWindow* background = new HorizImageWindow(left, ImageRes(GreenTile), ImageRes("SUR_BUTTON_GREEN_RIGHT"), false);
        int leftWidth = ImageWidth(left.GetSurface());
        m_content->AddToFront(background);
        inset += leftWidth;
    } else if (m_tile == BlueTile) {
        ImageRes left("SUR_BUTTON_BLUE_LEFT");
        HorizImageWindow* background = new HorizImageWindow(left, ImageRes(BlueTile), ImageRes("SUR_BUTTON_BLUE_RIGHT"), false);
        int leftWidth = ImageWidth(left.GetSurface());
        m_content->AddToFront(background);
        inset += leftWidth;
    } else if (m_tile != NULL) {
        m_content->AddToFront(new ImageWindow(ImageWindow::FILL_NONE, ImageRes(m_tile)));
    }
    if (!(m_text == XString(""))) {
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
    // Original never sets the attached flag; the dtor does not free the window.
    m_content->AddToFront(m_disabledBackground);
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

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AssertButtonWidgetSize[sizeof(CDH_ButtonWidget) == 0xd0 ? 1 : -1];
#endif
