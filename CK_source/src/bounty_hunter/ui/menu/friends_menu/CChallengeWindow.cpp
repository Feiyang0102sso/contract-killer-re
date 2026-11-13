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
}

CChallengeWindow::CChallengeWindow(const CChallenge* challenge)
    : Window(false), m_startHeight(100), m_endHeight(0), m_heightPerMs(0),
      m_animationTime(0), m_elapsedMs(0), m_animating(false), m_reservedC1(false),
      m_animationEnded(false), m_inviteCooldownMs(0)
{
    CreateUI(challenge);
    m_challengeId = challenge->m_id;
}

void CChallengeWindow::CreateUI(const CChallenge* challenge)
{
    Window* frame = new Window(false);
    AddToFront(frame);
    frame->AddToFront(CreateBgWindow());
    Window* content = new Window(false);
    content->SetInsetSpacing(0, 0, SScreenAdaptingConstant(10, 20)(), 0);
    content->SetPercentHeight(100, 0, 0);
    frame->AddToFront(content);

    TextWindow* title = new TextWindow(challenge->m_name, CFontMgr::GetInstance()->GetFont(15));
    title->SetLayoutType(LAYOUT_SIMPLE);
    title->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
    content->AddToFront(title, 0, 0);
    title->SetHeightByContent(0, 0);
    title->m_debugFrame = true;

    Window* row = new Window(false);
    row->SetLayoutType(LAYOUT_SIMPLE);
    row->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    row->SetPercentHeight(100, 0, 0);
    row->SetPercentWidth(100, 0, 0);
    content->AddToFront(row, 0, 1);

    Window* description = CreateDescription(challenge);
    description->SetLayoutType(LAYOUT_SIMPLE);
    description->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    description->SetPercentWidth(25, 0, 0);
    row->AddToFront(description, 0, 0);

    m_rewards = CreateRewardInfo(challenge);
    m_rewards->m_debugFrame = true;
    row->AddToFront(m_rewards, 1, 0);
    m_rewards->SetPercentHeight(100, 0, 0);
    m_rewards->SetPercentWidth(70, 0, 0);
    m_rewards->ClearFlags(FLAG_VISIBLE | FLAG_ENABLED);

    Window* actions = new Window(false);
    StretchImageButton* share = new StretchImageButton(ResString("IDS_DIALOG_BUTTON_FB_SHARE"),
        "IDB_HUD_TRANSPARENT_BLACK_LEFT", "IDB_HUD_TRANSPARENT_BLACK_TILE", "IDB_HUD_TRANSPARENT_BLACK_RIGHT");
    share->SetAlign(ALIGN_RIGHT | ALIGN_VCENTER);
    share->SetCommandParameters(OpenInvite, challenge->m_id, 0, 0);
    share->ForceSetWidth(SScreenAdaptingConstant(90, 180)());
    share->SetOutsetSpacing(0, SScreenAdaptingConstant(10, 20)(), 0, 0);
    actions->AddToFront(share);
    row->AddToFront(actions, 2, 0);
    actions->SetPercentWidth(25, 0, 0);
}

Window* CChallengeWindow::CreateBgWindow()
{
    Window* frame = new Window(false);
    ImageWindow* topLeft = new ImageWindow(ImageWindow::FILL_NONE, ImageRes("SUR_FRAME_GREY_LEFT_TOP"));
    frame->AddToFront(topLeft, 0, 0);
    ImageWindow* top = new ImageWindow(ImageWindow::FILL_HORIZONTAL, ImageRes("SUR_FRAME_GREY_TOP"));
    frame->AddToFront(top, 1, 0);
    ImageWindow* topRight = new ImageWindow(ImageWindow::FILL_NONE, ImageRes("SUR_FRAME_GREY_RIGHT_TOP"));
    frame->AddToFront(topRight, 2, 0);
    ImageWindow* left = new ImageWindow(ImageWindow::FILL_VERTICAL, ImageRes("SUR_FRAME_GREY_LEFT"));
    frame->AddToFront(left, 0, 1);
    ImageWindow* center = new ImageWindow(ImageWindow::FILL_BOTH, ImageRes("SUR_FRAME_GREY_CENTER"));
    frame->AddToFront(center, 1, 1);
    ImageWindow* right = new ImageWindow(ImageWindow::FILL_VERTICAL, ImageRes("SUR_FRAME_GREY_RIGHT"));
    frame->AddToFront(right, 2, 1);
    ImageWindow* bottomLeft = new ImageWindow(ImageWindow::FILL_NONE, ImageRes("SUR_FRAME_GREY_LEFT_DOWN"));
    frame->AddToFront(bottomLeft, 0, 2);
    ImageWindow* bottom = new ImageWindow(ImageWindow::FILL_HORIZONTAL, ImageRes("SUR_FRAME_GREY_DOWN"));
    frame->AddToFront(bottom, 1, 2);
    ImageWindow* bottomRight = new ImageWindow(ImageWindow::FILL_NONE, ImageRes("SUR_FRAME_GREY_RIGHT_DOWN"));
    frame->AddToFront(bottomRight, 2, 2);
    return frame;
}

