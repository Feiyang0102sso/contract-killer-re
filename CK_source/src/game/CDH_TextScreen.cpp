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
    TextWindow* text = new TextWindow();
    text->SetText(m_text, font);
    text->SetAlign(m_textAlign);
    int textWidth = font->MeasureTextWidth(m_text.c_str(), -1, -1, NULL);
    if (textWidth > m_maxWidth) {
        text->SetPercentWidth(100, 0, 0);
        text->SetHeightByContent(0, 0);
        SetDesiredWidth(m_maxWidth);
    } else {
        text->SetWidthByContent(0, 0);
        text->SetHeightByContent(0, 0);
    }
    body->AddToFront(text, 0, 0);
    if (m_experience != 0) {
        body->AddToFront(new CDH_XPWidget(m_experience), 0, 1);
    }
    setLayoutWidget(body, Body);
}

void CDH_TextScreen::OnCommand(Event& event)
{
    int command = event.m_id;
    if (command == (int)0x97204784 || command == 0x21204844) {
        SetInterrupt(2);
        event.Clear();
        return;
    }
    if (command == 0x07b18a47) {
        SetInterrupt(1);
        event.Clear();
    }
}

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AssertTextScreenSize[sizeof(CDH_TextScreen) == 0x1a0 ? 1 : -1];
#endif
