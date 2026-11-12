#include "CGiftRecievedNewsWindow.h"

#include "App.h"
#include "BaseDialog.h"
#include "CFontMgr.h"
#include "CGServeHelper.h"
#include "CNewsFeedManager.h"
#include "CShopItem.h"
#include "CSocialGiftsManager.h"
#include "SScreenAdaptingConstant.h"
#include "ServiceWindows.h"

namespace {
const int SendBack = -0x194ddb87;
const int ShowGiftCatalog = 0x61ab1bdd;
const SScreenAdaptingConstant s_buttonWidth(180, 360);
const SScreenAdaptingConstant s_lineHeight(5, 10);
}

CGiftRecievedNewsWindow::CGiftRecievedNewsWindow(const SGiftReceivedMessage* message)
    : Window(false), m_senderId(message->m_senderId)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    SGift* gift = app->m_socialGiftManager->GetGift(message->m_itemIndex);
    XString item = ResString("IDS_FRIENDS_TAB_GIFTS");
    if (gift != NULL && gift->GetItem() != NULL) {
        item = gift->GetItem()->GetName();
    }
    XString sender = CGServeHelper::GetFriendNickname(m_senderId);
    CreateUserInterface(sender, item, sender.Length() != 0);
}

void CGiftRecievedNewsWindow::CreateUserInterface(const XString& sender, const XString& item, bool fromFriend)
{
    SetHeightByContent(0, 0);
    XString text;
    if (fromFriend) {
        XString format = ResString("IDS_GIFT_FROM_FRIEND_NEWSFEED");
        text = XString::Format(format.c_str(), item.c_str(), sender.c_str());
    } else {
        XString format = ResString("IDS_GIFT_FROM_SERVER_NEWSFEED");
        text = XString::Format(format.c_str(), item.c_str());
    }
    TextWindow* label = new TextWindow(text, CFontMgr::GetInstance()->GetFont(15));
    label->SetAlign(ALIGN_LEFT | ALIGN_VCENTER);
    label->SetPercentWidth(60, 0, 0);
    label->SetHeightByContent(0, 0);
    AddToFront(label, 0, 0);
    Window* actions = new Window(false);
    actions->SetHeightByContent(0, 0);
    actions->SetPercentWidth(40, 0, 0);
    if (fromFriend) {
        StretchImageButton* send = new StretchImageButton(ResString("IDS_FRIENDS_SEND_GIFT_BACK"),
            "IDB_HUD_TRANSPARENT_BLACK_LEFT", "IDB_HUD_TRANSPARENT_BLACK_TILE", "IDB_HUD_TRANSPARENT_BLACK_RIGHT");
        send->SetAlign(ALIGN_RIGHT | ALIGN_VCENTER);
        send->m_command = SendBack;
        send->m_commandParameter1 = reinterpret_cast<int>(this);
        send->ForceSetWidth(s_buttonWidth());
        actions->AddToFront(send, 0, 0);
    }
    AddToFront(actions, 1, 0);

    SeparateLineRed* line = new SeparateLineRed();
    line->SetDesiredHeight(s_lineHeight());
    line->SetOutsetSpacing(s_lineHeight(), 0, 0, 0);
    line->SetCellPos(0, 1, 2, 1);
    AddToFront(line);
}

void CGiftRecievedNewsWindow::OnCommand(Event& event)
{
    if (event.m_id == SendBack && event.m_param1 == reinterpret_cast<int>(this)) {
        WindowApp::HandleTunnelCommand(ShowGiftCatalog, m_senderId, 0, 0);
        event.Clear();
    }
}
