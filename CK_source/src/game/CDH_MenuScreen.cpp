// CDH_MenuScreen.cpp - build list entries and publish the selected command.
#include "CDH_MenuScreen.h"

#include "App.h"
#include "ButtonWindow.h"
#include "CDH_SoundManager.h"
#include "CStringToKey.h"

namespace {
const int Back = static_cast<int>(0x97204784u);
const int LeftButton = 0x21204844;
const char* const SUR_MI_MINIGAMES = "SUR_MI_MINIGAMES";
}

CDH_MenuScreen::CDH_MenuScreen()
    : MenuDialog(NULL), m_selectedIndex(-1), m_showBack(true)
{
    SetFlags(0x8000);
}

CDH_MenuScreen::~CDH_MenuScreen()
{
}

Window* CDH_MenuScreen::PopulateSofkeys()
{
    Window* softkeys = new SoftkeysContainer(this);
    if (m_showBack) {
        XString back = ResString("IDS_SK_BACK");
        softkeys->AddToFront(App::NewSoftkeyButton(back, LeftButton, App::SoftkeyMenuBackAlign()));
    }
    return softkeys;
}

void CDH_MenuScreen::setMenuItems(const char* const* items, int count)
{
    for (int index = 0; index < count; ++index) {
        m_items.addElement(items[index]);
        int id = CStringToKey(items[index], 0);
        m_itemKeys.addElement(id);
    }
    for (int index = 0; index < count; ++index) {
        m_icons.addElement(SUR_MI_MINIGAMES);
    }
}

void CDH_MenuScreen::setTitleDelayed(const char* title)
{
    m_title = ResString(title);
}

void CDH_MenuScreen::InitCenter()
{
    if (m_frameStyle == 0) {
        SetCorner(0, 0);
    } else {
        SetCenter(m_parent->m_width / 2, m_parent->m_height / 2 - 30);
    }
}

void CDH_MenuScreen::PopulateContent()
{
    for (int index = 0; index < m_items.size(); ++index) {
        ImageRes icon;
        icon.Create(m_icons[index]);
        AddResourceItem(m_items[index], icon, icon, m_itemKeys[index], ITEM_NORMAL);
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_soundManager->PlaySound("IDM_MENU_WINDOW_OPEN_CLOSE", false, false);
}

void CDH_MenuScreen::ClickItem(int id, bool)
{
    if (m_items.size() > 0) {
        m_selectedIndex = 0;
        while (m_selectedIndex < m_items.size() && m_itemKeys[m_selectedIndex] != id) {
            ++m_selectedIndex;
        }
    } else {
        m_selectedIndex = id;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_soundManager->PlaySound("IDM_MENU_WINDOW_BUTTON", false, false);
    SetInterrupt(1);
}

void CDH_MenuScreen::OnCommand(Event& event)
{
    if (event.m_id == Back || event.m_id == LeftButton) {
        SetInterrupt(2);
        event.Clear();
    }
}

void CDH_MenuScreen::OnGetFocus()
{
    SetFlags(FLAG_VISIBLE);
}

void CDH_MenuScreen::OnLostFocus()
{
    ClearFlags(FLAG_VISIBLE);
}
