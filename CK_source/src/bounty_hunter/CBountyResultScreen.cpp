#include "CBountyResultScreen.h"
#include "CMath.h"

#include "App.h"
#include "CBH_Player.h"
#include "CDH_SoundManager.h"
#include "CFontMgr.h"
#include "CFriendAvatarWindow.h"
#include "CGameAnalytics.h"
#include "CHuntingInfo.h"
#include "CNGS.h"
#include "CNGSLocalUser.h"
#include "ServiceWindows.h"
#include "SScreenAdaptingConstant.h"
#include "CNetworkAvailability.h"
#include "SG_Instance.h"
#include "SG_Presenter.h"

namespace {
const int AcceptResult = 0x21204833;
const int AcceptTargetResult = 0x6e7d5207;
const int AcceptRevengeResult = 0x6e7d5208;
SScreenAdaptingConstant resultSideInset(15, 30);
SScreenAdaptingConstant resultBottomInset(15, 30);
}

CBountyResultScreen::CBountyResultScreen(bool fadeIn)
    : m_image(NULL), m_stamp(NULL), m_row(0), m_accepted(false),
      m_field108(false), m_fadeIn(fadeIn), m_elapsed(0)
{
    m_leftFrame.Create("SUR_FRAME_PAUSE_LEFT");
    m_rightFrame.Create("SUR_FRAME_PAUSE_RIGHT");
    m_topFrame.Create("SUR_FRAME_PAUSE_TOP");
    m_bottomFrame.Create("SUR_FRAME_PAUSE_DOWN");
    m_bottomLeftFrame.Create("SUR_FRAME_PAUSE_LEFT_DOWN");
    m_topLeftFrame.Create("SUR_FRAME_PAUSE_LEFT_TOP");
    m_bottomRightFrame.Create("SUR_FRAME_PAUSE_RIGHT_DOWN");
    m_topRightFrame.Create("SUR_FRAME_PAUSE_RIGHT_TOP");
    m_centerFrame.Create("SUR_FRAME_PAUSE_CENTER");
    int right = resultSideInset();
    int bottom = resultBottomInset();
    int left = resultSideInset();
    SetInsetSpacing(0, right, bottom, left);
    if (fadeIn) {
        SetOpacity(0.0f);
    }
    Create();
}

CBountyResultScreen::~CBountyResultScreen()
{
}

