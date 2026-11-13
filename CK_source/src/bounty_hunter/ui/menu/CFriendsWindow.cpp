#include "CFriendsWindow.h"
#include "AdBannerSizes.h"

#include "App.h"
#include "CBH_GPSMap.h"
#include "CBH_SurfaceManager.h"
#include "SScreenAdaptingConstant.h"
#include "ICSystemFont.h"
#include "GameCenter_Facade.h"
#include "DGHelper.h"
#include "CBHUtil_Cocoa.h"
#include "CBH_Player.h"
#include "CChallengeManager.h"
#include "CChallengesWindow.h"
#include "CDialogWindow.h"
#include "CFontMgr.h"
#include "CFriendListWindow.h"
#include "CGameAnalytics.h"
#include "CGiftsWindow.h"
#include "CGServeHandler.h"
#include "CImageButton.h"
#include "CIncentivizedWindow.h"
#include "CNGS.h"
#include "CNGSDataOffer.h"
#include "CNGSLocalUser.h"
#include "CNGSLoginFlow.h"
#include "CNGSOfferManager.h"
#include "CNGSRemoteUser.h"
#include "CNGSRemoteUserList.h"
#include "CNGSSocialInterface.h"
#include "CNGS_Platform.h"
#include "CNewsFeedManager.h"
#include "CNewsFeedWindow.h"
#include "CShopItem.h"
#include "CShopWindow.h"
#include "CTabWindow.h"
#include "CWaitWindow.h"
#include "CSocialGiftsManager.h"

namespace {
// Recovered from the static initializer at 0x13ad20. The original only writes this value.
const SScreenAdaptingConstant ScreenConstant(10, 20);

enum ETab { NewsTab, GuildTab, CoopTab, GiftsTab, TabCount };
const char* const TabNames[] = {
    "IDS_FRIENDS_TAB_NEWS_FEED", "IDS_FRIENDS_TAB_ASSASING_GUILD",
    "IDS_FRIENDS_TAB_COOP", "IDS_FRIENDS_TAB_GIFTS"
};
const int SelectTab = 0x43a30561;
const int StartLogin = 0x43a30562;
const int SelectGift = -0x5bc88ffe;
const int SendGift = -0x075d99b5;
const int ShowGiftCatalog = 0x61ab1bdd;
const int CloseFriends = 0x62b2a2ce;
const int SendFacebookInvitations = 0x4174fa07;
const int FacebookInviteFailed = 0x70b135b5;
const int FacebookInviteOffer = 8;
const int InviteGameCenter = (int)0xa8848c93;
const int InviteFacebook = (int)0xa8848c82;
const int InviteSms = (int)0x8848ddb6;
const int InviteMail = (int)0x848c14b4;
const int RefreshNewsFeed = 0x6e7d5209;
const int StartGameCenterLogin = 0x5be239f5;
const int StartFacebookLogin = (int)0xc7b99831;
const int SelectCoopTab = (int)0xa2457dae;
const int FacebookInviteSent = (int)0xa728ca73;
const int InviteFacebookFromList = (int)0xd1a0e880;
const int FriendsLoaded = (int)0xe4fd16fb;
const int LoginFailedFirst = (int)0xf8a2664c;
const int LoginFailedSecond = (int)0xf8a2664d;
const unsigned int GiftDurationSeconds = 7 * 24 * 60 * 60;

App* GetApp()
{
    return static_cast<App*>(WindowApp::m_instance);
}

}

CFriendsWindow::CFriendsWindow()
    : CBaseScreen(false, true, true, false, true, false), m_loginButton130(NULL), m_sentInviteCount(0),
      m_incentiveShown(false), m_inviteFailShown(false), m_selectingGiftRecipient(false), m_giftRecipientId(-1)
{
    CreateUserInteface();
    CreateLoginButtons();
    // Analytics: count the friends that came from the social network.
    CNGSRemoteUserList* friends = CNGS::GetInstance()->GetLocalUser()->m_remoteUserLists[0];
    int socialCount = 0;
    for (int index = 0; index < friends->m_users.Size(); ++index) {
        if (friends->m_users[index]->IsFromSocialNetwork(2)) {
            socialCount += 1;
        }
    }
    CGameAnalytics::logTotalFriendsCount(socialCount, 0);
}

void CFriendsWindow::CreateUserInteface()
{
    SetTitleText(ResString("IDS_BUTTON_FRIENDS"));
    m_tabs = new CTabWindow(false);
    m_tabs->AddTab(ResString(TabNames[NewsTab]), CreateNewsFeedTab(), NULL, false, -1);
    m_tabs->AddTab(ResString(TabNames[GuildTab]), CreateFriendsTab(), NULL, false, -1);
    m_tabs->AddTab(ResString(TabNames[CoopTab]), new CChallengesWindow(), NULL, false, -1);
    m_tabs->AddTab(ResString(TabNames[GiftsTab]), new CGiftsWindow(), NULL, false, -1);
    m_tabs->SetPercentHeight(100, 0, 0);
    m_tabs->SetPercentWidth(100, 0, 0);
    m_tabs->SetCurrTab(GuildTab, false);
    m_tabs->SetTabsWidthByWindowWidth();
    m_content->AddToFront(m_tabs);
    CreateInviteWindow();
    m_tabs->CreateCountNotification(NewsTab);
}

