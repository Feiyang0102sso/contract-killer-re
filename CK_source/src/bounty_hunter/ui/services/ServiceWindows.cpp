// b081j: original 15b358..15be60 complete service windows; borrowed resources and animation state kept as in the original.
#include "ServiceWindows.h"
#include "App.h"
#include "BaseDialog.h"
#include "CBH_SurfaceManager.h"
#include "CFontMgr.h"
#include "CFont.h"
#include "ICGraphics2d.h"
#include "MathLib.h"
#include "CStdUtil_Cocoa.h"
#include "SScreenAdaptingConstant.h"

const SScreenAdaptingConstant MARGIN_BARTEXT(2, 4);

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AnimatedWindowSizeCheck[sizeof(AnimatedWindow) == 0xf4 ? 1 : -1];
#endif
#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char LabelWithIconSizeCheck[sizeof(LabelWithIcon) == 0xb0 ? 1 : -1];
#endif

ViewWindow::ViewWindow(const World& world)
    : WindowTransparent(false), m_world(world)
{
    SetFlags(0x1000);
}

ViewWindow::~ViewWindow()
{
    // Original 15b41c clears the handle before the Window destructor; the automatic wrapper destructor sees NULL afterwards.
    m_world.Clear();
}

void ViewWindow::Paint3D(Graphics3D& graphics)
{
    if (m_world != World()) {
        graphics.render(m_world);
    }
}

void SetSizesByContent(Window* window, int flags)
{
    if (window == NULL) {
        return;
    }
    if (flags & 2) {
        window->SetWidthByContent(0, 0);
    }
    if (flags & 1) {
        window->SetHeightByContent(0, 0);
    }
}

FillerBar::FillerBar()
{
    m_topLeftFrame.Create("SUR_HUD_PROGRESSBAR_LEFT");
    m_topFrame.Create("SUR_HUD_PROGRESSBAR");
    m_topRightFrame.Create("SUR_HUD_PROGRESSBAR_RIGHT");
    int rightWidth;
    int leftWidth;
    int height;
    GetImageSize(m_topRightFrame.GetSurface(), rightWidth, height);
    GetImageSize(m_topLeftFrame.GetSurface(), leftWidth, height);
    SetInsetSpacing(0, rightWidth, 0, leftWidth);
    int width;
    GetImageSize(m_topFrame.GetSurface(), width, height);
    AddToFront(new SpacerWindow(width, height));
    SetDesiredHeight(height);
}

Window* createTD(Window* window, int flags)
{
    Window* container = new Window(false);
    if (window != NULL) {
        container->AddToFront(window);
    }
    SetSizesByContent(container, flags);
    return container;
}

Window* createTD(Window* window, int column, int row, int flags)
{
    Window* container = createTD(window, flags);
    container->SetCellPos(column, row, 1, 1);
    return container;
}

SeparateLineRed::SeparateLineRed() : Window(false)
{
    m_center.Create("SUR_BOUNTY_MENU_SEPARATE_LINE_CENTER");
    m_right.Create("SUR_BOUNTY_MENU_SEPARATE_LINE_RIGHT");
    m_left.Create("SUR_BOUNTY_MENU_SEPARATE_LINE_LEFT");
}

void SeparateLineRed::Paint(ICGraphics2d& graphics)
{
    ICRenderSurface* left = m_left.GetSurface();
    ICRenderSurface* center = m_center.GetSurface();
    ICRenderSurface* right = m_right.GetSurface();
    DrawRectImageHoriz(graphics, left, center, right, 0, 0, m_width, 1);
}

