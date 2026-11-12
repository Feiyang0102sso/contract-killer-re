// CBH_MainMenuBar.cpp - build the Network menu from its button descriptors.
#include "CBH_MainMenuBar.h"

#include "App.h"
#include "CBH_MenuButton.h"
#include "CBH_SurfaceManager.h"

void CBH_MainMenuBar::Paint(ICGraphics2d& graphics)
{
    DrawTileImageRect(graphics, m_background.GetSurface(), m_backgroundRect.m_x, m_backgroundRect.m_y,
                      m_backgroundRect.m_width, m_backgroundRect.m_height);
    DrawTileImageHoriz(graphics, m_border.GetSurface(), 0, 0, m_width);
}

void CBH_MainMenuBar::OnPointerEvent(Event& event)
{
    if (event.m_type == Event::POINTER_PRESSED &&
        m_backgroundRect.m_x < event.m_param1 && event.m_param1 < m_backgroundRect.m_x + m_backgroundRect.m_width &&
        m_backgroundRect.m_y < event.m_param2 && event.m_param2 < m_backgroundRect.m_y + m_backgroundRect.m_height) {
        event.Clear();
    }
}

CBH_MainMenuBar::CBH_MainMenuBar(int, int, int, int)
    : Window(false), m_backgroundRect()
{
    m_background = ImageRes("SUR_TAB_BACKGROUND");
    m_border = ImageRes("SUR_BG_BORDER_FLIP");
}

void CBH_MainMenuBar::ReleaseResources()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_surfaceManager->ReleaseSurface("IDB_HUD_BACKGROUND");
    m_buttons.removeAllElements();
}

void CBH_MainMenuBar::OnCommand(Event&)
{
}

void CBH_MainMenuBar::SetButtons(Vector<MenuButtonDesc*> descriptions)
{
    Window* row = new Window(false);
    row->SetPercentHeight(90, 0, 0);
    row->SetLayoutType(LAYOUT_SIMPLE);
    row->SetAlign(ALIGN_BOTTOM);
    for (int index = 0; index < descriptions.size(); ++index) {
        const MenuButtonDesc* description = descriptions[index];
        CBH_MenuButton* button = new CBH_MenuButton();
        button->setImage(description->m_image);
        button->m_command = description->m_command;
        button->m_commandParameter1 = 0;
        button->m_commandParameter2 = 0;
        button->m_commandParameter3 = 0;
        button->SetCaption(description->m_caption);
        button->SetLayoutType(LAYOUT_TABLE);
        button->SetCellPos(index, 0, 1, 1);
        row->AddToFront(button);
        m_buttons.addElement(button);
    }
    AddToFront(row);
}
