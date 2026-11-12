#include "CNewsFeedWindow.h"

#include "App.h"
#include "BaseDialog.h"
#include "CBH_Player.h"
#include "CCareerWindow.h"
#include "CChallenge.h"
#include "CChallengeInviteNewsWindow.h"
#include "CChallengeManager.h"
#include "CDialogWindow.h"
#include "CFontMgr.h"
#include "CGServeHelper.h"
#include "CGiftRecievedNewsWindow.h"
#include "CGuildNewsWindow.h"
#include "CMissionInviteNewsWindow.h"
#include "CNewsFeedManager.h"
#include "CShopItem.h"
#include "CSocialGiftsManager.h"
#include "CTargettedNewsWindow.h"
#include "SScreenAdaptingConstant.h"
#include "ServiceWindows.h"

namespace {
const SScreenAdaptingConstant s_topMargin(15, 30);
const SScreenAdaptingConstant s_rowMargin(5, 10);
const SScreenAdaptingConstant s_emptyWidth(350, 800);

void AddAwardAmount(XString& text, int amount, const char* label)
{
    if (amount > 0) {
        XString name = Window::ResString(label);
        text.Append(XString::Format(L"\n %d %ls", amount, name.c_str()));
    }
}
}

CNewsFeedWindow::CNewsFeedWindow()
    : Window(false)
{
    CreateUserInterface();
}

CNewsFeedWindow::~CNewsFeedWindow()
{
}

void CNewsFeedWindow::CreateUserInterface()
{
    Window* list = new Window(false);
    list->SetInsetSpacing(s_topMargin(), 0, 0, 0);
    list->SetLayoutType(LAYOUT_SIMPLE);
    list->SetPercentHeight(100, 0, 0);
    m_rows = new Window(false);
    m_rows->SetHeightByContent(0, 0);
    m_rows->SetLayoutType(LAYOUT_NONE);
    m_nextRow = 0;
    m_column = 0;

    ScrollingContainer* scroll = new ScrollingContainer();
    scroll->SetLayoutType(LAYOUT_NONE);
    scroll->SetPercentHeight(100, 0, 0);
    scroll->SetPercentWidth(100, 0, 0);
    scroll->AddToFront(m_rows);
    list->AddToFront(scroll, 0, 0);
    VertScrollerSimpleRed* scroller = new VertScrollerSimpleRed(scroll);
    scroller->SetOutsetSpacing(3, 5, 3, 4);
    list->AddToFront(scroller, 1, 0);
    AddToFront(list);
}

int CNewsFeedWindow::Populate()
{
    ClearNews();
    App* app = static_cast<App*>(WindowApp::m_instance);
    CNewsFeedManager* feed = app->m_newsFeedManager;
    int count = feed->GetNewsCount();
    for (int index = count - 1; index >= 0; --index) {
        const SMessage* message = feed->GetNewsAt(index);
        int before = m_nextRow;
        switch (message->m_type) {
        case NEWS_TARGETTED:
            Targetted(static_cast<const STargettedMessage*>(message));
            break;
        case NEWS_MISSION_INVITE:
            MissionInvite(static_cast<const SMissionInviteMessage*>(message));
            break;
        case NEWS_FRIEND_JOINED:
            FriendInstall(static_cast<const SFriendJoinedGameMessage*>(message));
            break;
        case NEWS_FRIEND_INVITED:
            AddText(static_cast<const STargettedMessage*>(message)->m_text.GetWCharPtr(), ALIGN_LEFT | ALIGN_VCENTER);
            break;
        case NEWS_GIFT:
            ReceiveGift(static_cast<const SGiftReceivedMessage*>(message));
            break;
        case NEWS_AWARD:
            ReceiveAward(static_cast<const SServerAwardMessage*>(message));
            break;
        case NEWS_GUILD_INVITE:
            ShowGuildInvite(static_cast<const STargettedMessage*>(message));
            break;
        case NEWS_GUILD_ACCEPTED:
        case NEWS_GUILD_DECLINED:
            ShowGuildInviteResponse(message, message->m_type == NEWS_GUILD_ACCEPTED);
            break;
        case NEWS_CHALLENGE_INVITE:
            ShowChallengeInvite(static_cast<const SMissionInviteMessage*>(message));
            break;
        case NEWS_CHALLENGE_ACCEPTED:
        case NEWS_CHALLENGE_DECLINED:
            ShowChallengeInviteResponse(static_cast<const SMissionInviteMessage*>(message),
                                        message->m_type == NEWS_CHALLENGE_ACCEPTED);
            break;
        default:
            break;
        }
        if (m_nextRow != before) {
            message->m_read = true;
        }
    }
    if (m_nextRow == 0) {
        ShowEmptyNews();
    }
    return count;
}

void CNewsFeedWindow::AddRow(Window* row)
{
    row->SetOutsetSpacing(0, 0, 0, s_rowMargin());
    m_rows->AddToFront(row, m_column, m_nextRow++);
}

void CNewsFeedWindow::AddText(const XString& text, int align)
{
    if (text.Length() == 0) {
        return;
    }
    TextWindow* label = new TextWindow(text, CFontMgr::GetInstance()->GetFont(15));
    label->SetHeightByContent(0, 0);
    label->SetAlign(align);
    AddRow(label);
    AddSeparator();
}

