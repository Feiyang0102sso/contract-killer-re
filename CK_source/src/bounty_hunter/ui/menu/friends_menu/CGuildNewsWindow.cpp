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
#include "ICSystemFont.h"
#include "SScreenAdaptingConstant.h"
#include "ServiceWindows.h"

namespace {
const int AcceptGuildInvite = 0x7d371742;
const int IgnoreGuildInvite = 0x0d611fcc;
const long long GuildInviteAccepted = 1006;
const long long GuildInviteDeclined = 1007;
const unsigned int GuildResponseDurationSeconds = 604800;
}

CGuildInviteNewsWindow::CGuildInviteNewsWindow(const STargettedMessage* message)
    : Window(false), m_sender(NULL), m_messageId(-1)
{
    // The original leaves the ID unset when the pending envelope is missing.
    m_senderId = message->m_senderId;
    m_message = message;
    CNGSLocalUser* local = CNGS::GetInstance()->GetLocalUser();
    m_sender = local->m_remoteUserLists[0]->findRemoteUserByID(m_senderId);
    CNGSFromServerMessageQ* queue = CNGS::GetInstance()->GetLocalUser()->m_messageQueue;
    TCVector<long long> ids;
    queue->GetAllMessagesByUserAndType(ids, 0, 0, CNGSFromServerMessage::CNGS_MSG_SUBTYPE_ANY);
    for (int index = 0; index < ids.Size(); ++index) {
        CNGSFromServerMessage* pending = queue->GetMessageById(ids[index]);
        if (pending != NULL && pending->m_senderId == message->m_senderId
            && pending->GetMessageText() == message->m_text) {
            m_messageId = static_cast<int>(pending->m_messageId);
        }
    }
    CreateUserInterface();
}

void CGuildInviteNewsWindow::CreateUserInterface()
{
    SetHeightByContent(0, 0);
    XString nickname;
    if (m_sender != NULL) {
        nickname = m_sender->GetNickName().GetWCharPtr();
    }
    XString text = XString::Format(ResString("IDS_YOU_ARE_INVITED_INTO_GUILD").c_str(), nickname.c_str());
    TextWindow* label = new TextWindow(text, CFontMgr::GetInstance()->GetFont(15));
    label->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    ICSystemFont* font = ICSystemFont::CreateInstance();
    font->Initialize(SScreenAdaptingConstant(12, 25)(), NULL, 1);
    label->SetFont(font);
    AddToFront(label, 0, 0);

    StretchImageButton* accept = new StretchImageButton(ResString("IDS_DIALOG_BUTTON_ACCEPT"),
        "IDB_HUD_TRANSPARENT_BLACK_LEFT", "IDB_HUD_TRANSPARENT_BLACK_TILE", "IDB_HUD_TRANSPARENT_BLACK_RIGHT");
    accept->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    accept->SetCommandParameters(AcceptGuildInvite, 0, 0, 0);
    AddToFront(accept, 1, 0);
    StretchImageButton* ignore = new StretchImageButton(ResString("IDS_DIALOG_BUTTON_IGNORE"),
        "IDB_HUD_TRANSPARENT_BLACK_LEFT", "IDB_HUD_TRANSPARENT_BLACK_TILE", "IDB_HUD_TRANSPARENT_BLACK_RIGHT");
    ignore->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    ignore->SetCommandParameters(IgnoreGuildInvite, 0, 0, 0);
    AddToFront(ignore, 2, 0);
}

void CGuildInviteNewsWindow::OnCommand(Event& event)
{
    if (event.m_id == AcceptGuildInvite) {
        CNGSRemoteUserList* friends = CNGS::GetInstance()->GetLocalUser()->m_remoteUserLists[0];
        for (int index = 0; index < friends->m_users.Size(); ++index) {
            CNGSRemoteUser* user = friends->m_users[index];
            if (user->GetClientID() == m_senderId) {
                CObjectMapString response(CStrWChar("gia"));
                user->SendMessage(&response, GuildResponseDurationSeconds, GuildInviteAccepted, false, NULL);
                static_cast<App*>(WindowApp::m_instance)->m_publicData->addGuildFriend(m_senderId);
                CNGS::GetInstance()->GetLocalUser()->m_notificationHandler->RemoveGameSpecificMessage(m_messageId);
                WindowApp::HandleTunnelCommand(4, 0, 0, 0);
                CGameAnalytics::logAssassinsInviteAccepted(m_senderId);
            }
        }
    } else if (event.m_id == IgnoreGuildInvite) {
        CNGSRemoteUserList* friends = CNGS::GetInstance()->GetLocalUser()->m_remoteUserLists[0];
        for (int index = 0; index < friends->m_users.Size(); ++index) {
            CNGSRemoteUser* user = friends->m_users[index];
            if (user->GetClientID() == m_senderId) {
                CObjectMapString response(CStrWChar("gid"));
                user->SendMessage(&response, GuildResponseDurationSeconds, GuildInviteDeclined, false, NULL);
                CNGS::GetInstance()->GetLocalUser()->m_notificationHandler->RemoveGameSpecificMessage(m_messageId);
                CGameAnalytics::logAssassinsInviteDeclined(m_senderId);
            }
        }
    } else {
        return;
    }
    static_cast<App*>(WindowApp::m_instance)->m_newsFeedManager->RemoveMessage(m_message);
    Close();
    event.Clear();
}
