#include "CLoadingScreen.h"

#include "App.h"
#include "BaseDialog.h"
#include "CDH_LoadingBarWidget.h"
#include "CFontMgr.h"
#include "CSwerveGame.h"
#include "CTip.h"
#include "CTipManager.h"
#include "DGHelper.h"

CLoadingScreen::CLoadingScreen(CSwerveGame* game)
    : Window(false), m_game(game), m_bar(NULL), m_tip(NULL), m_currentStep(0), m_ready(false)
{
    m_steps.ensureCapacity(10);
}

CLoadingScreen::~CLoadingScreen()
{
}

void CLoadingScreen::AddLoadingStep(LoadingStep step)
{
    m_steps.addElement(step);
}

void CLoadingScreen::Init()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    m_backgroundId = app->m_helper->GetRandomBackgroundResId();
    ImageWindow* background = new ImageWindow();
    XString::AnsiString id(m_backgroundId.c_str());
    background->SetImageById(id);
    background->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    AddToFront(background);

    m_bar = new CDH_LoadingBarWidget();
    m_bar->m_progress = 0;
    AddToFront(m_bar);
    m_tip = app->m_tipManager->GetRandomTip();
    if (m_tip != NULL) {
        CFont* font = CFontMgr::GetInstance()->GetFont(GAME_FONT_DEFAULT);
        if (font != NULL) {
            WindowTransparent* panel = new WindowTransparent(false);
            panel->SetAlign(ALIGN_RIGHT | ALIGN_VCENTER);
            panel->SetPercentWidth(50, 0, 0);
            TextWindow* text = new TextWindow(m_tip->m_description, font);
            text->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
            text->SetPercentWidth(100, 0, 0);
            text->SetPercentHeight(100, 0, 0);
            panel->AddToFront(text);
            AddToFront(panel);
        }
    }
}

void CLoadingScreen::Update()
{
    if (m_ready) {
        return;
    }
    if (m_steps.size() <= m_currentStep) {
        m_bar->setState(CDH_LoadingBarWidget::STATE_TAP);
        m_ready = true;
        return;
    }
    LoadingStep step = m_steps[m_currentStep++];
    (m_game->*step)();
    m_bar->m_progress = m_currentStep * 100 / m_steps.size();
}

void CLoadingScreen::OnPointerPressed(Event& event)
{
    if (m_ready) {
        SetInterrupt(1);
        event.Clear();
    }
}

typedef char AssertLoadingScreenSize[sizeof(CLoadingScreen) == 0xcc ? 1 : -1];
