// CDH_RadioGroupWidget.cpp - connect the choices to their selection index.
#include "CDH_RadioGroupWidget.h"

#include "CDH_RadioButtonWidget.h"

CDH_RadioGroupWidget::CDH_RadioGroupWidget() : Window(false)
{
}

CDH_RadioGroupWidget::~CDH_RadioGroupWidget()
{
}

void CDH_RadioGroupWidget::AddButton(CDH_RadioButtonWidget* button)
{
    button->m_userData = m_buttons.size();
    m_buttons.addElement(button);
    AddToFront(button);
}

int CDH_RadioGroupWidget::GetValue()
{
    return m_selected;
}

void CDH_RadioGroupWidget::SetValue(int selected)
{
    m_selected = selected;
    for (int index = 0; index < m_buttons.size(); ++index) {
        m_buttons[index]->SetValue(selected == index);
    }
}

void CDH_RadioGroupWidget::OnCommand(Event& event)
{
    if (event.m_id == CDH_RadioButtonWidget::SelectCommand) {
        SetValue(event.m_param1);
        event.Clear();
    }
}