void CBountyResultScreen::Create()
{
    SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    m_fieldfc = 400;
    m_field100 = 9;
    m_field104 = 0x24;
    SetWidthByContent(0, 0);
    SetHeightByContent(0, 0);
    CFont* body = CFontMgr::GetInstance()->GetFont(15);
    CFont* title = CFontMgr::GetInstance()->GetFont(9);
    bool friendTarget = false;
    int column = 0;
    // Left picture: the mission brief image, with the target's avatar on top of it.
    ImageWindow* picture = new ImageWindow();
    picture->SetFillType(static_cast<ImageWindow::EFillType>(0));
    AddToFront(picture);
    picture->ClearFlags(FLAG_VISIBLE | FLAG_ENABLED);
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (!app->m_huntingInfo->m_assignedData.m_hasFakeBoss) {
        if (!app->m_huntingInfo->m_assignedData.m_hasTarget && CBH_Player::GetInstance()->m_missionBriefTargetFB.GetLength() < 1) {
            const char* brief = CBH_Player::GetInstance()->m_missionBriefImage;
            CBH_Player* player = CBH_Player::GetInstance();
            if (brief != NULL) {
                // Original compares the pointer, not the text.
                if (player->m_missionBriefImage == "IDB_DIALOG_NOTEBOOK") {
                    CFriendAvatarWindow* avatar = new CFriendAvatarWindow(-1);
                    friendTarget = true;
                    avatar->SetLayoutType(LAYOUT_SIMPLE);
                    avatar->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
                    int top = SScreenAdaptingConstant(-28, -40)();
                    int left = SScreenAdaptingConstant(-10, -25)();
                    avatar->SetOutsetSpacing(top, 0, 0, left);
                    avatar->m_debugFrame = true;
                    avatar->SetDefaultImage("IDB_TARGETTING_DEFAULT_AVATAR");
                    picture->AddToFront(avatar);
                    picture->SetImageById(CBH_Player::GetInstance()->m_missionBriefImage);
                    picture->SetOutsetSpacing(SScreenAdaptingConstant(-9, -18)(), 0, 0, 0);
                } else {
                    picture->SetImageById(CBH_Player::GetInstance()->m_missionBriefImage);
                }
                column = 1;
            }
        } else {
            int friendId = CBH_Player::GetInstance()->m_targetFriendId;
            CStrWChar friendName(CBH_Player::GetInstance()->m_missionBriefTargetFB);
            CFriendAvatarWindow* avatar = NULL;
            if (friendId < 0) {
                if (friendName.GetLength() > 0) {
                    avatar = new CFriendAvatarWindow(CStrWChar(friendName));
                }
            } else {
                avatar = new CFriendAvatarWindow(friendId);
            }
            if (avatar != NULL) {
                friendTarget = true;
                avatar->SetLayoutType(LAYOUT_SIMPLE);
                avatar->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
                int top = SScreenAdaptingConstant(-28, -40)();
                int left = SScreenAdaptingConstant(-10, -25)();
                avatar->SetOutsetSpacing(top, 0, 0, left);
                avatar->m_debugFrame = true;
                avatar->SetDefaultImage("IDB_TARGETTING_DEFAULT_AVATAR");
                picture->AddToFront(avatar);
                picture->SetImageById(CBH_Player::GetInstance()->m_missionBriefImage);
                picture->SetOutsetSpacing(SScreenAdaptingConstant(-9, -18)(), 0, 0, 0);
                column = 1;
            }
        }
    }
    if (column == 1) {
        picture->SetCellPos(0, 1, 1, m_row - 1);
        picture->SetAlign(ALIGN_TOP);
        picture->SetFlags(FLAG_VISIBLE | FLAG_ENABLED);
    }
    if (static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->IsPlayerWin() == true) {
        XString complete = XString::Format(ResString("IDS_BOUNTY_CONTRACT_COMPLETE").c_str(),
                                           CBH_Player::GetInstance()->m_gameStats.m_successBounties + 1);
        AddTextLine(this, title, complete);
        int money = static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_moneyReward;
        AddStatisticLine(this, column, body, "IDS_BOUNTY_REWARDED_MONEY", money, XString("$ "));
        int experience = static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_experienceReward;
        AddStatisticLine(this, column, body, "IDS_BOUNTY_REWARDED_XP", experience, XString(""));
    } else {
        const char* failure = NULL;
        switch (static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_result) {
        case 1:
            failure = "IDS_BOUNTY_DEFEAT_DEATH";
            break;
        case 2:
            failure = "IDS_BOUNTY_DEFEAT_TRANQUILLIZE";
            break;
        case 5:
            failure = "IDS_BOUNTY_DEFEAT_ESCAPE";
            break;
        case 6:
            failure = "IDS_BOUNTY_DEFEAT_KILLING_CIVILIAN";
            break;
        case 7:
            failure = "IDS_BOUNTY_DEFEAT_OUT_OF_AMMO";
            break;
        case 8:
            failure = "IDS_BOUNTY_DEFEAT_MOVING_CAMERA_TIME_OUT";
            break;
        default:
            break;
        }
        if (failure != NULL) {
            AddTextLine(this, body, failure);
        }
    }
    int earnedExperience = static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_experienceEarned;
    AddStatisticLine(this, column, body, "IDS_BOUNTY_XP", earnedExperience, XString(""));
    int kills = static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_enemiesKilled;
    AddStatisticLine(this, column, body, "IDS_BOUNTY_KILLED", kills, XString(""));
    int tranquillized = static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_enemiesTranquillized;
    AddStatisticLine(this, column, body, "IDS_BOUNTY_TRANQUILLIZED", tranquillized, XString(""));
    AddButton(this, column);
    // Original: the stamp follows the picture column, win or lose.
    if (column == 1) {
        AddToFront(new CStampAnimWindow(friendTarget));
    }
}

void CBountyResultScreen::AddTextLine(Window* parent, CFont* font, XString text)
{
    if (parent != NULL && font != NULL) {
        TextWindow* line = new TextWindow(text.Upper(), font);
        line->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
        line->SetCellPos(0, m_row, 3, 1);
        parent->AddToFront(line);
        m_row++;
    }
}

void CBountyResultScreen::AddTextLine(Window* parent, CFont* font, const char* text)
{
    if (parent != NULL && font != NULL && text != NULL) {
        AddTextLine(parent, font, ResString(text));
    }
}

