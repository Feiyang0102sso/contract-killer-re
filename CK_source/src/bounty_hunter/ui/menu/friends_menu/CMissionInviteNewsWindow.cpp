#include "CMissionInviteNewsWindow.h"

#include "App.h"
#include "BaseDialog.h"
#include "CFontMgr.h"
#include "CNGS.h"
#include "CNGSLocalUser.h"
#include "CNGSRemoteUser.h"
#include "CNGSRemoteUserList.h"
#include "CNewsFeedManager.h"
#include "CObjectMapValue.h"
#include "ICStdUtil.h"
#include "ICSystemFont.h"
#include "SScreenAdaptingConstant.h"
#include "ServiceWindows.h"

namespace {
const int AcceptInvite = 0x7d371742;
const int IgnoreInvite = 0x0d611fcc;
const long long MissionInviteAccepted = 1003;
const unsigned int InviteResponseDurationSeconds = 604800;
}

CMissionInviteNewsWindow::CMissionInviteNewsWindow(const SMissionInviteMessage* message)
    : Window(false), m_sender(NULL)
{
    m_senderId = message->m_senderId;
    m_missionId = message->m_missionId;
    m_message = message;
    CNGSLocalUser* local = CNGS::GetInstance()->GetLocalUser();
    m_sender = local->m_remoteUserLists[0]->findRemoteUserByID(m_senderId);
    CreateUserInterface();
}

void CMissionInviteNewsWindow::CreateUserInterface()
{
    SetHeightByContent(0, 0);
    XString nickname;
    if (m_sender != NULL) {
        nickname = m_sender->GetNickName().GetWCharPtr();
    }
    XString text = XString::Format(ResString("IDS_FRIENDS_INVITE_TO_MISSION").c_str(), nickname.c_str());
    TextWindow* label = new TextWindow(text, CFontMgr::GetInstance()->GetFont(15));
    label->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    ICSystemFont* font = ICSystemFont::CreateInstance();
    font->Initialize(SScreenAdaptingConstant(12, 25)(), NULL, 1);
    label->SetFont(font);
    AddToFront(label, 0, 0);

    StretchImageButton* accept = new StretchImageButton(ResString("IDS_DIALOG_BUTTON_ACCEPT"),
        "IDB_HUD_TRANSPARENT_BLACK_LEFT", "IDB_HUD_TRANSPARENT_BLACK_TILE", "IDB_HUD_TRANSPARENT_BLACK_RIGHT");
    accept->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    accept->SetCommandParameters(AcceptInvite, 0, 0, 0);
    AddToFront(accept, 1, 0);

    StretchImageButton* ignore = new StretchImageButton(ResString("IDS_DIALOG_BUTTON_IGNORE"),
        "IDB_HUD_TRANSPARENT_BLACK_LEFT", "IDB_HUD_TRANSPARENT_BLACK_TILE", "IDB_HUD_TRANSPARENT_BLACK_RIGHT");
    ignore->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    ignore->SetCommandParameters(IgnoreInvite, 0, 0, 0);
    AddToFront(ignore, 2, 0);
}

void CMissionInviteNewsWindow::OnCommand(Event& event)
{
    switch (event.m_id) {
    case IgnoreInvite:
        static_cast<App*>(WindowApp::m_instance)->m_newsFeedManager->RemoveMessage(m_message);
        Close();
        break;
    case AcceptInvite: {
        CNGSRemoteUserList* friends = CNGS::GetInstance()->GetLocalUser()->m_remoteUserLists[0];
        for (int index = 0; index < friends->m_users.Size(); ++index) {
            CNGSRemoteUser* user = friends->m_users[index];
            if (user->GetClientID() == m_senderId) {
                char missionId[12] = {0};
                ICStdUtil::SPrintF(missionId, "%i", m_missionId);
                CObjectMapString response((CStrWChar(missionId)));
                user->SendMessage(&response, InviteResponseDurationSeconds, MissionInviteAccepted, false, NULL);
            }
        }
        static_cast<App*>(WindowApp::m_instance)->m_newsFeedManager->RemoveMessage(m_message);
        Close();
        break;
    }
    }
}
