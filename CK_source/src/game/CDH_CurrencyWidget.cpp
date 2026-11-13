#include "CDH_CurrencyWidget.h"

#include "App.h"
#include "BaseDialog.h"
#include "CFontMgr.h"
#include "DGHelper.h"

CDH_CurrencyWidget::CDH_CurrencyWidget(eValueType type) : Window(false), m_type(type), m_small(0)
{
    init();
}

void CDH_CurrencyWidget::init()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_PlayerData* player = app->m_helper->GetPlayerData();
    const char* icon;
    if (m_type == HunterPoints) {
        m_value = player->m_field98;
        m_format = XString(L"HP: %d");
        icon = "SUR_HUNTER_POINTS";
        if (m_small != 0) {
            icon = "SUR_HUNTER_POINTS_SMALL";
        }
    } else if (m_type == SkillPoints) {
        m_value = player->m_field9c;
        m_format = XString(L"Skill points: %d");
        icon = "SUR_SKILL_POINTS";
        if (m_small != 0) {
            icon = "SUR_SKILL_POINTS_SMALL";
        }
    } else {
        m_value = player->m_field94;
        m_format = XString(L"Credits: %d");
        icon = "SUR_CREDITS";
        if (m_small != 0) {
            icon = "SUR_CREDITS_SMALL";
        }
    }

    int fontIndex = 5;
    if (m_small != 0) {
        fontIndex = 1;
    }
    m_font = CFontMgr::GetInstance()->GetFont(fontIndex);
    Window* row = new Window(false);
    row->SetWidthByContent(0, 0);
    row->SetHeightByContent(0, 0);
    row->SetAlign(ALIGN_LEFT | ALIGN_VCENTER);
    AddToFront(row);
    m_label = new TextWindow();
    m_label->SetWidthByContent(0, 0);
    m_label->SetAlign(ALIGN_LEFT | ALIGN_VCENTER);
    row->AddToFront(m_label, 0, 0);
    m_image = new ImageWindow(ImageWindow::FILL_NONE, ImageRes(icon));
    row->AddToFront(m_image, 1, 0);
    SetWidthByContent(0, 0);
    SetHeightByContent(0, 0);
    updateData();
}

void CDH_CurrencyWidget::updateData()
{
    m_label->SetText(XString::Format(m_format.c_str(), m_value), m_font);
}

void CDH_CurrencyWidget::setText(XString format)
{
    m_format = format;
    updateData();
}

void CDH_CurrencyWidget::setValue(int value)
{
    m_value = value;
    updateData();
}

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AssertCurrencyWidgetSize[sizeof(CDH_CurrencyWidget) == 0xc0 ? 1 : -1];
#endif
