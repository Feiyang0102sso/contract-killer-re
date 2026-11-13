// CChallengeWindow.cpp - challenge cards and their countdown text.
#include "CChallengeWindow.h"

#include "App.h"
#include "CChallenge.h"
#include "CChallengeManager.h"
#include "CFontMgr.h"
#include "CGameAnalytics.h"
#include "CIncentivizedWindow.h"
#include "CNGS.h"
#include "CNGSLocalUser.h"
#include "CNGSPushNotificationDetails.h"
#include "CNGSRemoteUser.h"
#include "CNGSRemoteUserList.h"
#include "CObjectMapValue.h"
#include "CStdUtil_Cocoa.h"
#include "ServiceWindows.h"
#include "SScreenAdaptingConstant.h"

namespace {
const int ChallengePressed = 0x33e023ab;
const int OpenInvite = 0x0f294040;
const int SendInvites = -0x0d6c6a86;
const int CloseInvite = -0x6828c299;
const int InviteCooldownMs = 1000;
const unsigned int InviteDurationSeconds = 86400;
const long long ChallengeInviteMessage = 1008;
const SScreenAdaptingConstant s_rewardCheckTop(10, 20);
const SScreenAdaptingConstant s_rewardCheckLeft(20, 40);
const SScreenAdaptingConstant s_shareWidth(90, 180);

ImageWindow* CreateImage(const char* name, ImageWindow::EFillType fill)
{
    ImageWindow* image = new ImageWindow();
    image->SetFillType(fill);
    if (name != NULL) {
        image->SetImageById(name);
    }
    return image;
}
}

CChallengeWindow::CChallengeWindow(const CChallenge* challenge)
    : Window(false), m_rewards(NULL), m_progressFill(NULL), m_startHeight(100),
      m_endHeight(0), m_heightPerMs(0), m_animationTime(0), m_elapsedMs(0),
      m_animating(false), m_reservedC1(false), m_animationEnded(false),
      m_challengeId(challenge->m_id), m_inviteCooldownMs(0), m_progressText(NULL)
{
    CreateUI(challenge);
}

void CChallengeWindow::CreateUI(const CChallenge* challenge)
{
    Window* frame = new Window(false);
    AddToFront(frame);
    frame->AddToFront(CreateBgWindow());
    Window* content = new Window(false);
    content->SetInsetSpacing(0, 0, s_rewardCheckTop(), 0);
    content->SetPercentHeight(100, 0, 0);
    frame->AddToFront(content);

    TextWindow* title = new TextWindow(challenge->m_name, CFontMgr::GetInstance()->GetFont(15));
    title->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
    title->SetHeightByContent(0, 0);
    content->AddToFront(title, 0, 0);
    Window* row = new Window(false);
    row->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    row->SetPercentHeight(100, 0, 0);
    row->SetPercentWidth(100, 0, 0);
    content->AddToFront(row, 0, 1);

    Window* description = CreateDescription(challenge);
    description->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    description->SetPercentWidth(25, 0, 0);
    row->AddToFront(description, 0, 0);

    m_rewards = CreateRewardInfo(challenge);
    m_rewards->SetPercentHeight(100, 0, 0);
    m_rewards->SetPercentWidth(70, 0, 0);
    m_rewards->ClearFlags(FLAG_VISIBLE | FLAG_ENABLED);
    row->AddToFront(m_rewards, 1, 0);

    Window* actions = new Window(false);
    actions->SetPercentWidth(25, 0, 0);
    StretchImageButton* share = new StretchImageButton(ResString("IDS_DIALOG_BUTTON_FB_SHARE"),
        "IDB_HUD_TRANSPARENT_BLACK_LEFT", "IDB_HUD_TRANSPARENT_BLACK_TILE", "IDB_HUD_TRANSPARENT_BLACK_RIGHT");
    share->SetAlign(ALIGN_RIGHT | ALIGN_VCENTER);
    share->m_command = OpenInvite;
    share->m_commandParameter1 = challenge->m_id;
    share->ForceSetWidth(s_shareWidth());
    share->SetOutsetSpacing(0, s_rewardCheckTop(), 0, 0);
    actions->AddToFront(share);
    row->AddToFront(actions, 2, 0);
}

