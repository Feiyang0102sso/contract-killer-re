#include "CBountyResultScreen.h"

#include "App.h"
#include "CBH_Player.h"
#include "CDH_SoundManager.h"
#include "CFontMgr.h"
#include "CFriendAvatarWindow.h"
#include "CHuntingInfo.h"
#include "CNGS.h"
#include "CNGSLocalUser.h"
#include "ServiceWindows.h"
#include "SG_Instance.h"
#include "SG_Presenter.h"

namespace {
const int AcceptResult = 0x21204833;
const int AcceptTargetResult = 0x6e7d5207;
const int AcceptRevengeResult = 0x6e7d5208;
const char* const FailureTitles[] = {
    NULL, "IDS_BOUNTY_DEFEAT_DEATH", "IDS_BOUNTY_DEFEAT_TRANQUILLIZE", NULL, NULL,
    "IDS_BOUNTY_DEFEAT_ESCAPE", "IDS_BOUNTY_DEFEAT_KILLING_CIVILIAN", "IDS_BOUNTY_DEFEAT_OUT_OF_AMMO",
    "IDS_BOUNTY_DEFEAT_MOVING_CAMERA_TIME_OUT"
};
}

CBountyResultScreen::CBountyResultScreen(bool fadeIn)
    : m_image(NULL), m_stamp(NULL), m_row(0), m_accepted(false), m_fieldfc(0), m_field100(0), m_field104(0),
      m_field108(false), m_fadeIn(fadeIn), m_elapsed(0)
{
    const char* const resources[] = {
        "SUR_FRAME_PAUSE_LEFT", "SUR_FRAME_PAUSE_RIGHT", "SUR_FRAME_PAUSE_TOP", "SUR_FRAME_PAUSE_DOWN",
        "SUR_FRAME_PAUSE_LEFT_TOP", "SUR_FRAME_PAUSE_RIGHT_TOP", "SUR_FRAME_PAUSE_LEFT_DOWN",
        "SUR_FRAME_PAUSE_RIGHT_DOWN", "SUR_FRAME_PAUSE_CENTER"
    };
    SetFrame(resources);
    int margin = App::Scale() * 10;
    SetInsetSpacing(0, margin, margin, margin);
    if (m_fadeIn) {
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
    SetWidthByContent(0, 0);
    SetHeightByContent(0, 0);
    m_fieldfc = 400;
    m_field100 = 9;
    m_field104 = 0x24;

    App* app = static_cast<App*>(WindowApp::m_instance);
    CHuntingInfo* hunt = app->m_huntingInfo;
    CBH_Player* player = CBH_Player::GetInstance();
    CFontMgr* fonts = CFontMgr::GetInstance();
    CFont* body = fonts->GetFont(15);
    int column = 0;
    if (!hunt->m_assignedData.m_hasFakeBoss && hunt->m_assignedData.m_hasTarget) {
        CFriendAvatarWindow* picture = new CFriendAvatarWindow(player->m_targetFriendId);
        picture->SetDefaultImage("IDB_TARGETTING_DEFAULT_AVATAR");
        picture->SetAlign(ALIGN_TOP);
        m_image = picture;
        column = 1;
    }
    if (hunt->IsPlayerWin()) {
        XString title = ResString("IDS_BOUNTY_CONTRACT_COMPLETE");
        AddTextLine(this, fonts->GetFont(9), XString::Format(title.c_str(), player->m_gameStats.m_successBounties + 1));
        AddStatisticLine(this, column, body, "IDS_BOUNTY_REWARDED_MONEY", hunt->m_assignedData.m_moneyReward, XString("$ "));
        AddStatisticLine(this, column, body, "IDS_BOUNTY_REWARDED_XP", hunt->m_assignedData.m_experienceReward, XString());
    } else if (hunt->m_result >= BountyResultLose && hunt->m_result <= BountyResultCameraTimeOut) {
        AddTextLine(this, body, FailureTitles[hunt->m_result]);
    }
    AddStatisticLine(this, column, body, "IDS_BOUNTY_XP", hunt->m_experienceEarned, XString());
    AddStatisticLine(this, column, body, "IDS_BOUNTY_KILLED", hunt->m_enemiesKilled, XString());
    AddStatisticLine(this, column, body, "IDS_BOUNTY_TRANQUILLIZED", hunt->m_enemiesTranquillized, XString());
    AddButton(this, column);

    if (m_image != NULL) {
        m_image->SetCellPos(0, 1, 1, m_row - 1);
        AddToFront(m_image);
        if (hunt->IsPlayerWin()) {
            m_stamp = new CStampAnimWindow(hunt->m_assignedData.m_friendGated);
            m_stamp->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
            AddToFront(m_stamp);
        }
    }
}

void CBountyResultScreen::AddTextLine(Window* parent, CFont* font, XString text)
{
    if (parent != NULL && font != NULL) {
        TextWindow* line = new TextWindow(text.Upper(), font);
        line->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
        line->SetCellPos(0, m_row++, 3, 1);
        parent->AddToFront(line);
    }
}

void CBountyResultScreen::AddTextLine(Window* parent, CFont* font, const char* text)
{
    if (text != NULL) {
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
    label.Append(XString(L":"));
    TextWindow* name = new TextWindow(label, font);
    TextWindow* value = new TextWindow(prefix + XString(number), font);
    name->SetAlign(ALIGN_LEFT);
    value->SetAlign(ALIGN_RIGHT);
    name->SetOutsetSpacing(0, App::Scale() * 10, 0, 0);
    parent->AddToFront(name, column, m_row);
    parent->AddToFront(value, column + 1, m_row++);
}

void CBountyResultScreen::AddButton(Window* parent, int)
{
    StretchImageButton* button = new StretchImageButton(ResString("IDS_SK_OK"), "IDB_HUD_TRANSPARENT_BLACK_LEFT",
                                                        "IDB_HUD_TRANSPARENT_BLACK_TILE", "IDB_HUD_TRANSPARENT_BLACK_RIGHT");
    button->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    button->ForceSetWidth(App::Scale() * 100);
    button->m_command = AcceptResult;
    App* app = static_cast<App*>(WindowApp::m_instance);
    CHuntingInfo* hunt = app->m_huntingInfo;
    if (hunt->m_assignedData.m_hasTarget && hunt->IsPlayerWin() && CBH_Player::GetInstance()->m_targetFriendId > 0) {
        if (hunt->m_assignedData.m_revenge) {
            button->m_command = AcceptRevengeResult;
        } else {
            button->m_command = AcceptTargetResult;
        }
    }
    button->SetCellPos(0, m_row++, 3, 1);
    parent->AddToFront(button);
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
        break;
    case 0x21204844:
    case 0x6e7d5207:
    case 0x6e7d5208:
        Close();
        break;
    }
}

void CBountyResultScreen::Update()
{
    if (m_fadeIn) {
        m_elapsed += WindowApp::m_instance->m_updateFrameTime;
        float opacity = m_elapsed / 600.0f - 0.3f;
        if (opacity < 0.0f) {
            opacity = 0.0f;
        } else if (opacity > 1.0f) {
            opacity = 1.0f;
        }
        SetOpacity(opacity);
    }
}

CStampAnimWindow::CStampAnimWindow(bool friendTarget)
    : Window(false), m_sprites(new SG_Instance()), m_stamp(NULL), m_soundPlayed(false)
{
    char hvga[] = "STAMPS_HVGA";
    char vga[] = "STAMPS_VGA";
    char wvga[] = "STAMPS_WVGA";
    char* resources = hvga;
    if (App::IsWVGA()) {
        resources = wvga;
    } else if (App::IsHD()) {
        resources = vga;
    }
    m_sprites->Init(resources);
    m_sprites->QueueArchetypeCharacter(0, 0);
    unsigned char finished = 0;
    while (m_sprites->LoadQueued(1000, finished)) {
    }
    m_stamp = new SG_Presenter(0, 0);
    m_stamp->SetSGInstance(m_sprites);
    App* app = static_cast<App*>(WindowApp::m_instance);
    int animation = 0;
    if (app->m_huntingInfo->m_assignedData.m_tranquillize) {
        animation = 1;
    } else if (friendTarget) {
        CNGSLocalUser* player = CNGS::GetInstance()->GetLocalUser();
        if (player != NULL && player->isValid() && player->isAuthenticated(2)) {
            animation = 2;
        }
    }
    m_stamp->SetAnimation(animation);
    m_stamp->SetLoop(false);
}

CStampAnimWindow::~CStampAnimWindow()
{
    delete m_stamp;
    delete m_sprites;
}

void CStampAnimWindow::Paint(ICGraphics2d&)
{
    m_stamp->Draw(m_width / 2, m_height / 2);
}

void CStampAnimWindow::Update()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    m_stamp->Update(app->m_updateFrameTime);
    if (m_stamp->GetFrameIndex() == 1 && !m_soundPlayed) {
        app->m_soundManager->PlaySound("IDM_MISSION_COMPLETE", false, false);
        m_soundPlayed = true;
    }
}

typedef char AssertBountyResultSize[sizeof(CBountyResultScreen) == 0x110 ? 1 : -1];
typedef char AssertStampAnimSize[sizeof(CStampAnimWindow) == 0xb0 ? 1 : -1];
