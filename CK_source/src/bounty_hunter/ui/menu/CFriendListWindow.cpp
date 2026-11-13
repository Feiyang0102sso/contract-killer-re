#include "CFriendListWindow.h"

#include "BaseDialog.h"
#include "CCareerWindow.h"
#include "CFontMgr.h"
#include "CFriendWindow.h"
#include "CImageButton.h"
#include "SScreenAdaptingConstant.h"
#include "ServiceWindows.h"
#include "WindowApp.h"

CFriendListWindow::CFriendListWindow() : Window(false), m_row(0), m_friends(NULL)
{
    Window* buttons = new Window(false);
    buttons->m_debugFrame = true;
    AddToFront(buttons, 0, 0);
    buttons->SetPercentWidth(30, 0, 0);
    Window* invite = CreateMissionButton("IDB_INVITE_FRIEND", ResString("IDS_FRIENDS_INVITE_BUTTON"),
        INVITE_FRIENDS);
    buttons->AddToFront(invite, 0, 0);
    Window* coop = CreateMissionButton("IDB_ASSASINS_GUILD", ResString("IDS_COOP_MISSION"), SHOW_COOP);
    buttons->AddToFront(coop, 0, 1);

    Window* friends = new Window(false);
    friends->AddToFront(CreateFriendsWindow());
    AddToFront(friends, 1, 0);
    friends->SetPercentWidth(70, 0, 0);
}

Window* CFriendListWindow::CreateMissionButton(const char* image, const XString& caption, unsigned int command)
{
    Window* group = new Window(false);
    CImageButton* button = new CImageButton();
    button->AddImage(image, 0, 0);
    button->SetCommandParameters(command, 0, 0, 0);
    group->AddToFront(button, 0, 0);

    Window* label = new Window(false);
    group->AddToFront(label, 0, 1);
    TextWindow* text = new TextWindow(caption, CFontMgr::GetInstance()->GetFont(17));
    text->SetHeightByContent(0, 0);
    text->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
    label->AddToFront(text);
    return group;
}

Window* CFriendListWindow::CreateFriendsWindow()
{
    Window* panel = new Window(false);
    panel->SetLayoutType(LAYOUT_NONE);
    m_friends = new Window(false);
    m_friends->SetPercentWidth(100, 0, 0);
    m_friends->SetHeightByContent(0, 0);
    m_friends->SetLayoutType(LAYOUT_NONE);

    ScrollingContainer* scroll = new ScrollingContainer();
    scroll->SetLayoutType(LAYOUT_NONE);
    scroll->SetPercentHeight(100, 0, 0);
    scroll->SetPercentWidth(100, 0, 0);
    scroll->AddToFront(m_friends);
    panel->AddToFront(scroll, 0, 0);

    VertScrollerSimpleRed* scroller = new VertScrollerSimpleRed(scroll);
    scroller->SetOutsetSpacing(3, 5, 3, 4);
    panel->AddToFront(scroller, 1, 0);
    return panel;
}

void CFriendListWindow::AddFriend(int friendId)
{
    CFriendWindow* friendRow = new CFriendWindow(friendId);
    m_friends->AddToFront(friendRow, 0, m_row++);
    friendRow->SetHeightByContent(0, 0);
    SeparateLineRed* separator = new SeparateLineRed();
    const SScreenAdaptingConstant height(10, 20);
    separator->SetDesiredHeight(height());
    m_friends->AddToFront(separator, 0, m_row++);
}

void CFriendListWindow::OnCommand(Event& event)
{
    if (event.m_id == 0x236889e6) {
        WindowApp::HandleTunnelCommand(-0x76245f14, 0, 0, 0);
    }
}
