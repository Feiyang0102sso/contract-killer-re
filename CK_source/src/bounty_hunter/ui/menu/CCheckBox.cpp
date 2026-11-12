// CCheckBox.cpp - check box window; toggles on pointer release and sends a tunnel command.
#include "CCheckBox.h"
#include "App.h"
#include "BaseDialog.h"
#include "CBH_SurfaceManager.h"
#include "WindowApp.h"

CCheckBox::CCheckBox(int commandParam)
    : Window(false)
{
    m_checked = false;
    m_enabled = true;
    m_commandParam = commandParam;
    SetWidthByContent(0, 0);
    SetHeightByContent(0, 0);

    App* app = static_cast<App*>(WindowApp::m_instance);
    ImageWindow* background = new ImageWindow();
    background->SetFillType(ImageWindow::FILL_NONE);
    background->SetImage(app->m_surfaceManager->CreateSurface("IDB_CHECKBOX_BG_TRANSPARENT", true));
    AddToFront(background);
    background->m_debugFrame = true;

    m_check = new ImageWindow();
    m_check->SetFillType(ImageWindow::FILL_NONE);
    m_check->SetImage(app->m_surfaceManager->CreateSurface("IDB_CHECKBOX_CHECK", true));
    m_check->ClearFlags(0x2001);
    AddToFront(m_check);

    m_pressed = new ImageWindow();
    m_pressed->SetFillType(ImageWindow::FILL_NONE);
    m_pressed->SetImage(app->m_surfaceManager->CreateSurface("IDB_CHECKBOX_CHECK_PRESSED", true));
    m_pressed->ClearFlags(0x2001);
    AddToFront(m_pressed);
}

void CCheckBox::OnPointerEvent(Event& event)
{
    if (!m_enabled) {
        return;
    }
    if (event.m_type == Event::POINTER_PRESSED) {
        m_pressed->SetFlags(0x2001);
    } else if (event.m_type == Event::POINTER_RELEASED) {
        HandlePointerReleased();
    } else if (event.m_type != Event::POINTER_DRAGGED) {
        return;
    }
    event.Clear();
}

void CCheckBox::HandlePointerReleased()
{
    m_pressed->ClearFlags(0x2001);
    m_checked = !m_checked;
    UpdateCheckStatus();
    int command;
    if (m_checked) {
        command = 0x0d96b6ad;
    } else {
        command = 0x5f58cb68;
    }
    WindowApp::HandleTunnelCommand(command, m_commandParam, 0, 0);
}

bool CCheckBox::IsCheched()
{
    return m_checked;
}

void CCheckBox::UpdateCheckStatus()
{
    if (m_checked) {
        m_check->SetFlags(0x2001);
        return;
    }
    m_check->ClearFlags(0x2001);
}

CCheckBox::~CCheckBox()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_surfaceManager->ReleaseSurface("IDB_CHECKBOX_BG_TRANSPARENT");
    app->m_surfaceManager->ReleaseSurface("IDB_CHECKBOX_CHECK");
    app->m_surfaceManager->ReleaseSurface("IDB_CHECKBOX_CHECK_PRESSED");
}

void CCheckBox::SetChecked(bool checked)
{
    m_checked = checked;
    UpdateCheckStatus();
}