Window* CChallengeWindow::CreateBgWindow()
{
    Window* frame = new Window(false);
    frame->AddToFront(new ImageWindow(ImageWindow::FILL_NONE, ImageRes("SUR_FRAME_GREY_LEFT_TOP")), 0, 0);
    frame->AddToFront(new ImageWindow(ImageWindow::FILL_HORIZONTAL, ImageRes("SUR_FRAME_GREY_TOP")), 1, 0);
    frame->AddToFront(new ImageWindow(ImageWindow::FILL_NONE, ImageRes("SUR_FRAME_GREY_RIGHT_TOP")), 2, 0);
    frame->AddToFront(new ImageWindow(ImageWindow::FILL_VERTICAL, ImageRes("SUR_FRAME_GREY_LEFT")), 0, 1);
    frame->AddToFront(new ImageWindow(ImageWindow::FILL_BOTH, ImageRes("SUR_FRAME_GREY_CENTER")), 1, 1);
    frame->AddToFront(new ImageWindow(ImageWindow::FILL_VERTICAL, ImageRes("SUR_FRAME_GREY_RIGHT")), 2, 1);
    frame->AddToFront(new ImageWindow(ImageWindow::FILL_NONE, ImageRes("SUR_FRAME_GREY_LEFT_DOWN")), 0, 2);
    frame->AddToFront(new ImageWindow(ImageWindow::FILL_HORIZONTAL, ImageRes("SUR_FRAME_GREY_DOWN")), 1, 2);
    frame->AddToFront(new ImageWindow(ImageWindow::FILL_NONE, ImageRes("SUR_FRAME_GREY_RIGHT_DOWN")), 2, 2);
    return frame;
}

Window* CChallengeWindow::CreateDescription(const CChallenge* challenge)
{
    Window* description = new Window(false);
    int progress = challenge->GetCurrProgress();
    if (challenge->GetSoloReward()->m_awarded) {
        progress = challenge->m_goal;
    }
    Window* bar = CreateProgressWindow(progress, challenge->m_goal);
    bar->SetPercentHeight(100, 0, 0);
    bar->SetPercentWidth(70, 0, 0);
    bar->SetLayoutType(LAYOUT_SIMPLE);
    bar->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    description->AddToFront(bar);
    return description;
}

Window* CChallengeWindow::CreateProgressWindow(int progress, int goal)
{
    Window* progressWindow = new Window(false);
    Window* bar = new Window(false);
    progressWindow->AddToFront(bar);
    bar->AddToFront(CreateImage("IDB_COOP_PROGRESS_BAR_LEFT", ImageWindow::FILL_NONE), 0, 0);
    ImageWindow* center = CreateImage("IDB_COOP_PROGRESS_BAR_CENTER", ImageWindow::FILL_HORIZONTAL);
    bar->AddToFront(center, 1, 0);
    Window* fillArea = new Window(false);
    fillArea->SetPercentWidth(100, 0, 0);
    center->AddToFront(fillArea);
    m_progressFill = CreateImage("IDB_COOP_PROGRESS_BAR_FILL", ImageWindow::FILL_HORIZONTAL);
    m_progressFill->SetLayoutType(LAYOUT_SIMPLE);
    m_progressFill->SetAlign(ALIGN_LEFT | ALIGN_VCENTER);
    fillArea->AddToFront(m_progressFill);
    bar->AddToFront(CreateImage("IDB_COOP_PROGRESS_BAR_RIGHT", ImageWindow::FILL_NONE), 2, 0);

    m_progressText = new TextWindow(CFontMgr::GetInstance()->GetFont(15));
    m_progressText->SetLayoutType(LAYOUT_SIMPLE);
    m_progressText->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    m_progressText->SetWidthByContent(0, 0);
    m_progressText->SetHeightByContent(0, 0);
    bar->AddToFront(m_progressText);
    SetProgress(progress, goal);
    return progressWindow;
}