void CFriendsWindow::CreateLoginButtons()
{
    // Captions: "log in" until the network is authenticated, then the account alias.
    CStrWChar gameCenterCaption;
    gameCenterCaption.Concatenate(ResString("IDS_LOG_IN").c_str());
    CStrWChar facebookCaption;
    facebookCaption.Concatenate(ResString("IDS_LOG_IN").c_str());
    CNGSLocalUser* local = CNGS::GetInstance()->GetLocalUser();
    if (local->isValid() && local->isAuthenticated(1)) {
        CStrWChar alias = local->m_credentials.getAliasForSocialNetwork(static_cast<eSocialNetworkType>(1));
        gameCenterCaption = alias;
    }
    if (local->isValid() && local->isAuthenticated(2)) {
        CStrWChar alias = local->m_credentials.getAliasForSocialNetwork(static_cast<eSocialNetworkType>(2));
        facebookCaption = alias;
    }

    Window* bar = new Window(false);
    bar->SetLayoutType(LAYOUT_SIMPLE);
    bar->SetAlign(10);
    bar->SetSize(bar->m_width, GetTitleBarHeight());
    bar->SetPercentWidth(SScreenAdaptingConstant(65, 70)(), 0, 0);
    ICSystemFont* font = ICSystemFont::CreateInstance();
    m_loginFont = font;
    font->Initialize(SScreenAdaptingConstant(13, 26)(), NULL, 1);

    int column = 0;
    if (GameCenter_Facade::GetInstance()->isGameCenterSupported() == 1) {
        CImageButton* icon = new CImageButton();
        icon->SetAlign(0x24);
        icon->AddImage("IDB_GAMECENTER_ICON_LARGE", 0, 0);
        bar->AddToFront(icon, 0, 0);
        icon->SetPercentWidth(15, 0, 0);
        Window* gameCenterCell = new Window(false);
        m_loginButton130 = new CImageButton();
        XString gameCenterText(gameCenterCaption.GetWCharPtr());
        DGHelper::CropString(gameCenterText, 18);
        m_loginButton130->SetText(gameCenterText, 0x24, NULL, 0, 100);
        m_loginButton130->m_command = StartGameCenterLogin;
        m_loginButton130->m_commandParameter1 = 0;
        m_loginButton130->m_commandParameter2 = 0;
        m_loginButton130->m_commandParameter3 = 0;
        m_loginButton130->SetFont(m_loginFont);
        m_loginButton130->SetPercentHeight(100, 0, 0);
        gameCenterCell->AddToFront(m_loginButton130, 0, 0);
        CImageButton* gameCenterLogout = new CImageButton();
        m_loginButton134 = gameCenterLogout;
        gameCenterLogout->AddImage("IDB_LOGOUT_ICON", 0, 0);
        gameCenterLogout->m_command = StartGameCenterLogin;
        gameCenterLogout->m_commandParameter1 = 0;
        gameCenterLogout->m_commandParameter2 = 0;
        gameCenterLogout->m_commandParameter3 = 0;
        gameCenterCell->AddToFront(m_loginButton134, 0, 1);
        if (!local->isValid() || !local->isAuthenticated(1)) {
            m_loginButton134->ClearFlags(0x2001);
            m_loginButton130->SetCellPos(0, 0, 1, 2);
        } else {
            m_loginButton130->m_command = 0;
            m_loginButton130->m_commandParameter1 = 0;
            m_loginButton130->m_commandParameter2 = 0;
            m_loginButton130->m_commandParameter3 = 0;
        }
        bar->AddToFront(gameCenterCell, 1, 0);
        column = 2;
    }

    CImageButton* facebookIcon = new CImageButton();
    facebookIcon->AddImage("IDB_FACEBOOK_ICON_LARGE", 0, 0);
    bar->AddToFront(facebookIcon, column, 0);
    facebookIcon->SetPercentWidth(15, 0, 0);
    Window* facebookCell = new Window(false);
    m_loginButton12c = new CImageButton();
    XString facebookText(facebookCaption.GetWCharPtr());
    int maxLength = 36;
    if (GameCenter_Facade::GetInstance()->isGameCenterSupported()) {
        maxLength = 18;
    }
    DGHelper::CropString(facebookText, maxLength);
    m_loginButton12c->SetText(facebookText, 0x24, NULL, 0, 100);
    m_loginButton12c->SetPercentHeight(100, 0, 0);
    m_loginButton12c->m_command = StartFacebookLogin;
    m_loginButton12c->m_commandParameter1 = 0;
    m_loginButton12c->m_commandParameter2 = 0;
    m_loginButton12c->m_commandParameter3 = 0;
    m_loginButton12c->SetFont(m_loginFont);
    facebookCell->AddToFront(m_loginButton12c, 0, 0);
    int width = 100;
    if (GameCenter_Facade::GetInstance()->isGameCenterSupported()) {
        width = 40;
    }
    m_loginButton12c->SetPercentWidth(width, 0, 0);
    CImageButton* facebookLogout = new CImageButton();
    m_loginButton138 = facebookLogout;
    facebookLogout->AddImage("IDB_LOGOUT_ICON", 0, 0);
    facebookLogout->m_command = StartFacebookLogin;
    facebookLogout->m_commandParameter1 = 0;
    facebookLogout->m_commandParameter2 = 0;
    facebookLogout->m_commandParameter3 = 0;
    facebookCell->AddToFront(m_loginButton138, 0, 1);
    if (!local->isValid() || !local->isAuthenticated(2)) {
        m_loginButton138->ClearFlags(0x2001);
        m_loginButton12c->SetCellPos(0, 0, 1, 2);
    } else {
        m_loginButton12c->m_command = 0;
        m_loginButton12c->m_commandParameter1 = 0;
        m_loginButton12c->m_commandParameter2 = 0;
        m_loginButton12c->m_commandParameter3 = 0;
    }
    bar->AddToFront(facebookCell, column | 1, 0);
    AddToFront(bar);
}