Window* CChallengeWindow::CreateDescription(const CChallenge* challenge)
{
    Window* description = new Window(false);
    int progress = challenge->GetCurrProgress();
    int goal = challenge->m_goal;
    if (challenge->GetSoloReward()->m_awarded) {
        progress = goal;
    }
    Window* bar = CreateProgressWindow(progress, goal);
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
    int percent = progress * 100 / goal;
    Window* bar = new Window(false);
    progressWindow->AddToFront(bar);

    ImageWindow* left = new ImageWindow();
    left->SetFillType(ImageWindow::FILL_NONE);
    left->SetImageById("IDB_COOP_PROGRESS_BAR_LEFT");
    left->m_debugFrame = true;
    bar->AddToFront(left, 0, 0);

    ImageWindow* center = new ImageWindow();
    center->SetFillType(ImageWindow::FILL_HORIZONTAL);
    center->SetImageById("IDB_COOP_PROGRESS_BAR_CENTER");
    bar->AddToFront(center, 1, 0);
    Window* fillArea = new Window(false);
    fillArea->SetPercentWidth(100, 0, 0);
    center->AddToFront(fillArea);

    ImageWindow* fill = new ImageWindow();
    fill->SetLayoutType(LAYOUT_SIMPLE);
    fill->SetAlign(ALIGN_LEFT | ALIGN_VCENTER);
    fill->SetFillType(ImageWindow::FILL_HORIZONTAL);
    fill->SetImageById("IDB_COOP_PROGRESS_BAR_FILL");
    fill->SetPercentWidth(percent, 0, 0);
    fillArea->AddToFront(fill);

    ImageWindow* right = new ImageWindow();
    right->SetFillType(ImageWindow::FILL_NONE);
    right->SetImageById("IDB_COOP_PROGRESS_BAR_RIGHT");
    bar->AddToFront(right, 2, 0);

    TextWindow* text = new TextWindow(XString::Format(L"%d/%d", progress, goal), CFontMgr::GetInstance()->GetFont(15));
    text->SetWidthByContent(0, 0);
    text->SetHeightByContent(0, 0);
    text->SetLayoutType(LAYOUT_SIMPLE);
    text->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    bar->AddToFront(text);
    return progressWindow;
}

Window* CChallengeWindow::CreateRewardInfo(const CChallenge* challenge)
{
    Window* reward = new Window(false);
    Window* arrows = new Window(false);
    arrows->SetPercentHeight(90, 0, 0);
    arrows->SetPercentWidth(100, 0, 0);
    arrows->SetLayoutType(LAYOUT_SIMPLE);
    arrows->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);

    Window* firstArrow = CreateArrowWindow();
    firstArrow->SetWidthByContent(0, 0);
    firstArrow->SetPercentHeight(75, 0, 0);
    firstArrow->m_debugFrame = true;
    firstArrow->SetLayoutType(LAYOUT_SIMPLE);
    firstArrow->SetAlign(ALIGN_RIGHT);
    Window* firstCell = new Window(false);
    firstCell->AddToFront(firstArrow);
    firstCell->SetPercentWidth(36, 0, 0);
    arrows->AddToFront(firstCell, 0, 0);
    firstCell->m_debugFrame = true;

    Window* secondArrow = CreateArrowWindow();
    secondArrow->SetWidthByContent(0, 0);
    secondArrow->SetPercentHeight(75, 0, 0);
    secondArrow->SetLayoutType(LAYOUT_SIMPLE);
    secondArrow->SetAlign(ALIGN_RIGHT);
    Window* secondCell = new Window(false);
    secondCell->AddToFront(secondArrow);
    secondCell->SetPercentWidth(36, 0, 0);
    arrows->AddToFront(secondCell, 1, 0);

    Window* lastCell = new Window(false);
    lastCell->SetPercentWidth(37, 0, 0);
    arrows->AddToFront(lastCell, 2, 0);
    reward->AddToFront(arrows);

    Window* tiers = new Window(false);
    tiers->SetPercentHeight(100, 0, 0);
    tiers->SetPercentWidth(100, 0, 0);
    tiers->SetLayoutType(LAYOUT_SIMPLE);
    tiers->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    tiers->AddToFront(CreateRewardInfoWindow(challenge->GetSoloReward(), 0), 0, 0);
    tiers->AddToFront(CreateRewardInfoWindow(challenge->GetX1Reward(), 1), 1, 0);
    tiers->AddToFront(CreateRewardInfoWindow(challenge->GetX2Reward(), 2), 2, 0);
    reward->AddToFront(tiers);
    return reward;
}