void CChallengeWindow::SetProgress(int progress, int goal)
{
    int percent = 0;
    if (goal > 0) {
        percent = progress * 100 / goal;
    }
    m_progressFill->SetPercentWidth(percent, 0, 0);
    m_progressText->SetText(XString::Format(L"%d/%d", progress, goal));
}

Window* CChallengeWindow::CreateRewardInfo(const CChallenge* challenge)
{
    Window* reward = new Window(false);
    Window* arrows = new Window(false);
    arrows->SetPercentHeight(90, 0, 0);
    arrows->SetPercentWidth(100, 0, 0);
    arrows->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    for (int index = 0; index < 3; ++index) {
        Window* cell = new Window(false);
        int width = 36;
        if (index == 2) {
            width = 37;
        } else {
            Window* arrow = CreateArrowWindow();
            arrow->SetLayoutType(LAYOUT_SIMPLE);
            arrow->SetAlign(ALIGN_RIGHT);
            arrow->SetPercentHeight(75, 0, 0);
            cell->AddToFront(arrow);
        }
        cell->SetPercentWidth(width, 0, 0);
        arrows->AddToFront(cell, index, 0);
    }
    reward->AddToFront(arrows);

    Window* tiers = new Window(false);
    tiers->SetPercentHeight(100, 0, 0);
    tiers->SetPercentWidth(100, 0, 0);
    tiers->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    tiers->AddToFront(CreateRewardInfoWindow(challenge->GetSoloReward(), 0), 0, 0);
    tiers->AddToFront(CreateRewardInfoWindow(challenge->GetX1Reward(), 1), 1, 0);
    tiers->AddToFront(CreateRewardInfoWindow(challenge->GetX2Reward(), 2), 2, 0);
    reward->AddToFront(tiers);
    return reward;
}

Window* CChallengeWindow::CreateRewardInfoWindow(const SChallengeReward* reward, int tier)
{
    const char* const captions[] = { "IDS_PLAYER", "IDS_1_FRIEND", "IDS_2_FRIENDS" };
    CFont* font = CFontMgr::GetInstance()->GetFont(15);
    Window* slot = new Window(false);
    Window* content = new Window(false);
    content->SetHeightByContent(0, 0);
    content->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    slot->AddToFront(content);
    Window* amountCell = new Window(false);
    amountCell->SetWidthByContent(0, 0);
    amountCell->SetHeightByContent(0, 0);
    content->AddToFront(amountCell, 0, 0);
    ImageWindow* image = CreateImage(reward->m_image, ImageWindow::FILL_NONE);
    image->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    amountCell->AddToFront(image, 0, 0);

    const char* status = NULL;
    if (reward->m_awarded) {
        status = "IDB_COOP_CHECK_COOP";
    } else if (reward->m_friendId > 0) {
        status = "IDB_CHECKBOX_CHECK_GREY";
    }
    if (status != NULL) {
        ImageWindow* check = CreateImage(status, ImageWindow::FILL_NONE);
        check->SetAlign(ALIGN_LEFT | ALIGN_VCENTER);
        check->SetOutsetSpacing(s_rewardCheckTop(), 0, 0, s_rewardCheckLeft());
        amountCell->AddToFront(check);
    }

    int amount = reward->m_experience;
    if (amount == 0) {
        amount = reward->m_cash;
        if (amount < 1) {
            amount = reward->m_credits;
        }
        if (amount < 1) {
            amount = reward->m_itemAmount;
        }
    }
    if (amount > 0) {
        TextWindow* label = new TextWindow(XString(amount), font);
        label->SetAlign(ALIGN_LEFT | ALIGN_BOTTOM);
        amountCell->AddToFront(label, 1, 0);
    }
    TextWindow* caption = new TextWindow(ResString(captions[tier]), font);
    content->AddToFront(caption, 0, 1);
    return slot;
}

