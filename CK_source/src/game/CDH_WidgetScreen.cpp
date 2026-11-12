#include "CDH_WidgetScreen.h"

#include "BaseDialog.h"
#include "CFont.h"
#include "CFontMgr.h"
#include "CUtility.h"

CDH_WidgetScreen::CDH_WidgetScreen() : m_captionRow(NULL)
{
    for (int index = 0; index < LayoutPositionCount; ++index) {
        Window* section = new Window(false);
        section->SetCellPos(0, index, 1, 1);
        section->SetWidthByContent(0, 0);
        section->SetHeightByContent(0, 0);
        AddToFront(section);
        m_sections[index] = section;
    }
    SetWidthByContent(0, 0);
    SetHeightByContent(0, 0);
    m_showClose = false;
}

CDH_WidgetScreen::~CDH_WidgetScreen()
{
}

void CDH_WidgetScreen::setLayoutWidget(Window* widget, eLayoutPosition position)
{
    if (position >= Caption && position < LayoutPositionCount) {
        m_sections[position]->AddToFront(widget);
        widget->SetCellPos(0, 0, 1, 1);
    }
}

void CDH_WidgetScreen::setCaptionText(const char* text, bool showClose)
{
    CStrWChar caption = CUtility::GetString(text);
    setCaptionText(XString(caption.GetWCharPtr()), showClose);
}

void CDH_WidgetScreen::setCaptionText(const XString& text, bool showClose)
{
    m_caption = text;
    m_showClose = showClose;
    m_captionWidth = 0;
    if (m_caption.Length() != 0) {
        CFont* font = CFontMgr::GetInstance()->GetFont(0);
        if (font != NULL) {
            m_captionWidth = font->MeasureTextWidth(m_caption.c_str(), -1, -1, NULL);
        }
    }
}

void CDH_WidgetScreen::Init()
{
    CDH_SoftkeysScreen::Init();
    Window* header = new Window(false);
    header->SetAlign(ALIGN_LEFT | ALIGN_TOP);
    header->SetPercentWidth(100, 0, 0);
    header->SetHeightByContent(0, 0);
    m_captionRow = new Window(false);
    m_captionRow->SetCellPos(0, 0, 1, 1);
    m_captionRow->SetPercentWidth(100, 0, 0);
    m_captionRow->SetHeightByContent(0, 0);
    header->AddToFront(m_captionRow);

    CFont* font = CFontMgr::GetInstance()->GetFont(0);
    if (m_caption.Length() != 0) {
        TextWindow* caption = new TextWindow(m_caption, font);
        caption->SetAlign(ALIGN_LEFT | ALIGN_VCENTER);
        caption->SetWidthByContent(0, 0);
        caption->SetHeightByContent(0, 0);
        m_captionRow->AddToFront(caption, 0, 0);
    }
    if (m_subtitle.Length() != 0) {
        TextWindow* subtitle = new TextWindow(m_subtitle, font);
        subtitle->SetAlign(ALIGN_LEFT | ALIGN_VCENTER);
        subtitle->SetPercentWidth(100, 0, 0);
        subtitle->SetHeightByContent(0, 0);
        header->AddToFront(subtitle, 0, 1);
    }
    setLayoutWidget(header, Caption);
}

typedef char AssertWidgetScreenSize[sizeof(CDH_WidgetScreen) == 0x13c ? 1 : -1];
