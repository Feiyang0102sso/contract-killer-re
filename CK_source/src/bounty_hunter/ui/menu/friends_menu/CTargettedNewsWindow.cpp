#include "CTargettedNewsWindow.h"

#include "App.h"
#include "BaseDialog.h"
#include "CBH_GPSMap.h"
#include "CFontMgr.h"
#include "CNGS.h"
#include "CNGSFromServerMessageQ.h"
#include "CNGSLocalUser.h"
#include "CNGSRemoteUser.h"
#include "CNGSRemoteUserList.h"
#include "CNewsFeedManager.h"
#include "CNotificationHandler.h"
#include "ICSystemFont.h"
#include "SScreenAdaptingConstant.h"
#include "ServiceWindows.h"

namespace {
const int Revenge = 0x0f207fba;
const int Forgive = 0x0e6a3ddb;
const long long TargetMessage = 1001;
const long long RevengeMessage = 1004;
}

CTargettedNewsWindow::CTargettedNewsWindow(const STargettedMessage* message)
    : Window(false), m_revengeAvailable(false), m_sender(NULL)
{
    long long type = RevengeMessage;
    if (message->m_highlighted) {
        type = TargetMessage;
    }
    CNGSRemoteUserList* friends = CNGS::GetInstance()->GetLocalUser()->m_remoteUserLists[0];
    for (int index = 0; index < friends->m_users.Size(); ++index) {
        if (friends->m_users[index]->GetClientID() == message->m_senderId) {
            CNGSFromServerMessageQ* queue = CNGS::GetInstance()->GetLocalUser()->m_messageQueue;
            TCVector<long long> ids;
            queue->GetAllMessagesByUserAndType(ids, 0, 0, CNGSFromServerMessage::CNGS_MSG_SUBTYPE_ANY);
            for (int messageIndex = 0; messageIndex < ids.Size(); ++messageIndex) {
                CNGSFromServerMessage* pending = queue->GetMessageById(ids[messageIndex]);
                if (pending != NULL && pending->m_type == type && pending->m_senderId == message->m_senderId) {
                    bool textMatches = pending->GetMessageText() == message->m_text;
                    if (textMatches) {
                        m_messageId = static_cast<int>(pending->m_messageId);
                    }
                }
            }
            m_senderId = message->m_senderId;
            m_message = message;
            m_sender = CNGS::GetInstance()->GetLocalUser()->m_remoteUserLists[0]->findRemoteUserByID(m_senderId);
            CreateUserInterface();
        }
    }
}

void CTargettedNewsWindow::CreateUserInterface()
{
    SetHeightByContent(0, 0);
    XString nickname;
    if (m_sender != NULL) {
        nickname = m_sender->GetNickName().GetWCharPtr();
    }
    XString text;
    if (m_message->m_highlighted) {
        text = XString::Format(ResString("IDS_FRIENDS_YOU_KILLED_BY_FRIEND").c_str(), nickname.c_str());
    } else {
        text = XString::Format(ResString("IDS_FRIENDS_YOU_KILLED_BY_FRIEND_AVENGED").c_str(), nickname.c_str());
    }
    TextWindow* label = new TextWindow(text, CFontMgr::GetInstance()->GetFont(15));
    label->SetAlign(ALIGN_LEFT | ALIGN_VCENTER);
    ICSystemFont* font = ICSystemFont::CreateInstance();
    font->Initialize(SScreenAdaptingConstant(12, 25)(), NULL, 1);
    label->SetFont(font);
    AddToFront(label, 0, 0);
    label->SetPercentWidth(60, 0, 0);
    if (m_message->m_highlighted) {
        Window* actions = new Window(false);
        StretchImageButton* revenge = new StretchImageButton(ResString("IDS_REVENGE"),
            "IDB_HUD_TRANSPARENT_BLACK_LEFT", "IDB_HUD_TRANSPARENT_BLACK_TILE", "IDB_HUD_TRANSPARENT_BLACK_RIGHT");
        revenge->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
        revenge->SetCommandParameters(Revenge, 0, 0, 0);
        revenge->ForceSetWidth(SScreenAdaptingConstant(90, 180)());
        actions->AddToFront(revenge, 0, 0);
        StretchImageButton* forgive = new StretchImageButton(ResString("IDS_FORGIVE"),
            "IDB_HUD_TRANSPARENT_BLACK_LEFT", "IDB_HUD_TRANSPARENT_BLACK_TILE", "IDB_HUD_TRANSPARENT_BLACK_RIGHT");
        forgive->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
        forgive->SetCommandParameters(Forgive, 0, 0, 0);
        forgive->ForceSetWidth(SScreenAdaptingConstant(90, 180)());
        actions->AddToFront(forgive, 1, 0);
        AddToFront(actions, 1, 0);
        actions->SetHeightByContent(0, 0);
        actions->SetPercentWidth(40, 0, 0);
    }
    SeparateLineRed* line = new SeparateLineRed();
    line->SetOutsetSpacing(SScreenAdaptingConstant(5, 10)(), 0, 0, 0);
    line->SetDesiredHeight(SScreenAdaptingConstant(5, 10)());
    line->SetCellPos(0, 1, 2, 1);
    AddToFront(line);
}

void CTargettedNewsWindow::OnCommand(Event& event)
{
    if (event.m_id == Revenge) {
        HandleRevengePressed();
        event.Clear();
    } else if (event.m_id == Forgive) {
        static_cast<App*>(WindowApp::m_instance)->m_newsFeedManager->RemoveMessage(m_message);
        CNGSLocalUser* local = CNGS::GetInstance()->GetLocalUser();
        local->m_notificationHandler->RemoveGameSpecificMessage(m_messageId);
        Close();
        event.Clear();
    }
}

void CTargettedNewsWindow::HandleRevengePressed()
{
    if (CBH_GPSMap::m_instance->StartTargettingMission(m_senderId, true) == true) {
        static_cast<App*>(WindowApp::m_instance)->m_newsFeedManager->RemoveMessage(m_message);
        CNGSLocalUser* local = CNGS::GetInstance()->GetLocalUser();
        local->m_notificationHandler->RemoveGameSpecificMessage(m_messageId);
        Close();
    }
}
