#include "CLoadingScreen.h"

#include "App.h"
#include "BaseDialog.h"
#include "CDH_LoadingBarWidget.h"
#include "CFontMgr.h"
#include "CSwerveGame.h"
#include "CTip.h"
#include "CTipManager.h"
#include "DGHelper.h"
#include "CBH_SurfaceManager.h"
#include "CResourceManager.h"

CLoadingScreen::CLoadingScreen(CSwerveGame* game)
    : Window(false), m_game(game), m_bar(NULL), m_tip(NULL), m_currentStep(0), m_ready(false)
{
    m_steps.ensureCapacity(10);
}

CLoadingScreen::~CLoadingScreen()
{
    CResourceManager::GetInstance()->ReleaseResource("SUR_TITLE");
    CBH_SurfaceManager* surfaces = static_cast<App*>(WindowApp::m_instance)->m_surfaceManager;
    surfaces->ReleaseSurface(XString::AnsiString(m_backgroundId.c_str()));
}

void CLoadingScreen::AddLoadingStep(LoadingStep step)
{
    m_steps.addElement(step);
}

void CLoadingScreen::Init()
{
    // Original: preloads the title surface into a local that is never used.
    ICRenderSurface* title;
    static_cast<App*>(WindowApp::m_instance)->m_helper->LoadImageSurface(&title, "SUR_TITLE", false);
    ImageWindow* background = new ImageWindow();
    m_backgroundId = static_cast<App*>(WindowApp::m_instance)->m_helper->GetRandomBackgroundResId();
    background->SetImage(static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->CreateSurface(XString::AnsiString(m_backgroundId.c_str()), true));
    background->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    AddToFront(background);

    m_bar = new CDH_LoadingBarWidget();
    m_bar->m_progress = 0;
    AddToFront(m_bar);
    m_tip = static_cast<App*>(WindowApp::m_instance)->m_tipManager->GetRandomTip();
    if (m_tip != NULL) {
        CFont* font = CFontMgr::GetInstance()->GetFont(GAME_FONT_DEFAULT);
        if (font != NULL) {
            WindowTransparent* panel = new WindowTransparent(false);
            panel->SetAlign(ALIGN_RIGHT | ALIGN_VCENTER);
            panel->SetPercentWidth(50, 0, 0);
            TextWindow* text = new TextWindow();
            text->SetText(m_tip->m_description, font);
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
    int currentStep = m_currentStep;
    if (currentStep >= m_steps.size()) {
        m_bar->setState(CDH_LoadingBarWidget::STATE_TAP);
        m_ready = true;
        return;
    }
    LoadingStep step = m_steps[currentStep];
    m_currentStep = currentStep + 1;
    (m_game->*step)();
    m_bar->m_progress = (int)((unsigned int)m_currentStep * 100) / m_steps.size();
}

void CLoadingScreen::OnPointerPressed(Event& event)
{
    if (m_ready) {
        SetInterrupt(1);
        event.Clear();
    }
}

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AssertLoadingScreenSize[sizeof(CLoadingScreen) == 0xcc ? 1 : -1];
#endif