Window* CChallengeWindow::CreateRewardInfoWindow(const SChallengeReward* reward, int tier)
{
    Window* slot = new Window(false);
    Window* content = new Window(false);
    content->SetHeightByContent(0, 0);
    content->SetLayoutType(LAYOUT_SIMPLE);
    content->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    slot->AddToFront(content);
    Window* amountCell = new Window(false);
    amountCell->SetWidthByContent(0, 0);
    amountCell->SetHeightByContent(0, 0);
    content->AddToFront(amountCell, 0, 0);

    ImageWindow* image = new ImageWindow();
    const char* imageId = reward->m_image;
    image->SetLayoutType(LAYOUT_SIMPLE);
    image->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    image->SetImageById(imageId);
    amountCell->AddToFront(image, 0, 0);
    amountCell->m_debugFrame = true;

    if (reward->m_awarded) {
        ImageWindow* check = new ImageWindow();
        check->SetFillType(ImageWindow::FILL_NONE);
        check->SetImageById("IDB_COOP_CHECK_COOP");
        check->SetLayoutType(LAYOUT_SIMPLE);
        check->SetAlign(ALIGN_LEFT | ALIGN_VCENTER);
        int top = SScreenAdaptingConstant(10, 20)();
        int left = SScreenAdaptingConstant(20, 40)();
        check->SetOutsetSpacing(top, 0, 0, left);
        amountCell->AddToFront(check);
    } else if (reward->m_friendId > 0) {
        ImageWindow* check = new ImageWindow();
        check->SetFillType(ImageWindow::FILL_NONE);
        check->SetImageById("IDB_CHECKBOX_CHECK_GREY");
        check->SetLayoutType(LAYOUT_SIMPLE);
        check->SetAlign(ALIGN_LEFT | ALIGN_VCENTER);
        int top = SScreenAdaptingConstant(10, 20)();
        int left = SScreenAdaptingConstant(20, 40)();
        check->SetOutsetSpacing(top, 0, 0, left);
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
            if (amount < 1) {
                amount = -666;
            }
        }
    }
    TextWindow* label = new TextWindow(XString(amount), CFontMgr::GetInstance()->GetFont(15));
    label->SetLayoutType(LAYOUT_SIMPLE);
    label->SetAlign(ALIGN_LEFT | ALIGN_BOTTOM);
    amountCell->AddToFront(label, 1, 0);

    XString caption("");
    switch (tier) {
    case 0:
        caption = ResString("IDS_PLAYER");
        break;
    case 1:
        caption = ResString("IDS_1_FRIEND");
        break;
    case 2:
        caption = ResString("IDS_2_FRIENDS");
        break;
    }
    TextWindow* captionWindow = new TextWindow(caption, CFontMgr::GetInstance()->GetFont(15));
    content->AddToFront(captionWindow, 0, 1);
    return slot;
}

Window* CChallengeWindow::CreateArrowWindow()
{
    Window* arrow = new Window(false);
    arrow->SetWidthByContent(0, 0);
    ImageWindow* image = new ImageWindow();
    image->SetFillType(ImageWindow::FILL_NONE);
    image->SetImageById("IDB_COOP_ARROW_RIGHT");
    arrow->AddToFront(image);
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
    if (m_endHeight >= m_startHeight) {
        m_rewards->SetFlags(FLAG_VISIBLE | FLAG_ENABLED);
        EasyParams slide(m_rewards, (EasyLib::EMethod)0xe);
        slide.SetMoving(-m_rewards->m_width, m_rewards->m_y, m_rewards->m_x, m_rewards->m_y);
        slide.SetFullTime(0.2f);
        m_rewards->StartEasy(slide, FLAG_SHOW_AFTER_EASY);
    } else {
        m_rewards->ClearFlags(FLAG_VISIBLE | FLAG_ENABLED);
    }
}

