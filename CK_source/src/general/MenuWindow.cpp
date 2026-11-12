// MenuWindow.cpp - position menu dialogs and coordinate their transitions.
#include "MenuWindow.h"

#include "CStringToKey.h"
#include "WindowApp.h"

namespace {
const int MenuOpened = 0x49ad3e9c;
const int MenuCommand = static_cast<int>(0xa563bcbd);
}

MenuWindow::MenuDialog::MenuDialog(MenuWindow* owner)
    : SimpleDialog(DIALOG_TYPE_1), m_owner(owner), m_field174(false), m_selectedKey(-999999)
{
}

void MenuWindow::MenuDialog::Init()
{
    SimpleDialog::Init();
    WindowApp::HandleTunnelCommand(MenuOpened, reinterpret_cast<int>(this), 0, 0);
    SetSize(100, 100);
    PopulateContent();
    InitCenter();
    AdjustByDefRect();
    SetSelectedKey(m_selectedKey);
}

void MenuWindow::MenuDialog::Shutdown()
{
    SimpleDialog::Shutdown();
}

void MenuWindow::MenuDialog::CancelDialog()
{
    ClickItem(CStringToKey("IDS_MENU_BACK", 0) & 0x7fffffff, false);
}

void MenuWindow::MenuDialog::OnCommand(Event& event)
{
    SimpleDialog::OnCommand(event);
    if (event.m_id == MenuOpened && event.m_param1 != reinterpret_cast<int>(this)) {
        ReverseEasy(FLAG_CLOSE_AFTER_EASY);
    } else if (event.m_id == MenuCommand) {
        event.Clear();
    }
}

void MenuWindow::MenuDialog::PopulateContent()
{
    // Concrete menu dialogs populate their own lists.
}

void MenuWindow::MenuDialog::InitCenter()
{
    SetCenter(static_cast<int>(m_parent->m_width * 0.5f),
              static_cast<int>(m_parent->m_height * 0.5f));
}

void MenuWindow::MenuDialog::OnLayout()
{
    SimpleDialog::OnLayout();
    ClearFlags(FLAG_SHOW_AFTER_EASY);
    InitCenter();
    AdjustByDefRect();
}

void MenuWindow::MenuDialog::AdjustByDefRect()
{
    WindowApp* app = WindowApp::m_instance;
    AdjustByRect(5, 10, app->m_screenWidth - 10, app->m_screenHeight - 40);
}