void CFriendsWindow::UpdateLoginButtons()
{
    CNGSLocalUser* local = CNGS::GetInstance()->GetLocalUser();
    if (m_loginButton130 != NULL) {
        if (!local->isValid() || !local->isAuthenticated(1)) {
            m_loginButton130->SetText(XString(ResString("IDS_LOG_IN").c_str()), 0x24, NULL, 0, 0);
            m_loginButton130->SetCellPos(0, 0, 1, 2);
            m_loginButton130->m_command = StartGameCenterLogin;
            m_loginButton130->m_commandParameter1 = 0;
            m_loginButton130->m_commandParameter2 = 0;
            m_loginButton130->m_commandParameter3 = 0;
            m_loginButton134->ClearFlags(0x2001);
        } else {
            CStrWChar alias = local->m_credentials.getAliasForSocialNetwork(static_cast<eSocialNetworkType>(1));
            XString caption(alias.GetWCharPtr());
            DGHelper::CropString(caption, 18);
            m_loginButton130->SetText(caption, 0x24, NULL, 0, 0);
            m_loginButton130->SetFont(m_loginFont);
            m_loginButton130->SetCellPos(0, 0, 1, 1);
            m_loginButton130->m_command = 0;
            m_loginButton130->m_commandParameter1 = 0;
            m_loginButton130->m_commandParameter2 = 0;
            m_loginButton130->m_commandParameter3 = 0;
            m_loginButton134->SetFlags(0x2001);
        }
    }
    if (!local->isValid() || !local->isAuthenticated(2)) {
        m_loginButton12c->SetText(XString(ResString("IDS_LOG_IN").c_str()), 0x24, NULL, 0, 0);
        m_loginButton12c->SetCellPos(0, 0, 1, 2);
        m_loginButton12c->m_command = StartFacebookLogin;
        m_loginButton12c->m_commandParameter1 = 0;
        m_loginButton12c->m_commandParameter2 = 0;
        m_loginButton12c->m_commandParameter3 = 0;
        m_loginButton138->ClearFlags(0x2001);
    } else {
        bool gameCenter = GameCenter_Facade::GetInstance()->isGameCenterSupported();
        CStrWChar alias = local->m_credentials.getAliasForSocialNetwork(static_cast<eSocialNetworkType>(2));
        XString caption(alias.GetWCharPtr());
        int maxLength = 36;
        if (gameCenter) {
            maxLength = 18;
        }
        DGHelper::CropString(caption, maxLength);
        m_loginButton12c->SetText(caption, 0x24, NULL, 0, 0);
        m_loginButton12c->SetFont(m_loginFont);
        m_loginButton12c->SetCellPos(0, 0, 1, 1);
        m_loginButton12c->m_command = 0;
        m_loginButton12c->m_commandParameter1 = 0;
        m_loginButton12c->m_commandParameter2 = 0;
        m_loginButton12c->m_commandParameter3 = 0;
        m_loginButton138->SetFlags(0x2001);
    }
}

