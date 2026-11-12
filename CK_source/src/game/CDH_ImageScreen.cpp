#include "CDH_ImageScreen.h"
#include "BaseDialog.h"
#include "WindowApp.h"

CDH_ImageScreen::CDH_ImageScreen(unsigned int color, const char* image, int duration)
    : Window(false), m_color(color), m_remainingMs(duration)
{
    SetDesiredWidth(WindowApp::m_instance->m_screenWidth);
    SetDesiredHeight(WindowApp::m_instance->m_screenHeight);
    ImageWindow* artwork = new ImageWindow(ImageWindow::FILL_NONE, ImageRes(image));
    artwork->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    AddToFront(artwork);
}

CDH_ImageScreen::~CDH_ImageScreen()
{
}

void CDH_ImageScreen::OnPointerEvent(Event& event)
{
    if (event.m_type == Event::POINTER_PRESSED) {
        SetInterrupt(1);
        event.Clear();
    }
}

void CDH_ImageScreen::Update()
{
    Window::Update();
    m_remainingMs -= WindowApp::m_instance->m_updateFrameTime;
    if (m_remainingMs < 1) {
        SetInterrupt(1);
    }
}

void CDH_ImageScreen::Paint(ICGraphics2d& graphics)
{
    FillRect(graphics, 0, 0, m_width, m_height, m_color);
}