Window* CChallengeWindow::CreateArrowWindow()
{
    Window* arrow = new Window(false);
    arrow->SetWidthByContent(0, 0);
    arrow->AddToFront(CreateImage("IDB_COOP_ARROW_RIGHT", ImageWindow::FILL_NONE));
    return arrow;
}

void CChallengeWindow::StartHeightAnim(int start, int end, int duration)
{
    if (m_endHeight == end) {
        return;
    }
    m_elapsedMs = 0;
    m_startHeight = start;
    m_endHeight = end;
    m_animationTime = duration;
    m_heightPerMs = static_cast<float>(end - start) / duration;
    SetPercentHeight(start, 0, 0);
    m_animating = true;
}

void CChallengeWindow::OnHeightAnimEnded()
{
    if (m_endHeight < m_startHeight) {
        m_rewards->ClearFlags(FLAG_VISIBLE | FLAG_ENABLED);
    } else {
        m_rewards->SetFlags(FLAG_VISIBLE | FLAG_ENABLED);
        EasyParams slide(m_rewards, (EasyLib::EMethod)0xe);
        slide.SetMoving(-m_rewards->m_width, m_rewards->m_y, m_rewards->m_x, m_rewards->m_y);
        slide.SetFullTime(0.3f);
        m_rewards->StartEasy(slide, FLAG_SHOW_AFTER_EASY);
    }
}

void CChallengeWindow::Update()
{
    if (m_animationEnded) {
        m_animationEnded = false;
        OnHeightAnimEnded();
    }
    int delta = WindowApp::m_instance->m_updateFrameTime;
    if (m_animating) {
        m_elapsedMs += delta;
        SetPercentHeight(m_startHeight + m_heightPerMs * m_elapsedMs, 0, 0);
        if (m_elapsedMs > m_animationTime) {
            m_animating = false;
            SetPercentHeight(m_endHeight, 0, 0);
            HandleUpdate(false);
            m_animationEnded = true;
        }
    }
    m_reservedC1 = false;
    if (m_inviteCooldownMs > 0) {
        m_inviteCooldownMs -= delta;
        if (m_inviteCooldownMs < 0) {
            m_inviteCooldownMs = 0;
        }
    }
}

void CChallengeWindow::OnPointerPressed(Event&)
{
    WindowApp::HandleTunnelCommand(ChallengePressed, reinterpret_cast<int>(this), 0, 0);
}

void CChallengeWindow::OnCommand(Event& event)
{
    // the original neither clears the event nor passes it on to Window::OnCommand
    switch (event.m_id) {
    case OpenInvite:
        if (event.m_param1 == m_challengeId && m_inviteCooldownMs == 0) {
            HandleShowInviteToChallenge(event.m_param1);
            m_inviteCooldownMs = InviteCooldownMs;
        }
        break;
    case SendInvites: {
        CIncentivizedWindow* picker = reinterpret_cast<CIncentivizedWindow*>(event.m_param1);
        Vector<int> friendIds = picker->GetSelectedIds();
        HandleShareChallengePressed(friendIds, m_challengeId);
        break;
    }
    }
}

