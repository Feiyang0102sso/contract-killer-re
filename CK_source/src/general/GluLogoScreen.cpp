#include "GluLogoScreen.h"
#include "App.h"
#include "CColor.h"
#include "CMathFixed.h"
#include "CDH_ResourceManager.h"
#include "ICMediaPlayer.h"
#include "ICRenderSurface.h"

namespace {
const char* const LogoResource = "IDB_GLU";
const char* const SoundResource = "IDM_GLUTHEME";
const int LogoDuration = 3250;
}

CGluLogoScreen::CGluLogoScreen()
    : Window(false), m_logo(NULL), m_theme(NULL), m_elapsed(-1), m_soundEvent(0)
{
}

CGluLogoScreen::~CGluLogoScreen()
{
}

void CGluLogoScreen::Init()
{
    m_logo = static_cast<ICRenderSurface*>(
        static_cast<App*>(WindowApp::m_instance)->m_resourceManager->getResourceData(LogoResource, false));
    m_theme = static_cast<CMedia*>(
        static_cast<App*>(WindowApp::m_instance)->m_resourceManager->getResourceData(SoundResource, false));
    m_soundEvent = ICMediaPlayer::GetInstance()->Play(m_theme, 0, 0);
}

void CGluLogoScreen::Shutdown()
{
    if (m_soundEvent > 0) {
        ICMediaPlayer::GetInstance()->Stop(m_soundEvent);
    }
    ((App*)WindowApp::m_instance)->m_resourceManager->releaseResource(LogoResource);
}

void CGluLogoScreen::OnPointerEvent(Event& event)
{
    if (event.m_type == Event::POINTER_PRESSED) {
        SetInterrupt(1);
        event.Clear();
    }
}

void CGluLogoScreen::Update()
{
    if (m_elapsed < 0) {
        m_elapsed = 0;
    } else {
        m_elapsed += WindowApp::m_instance->m_updateFrameTime;
        if (m_elapsed >= LogoDuration) {
            ICMediaPlayer::GetInstance()->Stop(m_soundEvent);
            SetInterrupt(1);
        }
    }
}

void CGluLogoScreen::Paint(ICGraphics2d& graphics)
{
    unsigned int width;
    unsigned int height;
    ICGraphics::GetInstance()->GetTargetSurface()->GetWidthAndHeight(width, height);
    unsigned int logoWidth;
    unsigned int logoHeight;
    m_logo->GetWidthAndHeight(logoWidth, logoHeight);

    unsigned int center = (width + logoWidth) >> 1;
    int x = width - center - (logoWidth & 1);
    int elapsed = m_elapsed;
    int animationTime = elapsed - 250;
    int y = (height - logoHeight) >> 1;
    unsigned int frameWidth = logoWidth;
    if (animationTime < 1501) {
        if (animationTime > 0) {
            frameWidth = 97;
            int bounce = animationTime % 500 - 250;
            y = bounce * y * bounce / 62500;
            x += static_cast<int>((1750 - elapsed) * center) / 1500;
        }
    } else if (animationTime < 2001) {
        frameWidth = 97;
        if (animationTime > 1750) {
            frameWidth = 129;
        }
    }

    FillRect(graphics, 0, 0, width, height, Color_A8B8G8R8_t::Black.value);
    if (animationTime > 0) {
        int opacity = 0x10000;
        CRectangle source;
        source.m_x = 0;
        source.m_y = 0;
        source.m_width = frameWidth;
        source.m_height = logoHeight;
        if (animationTime > 2749) {
            opacity = 0;
            if (animationTime < LogoDuration) {
                opacity = 0x10000 - CMathFixed::Div((elapsed - 3000) * 0x10000, 500 * 0x10000);
            }
        }
        graphics.PushState(ICGraphics2d::State_Blend);
        graphics.PushState(ICGraphics2d::State_2);
        graphics.Enable(ICGraphics2d::State_Blend);
        graphics.SetBlendArg(ICGraphics2d::BlendArg_0);
        graphics.SetColor(opacity, 0, 0, 0);
        graphics.PushTransform();
        graphics.LoadIdentity();
        graphics.Translate(static_cast<unsigned int>(x) << 16, static_cast<unsigned int>(y) << 16);
        graphics.Draw(*m_logo, ICGraphics2d::Flip_None, &source);
        graphics.PopTransform();
        graphics.PopState(ICGraphics2d::State_2);
        graphics.PopState(ICGraphics2d::State_Blend);
    }
}
