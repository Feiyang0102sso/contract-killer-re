// CBH_TriggerButton.cpp - draw and dispatch the active or inactive cover button.
#include "CBH_TriggerButton.h"

#include "App.h"
#include "AppSettings.h"
#include "ICRenderSurface.h"

CBH_TriggerButton::CBH_TriggerButton(const char* active, const char* inactive)
    : m_activeImage(active), m_inactiveImage(inactive),
      m_activeSurface(m_activeImage.GetSurface()), m_inactiveSurface(m_inactiveImage.GetSurface()),
      m_activeWidth(0), m_activeHeight(0), m_inactiveWidth(0), m_inactiveHeight(0),
      m_active(false), m_acceptsInput(true)
{
    unsigned int width;
    unsigned int height;
    if (m_activeSurface != NULL) {
        m_activeSurface->GetWidthAndHeight(width, height);
        m_activeWidth = width;
        m_activeHeight = height;
    }
    if (m_inactiveSurface != NULL) {
        m_inactiveSurface->GetWidthAndHeight(width, height);
        m_inactiveWidth = width;
        m_inactiveHeight = height;
    }
    SetButtonSize();
}

void CBH_TriggerButton::SetButtonSize()
{
    if (m_active) {
        SetSize(m_activeWidth, m_activeHeight);
    } else {
        SetSize(m_inactiveWidth, m_inactiveHeight);
    }
}

void CBH_TriggerButton::Enable(bool enabled)
{
    m_acceptsInput = enabled;
}

void CBH_TriggerButton::ChangeState()
{
    m_active = !m_active;
    SetButtonSize();
}

void CBH_TriggerButton::Paint(ICGraphics2d& graphics)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    if ((m_flags & FLAG_VISIBLE) != 0 && app->m_settings->m_field1f) {
        ICRenderSurface* surface = m_inactiveSurface;
        if (m_active) {
            surface = m_activeSurface;
        }
        DrawImage(graphics, surface, 0, 0, ALIGN_LEFT | ALIGN_TOP, 0);
    }
}

void CBH_TriggerButton::OnPointerPressed(Event& event)
{
    if (m_acceptsInput) {
        ButtonWindow::OnPointerPressed(event);
    }
}

void CBH_TriggerButton::OnPointerReleased(Event& event)
{
    if (m_acceptsInput) {
        ButtonWindow::OnPointerReleased(event);
    }
}
