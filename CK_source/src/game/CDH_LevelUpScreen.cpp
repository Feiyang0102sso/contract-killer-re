#include "CDH_LevelUpScreen.h"

#include "App.h"
#include "BaseDialog.h"
#include "CDH_CurrencyWidget.h"
#include "CFontMgr.h"
#include "DGHelper.h"

CDH_LevelUpScreen::CDH_LevelUpScreen()
{
}

CDH_LevelUpScreen::~CDH_LevelUpScreen()
{
}

void CDH_LevelUpScreen::OnStartupEasy()
{
    EasyParams ease(this, EasyLib::OUT_CIRC);
    ease.SetFullTimes(0.2f, 0.2f);
    ease.SetBubble(m_x + m_width / 2 - 75, m_y + m_height / 2 - 50, 150, 100);
    ease.m_clearFlagsOnStart = FLAG_PAINT_CHILDREN;
    ease.m_setFlagsOnFinish = FLAG_PAINT_CHILDREN;
    StartEasy(ease, FLAG_SHOW_AFTER_EASY);
}

void CDH_LevelUpScreen::Init()
{
    SetOutsetSpacing(-16);
    setCaptionText("", false);
    setButtons(NULL, "IDS_BTN_DONE");

    App* app = static_cast<App*>(WindowApp::m_instance);
    GamePlayManager& gameplay = app->m_helper->m_gamePlayManager;
    CDH_PlayerData* player = app->m_helper->GetPlayerData();
    int points = player->GetSkillPointsPerLevel();
    player->changeSkillPoints(points);
    CFontMgr* fonts = CFontMgr::GetInstance();

    Window* body = new Window(false);
    body->SetAlign(ALIGN_LEFT);
    body->SetDesiredWidth(300);
    body->SetHeightByContent(0, 0);

    TextWindow* level = new TextWindow(XString::Format(L"Level %d Achived", gameplay.GetLevel()),
                                       fonts->GetFont(6));
    level->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
    body->AddToFront(level, 0, 0);
    body->AddToFront(createLineSelector(0, 1));

    TextWindow* remaining = new TextWindow(XString::Format(L"%d XP remaining until the next level",
                                                          gameplay.GetXPRemaining()), fonts->GetFont(5));
    remaining->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    remaining->SetPercentWidth(100, 0, 0);
    body->AddToFront(remaining, 0, 2);

    CDH_CurrencyWidget* gained = new CDH_CurrencyWidget(CDH_CurrencyWidget::SkillPoints);
    gained->setText(XString("Skill points gained: %d"));
    gained->setValue(points);
    gained->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    body->AddToFront(gained, 0, 3);
    setLayoutWidget(body, Body);
    CDH_BaseScreen::Init();
}

void CDH_LevelUpScreen::OnCommand(Event& event)
{
    if (event.m_id == 0x07b18a47 || event.m_id == 0x21204844 || event.m_id == (int)0x97204784) {
        SetInterrupt(2);
        event.Clear();
    }
}
