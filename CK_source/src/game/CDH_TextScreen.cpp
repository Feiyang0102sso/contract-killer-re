#include "CDH_TextScreen.h"

#include "BaseDialog.h"
#include "CDH_XPWidget.h"
#include "CFont.h"
#include "CFontMgr.h"
#include "CUtility.h"
#include "WindowApp.h"

CDH_TextScreen::CDH_TextScreen()
    : m_experience(0), m_maxWidth(400), m_font(0), m_textAlign(ALIGN_HCENTER | ALIGN_VCENTER)
{
}

CDH_TextScreen::CDH_TextScreen(const char* caption, const char* text)
    : m_experience(0), m_maxWidth(400), m_font(0), m_textAlign(ALIGN_HCENTER | ALIGN_VCENTER)
{
    setCaptionText(caption, false);
    setText(text);
}

CDH_TextScreen::~CDH_TextScreen()
{
}

void CDH_TextScreen::setText(const char* text)
{
    m_text = XString(CUtility::GetString(text).GetWCharPtr());
}

void CDH_TextScreen::setText(const XString& text)
{
    m_text = text;
}

Window* CDH_TextScreen::PopulateSofkeys()
{
    return new SoftkeysContainer(this);
}

void CDH_TextScreen::Init()
{
    CDH_BaseScreen::Init();
    Window* body = new Window(false);
    body->SetWidthByContent(0, 0);
    body->SetHeightByContent(0, 0);

    CFont* font = CFontMgr::GetInstance()->GetFont(m_font);
    TextWindow* text = new TextWindow(m_text, font);
    text->SetAlign(m_textAlign);
    if (font != NULL && m_maxWidth < font->MeasureTextWidth(m_text.c_str(), -1, -1, NULL)) {
        text->SetPercentWidth(100, 0, 0);
        SetDesiredWidth(m_maxWidth);
    } else {
        text->SetWidthByContent(0, 0);
    }
    text->SetHeightByContent(0, 0);
    body->AddToFront(text, 0, 0);
    if (m_experience != 0) {
        body->AddToFront(new CDH_XPWidget(m_experience), 0, 1);
    }
    setLayoutWidget(body, Body);
}

void CDH_TextScreen::OnCommand(Event& event)
{
    CDH_BaseScreen::OnCommand(event);
}

typedef char AssertTextScreenSize[sizeof(CDH_TextScreen) == 0x1a0 ? 1 : -1];
