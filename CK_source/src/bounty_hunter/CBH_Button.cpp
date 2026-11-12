// CBH_Button.cpp - base button of the game UI (CK 1.5.2, written by game2).
#include "CBH_Button.h"
#include "WindowApp.h"
#include "App.h"
#include "CDH_SoundManager.h"
#include "MathLib.h"

CBH_Button::CBH_Button() : Window(false)
{
    SetLayoutType(LAYOUT_NONE);
    m_commandParameter3 = 0;
    m_commandParameter2 = 0;
    m_commandParameter1 = 0;
    m_pressed = false;
    m_expandDragArea = false;
    m_command = 0;
}

void CBH_Button::OnPointerEvent(Event& event)
{
    bool hit = false;
    // Only drag/release compute the hit; a press records the screen position first.
    if ((unsigned int)(event.m_type - Event::POINTER_DRAGGED) <= 1) {
        int width = m_width;
        int height = m_height;
        if (m_expandDragArea) {
            width *= 2;
            height *= 2;
        }
        if (event.m_param1 >= 0 && event.m_param1 < width &&
            event.m_param2 >= 0 && event.m_param2 < height) {
            // The pointer must not have moved more than 10 (scaled) pixels since the press.
            if (MathLib::Fabs((float)(m_pressGlobalX - m_parent->LocalToGlobalX(m_x))) < (float)(App::Scale() * 10)) {
                if (MathLib::Fabs((float)(m_pressGlobalY - m_parent->LocalToGlobalX(m_y))) < (float)(App::Scale() * 10)) {
                    hit = true;
                }
            }
        }
    }
    switch (event.m_type) {
    case Event::POINTER_DRAGGED:
        m_pressed = hit;
        event.Clear();
        break;
    case Event::POINTER_PRESSED:
        m_pressed = true;
        m_pressGlobalX = m_parent->LocalToGlobalX(m_x);
        m_pressGlobalY = m_parent->LocalToGlobalX(m_y);
        // The original reads the sound manager at App+0x200 (App.h still has the CKZ layout).
        ((App*)WindowApp::m_instance)->m_soundManager->PlaySound("IDM_MENU_WINDOW_BUTTON", false, false);
        OnPointerPressed(event);
        event.Clear();
        break;
    case Event::POINTER_RELEASED:
        OnPointerReleased(event);
        m_pressed = false;
        if (hit != 1) {
            return;
        }
        if (m_command != 0) {
            WindowApp::HandleTunnelCommand(m_command, m_commandParameter1, m_commandParameter2, m_commandParameter3);
        }
        event.Clear();
        break;
    }
}