void CBountyResultScreen::AddStatisticLine(Window* parent, int column, CFont* font, const char* title,
                                           int number, XString prefix)
{
    if (parent == NULL || font == NULL || title == NULL) {
        return;
    }
    XString label = ResString(title);
    XString text = XString::Format(L"%s", label.c_str());
    TextWindow* name = new TextWindow(text, font);
    text = prefix + XString::Format(L"%d", number);
    TextWindow* value = new TextWindow(text, font);
    name->SetAlign(ALIGN_LEFT);
    value->SetAlign(ALIGN_RIGHT);
    name->SetOutsetSpacing(0, App::Scale() * 10, 0, 0);
    parent->AddToFront(name, column, m_row);
    parent->AddToFront(value, column + 1, m_row);
    m_row++;
}

void CBountyResultScreen::AddButton(Window* parent, int column)
{
    if (parent == NULL) {
        return;
    }
    StretchImageButton* button = new StretchImageButton(ResString("IDS_SK_OK"), "IDB_HUD_TRANSPARENT_BLACK_LEFT",
                                                        "IDB_HUD_TRANSPARENT_BLACK_TILE", "IDB_HUD_TRANSPARENT_BLACK_RIGHT");
    button->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    App* app = static_cast<App*>(WindowApp::m_instance);
    CHuntingInfo* hunt = app->m_huntingInfo;
    int command;
    if (!hunt->m_assignedData.m_hasTarget) {
        command = AcceptResult;
    } else if (!hunt->m_assignedData.m_revenge) {
        command = AcceptTargetResult;
    } else {
        command = AcceptRevengeResult;
    }
    button->SetCommandParameters(command, 0, 0, 0);
    button->SetOutsetSpacing(App::Scale() * 10, 0, 0, 0);
    int width = App::Scale() * 100;
    button->m_fixedWidth = width;
    button->SetSize(width, button->m_height);

    CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
    bool loggedIn = user->isValid() != 0 && user->isAuthenticated(SOCIAL_NETWORK_FACEBOOK) != 0;
    app = static_cast<App*>(WindowApp::m_instance);
    hunt = app->m_huntingInfo;
    bool noShare = false;
    if (!hunt->m_assignedData.m_revenge && !hunt->m_assignedData.m_hasTarget) {
        noShare = true;
    }
    bool online = app->m_networkAvailability->IsAvailable();

    Window* cell = new WindowTransparent(false);
    TextWindow* spacer = new TextWindow(L" ", CFontMgr::GetInstance()->GetFont(9));
    spacer->SetCellPos(0, 0, 1, 1);
    Window* buttonCell = createTD(button, 3);
    buttonCell->SetCellPos(0, 1, 1, 1);
    cell->AddToFront(spacer);
    cell->AddToFront(buttonCell);
    cell->SetHeightByContent(0, 0);
    cell->SetWidthByContent(0, 0);
    if (!noShare && online) {
        // Target/revenge results also offer a Facebook share button (shown when logged in).
        parent->AddToFront(cell, column, m_row);
        button->SetAlign(ALIGN_LEFT | ALIGN_VCENTER);
        StretchImageButton* share = new StretchImageButton(XString(L""), "IDB_HUD_TRANSPARENT_BLACK_LEFT",
                                                           "IDB_HUD_TRANSPARENT_BLACK_TILE", "IDB_HUD_TRANSPARENT_BLACK_RIGHT");
        share->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
        share->ForceSetWidth(SScreenAdaptingConstant(75, 150)());
        int shareCommand;
        if (!static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_revenge) {
            shareCommand = 0x6e7d5205;
        } else {
            shareCommand = 0x6e7d5206;
        }
        share->SetCommandParameters(shareCommand, 0, 0, 0);
        LabelWithIcon* label = new LabelWithIcon("IDB_FACEBOOK_ICON", ResString("IDS_DIALOG_BUTTON_FB_SHARE"), false,
                                                 CFontMgr::GetInstance()->GetFont(15), 0);
        label->SetPercentHeight(80, 0, 0);
        label->SetPercentWidth(73, 0, 0);
        label->SetLayoutType(LAYOUT_SIMPLE);
        label->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
        share->AddToFront(label);
        share->SetOutsetSpacing(0, 0, 0, 0);
        Window* shareCell = new WindowTransparent(false);
        Window* shareButtonCell = createTD(share, 3);
        shareButtonCell->SetCellPos(0, 1, 1, 1);
        shareCell->AddToFront(shareButtonCell);
        shareCell->SetHeightByContent(0, 0);
        shareCell->SetWidthByContent(0, 0);
        if (loggedIn) {
            parent->AddToFront(shareCell, column + 1, m_row);
        }
    } else {
        parent->AddToFront(cell);
        cell->SetCellPos(column, m_row, 2, 1);
    }
    m_row++;
}

