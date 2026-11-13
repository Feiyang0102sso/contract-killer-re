// CDH_RadioButtonWidget.cpp - select a choice through the parent window.
#include "CDH_RadioButtonWidget.h"

#include "App.h"
#include "BaseDialog.h"
#include "CDH_SoundManager.h"
#include "CFontMgr.h"

CDH_RadioButtonWidget::CDH_RadioButtonWidget() : Window(false), m_image(NULL)
{
    m_fieldA8 = 0;
    m_fieldA4 = true;
    m_off = ImageRes("SUR_RADIO_BUTTON_OFF");
    m_on = ImageRes("SUR_RADIO_BUTTON_ON");
}

CDH_RadioButtonWidget::~CDH_RadioButtonWidget()
{
}

void CDH_RadioButtonWidget::Init(bool selected, XString label, int font)
{
    Window* content = new Window(false);
    content->SetAlign(ALIGN_LEFT);
    content->SetWidthByContent(0, 0);
    content->SetHeightByContent(0, 0);
    m_image = new ImageWindow();
    m_image->SetImage(m_off);
    m_image->SetDesiredWidth(ImageWidth(m_off.GetSurface()));
    m_image->SetDesiredHeight(ImageHeight(m_off.GetSurface()));
    m_image->SetAlign(ALIGN_LEFT | ALIGN_VCENTER);
    m_image->SetCellPos(1, 0, 1, 1);
    content->AddToFront(m_image);
    TextWindow* text = new TextWindow(label, CFontMgr::GetInstance()->GetFont(font));
    text->SetAlign(ALIGN_LEFT | ALIGN_VCENTER);
    text->SetCellPos(0, 0, 1, 1);
    content->AddToFront(text);
    SetValue(selected);
    AddToFront(content);
}

void CDH_RadioButtonWidget::SetValue(bool selected)
{
    // the original picks the image first and makes a single call
    ImageRes* image = &m_off;
    if (selected) {
        image = &m_on;
    }
    m_image->SetImage(*image);
}

void CDH_RadioButtonWidget::OnPointerEvent(Event& event)
{
    if (event.m_type == Event::POINTER_PRESSED) {
        SetValue(true);
        App* app = static_cast<App*>(WindowApp::m_instance);
        app->m_soundManager->PlaySound("IDM_MENU_WINDOW_BUTTON", false, false);
        m_parent->HandleEvent(Event::COMMAND, SelectCommand, m_userData, 0, 0);
        event.Clear();
    }
}
