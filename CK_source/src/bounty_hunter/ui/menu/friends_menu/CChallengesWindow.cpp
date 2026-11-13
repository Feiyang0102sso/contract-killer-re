#include "CChallengesWindow.h"

#include "App.h"
#include "CChallenge.h"
#include "CChallengeManager.h"
#include "CChallengeWindow.h"
#include "CFontMgr.h"
#include "CStdUtil_Cocoa.h"
#include "SScreenAdaptingConstant.h"

namespace {
const int ChallengePressed = 0x33e023ab;
const int ExpandedHeight = 60;
const int CollapsedHeight = 40;
const int HeightAnimationMs = 300;
}

CChallengesWindow::CChallengesWindow()
    : Window(false), m_unknownA4(-1), m_page(NULL)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_challengeManager->CheckChallengesFinishByTime();
    CreateUI();
}

void CChallengesWindow::CreateUI()
{
    // The original only builds the timer row and the challenge list.
    m_page = new Window(false);
    Window* timer = CreateTimer();
    m_page->AddToFront(timer, 0, 0);
    timer->SetPercentHeight(10, 0, 0);
    Window* list = CreateChallengesList();
    m_page->AddToFront(list, 0, 1);
    AddToFront(m_page);
}

Window* CChallengesWindow::CreateTimer()
{
    Window* timerPanel = new Window(false);
    timerPanel->SetInsetSpacing(SScreenAdaptingConstant(5, 10)(), 0, 0, 0);

    Window* column = new Window(false);
    TextWindow* caption = new TextWindow();
    caption->SetText(ResString("IDS_CHALLENGE_TIME_REMAINING"), CFontMgr::GetInstance()->GetFont(15));
    column->AddToFront(caption, 0, 0);
    caption->SetPercentWidth(50, 0, 0);

    CChallengeTimerWindow* timer = new CChallengeTimerWindow(
        static_cast<App*>(WindowApp::m_instance)->m_challengeManager->GetChallengesFinishTime(),
                                                             CFontMgr::GetInstance()->GetFont(8));
    timer->SetLayoutType(LAYOUT_SIMPLE);
    timer->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    Window* timerHolder = new Window(false);
    timerHolder->AddToFront(timer);

    // Progress bar frame: left cap, tiled middle, right cap.
    Window* bar = new Window(false);
    ImageWindow* left = new ImageWindow();
    left->SetFillType(ImageWindow::FILL_NONE);
    left->SetImageById("IDB_HUD_PROGRESSBAR_LEFT");
    bar->AddToFront(left, 0, 0);
    ImageWindow* middle = new ImageWindow();
    middle->SetFillType(ImageWindow::FILL_HORIZONTAL);
    middle->SetImageById("IDB_HUD_PROGRESSBAR");
    bar->AddToFront(middle, 1, 0);
    ImageWindow* right = new ImageWindow();
    right->SetFillType(ImageWindow::FILL_NONE);
    right->SetImageById("IDB_HUD_PROGRESSBAR_RIGHT");
    bar->AddToFront(right, 2, 0);
    bar->AddToFront(timerHolder);
    bar->m_debugFrame = true;
    column->AddToFront(bar, 1, 0);
    bar->SetLayoutType(LAYOUT_SIMPLE);
    bar->SetAlign(10);
    bar->SetHeightByContent(0, 0);
    bar->SetPercentWidth(50, 0, 0);

    timerPanel->AddToFront(column);
    column->SetLayoutType(LAYOUT_SIMPLE);
    column->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    column->SetPercentWidth(45, 0, 0);
    column->SetPercentHeight(100, 0, 0);
    column->m_debugFrame = true;
    return timerPanel;
}

Window* CChallengesWindow::CreateChallengesList()
{
    Window* list = new Window(false);
    m_cards.removeAllElements();
    App* app = static_cast<App*>(WindowApp::m_instance);
    Vector<CChallenge*> challenges = app->m_challengeManager->GetActiveChallenges();
    for (int index = 0; index < challenges.size(); ++index) {
        const CChallenge* challenge = challenges[index];
        CChallengeWindow* card = new CChallengeWindow(challenge);
        card->SetPercentHeight(60, 0, 0);
        list->AddToFront(card, 0, index);
        card->SetOutsetSpacing(SScreenAdaptingConstant(0, 0, 10)(), SScreenAdaptingConstant(0, 0, 50)());
        m_cards.addElement(card);
    }
    if (m_cards.size() > 0) {
        HandleChallengeWindowPressed(m_cards[0]);
    }
    return list;
}

void CChallengesWindow::HandleChallengeWindowPressed(CChallengeWindow* selected)
{
    for (int index = 0; index < m_cards.size(); ++index) {
        CChallengeWindow* card = m_cards[index];
        if (card != selected) {
            card->StartHeightAnim(card->GetPercentHeight(), CollapsedHeight, HeightAnimationMs);
        }
    }
    selected->StartHeightAnim(selected->GetPercentHeight(), ExpandedHeight, HeightAnimationMs);
}

void CChallengesWindow::OnCommand(Event& event)
{
    if (event.m_id == ChallengePressed) {
        HandleChallengeWindowPressed(reinterpret_cast<CChallengeWindow*>(event.m_param1));
    }
}
