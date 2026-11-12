#include "CGuildNewsWindow.h"

#include "App.h"
#include "BaseDialog.h"
#include "CFontMgr.h"
#include "CGameAnalytics.h"
#include "CNGS.h"
#include "CNGSFromServerMessageQ.h"
#include "CNGSLocalUser.h"
#include "CNGSRemoteUser.h"
#include "CNGSRemoteUserList.h"
#include "CNewsFeedManager.h"
#include "CNotificationHandler.h"
#include "CObjectMapValue.h"
#include "CPublicData1.h"
#include "ServiceWindows.h"

namespace {
const int AcceptGuildInvite = 0x7d371742;
const int IgnoreGuildInvite = 0x0d611fcc;
const long long GuildInvite = 1005;
const long long GuildInviteAccepted = 1006;
const long long GuildInviteDeclined = 1007;
const unsigned int GuildResponseDurationSeconds = 604800;

StretchImageButton* CreateButton(const char* caption, int command, Window* owner)
{
    StretchImageButton* button = new StretchImageButton(Window::ResString(caption),
        "IDB_HUD_TRANSPARENT_BLACK_LEFT", "IDB_HUD_TRANSPARENT_BLACK_TILE", "IDB_HUD_TRANSPARENT_BLACK_RIGHT");
    button->SetAlign(Window::ALIGN_HCENTER | Window::ALIGN_VCENTER);
    button->m_command = command;
    button->m_commandParameter1 = reinterpret_cast<int>(owner);
    return button;
}
}

CGuildInviteNewsWindow::CGuildInviteNewsWindow(const STargettedMessage* message)
    : Window(false), m_senderId(message->m_senderId), m_sender(NULL), m_message(message), m_messageId(-1)
{
    CNGSLocalUser* local = CNGS::GetInstance()->GetLocalUser();
    m_sender = local->m_remoteUserLists[0]->findRemoteUserByID(m_senderId);
    CNGSFromServerMessageQ* queue = local->m_messageQueue;
    TCVector<long long> ids;
    queue->GetAllMessagesByUserAndType(ids, m_senderId, GuildInvite, CNGSFromServerMessage::CNGS_MSG_SUBTYPE_ANY);
    for (int index = 0; index < ids.Size(); ++index) {
        CNGSFromServerMessage* pending = queue->GetMessageById(ids[index]);
        if (pending != NULL && pending->GetMessageText() == message->m_text) {
            m_messageId = static_cast<int>(pending->m_messageId);
        }
    }
    CreateUserInterface();
}

void CGuildInviteNewsWindow::CreateUserInterface()
{
    SetHeightByContent(0, 0);
    XString text = m_message->m_text.GetWCharPtr();
    if (m_sender != NULL) {
        CStrWChar nickname = m_sender->GetNickName();
        XString format = ResString("IDS_YOU_ARE_INVITED_INTO_GUILD");
        text = XString::Format(format.c_str(), nickname.GetWCharPtr());
    }
    TextWindow* label = new TextWindow(text, CFontMgr::GetInstance()->GetFont(15));
    label->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    label->SetHeightByContent(0, 0);
    AddToFront(label, 0, 0);
    if (m_sender != NULL) {
        label->SetPercentWidth(60, 0, 0);
        AddToFront(CreateButton("IDS_DIALOG_BUTTON_ACCEPT", AcceptGuildInvite, this), 1, 0);
        AddToFront(CreateButton("IDS_DIALOG_BUTTON_IGNORE", IgnoreGuildInvite, this), 2, 0);
    }
}

void CGuildInviteNewsWindow::OnCommand(Event& event)
{
    if ((event.m_id != AcceptGuildInvite && event.m_id != IgnoreGuildInvite)
        || event.m_param1 != reinterpret_cast<int>(this)) {
        return;
    }
    CNGSLocalUser* local = CNGS::GetInstance()->GetLocalUser();
    CNGSRemoteUser* sender = local->m_remoteUserLists[0]->findRemoteUserByID(m_senderId);
    if (sender == NULL) {
        event.Clear();
        return;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (event.m_id == AcceptGuildInvite) {
        CObjectMapString response(CStrWChar("gia"));
        sender->SendMessage(&response, GuildResponseDurationSeconds, GuildInviteAccepted, false, NULL);
        app->m_publicData->addGuildFriend(m_senderId);
        CGameAnalytics::logAssassinsInviteAccepted(m_senderId);
    } else {
        CObjectMapString response(CStrWChar("gid"));
        sender->SendMessage(&response, GuildResponseDurationSeconds, GuildInviteDeclined, false, NULL);
        CGameAnalytics::logAssassinsInviteDeclined(m_senderId);
    }
    local->m_notificationHandler->RemoveGameSpecificMessage(m_messageId);
    app->m_newsFeedManager->RemoveMessage(m_message);
    Close();
    event.Clear();
}
