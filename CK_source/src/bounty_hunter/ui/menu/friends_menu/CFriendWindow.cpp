#include "CFriendWindow.h"

#include "BaseDialog.h"
#include "CBHFriend.h"
#include "CBH_GPSMap.h"
#include "CFontMgr.h"
#include "CFriendAvatarWindow.h"
#include "ICSystemFont.h"
#include "CNGS.h"
#include "CNGSLocalUser.h"
#include "CNGSRemoteUserList.h"
#include "ServiceWindows.h"
#include "SScreenAdaptingConstant.h"
#include "WindowApp.h"

namespace {
const int TargetFriend = -0x11ac059d;
const int CloseFriends = 0x62b2a2ce;

}

CFriendWindow::CFriendWindow(int friendId) : Window(false), m_friendId(0)
{
    Window* row = new Window(false);
    row->SetHeightByContent(0, 0);
    AddToFront(row);

    CNGSLocalUser* local = CNGS::GetInstance()->GetLocalUser();
    m_friend = static_cast<CBHFriend*>(local->m_remoteUserLists[0]->findRemoteUserByID(friendId));
    if (m_friend != NULL) {
        m_friendId = m_friend->GetClientID();
        m_friend->IsFromSocialNetwork(SOCIAL_NETWORK_FACEBOOK);
        m_friend->IsFromSocialNetwork(1);
        Window* icon = CreateFriendIcon(m_friendId, false, false);
        row->AddToFront(icon, 0, 0);
        icon->SetPercentWidth(30, 0, 0);
        icon->SetHeightByContent(0, 0);

        XString nickname("win32 user");
        nickname = m_friend->GetNickName().GetWCharPtr();
        const CPublicData1& profile = m_friend->m_publicData;
        Window* info = CreateFriendInfo(nickname, profile.m_level, profile.m_successBounties);
        row->AddToFront(info, 1, 0);
        info->SetHeightByContent(0, 0);

        StretchImageButton* target = new StretchImageButton(XString("Target"),
            "IDB_HUD_TRANSPARENT_BLACK_LEFT", "IDB_HUD_TRANSPARENT_BLACK_TILE", "IDB_HUD_TRANSPARENT_BLACK_RIGHT");
        target->SetLayoutType(LAYOUT_SIMPLE);
        target->SetAlign(ALIGN_RIGHT | ALIGN_BOTTOM);
        target->SetCommandParameters(TargetFriend, m_friendId, 0, 0);
        row->AddToFront(target);
    } else {
        // A missing friend leaves an empty row with no profile to refresh.
        m_populated = true;
    }
}

Window* CFriendWindow::CreateFriendIcon(int friendId, bool, bool)
{
    Window* icon = new Window(false);
    CFriendAvatarWindow* avatar = new CFriendAvatarWindow(friendId);
    avatar->ShowBorder();
    avatar->SetLayoutType(LAYOUT_SIMPLE);
    avatar->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    avatar->SetDefaultImage("IDB_AVATAR_10");
    icon->AddToFront(avatar);
    return icon;
}

Window* CFriendWindow::CreateFriendInfo(const XString& name, int level, int contracts)
{
    Window* info = new Window(false);
    Window* nameRow = new Window(false);
    TextWindow* label = new TextWindow(name, CFontMgr::GetInstance()->GetFont(15));
    ICSystemFont* font = ICSystemFont::CreateInstance();
    font->Initialize(SScreenAdaptingConstant(15, 30)(), NULL, 1);
    label->SetFont(font);
    label->SetPercentHeight(100, 0, 0);
    nameRow->AddToFront(label);
    info->AddToFront(nameRow, 0, 0);

    Window* levelRow = new Window(false);
    m_level = new TextWindow(XString::Format(L"Level %u", level), CFontMgr::GetInstance()->GetFont(15));
    levelRow->AddToFront(m_level);
    info->AddToFront(levelRow, 0, 1);

    Window* contractRow = new Window(false);
    m_contracts = new TextWindow(XString::Format(L"Contracts %u", contracts), CFontMgr::GetInstance()->GetFont(15));
    contractRow->AddToFront(m_contracts);
    info->AddToFront(contractRow, 0, 2);
    m_populated = false;
    return info;
}

void CFriendWindow::Update()
{
    if (m_populated) {
        return;
    }
    if (m_friend->isPopulated()) {
        m_populated = true;
        m_level->SetText(XString::Format(L"Level %u", m_friend->m_publicData.m_level),
                         CFontMgr::GetInstance()->GetFont(15));
        m_contracts->SetText(XString::Format(L"Contracts %u", m_friend->m_publicData.m_successBounties),
                             CFontMgr::GetInstance()->GetFont(15));
    } else if (m_friend->isAvailable()) {
        m_friend->deepPopulate(false);
    }
}

void CFriendWindow::OnCommand(Event& event)
{
    if (event.m_id != TargetFriend) {
        return;
    }
    // the original targets the friend from the event and always closes the friends list
    CBH_GPSMap::m_instance->StartTargettingMission(event.m_param1, false);
    WindowApp::HandleTunnelCommand(CloseFriends, 0, 0, 0);
    event.Clear();
}
