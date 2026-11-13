#include "CDH_WidgetScreen.h"

#include "BaseDialog.h"
#include "CFont.h"
#include "CFontMgr.h"
#include "CUtility.h"

CDH_WidgetScreen::CDH_WidgetScreen()
{
    m_sections[Caption] = new Window(false);
    m_sections[Body] = new Window(false);
    m_sections[Footer] = new Window(false);
    m_sections[Caption]->SetCellPos(0, 0, 1, 1);
    m_sections[Body]->SetCellPos(0, 1, 1, 1);
    m_sections[Footer]->SetCellPos(0, 2, 1, 1);
    m_sections[Caption]->SetWidthByContent(0, 0);
    m_sections[Caption]->SetHeightByContent(0, 0);
    m_sections[Body]->SetWidthByContent(0, 0);
    m_sections[Body]->SetHeightByContent(0, 0);
    m_sections[Footer]->SetWidthByContent(0, 0);
    m_sections[Footer]->SetHeightByContent(0, 0);
    AddToFront(m_sections[Caption]);
    AddToFront(m_sections[Body]);
    AddToFront(m_sections[Footer]);
    SetWidthByContent(0, 0);
    SetHeightByContent(0, 0);
    m_showClose = false;
}

CDH_WidgetScreen::~CDH_WidgetScreen()
{
}

void CDH_WidgetScreen::setLayoutWidget(Window* widget, eLayoutPosition position)
{
    Window* section;
    if (position == Footer) {
        section = m_sections[Footer];
    } else if (position == Body) {
        section = m_sections[Body];
    } else if (position == Caption) {
        section = m_sections[Caption];
    } else {
        return;
    }
    section->AddToFront(widget);
    widget->SetCellPos(0, 0, 1, 1);
}

void CDH_WidgetScreen::setCaptionText(const char* text, bool showClose)
{
    CStrWChar caption = CUtility::GetString(text);
    XString localized(caption.GetWCharPtr());
    setCaptionText(localized, showClose);
}

void CDH_WidgetScreen::setCaptionText(const XString& text, bool showClose)
{
    m_caption = text;
    m_showClose = showClose;
    int captionWidth;
    if (m_caption.ByteSize() == 0) {
        captionWidth = 0;
    } else {
        CFont* font = CFontMgr::GetInstance()->GetFont(0);
        captionWidth = font->MeasureTextWidth(m_caption.c_str(), -1, -1, NULL);
    }
    m_captionWidth = captionWidth;
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

    if (m_caption.ByteSize() != 0) {
        TextWindow* caption = new TextWindow(m_caption, CFontMgr::GetInstance()->GetFont(0));
        caption->SetAlign(ALIGN_LEFT | ALIGN_VCENTER);
        caption->SetWidthByContent(0, 0);
        caption->SetHeightByContent(0, 0);
        m_captionRow->AddToFront(caption, 0, 0);
    }
    if (m_subtitle.ByteSize() != 0) {
        TextWindow* subtitle = new TextWindow(m_subtitle, CFontMgr::GetInstance()->GetFont(0));
        subtitle->SetAlign(ALIGN_LEFT | ALIGN_VCENTER);
        subtitle->SetPercentWidth(100, 0, 0);
        subtitle->SetHeightByContent(0, 0);
        subtitle->SetCellPos(0, 1, 1, 1);
        header->AddToFront(subtitle);
    }
    header->SetPercentWidth(100, 0, 0);
    header->SetHeightByContent(0, 0);
    setLayoutWidget(header, Caption);
}

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AssertWidgetScreenSize[sizeof(CDH_WidgetScreen) == 0x13c ? 1 : -1];
#endif