void CBountyResultScreen::OnEasyFinished()
{
}

void CBountyResultScreen::OnStartupEasy()
{
}

void CBountyResultScreen::OnCommand(Event& event)
{
    switch (event.m_id) {
    case 0x2074d009:
    case AcceptResult:
        m_accepted = true;
        Close();
        event.Clear();
        if (CBH_Player::GetInstance()->m_tutorialState == 0) {
            CGameAnalytics::logTutorialEvents(CGameAnalytics::TUTORIAL_EVENT_FIRST_MISSION_RESULT);
            return;
        }
        if (CBH_Player::GetInstance()->m_tutorialState < 13) {
            CGameAnalytics::logTutorialEvents(static_cast<CGameAnalytics::ETutorialEvent>(3));
        }
        return;
    case 0x21204844:
    case AcceptTargetResult:
    case AcceptRevengeResult:
        Close();
        return;
    }
}

void CBountyResultScreen::Update()
{
    if (m_fadeIn) {
        m_elapsed += WindowApp::m_instance->m_updateFrameTime;
        // the original only clamps the upper end
        SetOpacity(CMath::Min(1.0f, m_elapsed / 600.0f - 0.3f));
    }
}

CStampAnimWindow::CStampAnimWindow(bool friendTarget)
    : Window(false)
{
    SG_Instance* sprites = new SG_Instance();
    m_sprites = sprites;
    const char* resources = "STAMPS_WVGA";
    if (!App::IsWVGA()) {
        if (App::IsHD()) {
            resources = "STAMPS_VGA";
        } else {
            resources = "STAMPS_HVGA";
        }
    }
    sprites->Init(const_cast<char*>(resources));
    sprites = m_sprites;
    unsigned char finished = 0;
    if (!App::IsWVGA()) {
        App::IsHD();
    }
    sprites->QueueArchetypeCharacter(0, 0);
    while (sprites->LoadQueued(1000, finished)) {
    }
    m_stamp = new SG_Presenter(0, 0);
    m_stamp->SetSGInstance(sprites);
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (app->m_huntingInfo->m_assignedData.m_tranquillize) {
        m_stamp->SetAnimation(1);
    } else {
        int animation = 0;
        if (friendTarget == true) {
            CNGSLocalUser* player = CNGS::GetInstance()->GetLocalUser();
            if (player->isValid()) {
                if (player->isAuthenticated(SOCIAL_NETWORK_FACEBOOK)) {
                    animation = 2;
                }
            }
        }
        m_stamp->SetAnimation(animation);
    }
    m_stamp->SetLoop(false);
    m_soundPlayed = false;
}

CStampAnimWindow::~CStampAnimWindow()
{
    SG_Instance* sprites = static_cast<App*>(WindowApp::m_instance)->m_sgInstance;
    if (!App::IsWVGA()) {
        App::IsHD();
    }
    sprites->DumpArchetypeCharacter(0, 0);
    if (m_stamp != NULL) {
        delete m_stamp;
        m_stamp = NULL;
    }
    if (m_sprites != NULL) {
        delete m_sprites;
        m_sprites = NULL;
    }
}

void CStampAnimWindow::Paint(ICGraphics2d&)
{
    m_stamp->Draw(m_width / 2, m_height / 2);
}

void CStampAnimWindow::Update()
{
    m_stamp->Update(WindowApp::m_instance->m_updateFrameTime);
    if (m_stamp->GetFrameIndex() == 1 && !m_soundPlayed) {
        static_cast<App*>(WindowApp::m_instance)->m_soundManager->PlaySound("IDM_MISSION_COMPLETE", false, false);
        m_soundPlayed = true;
    }
}

typedef char AssertBountyResultSize[sizeof(CBountyResultScreen) == 0x110 ? 1 : -1];
typedef char AssertStampAnimSize[sizeof(CStampAnimWindow) == 0xb0 ? 1 : -1];
