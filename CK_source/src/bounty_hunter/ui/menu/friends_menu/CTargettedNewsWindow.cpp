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
#include "SScreenAdaptingConstant.h"
#include "ServiceWindows.h"

namespace {
const int Revenge = 0x0f207fba;
const int Forgive = 0x0e6a3ddb;
const long long TargetMessage = 1001;
const long long RevengeMessage = 1004;
const SScreenAdaptingConstant s_buttonWidth(90, 180);
const SScreenAdaptingConstant s_lineHeight(5, 10);

StretchImageButton* CreateButton(const char* caption, int command, Window* owner)
{
    StretchImageButton* button = new StretchImageButton(Window::ResString(caption),
        "IDB_HUD_TRANSPARENT_BLACK_LEFT", "IDB_HUD_TRANSPARENT_BLACK_TILE", "IDB_HUD_TRANSPARENT_BLACK_RIGHT");
    button->SetAlign(Window::ALIGN_HCENTER | Window::ALIGN_VCENTER);
    button->m_command = command;
    button->m_commandParameter1 = reinterpret_cast<int>(owner);
    button->ForceSetWidth(s_buttonWidth());
    return button;
}
}

CTargettedNewsWindow::CTargettedNewsWindow(const STargettedMessage* message)
    : Window(false), m_senderId(message->m_senderId), m_revengeAvailable(false),
      m_sender(NULL), m_message(message), m_messageId(-1)
{
    CNGSLocalUser* local = CNGS::GetInstance()->GetLocalUser();
    m_sender = local->m_remoteUserLists[0]->findRemoteUserByID(m_senderId);
    if (m_sender != NULL) {
        m_revengeAvailable = message->m_highlighted;
        long long type = RevengeMessage;
        if (m_revengeAvailable) {
            type = TargetMessage;
        }
        CNGSFromServerMessageQ* queue = local->m_messageQueue;
        TCVector<long long> ids;
        queue->GetAllMessagesByUserAndType(ids, m_senderId, type, CNGSFromServerMessage::CNGS_MSG_SUBTYPE_ANY);
        for (int index = 0; index < ids.Size(); ++index) {
            CNGSFromServerMessage* pending = queue->GetMessageById(ids[index]);
            if (pending != NULL && pending->GetMessageText() == message->m_text) {
                m_messageId = static_cast<int>(pending->m_messageId);
            }
        }
    }
    CreateUserInterface();
}

void CTargettedNewsWindow::CreateUserInterface()
{
    SetHeightByContent(0, 0);
    XString text = m_message->m_text.GetWCharPtr();
    if (m_sender != NULL) {
        const char* caption = "IDS_FRIENDS_YOU_KILLED_BY_FRIEND_AVENGED";
        if (m_revengeAvailable) {
            caption = "IDS_FRIENDS_YOU_KILLED_BY_FRIEND";
        }
        XString format = ResString(caption);
        CStrWChar nickname = m_sender->GetNickName();
        text = XString::Format(format.c_str(), nickname.GetWCharPtr());
    }
    TextWindow* label = new TextWindow(text, CFontMgr::GetInstance()->GetFont(15));
    label->SetAlign(ALIGN_LEFT | ALIGN_VCENTER);
    label->SetHeightByContent(0, 0);
    AddToFront(label, 0, 0);
    if (m_revengeAvailable) {
        label->SetPercentWidth(60, 0, 0);
        Window* actions = new Window(false);
        actions->SetHeightByContent(0, 0);
        actions->SetPercentWidth(40, 0, 0);
        actions->AddToFront(CreateButton("IDS_REVENGE", Revenge, this), 0, 0);
        actions->AddToFront(CreateButton("IDS_FORGIVE", Forgive, this), 1, 0);
        AddToFront(actions, 1, 0);
    }
    SeparateLineRed* line = new SeparateLineRed();
    line->SetOutsetSpacing(s_lineHeight(), 0, 0, 0);
    line->SetDesiredHeight(s_lineHeight());
    line->SetCellPos(0, 1, 2, 1);
    AddToFront(line);
}

void CTargettedNewsWindow::OnCommand(Event& event)
{
    if (event.m_param1 != reinterpret_cast<int>(this)) {
        return;
    }
    if (event.m_id == Forgive) {
        DismissMessage();
    } else if (event.m_id == Revenge) {
        HandleRevengePressed();
    } else {
        return;
    }
    event.Clear();
}

void CTargettedNewsWindow::HandleRevengePressed()
{
    CBH_GPSMap* map = CBH_GPSMap::m_instance;
    if (map != NULL && map->StartTargettingMission(m_senderId, true)) {
        DismissMessage();
    }
}

void CTargettedNewsWindow::DismissMessage()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_newsFeedManager->RemoveMessage(m_message);
    CNGSLocalUser* local = CNGS::GetInstance()->GetLocalUser();
    local->m_notificationHandler->RemoveGameSpecificMessage(m_messageId);
    Close();
}
