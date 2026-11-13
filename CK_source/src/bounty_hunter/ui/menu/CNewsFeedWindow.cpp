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
#include "CGiftRecievedNewsWindow.h"
#include "CGuildNewsWindow.h"
#include "CMissionInviteNewsWindow.h"
#include "CNewsFeedManager.h"
#include "CNGS.h"
#include "CNGSLocalUser.h"
#include "CNGSRemoteUser.h"
#include "CNGSRemoteUserList.h"
#include "CShopItem.h"
#include "CSocialGiftsManager.h"
#include "CTargettedNewsWindow.h"
#include "ICSystemFont.h"
#include "SScreenAdaptingConstant.h"
#include "ServiceWindows.h"

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
    list->SetInsetSpacing(SScreenAdaptingConstant(15, 30)(), 0, 0, 0);
    list->SetLayoutType(LAYOUT_NONE);
    list->SetPercentHeight(100, 0, 0);
    Window* rows = new Window(false);
    m_rows = rows;
    rows->SetHeightByContent(0, 0);
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

void CNewsFeedWindow::ReceiveGift(const SGiftReceivedMessage* message)
{
    CGiftRecievedNewsWindow* row = new CGiftRecievedNewsWindow(message);
    row->SetOutsetSpacing(0, 0, 0, SScreenAdaptingConstant(5, 10)());
    m_rows->AddToFront(row, m_column, m_nextRow++);
}

void CNewsFeedWindow::ReceiveAward(const SServerAwardMessage* message)
{
    if (message->m_rareCurrency <= 0 && message->m_commonCurrency <= 0
        && message->m_experience <= 0 && message->m_itemIndex < 0) {
        return;
    }

    CServerGiftDialog* award = new CServerGiftDialog(message);
    award->SetTitleText(ResString("IDS_AWARD_FROM_SERVER_TITLE"));
    const wchar_t* messageText = message->m_text.GetWCharPtr();
    if (messageText != NULL && message->m_text.GetLength() > 0) {
        if (message->m_itemIndex >= 0) {
            SGift* gift = static_cast<App*>(WindowApp::m_instance)->m_socialGiftManager->GetGift(message->m_itemIndex);
            if (gift != NULL) {
                CItem* item = gift->GetItem();
                if (item->getType() == CItem::ITEM_TYPE_FIRST && item->isAvailable() == true) {
                    award->SetMessageText(ResString("IDS_GIFT_WEAPON_ALREADY_OWNED"));
                } else {
                    award->SetMessageText(XString(message->m_text.GetWCharPtr()));
                }
            }
        } else {
            award->SetMessageText(XString(messageText));
        }
    } else {
        XString contents;
        int amountCredits = message->m_rareCurrency;
        if (amountCredits > 0) {
            contents.Append(XString::Format(L"\n %d %s", amountCredits, ResString("IDS_AWARD_CREDITS").c_str()));
        }
        int amountMoney = message->m_commonCurrency;
        if (amountMoney > 0) {
            contents.Append(XString::Format(L"\n %d %s", amountMoney, ResString("IDS_AWARD_MONEY").c_str()));
        }
        int amountXp = message->m_experience;
        if (amountXp > 0) {
            contents.Append(XString::Format(L"\n %d %s", amountXp, ResString("IDS_AWARD_XP").c_str()));
        }
        if (message->m_itemIndex >= 0) {
            SGift* gift = static_cast<App*>(WindowApp::m_instance)->m_socialGiftManager->GetGift(message->m_itemIndex);
            if (gift != NULL) {
                CItem* item = gift->GetItem();
                if (item->getType() == CItem::ITEM_TYPE_FIRST && item->isAvailable() == true) {
                    contents.Append(ResString("IDS_GIFT_WEAPON_ALREADY_OWNED"));
                } else {
                    contents.Append(XString::Format(L"\n %s", item->GetName().c_str()));
                }
            }
        }
        award->SetMessageText(XString::Format(L"%s %s", ResString("IDS_AWARD_FROM_SERVER").c_str(), contents.c_str()));
    }
    award->SetButtons(CDialogWindow::BUTTON_OK);
    static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddModal(award);
}

void CNewsFeedWindow::FriendInstall(const SFriendJoinedGameMessage* message)
{
    XString text = XString::Format(ResString("IDS_FRIEND_JOINED_GAME_NEWSFEED").c_str(),
                                  message->m_text.GetWCharPtr());
    TextWindow* label = new TextWindow(text, CFontMgr::GetInstance()->GetFont(15));
    ICSystemFont* font = ICSystemFont::CreateInstance();
    font->Initialize(SScreenAdaptingConstant(12, 25)(), NULL, 1);
    label->SetFont(font);
    label->SetAlign(ALIGN_LEFT | ALIGN_VCENTER);
    label->SetOutsetSpacing(0, 0, 0, SScreenAdaptingConstant(5, 10)());
    m_rows->AddToFront(label, m_column, m_nextRow++);
    SeparateLineRed* line = new SeparateLineRed();
    line->SetDesiredHeight(SScreenAdaptingConstant(5, 10)());
    line->SetOutsetSpacing(0, 0, 0, SScreenAdaptingConstant(5, 10)());
    m_rows->AddToFront(line, m_column, m_nextRow++);
}

void CNewsFeedWindow::Targetted(const STargettedMessage* message)
{
    CTargettedNewsWindow* row = new CTargettedNewsWindow(message);
    row->m_debugFrame = true;
    row->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    row->SetOutsetSpacing(0, 0, 0, SScreenAdaptingConstant(5, 10)());
    m_rows->AddToFront(row, m_column, m_nextRow++);
}

