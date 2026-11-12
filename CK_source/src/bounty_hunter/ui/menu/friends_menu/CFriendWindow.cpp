#include "CFriendWindow.h"

#include "BaseDialog.h"
#include "CBHFriend.h"
#include "CBH_GPSMap.h"
#include "CFontMgr.h"
#include "CFriendAvatarWindow.h"
#include "CNGS.h"
#include "CNGSLocalUser.h"
#include "CNGSRemoteUserList.h"
#include "ServiceWindows.h"
#include "WindowApp.h"

namespace {
const int TargetFriend = -0x11ac059d;
const int CloseFriends = 0x62b2a2ce;

CBHFriend* FindFriend(int friendId)
{
    CNGSLocalUser* local = CNGS::GetInstance()->GetLocalUser();
    return static_cast<CBHFriend*>(local->m_remoteUserLists[0]->findRemoteUserByID(friendId));
}
}

CFriendWindow::CFriendWindow(int friendId)
    : Window(false), m_friendId(friendId), m_friend(FindFriend(friendId)), m_populated(false),
      m_level(NULL), m_contracts(NULL)
{
    if (m_friend == NULL) {
        return;
    }
    Window* row = new Window(false);
    row->SetHeightByContent(0, 0);
    AddToFront(row);

    bool facebook = m_friend->IsFromSocialNetwork(SOCIAL_NETWORK_FACEBOOK) != 0;
    bool gameCenter = m_friend->IsFromSocialNetwork(1) != 0;
    Window* icon = CreateFriendIcon(m_friendId, facebook, gameCenter);
    icon->SetPercentWidth(30, 0, 0);
    icon->SetHeightByContent(0, 0);
    row->AddToFront(icon, 0, 0);

    CStrWChar nickname = m_friend->GetNickName();
    const CPublicData1& profile = m_friend->m_publicData;
    Window* info = CreateFriendInfo(nickname.GetWCharPtr(), profile.m_level, profile.m_successBounties);
    info->SetHeightByContent(0, 0);
    row->AddToFront(info, 1, 0);
    StretchImageButton* target = new StretchImageButton(XString("Target"),
        "IDB_HUD_TRANSPARENT_BLACK_LEFT", "IDB_HUD_TRANSPARENT_BLACK_TILE", "IDB_HUD_TRANSPARENT_BLACK_RIGHT");
    target->SetLayoutType(LAYOUT_SIMPLE);
    target->SetAlign(ALIGN_RIGHT | ALIGN_BOTTOM);
    target->m_command = TargetFriend;
    target->m_commandParameter1 = m_friendId;
    row->AddToFront(target);
    m_populated = m_friend->isPopulated() != 0;
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
    CFont* font = CFontMgr::GetInstance()->GetFont(15);
    Window* info = new Window(false);
    Window* nameRow = new Window(false);
    TextWindow* label = new TextWindow(name, font);
    label->SetPercentHeight(100, 0, 0);
    label->SetAlign(ALIGN_LEFT | ALIGN_VCENTER);
    nameRow->AddToFront(label);
    info->AddToFront(nameRow, 0, 0);

    Window* levelRow = new Window(false);
    m_level = new TextWindow(XString::Format(L"Level %u", level), font);
    levelRow->AddToFront(m_level);
    info->AddToFront(levelRow, 0, 1);

    Window* contractRow = new Window(false);
    m_contracts = new TextWindow(XString::Format(L"Contracts %u", contracts), font);
    contractRow->AddToFront(m_contracts);
    info->AddToFront(contractRow, 0, 2);
    return info;
}

void CFriendWindow::Update()
{
    if (m_populated || m_level == NULL) {
        return;
    }
    m_friend = FindFriend(m_friendId);
    if (m_friend == NULL) {
        return;
    }
    if (!m_friend->isPopulated()) {
        if (m_friend->isAvailable()) {
            m_friend->deepPopulate(true);
        }
        return;
    }
    const CPublicData1& profile = m_friend->m_publicData;
    m_level->SetText(XString::Format(L"Level %u", profile.m_level));
    m_contracts->SetText(XString::Format(L"Contracts %u", profile.m_successBounties));
    m_populated = true;
}

void CFriendWindow::OnCommand(Event& event)
{
    if (event.m_id != TargetFriend || event.m_param1 != m_friendId) {
        return;
    }
    CBH_GPSMap* map = CBH_GPSMap::m_instance;
    if (map != NULL && map->StartTargettingMission(m_friendId, false)) {
        WindowApp::HandleTunnelCommand(CloseFriends, 0, 0, 0);
    }
    event.Clear();
}
