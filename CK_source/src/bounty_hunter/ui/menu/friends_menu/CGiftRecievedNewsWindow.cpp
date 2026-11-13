#include "CGiftRecievedNewsWindow.h"

#include "App.h"
#include "BaseDialog.h"
#include "CFontMgr.h"
#include "CNGS.h"
#include "CNGSLocalUser.h"
#include "CNGSRemoteUser.h"
#include "CNGSRemoteUserList.h"
#include "ICSystemFont.h"
#include "CNewsFeedManager.h"
#include "CShopItem.h"
#include "CSocialGiftsManager.h"
#include "SScreenAdaptingConstant.h"
#include "ServiceWindows.h"

namespace {
const int SendBack = -0x194ddb87;
const int ShowGiftCatalog = 0x61ab1bdd;
}

CGiftRecievedNewsWindow::CGiftRecievedNewsWindow(const SGiftReceivedMessage* message)
    : Window(false), m_item(NULL), m_message(message), m_fieldB4(0), m_sendButton(NULL)
{
    SGift* gift = static_cast<App*>(WindowApp::m_instance)->m_socialGiftManager->GetGift(message->m_itemIndex);
    if (gift != NULL) {
        CItem* item = gift->GetItem();
        XString sender("");
        CNGSRemoteUserList* friends = CNGS::GetInstance()->GetLocalUser()->m_remoteUserLists[0];
        bool fromFriend = false;
        for (int index = 0; index < friends->m_users.Size(); ++index) {
            int clientId = friends->m_users[index]->GetClientID();
            fromFriend |= clientId == m_message->m_senderId;
        }
        if (fromFriend) {
            CNGSRemoteUser* user =
                CNGS::GetInstance()->GetLocalUser()->m_remoteUserLists[0]->findRemoteUserByID(m_message->m_senderId);
            if (user != NULL) {
                CStrWChar nickname = user->GetNickName();
                sender = nickname.GetWCharPtr();
            }
        }
        CreateUserInterface(sender, item->GetName(), fromFriend);
        m_item = item;
    }
    m_fieldA8 = 0;
    m_messageId = message->m_messageId;
}

void CGiftRecievedNewsWindow::CreateUserInterface(const XString& sender, const XString& item, bool fromFriend)
{
    SetHeightByContent(0, 0);
    XString text;
    if (fromFriend) {
        text = XString::Format(ResString("IDS_GIFT_FROM_FRIEND_NEWSFEED").c_str(), item.c_str(), sender.c_str());
    } else {
        text = XString::Format(ResString("IDS_GIFT_FROM_SERVER_NEWSFEED").c_str(), item.c_str());
    }
    TextWindow* label = new TextWindow(text, CFontMgr::GetInstance()->GetFont(15));
    label->SetAlign(ALIGN_LEFT | ALIGN_VCENTER);
    ICSystemFont* font = ICSystemFont::CreateInstance();
    font->Initialize(SScreenAdaptingConstant(12, 25)(), NULL, 1);
    label->SetFont(font);
    AddToFront(label, 0, 0);
    label->SetPercentWidth(60, 0, 0);

    Window* actions = new Window(false);
    if (fromFriend) {
        m_sendButton = new StretchImageButton(ResString("IDS_FRIENDS_SEND_GIFT_BACK"),
            "IDB_HUD_TRANSPARENT_BLACK_LEFT", "IDB_HUD_TRANSPARENT_BLACK_TILE", "IDB_HUD_TRANSPARENT_BLACK_RIGHT");
        m_sendButton->SetAlign(ALIGN_RIGHT | ALIGN_VCENTER);
        m_sendButton->SetCommandParameters(SendBack, reinterpret_cast<int>(this), 0, 0);
        StretchImageButton* button = m_sendButton;
        button->ForceSetWidth(SScreenAdaptingConstant(180, 360)());
        actions->AddToFront(m_sendButton, 0, 0);
    }
    AddToFront(actions, 1, 0);
    actions->SetHeightByContent(0, 0);
    actions->SetPercentWidth(40, 0, 0);

    SeparateLineRed* line = new SeparateLineRed();
    line->SetOutsetSpacing(SScreenAdaptingConstant(5, 10)(), 0, 0, 0);
    line->SetDesiredHeight(SScreenAdaptingConstant(5, 10)());
    line->SetCellPos(0, 1, 2, 1);
    AddToFront(line);
}

void CGiftRecievedNewsWindow::OnCommand(Event& event)
{
    if (event.m_id == SendBack && event.m_param1 == reinterpret_cast<int>(this)) {
        WindowApp::HandleTunnelCommand(ShowGiftCatalog, m_message->m_senderId, 0, 0);
        event.Clear();
    }
}