void CNewsFeedWindow::ShowGuildInvite(const STargettedMessage* message)
{
    CGuildInviteNewsWindow* row = new CGuildInviteNewsWindow(message);
    row->m_debugFrame = true;
    row->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    row->SetOutsetSpacing(0, 0, 0, SScreenAdaptingConstant(5, 10)());
    m_rows->AddToFront(row, m_column, m_nextRow++);
}

void CNewsFeedWindow::ShowGuildInviteResponse(const SMessage* message, bool accepted)
{
    XString nickname;
    CNGSRemoteUser* user = CNGS::GetInstance()->GetLocalUser()->m_remoteUserLists[0]->findRemoteUserByID(message->m_senderId);
    if (user != NULL) {
        nickname = user->GetNickName().GetWCharPtr();
    }
    XString text;
    if (accepted) {
        text = XString::Format(ResString("IDS_INVITATION_ACCEPTED").c_str(), nickname.c_str());
    } else {
        text = XString::Format(ResString("IDS_INVITATION_DECLINED").c_str(), nickname.c_str());
    }
    TextWindow* label = new TextWindow(text, CFontMgr::GetInstance()->GetFont(15));
    ICSystemFont* font = ICSystemFont::CreateInstance();
    font->Initialize(SScreenAdaptingConstant(12, 25)(), NULL, 1);
    label->SetFont(font);
    label->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    m_rows->AddToFront(label, m_column, m_nextRow++);
    SeparateLineRed* line = new SeparateLineRed();
    line->SetDesiredHeight(SScreenAdaptingConstant(5, 10)());
    m_rows->AddToFront(line, m_column, m_nextRow++);
    if (!accepted) {
        CBH_Player::GetInstance()->CancelFriendInvitation(message->m_senderId);
    }
}

void CNewsFeedWindow::ShowChallengeInvite(const SMissionInviteMessage* message)
{
    if (static_cast<App*>(WindowApp::m_instance)->m_challengeManager->IsChallengeActive(message->m_missionId)) {
        CNGSRemoteUser* user = CNGS::GetInstance()->GetLocalUser()->m_remoteUserLists[0]->findRemoteUserByID(message->m_senderId);
        if (user != NULL) {
            CChallengeInviteNewsWindow* row = new CChallengeInviteNewsWindow(message);
            row->m_debugFrame = true;
            row->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
            row->SetOutsetSpacing(0, 0, 0, SScreenAdaptingConstant(5, 10)());
            m_rows->AddToFront(row, m_column, m_nextRow++);
        }
    }
}

void CNewsFeedWindow::ShowChallengeInviteResponse(const SMissionInviteMessage* message, bool accepted)
{
    if (static_cast<App*>(WindowApp::m_instance)->m_challengeManager->IsChallengeActive(message->m_missionId)) {
        CChallenge* challenge = static_cast<App*>(WindowApp::m_instance)->m_challengeManager->GetChallengeById(message->m_missionId);
        challenge->FriendInvited(message->m_senderId);

        XString nickname;
        CNGSRemoteUser* user = CNGS::GetInstance()->GetLocalUser()->m_remoteUserLists[0]->findRemoteUserByID(message->m_senderId);
        if (user != NULL) {
            nickname = user->GetNickName().GetWCharPtr();
        }
        XString text;
        challenge = static_cast<App*>(WindowApp::m_instance)->m_challengeManager->GetChallengeById(message->m_missionId);
        XString challengeName(L"");
        if (challenge != NULL) {
            challengeName = challenge->m_name;
        }
        if (accepted) {
            text = XString::Format(ResString("IDS_CHALLENGE_INVITE_ACCEPTED").c_str(), nickname.c_str(), challengeName.c_str());
        } else {
            text = XString::Format(ResString("IDS_CHALLENGE_INVITE_DECLINED").c_str(), nickname.c_str(), challengeName.c_str());
        }
        TextWindow* label = new TextWindow(text, CFontMgr::GetInstance()->GetFont(15));
        ICSystemFont* font = ICSystemFont::CreateInstance();
        font->Initialize(SScreenAdaptingConstant(12, 25)(), NULL, 1);
        label->SetFont(font);
        label->SetDesiredWidth(SScreenAdaptingConstant(350, 800)());
        m_rows->AddToFront(label, m_column, m_nextRow++);
        SeparateLineRed* line = new SeparateLineRed();
        line->SetDesiredHeight(SScreenAdaptingConstant(5, 10)());
        m_rows->AddToFront(line, m_column, m_nextRow++);
    }
}

void CNewsFeedWindow::MissionInvite(const SMissionInviteMessage* message)
{
    CMissionInviteNewsWindow* row = new CMissionInviteNewsWindow(message);
    m_rows->AddToFront(row, m_column, m_nextRow++);
    SeparateLineRed* line = new SeparateLineRed();
    line->SetDesiredHeight(SScreenAdaptingConstant(5, 10)());
    m_rows->AddToFront(line, m_column, m_nextRow++);
}

void CNewsFeedWindow::ShowEmptyNews()
{
    TextWindow* label = new TextWindow(ResString("IDS_NEWSFEED_NO_NEWS"), CFontMgr::GetInstance()->GetFont(15));
    ICSystemFont* font = ICSystemFont::CreateInstance();
    font->Initialize(SScreenAdaptingConstant(12, 25)(), NULL, 1);
    label->SetFont(font);
    label->SetDesiredWidth(SScreenAdaptingConstant(350, 800)());
    label->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    m_rows->AddToFront(label, m_column, m_nextRow++);
    SeparateLineRed* line = new SeparateLineRed();
    line->SetDesiredHeight(SScreenAdaptingConstant(5, 10)());
    m_rows->AddToFront(line, m_column, m_nextRow++);
}

void CNewsFeedWindow::ClearNews()
{
    m_rows->DeleteChilds();
}
