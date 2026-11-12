#include "GluLogoScreen.h"
#include "App.h"
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
    CDH_ResourceManager* resources = ((App*)WindowApp::m_instance)->m_resourceManager;
    m_logo = (ICRenderSurface*)resources->getResourceData(LogoResource, false);
    m_theme = (CMedia*)resources->getResourceData(SoundResource, false);
    if (m_theme != NULL) {
        m_soundEvent = ICMediaPlayer::GetInstance()->Play(m_theme, 0, 0);
    }
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
    FillRect(graphics, 0, 0, width, height, 0xff000000);
    int animationTime = m_elapsed - 250;
    if (animationTime <= 0 || m_logo == NULL) {
        return;
    }

    unsigned int logoWidth;
    unsigned int logoHeight;
    m_logo->GetWidthAndHeight(logoWidth, logoHeight);
    int x = (int)(width - logoWidth) / 2;
    int y = (int)(height - logoHeight) / 2;
    if (animationTime < 1501) {
        int bounce = animationTime % 500 - 250;
        y = bounce * y * bounce / 62500;
        x += (1750 - m_elapsed) * ((width + logoWidth) >> 1) / 1500;
        logoWidth = 97;
    } else if (animationTime < 2001) {
        logoWidth = 97;
        if (animationTime > 1750) {
            logoWidth = 129;
        }
    }

    float opacity = 1.0f;
    if (animationTime > 2749) {
        opacity = (LogoDuration - animationTime) / 500.0f;
        if (opacity < 0.0f) {
            opacity = 0.0f;
        }
    }
    CRectangle source;
    source.m_x = 0;
    source.m_y = 0;
    source.m_width = logoWidth;
    source.m_height = logoHeight;
    BeginBlend(graphics, opacity, 0.0f, 0.0f, 0.0f, false, false);
    graphics.PushTransform();
    graphics.Translate(x << 16, y << 16);
    graphics.Draw(*m_logo, ICGraphics2d::Flip_None, &source);
    graphics.PopTransform();
    EndBlend(graphics);
}