void CNewsFeedWindow::AddSeparator()
{
    SeparateLineRed* line = new SeparateLineRed();
    line->SetDesiredHeight(s_rowMargin());
    AddRow(line);
}

void CNewsFeedWindow::ReceiveGift(const SGiftReceivedMessage* message)
{
    AddRow(new CGiftRecievedNewsWindow(message));
}

void CNewsFeedWindow::ReceiveAward(const SServerAwardMessage* message)
{
    if (message->m_read
        || (message->m_rareCurrency <= 0 && message->m_commonCurrency <= 0
            && message->m_experience <= 0 && message->m_itemIndex < 0)) {
        return;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    SGift* gift = app->m_socialGiftManager->GetGift(message->m_itemIndex);
    CItem* item = NULL;
    if (gift != NULL) {
        item = gift->GetItem();
    }
    bool ownsWeapon = item != NULL && item->getType() == 0 && item->isAvailable();
    XString text = message->m_text.GetWCharPtr();
    if (text.Length() == 0) {
        XString contents;
        AddAwardAmount(contents, message->m_rareCurrency, "IDS_AWARD_CREDITS");
        AddAwardAmount(contents, message->m_commonCurrency, "IDS_AWARD_MONEY");
        AddAwardAmount(contents, message->m_experience, "IDS_AWARD_XP");
        if (ownsWeapon) {
            contents.Append(ResString("IDS_GIFT_WEAPON_ALREADY_OWNED"));
        } else if (item != NULL) {
            contents.Append(XString::Format(L"\n %ls", item->GetName().c_str()));
        }
        text = XString::Format(L"%ls %ls", ResString("IDS_AWARD_FROM_SERVER").c_str(), contents.c_str());
    } else if (ownsWeapon) {
        text = ResString("IDS_GIFT_WEAPON_ALREADY_OWNED");
    }
    CServerGiftDialog* award = new CServerGiftDialog(message);
    award->SetTitleText("IDS_AWARD_FROM_SERVER_TITLE");
    award->SetMessageText(text);
    award->SetButtons(CDialogWindow::BUTTON_OK);
    message->m_read = true;
    app->m_mainLayer->AddModal(award);
}

void CNewsFeedWindow::FriendInstall(const SFriendJoinedGameMessage* message)
{
    XString format = ResString("IDS_FRIEND_JOINED_GAME_NEWSFEED");
    AddText(XString::Format(format.c_str(), message->m_text.GetWCharPtr()), ALIGN_LEFT | ALIGN_VCENTER);
}

void CNewsFeedWindow::Targetted(const STargettedMessage* message)
{
    CTargettedNewsWindow* row = new CTargettedNewsWindow(message);
    row->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    AddRow(row);
}

void CNewsFeedWindow::ShowGuildInvite(const STargettedMessage* message)
{
    CGuildInviteNewsWindow* row = new CGuildInviteNewsWindow(message);
    row->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    AddRow(row);
}

void CNewsFeedWindow::ShowGuildInviteResponse(const SMessage* message, bool accepted)
{
    XString nickname = CGServeHelper::GetFriendNickname(message->m_senderId);
    const char* caption = "IDS_INVITATION_DECLINED";
    if (accepted) {
        caption = "IDS_INVITATION_ACCEPTED";
    } else {
        CBH_Player::GetInstance()->CancelFriendInvitation(message->m_senderId);
    }
    XString format = ResString(caption);
    AddText(XString::Format(format.c_str(), nickname.c_str()), ALIGN_HCENTER | ALIGN_VCENTER);
}

void CNewsFeedWindow::ShowChallengeInvite(const SMissionInviteMessage* message)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (app->m_challengeManager->IsChallengeActive(message->m_missionId)
        && CGServeHelper::GetFriendNickname(message->m_senderId).Length() != 0) {
        CChallengeInviteNewsWindow* row = new CChallengeInviteNewsWindow(message);
        row->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
        AddRow(row);
    }
}

void CNewsFeedWindow::ShowChallengeInviteResponse(const SMissionInviteMessage* message, bool accepted)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CChallenge* challenge = app->m_challengeManager->GetChallengeById(message->m_missionId);
    if (challenge == NULL || !challenge->m_active) {
        return;
    }
    if (!message->m_read) {
        challenge->FriendInvited(message->m_senderId);
    }
    const char* resource = "IDS_CHALLENGE_INVITE_DECLINED";
    if (accepted) {
        resource = "IDS_CHALLENGE_INVITE_ACCEPTED";
    }
    XString format = ResString(resource);
    XString sender = CGServeHelper::GetFriendNickname(message->m_senderId);
    AddText(XString::Format(format.c_str(), sender.c_str(), challenge->m_name.c_str()), ALIGN_LEFT | ALIGN_VCENTER);
}

void CNewsFeedWindow::MissionInvite(const SMissionInviteMessage* message)
{
    AddRow(new CMissionInviteNewsWindow(message));
    AddSeparator();
}

void CNewsFeedWindow::ShowEmptyNews()
{
    TextWindow* label = new TextWindow(ResString("IDS_NEWSFEED_NO_NEWS"), CFontMgr::GetInstance()->GetFont(15));
    label->SetDesiredWidth(s_emptyWidth());
    label->SetHeightByContent(0, 0);
    label->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    AddRow(label);
    AddSeparator();
}

void CNewsFeedWindow::ClearNews()
{
    m_rows->DeleteChilds();
    m_nextRow = 0;
}