void CChallengeWindow::Update()
{
    if (m_animationEnded) {
        m_animationEnded = false;
        OnHeightAnimEnded();
    }
    if (m_animating) {
        m_elapsedMs += WindowApp::m_instance->m_updateFrameTime;
        SetPercentHeight(m_heightPerMs * m_elapsedMs + m_startHeight, 0, 0);
        if (m_elapsedMs > m_animationTime) {
            m_animating = false;
            SetPercentHeight(m_endHeight, 0, 0);
            HandleUpdate(false);
            m_animationEnded = true;
        }
    }
    m_reservedC1 = false;
    if (m_inviteCooldownMs > 0) {
        m_inviteCooldownMs -= WindowApp::m_instance->m_updateFrameTime;
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
    CChallenge* challenge = static_cast<App*>(WindowApp::m_instance)->m_challengeManager->GetChallengeById(challengeId);
    if (challenge == NULL) {
        return;
    }

    CNGSRemoteUserList* friends = CNGS::GetInstance()->GetLocalUser()->m_remoteUserLists[0];
    CIncentivizedWindow* picker = NULL;
    for (int index = 0; index < friends->m_users.Size(); ++index) {
        int friendId = friends->m_users[index]->GetClientID();
        if (!challenge->IsFriendInvited(friendId)) {
            if (picker == NULL) {
                picker = new CIncentivizedWindow(XString("SHARE"));
                picker->SetEventReciever(this);
            }
            picker->AddFriend(XString(friends->m_users[index]->GetNickName().GetWCharPtr()), friendId);
        }
    }

    if (picker != NULL) {
        picker->SetCommand(SendInvites);
        picker->SetTitleText(ResString("IDS_CHALLENGE_INVITE_TITLE"));
        picker->ShowSelectAllButton();
        static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddModal(picker);
    } else {
        CTextDialog* warning = new CTextDialog();
        warning->SetMessageText(ResString("IDS_CHALLENGE_NO_FRIENDS_TO_INVITE"));
        warning->SetTitleText(ResString("IDS_CHALLENGE_INVITE_TITLE"));
        warning->SetButtons(CDialogWindow::BUTTON_OK);
        static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddModal(warning);
    }
}

void CChallengeWindow::HandleShareChallengePressed(Vector<int>& friendIds, int challengeId)
{
    CChallenge* challenge = static_cast<App*>(WindowApp::m_instance)->m_challengeManager->GetChallengeById(challengeId);
    CNGSLocalUser* local = CNGS::GetInstance()->GetLocalUser();
    CNGSRemoteUserList* friends = local->m_remoteUserLists[0];
    int index = 0;
    while (index < friendIds.size()) {
        CNGSRemoteUser* user = friends->findRemoteUserByID(friendIds[index]);
        if (user == NULL) {
            break;
        }
        XString id = XString::Format(L"%i", challengeId);
        CObjectMapString payload(CStrWChar(id.c_str()));
        XString message = XString::Format(ResString("IDS_CHALLENGE_INVITE_PUSH_TEXT").c_str(),
                                          local->GetNickName().GetWCharPtr(), challenge->m_name.c_str());
        XString button = ResString("IDS_CHALLENGE_PUSH_BUTTON");
        CNGSPushNotificationDetails* details = new CNGSPushNotificationDetails;
        {
            XString::AnsiString alertText(message.c_str());
            XString::AnsiString buttonText(button.c_str());
            details->m_buttonText.Concatenate(buttonText);
            details->m_alertText.Concatenate(alertText);
            user->SendMessage(&payload, InviteDurationSeconds, ChallengeInviteMessage, true, details);
        }
        ++index;
        challenge->FriendInvited(user->GetClientID());
        static_cast<App*>(WindowApp::m_instance)->m_challengeManager->Save();
    }
    if (index != 0) {
        CGameAnalytics::logChalengeShared(index);
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
        now = static_cast<App*>(WindowApp::m_instance)->m_challengeManager->GetTimeSec();
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
