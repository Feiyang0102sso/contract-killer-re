// CImageButton.cpp - image / text button of the game UI (CK 1.5.2, written by game2).
#include "CImageButton.h"
#include "BaseDialog.h"
#include "WindowApp.h"
#include "App.h"
#include "CBH_SurfaceManager.h"
#include "CFontMgr.h"

void CImageButton::AddImage(const char* name, int x, int y)
{
    m_imageNames.addElement(XString(name));
    AddImage(((App*)WindowApp::m_instance)->m_surfaceManager->CreateSurface(name, true), x, y);
}

void CImageButton::AddImage(ICRenderSurface* surface, int x, int y)
{
    ImageWindow* image = new ImageWindow();
    image->SetFillType(ImageWindow::FILL_NONE);
    image->SetImage(surface);
    if (m_width < Window::ImageWidth(surface)) {
        SetSize(Window::ImageWidth(surface), m_height + m_extraHeight);
    }
    if (m_height < Window::ImageHeight(surface)) {
        int width = m_width;
        int height = Window::ImageHeight(surface);
        SetSize(width, m_extraHeight + height);
    }
    AddToFront(image);
    m_imageWindow = image;
}

CImageButton::CImageButton()
    : CBH_Button(), m_extraHeight(0), m_fieldD8(0), m_imageNames(), m_imageWindow(NULL),
      m_blinking(false), m_blinkTime(0), m_textWindow(NULL)
{
    SetSize(0, 0);
    SetHeightByContent(0, 0);
    SetWidthByContent(0, 0);
}

void CImageButton::SetText(XString text, int align, CFont* font, int leftSpacing, int percentHeight)
{
    if (m_textWindow != NULL) {
        m_textWindow->Close();
    }
    TextWindow* window = new TextWindow();
    if (font == NULL) {
        font = CFontMgr::GetInstance()->GetFont(15);
    }
    window->SetText(text, font);
    window->SetAlign(align);
    window->SetHeightByContent(0, 0);
    if (percentHeight > 0) {
        window->SetPercentHeight(percentHeight, 0, 0);
    }
    window->m_debugFrame = true;
    window->SetWidthByContent(0, 0);
    window->SetOutsetSpacing(leftSpacing, 0, 0, 0);
    AddToFront(window);
    m_textWindow = window;
}

void CImageButton::UpdateText(XString text)
{
    if (m_textWindow != NULL) {
        m_textWindow->SetText(text);
    }
}

void CImageButton::Update()
{
    if (m_blinking) {
        m_blinkTime -= WindowApp::m_instance->m_updateFrameTime;
        if (m_blinkTime <= 0) {
            m_imageWindow->m_blinking = false;
            m_blinking = false;
            m_blinkTime = 0;
        }
    }
    if (m_pressed && m_command != 0) {
        SetOpacity(0.7f);
    } else {
        SetOpacity(1.0f);
    }
}

CImageButton::~CImageButton()
{
    for (int index = 0; index < m_imageNames.size(); index++) {
        CBH_SurfaceManager* manager = ((App*)WindowApp::m_instance)->m_surfaceManager;
        manager->ReleaseSurface(XString::AnsiString(m_imageNames[index].c_str()));
    }
    // Explicit clear first; the member destructor frees the (now empty) storage again.
    m_imageNames.removeAllElements();
}

void CImageButton::SetFont(CFont* font)
{
    m_textWindow->SetFont(font);
}

void CImageButton::Blink(int time)
{
    m_blinking = true;
    m_blinkTime = time;
    m_imageWindow->m_blinking = true;
}

void CImageButton::Disable()
{
    if (m_imageWindow != NULL) {
        m_imageWindow->SetOpacity(0.5f);
    }
    if (m_textWindow != NULL) {
        m_textWindow->SetOpacity(0.5f);
    }
    ClearFlags(FLAG_ENABLED);
}

void CImageButton::Enable()
{
    if (m_imageWindow != NULL) {
        m_imageWindow->SetOpacity(1.0f);
    }
    if (m_textWindow != NULL) {
        m_textWindow->SetOpacity(1.0f);
    }
    SetFlags(FLAG_ENABLED);
}
