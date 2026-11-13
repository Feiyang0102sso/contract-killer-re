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

    CFont* titleFont = CFontMgr::GetInstance()->GetFont(6);

    Window* body = new Window(false);
    body->SetAlign(ALIGN_LEFT);
    body->SetDesiredWidth(300);
    body->SetHeightByContent(0, 0);

    int level = static_cast<App*>(WindowApp::m_instance)->m_helper->m_gamePlayManager.GetLevel();
    int xpRemaining = static_cast<App*>(WindowApp::m_instance)->m_helper->m_gamePlayManager.GetXPRemaining();
    int points = static_cast<App*>(WindowApp::m_instance)->m_helper->GetPlayerData()->GetSkillPointsPerLevel();
    static_cast<App*>(WindowApp::m_instance)->m_helper->GetPlayerData()->changeSkillPoints(points);

    // Each row is a content-height window holding one widget
    Window* row = new Window(false);
    row->SetHeightByContent(0, 0);
    body->AddToFront(row, 0, 0);
    TextWindow* levelText = new TextWindow(XString::Format(L"Level %d Achived", level), titleFont);
    levelText->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
    row->AddToFront(levelText);

    body->AddToFront(createLineSelector(0, 1));

    row = new Window(false);
    row->SetHeightByContent(0, 0);
    body->AddToFront(row, 0, 2);
    TextWindow* remaining = new TextWindow(XString::Format(L"%d XP remaining until the next level", xpRemaining),
                                           CFontMgr::GetInstance()->GetFont(5));
    remaining->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    remaining->SetPercentWidth(100, 0, 0);
    row->AddToFront(remaining);

    row = new Window(false);
    row->SetHeightByContent(0, 0);
    body->AddToFront(row, 0, 3);
    CDH_CurrencyWidget* gained = new CDH_CurrencyWidget(CDH_CurrencyWidget::SkillPoints);
    gained->setText(XString("Skill points gained: %d"));
    gained->setValue(points);
    gained->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    row->AddToFront(gained);
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