void CFriendsWindow::UpdateFriendsTab()
{
    CNGSRemoteUserList* friends = CNGS::GetInstance()->GetLocalUser()->m_remoteUserLists[0];
    CStrWChar picType;
    picType.Concatenate(L"square");
    bool changed = false;
    for (int index = 0; index < m_friendInfos.Size(); ++index) {
        SFriendInfo& info = m_friendInfos[index];
        CNGSRemoteUser* user = friends->m_users[info.m_userIndex];
        if (info.m_avatar == NULL && info.m_hasAvatar) {
            CNGSSocialInterface* facebook = CNGS_Platform::GetInstance()->getSocialInterface(2);
            CStrWChar facebookId = user->GetIDForSocialNetwork(static_cast<eSocialNetworkType>(2));
            const void* data = NULL;
            unsigned int size = 0;
            facebook->getFriendPicWithID(facebookId, data, size);
            if (size == 0 || data == NULL) {
                // Picture not downloaded yet: ask for it and retry next frame.
                facebook->requestFriendPicWithID(facebookId, &picType);
                break;
            }
            if (*static_cast<const unsigned char*>(data) == 0xff) {
                App* app = static_cast<App*>(WindowApp::m_instance);
                info.m_avatar = app->m_surfaceManager->CreateSurface(data, size, 0x7179dacc);
                changed = true;
            }
        }
        if (info.m_done) {
            continue;
        }
        // Original dispatches through CNGSRemoteUser vtable slot +0x30 (deep populated), see CNGSRemoteUser.h.
        typedef unsigned char (*UserQuery)(CNGSRemoteUser*);
#ifdef CK_PC
        // PORT: slot +0x30 is an ARM vtable offset and empty in the original; the offline port has no
        // remote users, so a user never counts as deep populated.
        if (false) {
#else
        void** vtable = *reinterpret_cast<void***>(user);
        UserQuery isDeepPopulated = reinterpret_cast<UserQuery>(vtable[0x30 / 4]);
        if (isDeepPopulated(user)) {
#endif
            info.m_done = true;
            changed = true;
            info.m_avatarWidth = *reinterpret_cast<short*>(reinterpret_cast<char*>(user) + 0x9c);
            info.m_avatarData = *reinterpret_cast<int*>(reinterpret_cast<char*>(user) + 0xa0);
        } else if (user->isAvailable()) {
            user->deepPopulate(1);
        }
    }
    if (changed) {
        RefreshFriendTab();
    }
}

Window* CFriendsWindow::CreateNewsFeedTab()
{
    m_newsFeed = new CNewsFeedWindow();
    FillNewsFeedTab(true);
    return m_newsFeed;
}

Window* CFriendsWindow::CreateFriendsTab()
{
    m_friendList = new CFriendListWindow();
    CNGSLocalUser* local = CNGS::GetInstance()->GetLocalUser();
    if (m_friendInfos.Size() == 0) {
        CNGSRemoteUserList* friends = local->m_remoteUserLists[0];
        for (int index = 0; index < friends->m_users.Size(); ++index) {
            int clientId = friends->m_users[index]->GetClientID();
            m_friendList->AddFriend(clientId);
        }
    }
    return m_friendList;
}

void CFriendsWindow::CreateInviteWindow()
{
    m_inviteWindow = new TextWindow(ResString("IDS_INVITE_FRIENDS_TITLE"), CFontMgr::GetInstance()->GetFont(15));
    m_inviteWindow->SetLayoutType(LAYOUT_NONE);
    m_inviteWindow->SetOutsetSpacing(10, 10, 10, 10);
    m_inviteWindow->SetPercentWidth(100, 0, 0);
    m_inviteWindow->SetPercentHeight(100, 0, 0);
    m_inviteWindow->SetAlign(0x24);

    // One row of placeholder invite buttons: Game Center, Facebook, SMS, Mail.
    Window* row = new Window(false);
    row->SetLayoutType(LAYOUT_SIMPLE);
    row->SetAlign(0xc);
    row->SetPercentWidth(100, 0, 0);
    row->SetPercentHeight(30, 0, 0);

    CImageButton* button = new CImageButton();
    button->SetText(XString("GC\nX CASH"), 0x24, NULL, 0, 0);
    button->m_command = InviteGameCenter;
    button->m_commandParameter1 = 0;
    button->m_commandParameter2 = 0;
    button->m_commandParameter3 = 0;
    row->AddToFront(button, 0, 0);

    button = new CImageButton();
    button->SetText(XString("FB\nX CASH\nX CREDITS"), 0x24, NULL, 0, 0);
    button->m_command = InviteFacebook;
    button->m_commandParameter1 = 0;
    button->m_commandParameter2 = 0;
    button->m_commandParameter3 = 0;
    row->AddToFront(button, 1, 0);

    button = new CImageButton();
    button->SetText(XString("SMS\nX CASH"), 0x24, NULL, 0, 0);
    button->m_command = InviteSms;
    button->m_commandParameter1 = 0;
    button->m_commandParameter2 = 0;
    button->m_commandParameter3 = 0;
    row->AddToFront(button, 2, 0);

    button = new CImageButton();
    button->SetText(XString("Mail\nX CASH"), 0x24, NULL, 0, 0);
    button->m_command = InviteMail;
    button->m_commandParameter1 = 0;
    button->m_commandParameter2 = 0;
    button->m_commandParameter3 = 0;
    row->AddToFront(button, 3, 0);

    m_inviteWindow->AddToFront(row);
}

void CFriendsWindow::FillNewsFeedTab(bool firstFill)
{
    for (int index = 0; index < static_cast<App*>(WindowApp::m_instance)->m_newsFeedManager->GetNewsCount(); ++index) {
        CNewsFeedManager* manager = static_cast<App*>(WindowApp::m_instance)->m_newsFeedManager;
        SMessage* message = const_cast<SMessage*>(manager->GetNewsAt(index));
        // Later refills only add the messages that were not shown yet.
        if (!firstFill && message->m_flag09) {
            continue;
        }
        message->m_flag09 = true;
        switch (message->m_type) {
        case NEWS_TARGETTED:
            m_newsFeed->Targetted(static_cast<const STargettedMessage*>(message));
            break;
        case NEWS_MISSION_INVITE:
            m_newsFeed->MissionInvite(static_cast<const SMissionInviteMessage*>(message));
            break;
        case NEWS_FRIEND_JOINED:
            m_newsFeed->FriendInstall(static_cast<const SFriendJoinedGameMessage*>(message));
            break;
        case NEWS_GIFT:
            m_newsFeed->ReceiveGift(static_cast<const SGiftReceivedMessage*>(message));
            break;
        case NEWS_AWARD:
            m_newsFeed->ReceiveAward(static_cast<const SServerAwardMessage*>(message));
            break;
        case NEWS_GUILD_INVITE:
            m_newsFeed->ShowGuildInvite(static_cast<const STargettedMessage*>(message));
            break;
        case NEWS_GUILD_ACCEPTED:
            m_newsFeed->ShowGuildInviteResponse(message, true);
            break;
        case NEWS_GUILD_DECLINED:
            m_newsFeed->ShowGuildInviteResponse(message, false);
            break;
        case NEWS_CHALLENGE_INVITE:
            m_newsFeed->ShowChallengeInvite(static_cast<const SMissionInviteMessage*>(message));
            break;
        case NEWS_CHALLENGE_ACCEPTED:
            m_newsFeed->ShowChallengeInviteResponse(static_cast<const SMissionInviteMessage*>(message), true);
            break;
        case NEWS_CHALLENGE_DECLINED:
            m_newsFeed->ShowChallengeInviteResponse(static_cast<const SMissionInviteMessage*>(message), false);
            break;
        case NEWS_EMPTY:
            m_newsFeed->ShowEmptyNews();
            break;
        default:
            break;
        }
    }
}

void CFriendsWindow::SetTab(int tab)
{
    m_tabs->SetCurrTab(tab, false);
}

void CFriendsWindow::Update()
{
    CBaseScreen::Update();
    UpdateFriendsTab();
    UpdateLoginButtons();
    UpdateNewsFeedTab();
    UpdateCountNotification();
    if (!m_selectingGiftRecipient) {
        return;
    }
    // Leaving the gifts tab cancels the pending recipient.
    if (m_tabs->m_currentTab != GiftsTab) {
        m_selectingGiftRecipient = false;
        m_giftRecipientId = -1;
    }
}

void CFriendsWindow::UpdateNewsFeedTab()
{
    if (static_cast<App*>(WindowApp::m_instance)->m_newsFeedManager->GetNewsCount() != 0) {
        return;
    }
    static_cast<App*>(WindowApp::m_instance)->m_newsFeedManager->AddEmptyNewsMessage();
    FillNewsFeedTab(false);
}

void CFriendsWindow::UpdateCountNotification()
{
    if (CBH_GPSMap::m_instance == NULL) {
        return;
    }
    CNewsFeedManager* manager = static_cast<App*>(WindowApp::m_instance)->m_newsFeedManager;
    if (manager == NULL) {
        return;
    }
    int unread = manager->GetUnreadNonEmptyMessageCount();
    int unreadType12 = manager->GetUnreadNonEmptyMessageCount(static_cast<ENewsType>(12));
    m_tabs->SetCountToNotification(NewsTab, unread - unreadType12);
}

void CFriendsWindow::RefreshFriendTab()
{
    m_tabs->Close();
    CreateUserInteface();
}

int CFriendsWindow::GetNonPlayingFriendsCount()
{
    // The original body only touches the social interface and the local user.
    CNGS_Platform::GetInstance()->getSocialInterface(2);
    CNGS::GetInstance()->GetLocalUser();
    return 0;
}

void CFriendsWindow::ShowFacebookIncentiveWindow(unsigned int sentCount)
{
    // Shown once per window, and only after at least one invitation went out.
    if (sentCount == 0) {
        return;
    }
    if (m_incentiveShown) {
        return;
    }
    COfferManager* offers = COfferManager::GetInstance();
    offers->setOfferId(FacebookInviteOffer);
    CStrWChar* installUnit = offers->getFriendInstallIncentiveUnitForOffer();
    int installQuantity = offers->getFriendInstallIncentiveQuantityForOffer();
    CStrWChar* inviteUnit = offers->getFriendInviteIncentiveUnitForOffer();
    int inviteQuantity = offers->getFriendInviteIncentiveQuantityForOffer();

    CDialogWindow* dialog = new CDialogWindow(true);
    dialog->SetButtons(CDialogWindow::BUTTON_OK);
    dialog->SetTitleText(ResString("IDS_YOU_INVITE_SENT_TITLE"));
    XString message = ResString("IDS_YOU_INVITE_SENT_MSG");
    if (inviteQuantity > 0) {
        message.Append(XString::Format(ResString("IDS_YOU_INVITE_SENT_INVITE_REWARD").c_str(), inviteQuantity,
                                       inviteUnit->GetWCharPtr()));
        if (installQuantity != 0) {
            message.Append(XString::Format(ResString("IDS_YOU_INVITE_SENT_INVITE_ACCEPTED_REWARD").c_str(),
                                           installQuantity, installUnit->GetWCharPtr()));
        }
    }
    dialog->SetMessageText(message);
    m_content->AddModal(dialog);
    m_incentiveShown = true;
}

void CFriendsWindow::ShowSelectGiftRecipient()
{
    CNGSRemoteUserList* friends = CNGS::GetInstance()->GetLocalUser()->m_remoteUserLists[0];
    int today = App::GetCurrentDay();
    // m_giftRecipients maps picker rows back to friend list indices.
    m_giftRecipients.setSize(0);
    CDialogWindow* dialog = NULL;
    int rowCount = 0;
    if (App::IsNetTimeAvailable() == 1) {
        for (int index = 0; index < friends->m_users.Size(); ++index) {
            CNGSRemoteUser* user = friends->m_users[index];
            if (user == NULL) {
                continue;
            }
            int clientId = user->GetClientID();
            int lastDay = CBH_Player::GetInstance()->GetFriendGiftLastDay(clientId);
            if (lastDay >= 0 && today - lastDay <= 0) {
                continue;
            }
            m_giftRecipients.addElement(index);
            if (dialog == NULL) {
                CIncentivizedWindow* picker = new CIncentivizedWindow(ResString("IDS_FRIENDS_SELECT_GIFT_BUTTON"));
                picker->SetTitleText(ResString("IDS_FRIENDS_SELECT_GIFT_RECIPIENT_TITLE"));
                picker->SetCommand(SendGift);
                dialog = picker;
            }
            {
                CStrWChar nickName = user->GetNickName();
                static_cast<CIncentivizedWindow*>(dialog)->AddFriend(XString(nickName.GetWCharPtr()), rowCount);
            }
            if (m_selectingGiftRecipient && m_giftRecipientId == user->GetClientID()) {
                static_cast<CIncentivizedWindow*>(dialog)->SelectCheckBox(rowCount);
            }
            rowCount += 1;
        }
    }
    if (rowCount == 0) {
        dialog = new CTextDialog();
        dialog->SetMessageText(ResString("IDS_NONE_FRIENDS_TO_SEND_GIFT_TEXT"));
        dialog->SetTitleText(ResString("IDS_NONE_FRIENDS_TO_SEND_GIFT_CAPTION"));
        dialog->SetButtons(CDialogWindow::BUTTON_OK);
    }
    static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddModal(dialog);
}

void CFriendsWindow::ShowFacebookInviteFriendsWindow()
{
    m_facebookInviteIds.Clear();
    CNGSSocialInterface* facebook = CNGS_Platform::GetInstance()->getSocialInterface(SOCIAL_INTERFACE_FACEBOOK);
    CNGSRemoteUserList* friends = CNGS::GetInstance()->GetLocalUser()->m_remoteUserLists[0];
    COfferManager::GetInstance()->setOfferId(FacebookInviteOffer);
    CIncentivizedWindow* picker = new CIncentivizedWindow(ResString("IDS_INVITE_BUTTON_NAME"));
    picker->SetSingleSelection();
    picker->SetTitleText(ResString("IDS_FRIENDS_INVITE"));
    picker->SetCommand(SendFacebookInvitations);

    int added = 0;
    if (COfferManager::GetInstance()->isInitialized()) {
        for (int index = 0; index < facebook->getFriendsCount(); ++index) {
            CStrWChar name = facebook->getFriendNameFromSortedListAtIndex(index);
            CStrWChar id = facebook->getFriendIDFromSortedListAtIndex(index);
            int friendIndex = 0;
            for (; friendIndex < friends->m_users.Size(); ++friendIndex) {
                CNGSRemoteUser* friendUser = friends->m_users[friendIndex];
                if (friendUser->IsFromSocialNetwork(SOCIAL_INTERFACE_FACEBOOK)) {
                    CStrWChar friendId = friendUser->GetIDForSocialNetwork(SOCIAL_NETWORK_FACEBOOK);
                    if (id == friendId) {
                        break;
                    }
                }
            }
            CStrWChar inviteId = facebook->getFriendIDFromSortedListAtIndex(index);
            int previouslyInvited = COfferManager::GetInstance()->wasPreviouslyInvited(SOCIAL_INTERFACE_FACEBOOK, &inviteId);
            if (friendIndex >= friends->m_users.Size() && previouslyInvited == 0) {
                ++added;
                picker->AddFriend(XString(name.GetWCharPtr()), m_facebookInviteIds.Size());
                m_facebookInviteIds.Add(id);
            }
        }
    }

    CDialogWindow* dialog = picker;
    if (added == 0) {
        CTextDialog* warning = new CTextDialog();
        warning->SetMessageText(ResString("IDS_FRIENDS_NO_FRIENDS_TO_INVITE_TEXT"));
        warning->SetTitleText(ResString("IDS_FRIENDS_NO_FRIENDS_TO_INVITE_TITLE"));
        warning->SetButtons(CDialogWindow::BUTTON_OK);
        GetApp()->m_mainLayer->AddModal(warning);
    } else {
        GetApp()->m_mainLayer->AddModal(dialog);
    }
}

void CFriendsWindow::HandleFacebookInvitation(CIncentivizedWindow* picker)
{
    if (picker == NULL) {
        return;
    }

    CStrWChar description(ResString("IDS_FB_DESCRIPTION").c_str());
    CStrWChar link(L"http://gcs.glu.com/gcs/iphone/iphone.jsp?t=bounty&src=facebook&p=iphone");
    CStrWChar message(ResString("IDS_FB_SHARE_INVITE").c_str());
    CStrWChar picture(L"http://gcs.glu.com/gcs/images/ContractKiller_114icon.jpg");
    CStrWChar caption(ResString("IDS_FB_CAPTION").c_str());
    CStrWChar name(ResString("IDS_FB_LINK_DESC").c_str());
    CStrWChar privacy(L"");

    COfferManager* offers = COfferManager::GetInstance();
    offers->setOfferId(FacebookInviteOffer);
    if (offers->m_offers.Size() > 0) {
        int incentiveOffer = -1;
        for (int index = 0; index < offers->m_offers.Size(); ++index) {
            offers->setOfferIndex(index);
            if (offers->offerHasFriendInviteIncentive()) {
                incentiveOffer = index;
            }
        }
        if (incentiveOffer != -1) {
            offers->setOfferIndex(incentiveOffer);
        }
    }

    Vector<int> selected = picker->GetSelectedIds();
    unsigned char queued = 0;
    for (int index = 0; index < selected.size(); ++index) {
        CStrWChar id;
        id.Concatenate(m_facebookInviteIds[selected[index]].GetWCharPtr());
        if (offers->queueInvite(SOCIAL_INTERFACE_FACEBOOK, &id, false) == 0) {
            XString invitedName;
            CNGSSocialInterface* facebook = CNGS_Platform::GetInstance()->getSocialInterface(SOCIAL_INTERFACE_FACEBOOK);
            {
                CStrWChar friendName = facebook->getFriendNameFromSortedListWithId(id);
                const wchar_t* text = friendName.GetWCharPtr();
                queued = 1;
                invitedName.Assign(text, wcslen(text) * sizeof(wchar_t));
            }
            CGameAnalytics::logFriendInvited(invitedName);
        }
    }

    GetNonPlayingFriendsCount();
    CGameAnalytics::logFriendInvited(selected.size(), 0);
    picker->Close();
    if (queued) {
        if (selected.size() > 0) {
            COfferManager::GetInstance()->sendQueuedInvitations(
                &message, &link, &name, &caption, &description, &picture, &privacy);
        }
        m_waitWindow = new CWaitWindow();
        static_cast<CWaitWindow*>(m_waitWindow)->SetBackgroundColor(0x64000000);
        GetApp()->m_mainLayer->AddModal(m_waitWindow);
        m_sentInviteCount = selected.size();
    } else {
        WindowApp::HandleTunnelCommand(FacebookInviteFailed, 0, 0, 0);
    }
}

void CFriendsWindow::HandleGiftSending(CIncentivizedWindow* picker)
{
    if (picker == NULL) {
        return;
    }

    Vector<int> selected = picker->GetSelectedIds();
    CNGSLocalUser* local = CNGS::GetInstance()->GetLocalUser();
    if (selected.size() > 0) {
        CNGSRemoteUserList* friends = local->m_remoteUserLists[0];
        local = CNGS::GetInstance()->GetLocalUser();
        for (int index = 0; index < selected.size(); ++index) {
            SGift* gift = GetApp()->m_socialGiftManager->GetGift(m_giftIndex);
            if (gift == NULL) {
                continue;
            }
            CItem* item = gift->GetItem();
            CStrWChar itemName(item->GetName().c_str());
            CStrWChar giftText(" sent you a gift! It's ");
            CStrWChar message = local->GetNickName() + giftText + itemName;
            // Picker rows map to friend list indices through m_giftRecipients (original layout 0x110).
            int row = selected[index];
            if (row >= 0 && row < m_giftRecipients.size()) {
                int recipientIndex = m_giftRecipients[row];
                friends->m_users[recipientIndex]->SendGift(message, m_giftIndex + 1, 1, GiftDurationSeconds, false, false, NULL);
                CNGSRemoteUser* recipient = friends->m_users[recipientIndex];
                int friendId = recipient->GetClientID();
                int today = App::GetCurrentDay();
                CBH_Player::GetInstance()->SetFriendGiftLastDay(friendId, today);
                CGameAnalytics::logGiftSentIngame(recipient->GetClientID(), item->GetName());
            }
        }
    }

    CTextDialog* dialog = new CTextDialog();
    dialog->SetMessageText(ResString("IDS_GIFT_WAS_SENT_TEXT"));
    dialog->SetTitleText(ResString("IDS_GIFT_WAS_SENT_CAPTION"));
    dialog->SetButtons(CDialogWindow::BUTTON_OK);
    GetApp()->m_mainLayer->AddModal(dialog);
    picker->Close();
}

void CFriendsWindow::OnCommand(Event& event)
{
    switch (event.m_id) {
    case RefreshNewsFeed:
        m_newsFeed->ClearNews();
        FillNewsFeedTab(true);
        break;
    case SelectCoopTab:
        m_tabs->SetCurrTab(CoopTab, false);
        break;
    case SelectGift:
        m_giftIndex = event.m_param1;
        ShowSelectGiftRecipient();
        break;
    case FacebookInviteSent:
        ShowFacebookIncentiveWindow(m_sentInviteCount);
        goto closeInviteWait;
    case CloseFriends:
        Close();
        break;
    case InviteFacebook:
    case InviteFacebookFromList:
        ShowFacebookInviteFriendsWindow();
        break;
    case InviteGameCenter: {
        CNGSSocialInterface* gameCenter = CNGS_Platform::GetInstance()->getSocialInterface(1);
        gameCenter->showFriendRequestScreen(CBHUtil_Cocoa::GetRootViewController());
        break;
    }
    case FriendsLoaded:
        if ((unsigned int)(event.m_param1 - 1) < 2) {
            m_friendInfos.m_size = 0;
            RefreshFriendTab();
        }
        break;
    case SendGift:
        HandleGiftSending(reinterpret_cast<CIncentivizedWindow*>(event.m_param1));
        break;
    case LoginFailedFirst: {
        CImageButton* button = m_loginButton12c;
        button->SetText(ResString("IDS_LOG_IN"), 0x24, NULL, 0, 0);
        button = m_loginButton12c;
        button->SetFont(m_loginFont);
        break;
    }
    case LoginFailedSecond: {
        CImageButton* button = m_loginButton130;
        button->SetText(ResString("IDS_LOG_IN"), 0x24, NULL, 0, 0);
        button = m_loginButton130;
        button->SetFont(m_loginFont);
        break;
    }
    case SendFacebookInvitations:
        m_incentiveShown = false;
        m_inviteFailShown = false;
        HandleFacebookInvitation(reinterpret_cast<CIncentivizedWindow*>(event.m_param1));
        break;
    case ShowGiftCatalog:
        m_selectingGiftRecipient = true;
        m_giftRecipientId = event.m_param1;
        m_tabs->SetCurrTab(GiftsTab, false);
        break;
    case FacebookInviteFailed:
        if (!m_inviteFailShown) {
            CTextDialog* dialog = new CTextDialog();
            dialog->SetButtons(CDialogWindow::BUTTON_OK);
            dialog->SetTitleText(ResString("IDS_FRIEND_INVITE_POST_FAIL_TITLE"));
            dialog->SetMessageText(ResString("IDS_FRIEND_INVITE_POST_FAIL_MESSAGE"));
            GetApp()->m_mainLayer->AddModal(dialog);
            m_inviteFailShown = true;
        }
    closeInviteWait:
        // The original checks the address against the desktop's windows before closing it.
        m_waitWindow->CloseSafely();
        event.Clear();
        break;
    default:
        break;
    }
    CBaseScreen::OnCommand(event);
}
