// CBH_GPSScan.cpp - load, draw and release the map scan animation.
#include "CBH_GPSScan.h"

#include "App.h"
#include "ICGraphics2d.h"
#include "SG_Instance.h"
#include "SG_Presenter.h"

CBH_GPSScan::CBH_GPSScan()
{
    m_field30 = 15;
    m_visible = false;
    CreateScanAnimation();
    m_field34 = 0;
    m_field24 = 0;
    m_field28 = 0;
    m_field2c = 0;
    m_presenter->Reset();
}

void CBH_GPSScan::CreateScanAnimation()
{
    m_instance = new SG_Instance();
    const char* archetypes = "BOUNTY_HUNTER_HVGA";
    if (App::IsHD()) {
        archetypes = "BOUNTY_HUNTER_VGA";
    }
    m_instance->Init(const_cast<char*>(archetypes));
    SG_Instance* instance = m_instance;
    unsigned char state = 0;
    App::IsHD();
    instance->QueueArchetypeCharacter(0, 0);
    while (instance->LoadQueued(1000, state) != 0) {
    }
    m_presenter = new SG_Presenter(0, 0);
    m_presenter->SetSGInstance(instance);
    m_presenter->SetAnimation(0);
    m_presenter->SetLoop(0);
}

void CBH_GPSScan::Paint(int x, int y)
{
    if (m_visible) {
        ICGraphics2d::GetInstance();
        m_presenter->Draw(m_x + x, m_y + y);
    }
}

CBH_GPSScan::~CBH_GPSScan()
{
    DeleteScanAnimation();
    delete m_instance;
    m_instance = NULL;
}

void CBH_GPSScan::DeleteScanAnimation()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    SG_Instance* instance = app->m_sgInstance;
    App::IsHD();
    instance->DumpArchetypeCharacter(0, 0);
    delete m_presenter;
    m_presenter = NULL;
}