StretchImageButton::StretchImageButton(XString text, const char* left,
                                     const char* center, const char* right)
    : m_forcedWidth(0)
{
    m_left = ((App*)WindowApp::m_instance)->m_surfaceManager->CreateSurface(left, true);
    m_right = ((App*)WindowApp::m_instance)->m_surfaceManager->CreateSurface(right, true);
    m_center = ((App*)WindowApp::m_instance)->m_surfaceManager->CreateSurface(center, true);
    m_text.Assign(text);
    SetLayoutType(LAYOUT_SIMPLE);
    SetAlign(0xc);
    m_font = CFontMgr::GetInstance()->GetFont(15);
    int textWidth = m_font->MeasureTextWidth(m_text.m_data, -1, -1, NULL);
    int width = textWidth + ImageWidth(m_left);
    TextWindow* label = new TextWindow(m_text, m_font);
    label->SetLayoutType(LAYOUT_SIMPLE);
    label->SetAlign(0xc);
    label->SetOutsetSpacing(App::Scale() * 4, 0, 0, 0);
    label->SetSize(width, ImageHeight(m_center));
    AddToFront(label);
    SetSize(width, ImageHeight(m_center));
}

StretchImageButton::~StretchImageButton()
{
    ((App*)WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(m_left);
    ((App*)WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(m_right);
    ((App*)WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(m_center);
}

void StretchImageButton::Paint(ICGraphics2d& graphics)
{
    if (m_pressed) {
        BeginBlend(graphics, 0.7f, 0.0f, 0.0f, 0.0f, false, false);
    }
    DrawRectImageHoriz(graphics, m_left, m_center, m_right, 0, 0, m_width, m_height);
    // The original re-reads pressed after the draw callback instead of caching the first check.
    if (m_pressed) {
        EndBlend(graphics);
    }
    Window::Paint(graphics);
}

void StretchImageButton::SetRect(int x, int y, int width, int height, bool force)
{
    int textWidth = m_font->MeasureTextWidth(m_text.m_data, -1, -1, NULL);
    int leftWidth = ImageWidth(m_left);
    int forcedWidth = m_forcedWidth;
    int imageHeight = ImageHeight(m_center);
    int finalWidth = textWidth + leftWidth * 4;
    if (forcedWidth > 0) {
        finalWidth = forcedWidth;
    }
    Window::SetRect(x, y, finalWidth, imageHeight, force);
}

void StretchImageButton::ForceSetWidth(int width)
{
    m_forcedWidth = width;
    SetRect(m_x, m_y, m_width, m_height, true);
}

LabelWithIcon::LabelWithIcon(const char* image, XString text, bool flag, CFont* font, int spacing)
    : Window(false)
{
    Create(image, text, flag, font, spacing);
}

LabelWithIcon::LabelWithIcon(const char* image, int value)
    : Window(false)
{
    Create(image, XString::Format(L"+%d", value), true, NULL, 0);
}

LabelWithIcon::~LabelWithIcon()
{
    ((App*)WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(XString::AnsiString(m_imageName.c_str()));
}

void LabelWithIcon::Create(const char* image, XString text, bool flag, CFont* font, int spacing)
{
    m_icon = NULL;
    if (image != NULL) {
        m_imageName = XString(image);
    }
    if (font == NULL) {
        font = CFontMgr::GetInstance()->GetFont(8);
    }
    m_text = new TextWindow(text, font);
    m_text->SetAlign(0x24);
    m_text->SetOutsetSpacing(spacing, 0, 0, 0);
    if (image != NULL) {
        m_icon = new ImageWindow;
        m_icon->SetImage(((App*)WindowApp::m_instance)->m_surfaceManager->CreateSurface(image, true));
        m_icon->SetAlign(0x24);
        if (flag == 1) {
            AddToFront(m_text, 0, 0);
            AddToFront(m_icon, 1, 0);
        } else {
            AddToFront(m_icon, 0, 0);
            AddToFront(m_text, 1, 0);
        }
    } else {
        AddToFront(m_text, 0, 0);
    }
    SetWidthByContent(0, 0);
    SetHeightByContent(0, 0);
}

void LabelWithIcon::SetText(const XString& text)
{
    m_text->SetText(text, CFontMgr::GetInstance()->GetFont(8));
}

void LabelWithIcon::SetTextWithSameFont(const XString& text)
{
    m_text->SetText(text);
}

void LabelWithIcon::SetImage(const char* name)
{
    if (m_icon != NULL) {
        CBH_SurfaceManager* surfaces = static_cast<App*>(WindowApp::m_instance)->m_surfaceManager;
        m_icon->SetImage(surfaces->CreateSurface(name, true));
        surfaces->ReleaseSurface(XString::AnsiString(m_imageName.c_str()));
        m_imageName = XString(name);
    }
}

AnimatedWindow::AnimatedWindow(EMode mode) : WindowTransparent(false)
{
    // The original writes only these six groups of fields; the remaining parameters stay uninitialized until SetStart/CloseEasy.
    m_start.m_enabled = false;
    m_start.m_reverse = false;
    m_close.m_enabled = false;
    m_close.m_reverse = false;
    m_mode = mode; m_closing = 0;  // assigned after the easy parameters (original store order)
    SetWidthByContent(0, 0);
    SetHeightByContent(0, 0);
}

void AnimatedWindow::StartCustomEasy(SEasyParams& parameters)
{
    EasyParams easy(this, parameters.m_method);
    easy.SetFullTime(parameters.m_time);
    easy.SetOpacity(parameters.m_firstOpacity, parameters.m_lastOpacity);
    easy.SetDelayTime(parameters.m_delay);
    int x = m_x;
    int y = m_y;
    int newX = x + parameters.m_x;
    int newY = y + parameters.m_y;
    if (parameters.m_reverse) {
        easy.SetMoving(newX, newY, x, y);
    } else {
        easy.SetMoving(x, y, newX, newY);
    }
    StartEasy(easy, 0x100);
}

void AnimatedWindow::SetStartEasy(int x, int y, float time, float firstOpacity,
    float lastOpacity, float delay, EasyLib::EMethod method)
{
    m_start.m_x = x;
    m_start.m_y = y;
    m_start.m_firstOpacity = firstOpacity;
    m_start.m_lastOpacity = lastOpacity;
    m_start.m_time = time;
    m_start.m_delay = delay;
    m_start.m_enabled = true;
    m_start.m_method = method;
    m_start.m_reverse = true;
}

void AnimatedWindow::SetCloseEasy(int x, int y, float time, float firstOpacity,
    float lastOpacity, float delay, EasyLib::EMethod method)
{
    m_close.m_x = x;
    m_close.m_y = y;
    m_close.m_firstOpacity = firstOpacity;
    m_close.m_lastOpacity = lastOpacity;
    m_close.m_time = time;
    m_close.m_delay = delay;
    m_close.m_enabled = true;
    m_close.m_method = method;
}

bool AnimatedWindow::IsWaitingState()
{
    return m_closing == 1;
}

void AnimatedWindow::OnStartupEasy()
{
    if (m_closing != 0) {
        return;
    }
    if (m_start.m_enabled) {
        StartCustomEasy(m_start);
    } else {
        OnEasyFinished();
    }
}

void AnimatedWindow::OnEasyFinished()
{
    // b147: original compares closing 2 before 0 and mode 2 before 1, i.e. the
    // "0" and "mode 1" branches are written first.
    if (m_closing == 0) {
        m_closing = 1;
        if ((int)m_mode == 1) {
            Window::Close();
        } else if ((int)m_mode == 2) {
            if (m_close.m_enabled) {
                StartCustomEasy(m_close);
                m_closing = 2;
            } else {
                Window::Close();
            }
        }
    } else if (m_closing == 2) {
        Window::Close();
    }
}

void AnimatedWindow::Close()
{
    if (m_close.m_enabled) {
        StartCustomEasy(m_close);
        m_closing = 2;
    } else {
        Window::Close();
    }
}

void AnimatedWindow::OnLayoutEasy(int x, int y, int width, int height) {} // Original 15bd30 is really empty.

void BlinkingContainer::Update()
{
    if (m_blinking) {
        unsigned int time = CStdUtil_Cocoa::GetUpTimeMS();
        float distance = (int)(time % 600 - 300);
        SetOpacity(MathLib::Fabs(distance) / 360.0f + 0.2f);
    } else {
        SetOpacity(1.0f);
    }
}
