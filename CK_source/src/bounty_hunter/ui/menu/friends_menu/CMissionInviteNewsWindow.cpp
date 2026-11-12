#include "CMissionInviteNewsWindow.h"

#include "App.h"
#include "BaseDialog.h"
#include "CFontMgr.h"
#include "CNGS.h"
#include "CNGSFromServerMessageQ.h"
#include "CNGSLocalUser.h"
#include "CNGSRemoteUser.h"
#include "CNGSRemoteUserList.h"
#include "CNewsFeedManager.h"
#include "CNotificationHandler.h"
#include "CObjectMapValue.h"
#include "ServiceWindows.h"

namespace {
const int AcceptInvite = 0x7d371742;
const int IgnoreInvite = 0x0d611fcc;
const long long MissionInvite = 1002;
const long long MissionInviteAccepted = 1003;
const unsigned int InviteResponseDurationSeconds = 604800;

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

CMissionInviteNewsWindow::CMissionInviteNewsWindow(const SMissionInviteMessage* message)
    : Window(false), m_senderId(message->m_senderId), m_missionId(message->m_missionId),
      m_sender(NULL), m_message(message)
{
    CNGSLocalUser* local = CNGS::GetInstance()->GetLocalUser();
    m_sender = local->m_remoteUserLists[0]->findRemoteUserByID(m_senderId);
    CreateUserInterface();
}

void CMissionInviteNewsWindow::CreateUserInterface()
{
    SetHeightByContent(0, 0);
    CStrWChar nickname;
    if (m_sender != NULL) {
        nickname = m_sender->GetNickName();
    }
    XString format = ResString("IDS_FRIENDS_INVITE_TO_MISSION");
    TextWindow* text = new TextWindow(XString::Format(format.c_str(), nickname.GetWCharPtr()),
                                      CFontMgr::GetInstance()->GetFont(15));
    text->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    text->SetHeightByContent(0, 0);
    text->SetPercentWidth(60, 0, 0);
    AddToFront(text, 0, 0);
    if (m_sender != NULL) {
        AddToFront(CreateButton("IDS_DIALOG_BUTTON_ACCEPT", AcceptInvite, this), 1, 0);
    }
    AddToFront(CreateButton("IDS_DIALOG_BUTTON_IGNORE", IgnoreInvite, this), 2, 0);
}

void CMissionInviteNewsWindow::OnCommand(Event& event)
{
    if ((event.m_id != AcceptInvite && event.m_id != IgnoreInvite)
        || event.m_param1 != reinterpret_cast<int>(this)) {
        return;
    }
    CNGSLocalUser* local = CNGS::GetInstance()->GetLocalUser();
    if (event.m_id == AcceptInvite) {
        CNGSRemoteUser* sender = local->m_remoteUserLists[0]->findRemoteUserByID(m_senderId);
        if (sender == NULL) {
            event.Clear();
            return;
        }
        XString missionId(m_missionId);
        CObjectMapString response(CStrWChar(missionId.c_str()));
        sender->SendMessage(&response, InviteResponseDurationSeconds, MissionInviteAccepted, false, NULL);
    }

    CNGSFromServerMessageQ* queue = local->m_messageQueue;
    TCVector<long long> ids;
    queue->GetAllMessagesByUserAndType(ids, m_senderId, MissionInvite, CNGSFromServerMessage::CNGS_MSG_SUBTYPE_ANY);
    for (int index = 0; index < ids.Size(); ++index) {
        CNGSFromServerMessage* pending = queue->GetMessageById(ids[index]);
        if (pending != NULL) {
            CStrWChar payload = pending->GetMessageText();
            XString text(payload.GetWCharPtr());
            if (text.SubString(1, text.Length() - 2).ToInt() == m_missionId) {
                local->m_notificationHandler->RemoveGameSpecificMessage(static_cast<int>(pending->m_messageId));
            }
        }
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_newsFeedManager->RemoveMessage(m_message);
    Close();
    event.Clear();
}