void CChallengeWindow::HandleShowInviteToChallenge(int challengeId)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CChallenge* challenge = app->m_challengeManager->GetChallengeById(challengeId);
    if (challenge == NULL || !challenge->m_active) {
        return;
    }
    CNGSLocalUser* local = CNGS::GetInstance()->GetLocalUser();
    CNGSRemoteUserList* friends = local->m_remoteUserLists[0];
    CIncentivizedWindow* picker = NULL;
    if (local->GetClientID() > 0) {
        for (int index = 0; index < friends->m_users.Size(); ++index) {
            CNGSRemoteUser* user = friends->m_users[index];
            int friendId = user->GetClientID();
            if (challenge->IsFriendInvited(friendId)) {
                continue;
            }
            if (picker == NULL) {
                picker = new CIncentivizedWindow(XString("SHARE"));
            }
            CStrWChar name = user->GetNickName();
            picker->AddFriend(name.GetWCharPtr(), friendId);
        }
    }
    if (picker != NULL) {
        picker->SetCommand(SendInvites);
        picker->SetCommandParameter(challengeId);
        picker->SetTitleText("IDS_CHALLENGE_INVITE_TITLE");
        picker->ShowSelectAllButton();
        app->m_mainLayer->AddModal(picker);
    } else {
        CTextDialog* warning = new CTextDialog();
        warning->SetTitleText("IDS_CHALLENGE_INVITE_TITLE");
        warning->SetMessageText(ResString("IDS_CHALLENGE_NO_FRIENDS_TO_INVITE"));
        warning->SetButtons(CDialogWindow::BUTTON_OK);
        app->m_mainLayer->AddModal(warning);
    }
}

void CChallengeWindow::HandleShareChallengePressed(Vector<int>& friendIds, int challengeId)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CChallenge* challenge = app->m_challengeManager->GetChallengeById(challengeId);
    CNGSLocalUser* local = CNGS::GetInstance()->GetLocalUser();
    if (challenge == NULL || !challenge->m_active || local->GetClientID() <= 0) {
        WindowApp::HandleTunnelCommand(CloseInvite, 0, 0, 0);
        return;
    }

    XString id(challengeId);
    CObjectMapString payload(CStrWChar(id.c_str()));
    CNGSRemoteUserList* friends = local->m_remoteUserLists[0];
    XString format = ResString("IDS_CHALLENGE_INVITE_PUSH_TEXT");
    CStrWChar nickname = local->GetNickName();
    XString message = XString::Format(format.c_str(), nickname.GetWCharPtr(), challenge->m_name.c_str());
    CNGSPushNotificationDetails details;
    details.m_buttonText = ResString("IDS_CHALLENGE_PUSH_BUTTON").c_str();
    details.m_alertText = message.c_str();

    unsigned int count = 0;
    for (int index = 0; index < friendIds.size(); ++index) {
        int friendId = friendIds[index];
        CNGSRemoteUser* user = friends->findRemoteUserByID(friendId);
        if (user != NULL && !challenge->IsFriendInvited(friendId)) {
            user->SendMessage(&payload, InviteDurationSeconds, ChallengeInviteMessage, true, &details);
            challenge->FriendInvited(friendId);
            ++count;
        }
    }
    if (count > 0) {
        CGameAnalytics::logChalengeShared(count);
    }
    WindowApp::HandleTunnelCommand(CloseInvite, 0, 0, 0);
}

CChallengeTimerWindow::CChallengeTimerWindow(unsigned int endTime, CFont* font)
    : TextWindow(font), m_useChallengeTime(true), m_endTime(endTime),
      m_displayedSeconds(-1), m_format("%02d:%02d:%02d")
{
}

void CChallengeTimerWindow::Update()
{
    int now;
    if (m_useChallengeTime) {
        now = CChallengeManager::GetTimeSec();
    } else {
        now = CStdUtil_Cocoa::GetTimeSeconds();
    }
    if (now == 0) {
        return;
    }
    int remaining = static_cast<int>(m_endTime - now);
    if (remaining < 0) {
        remaining = 0;
    }
    if (remaining != m_displayedSeconds) {
        m_displayedSeconds = remaining;
        int minutes = remaining / 60;
        SetText(XString::Format(m_format.c_str(), remaining / 3600, minutes % 60, remaining % 60));
        Window::Update();
    }
}

void CChallengeTimerWindow::SetTimeFormat(XString format)
{
    m_format.Assign(format);
}
