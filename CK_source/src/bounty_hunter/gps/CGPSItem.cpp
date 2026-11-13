#include "CGPSItem.h"

#include "App.h"
#include "CBH_SurfaceManager.h"
#include "CStdUtil_Cocoa.h"
#include "DGHelper.h"
#include "ICRenderSurface.h"
#include "MathLib.h"

#include <math.h>

// Original 0x12c1ac: position, size and press time are not initialized; an HD device that is
// neither XGA, VGA nor WVGA leaves the scale unset too (original).
CGPSItem::CGPSItem()
    : m_attractSpeed(0), m_blinking(false), m_pressed(false), m_visible(true), m_image(NULL),
      m_blinkImage(NULL), m_pressImage(NULL)
{
    if (App::IsHD()) {
        if (App::IsXGA() || App::IsVGA() || App::IsWVGA()) {
            m_scaleX = 2.0f;
            m_scaleY = 2.0f;
        }
    } else {
        m_scaleX = 1.0f;
        m_scaleY = 1.0f;
    }
}

CGPSItem::~CGPSItem()
{
    if (m_image != NULL) {
        static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(m_image);
    }
    if (m_blinkImage != NULL) {
        static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(m_blinkImage);
    }
    if (m_pressImage != NULL) {
        static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(m_pressImage);
    }
}

CRectangle CGPSItem::GetRect() const
{
    return CRectangle(GetX(), GetY(), m_width, m_height);
}

void CGPSItem::SetPos(int x, int y)
{
    m_x = x + 144;
    m_y = y + 60;
}

void CGPSItem::SetScaledPos(int x, int y)
{
    m_x = static_cast<int>(x / m_scaleX);
    m_y = static_cast<int>(y / m_scaleY);
}

int CGPSItem::GetX() const
{
    return static_cast<int>(m_x * m_scaleX);
}

int CGPSItem::GetY() const
{
    return static_cast<int>(m_y * m_scaleY);
}

void CGPSItem::SetSize(int width, int height)
{
    m_width = width;
    m_height = height;
}

/** Horizontal center of a rectangle (original reads GetRect() once per axis). */
static inline int RectCenterX(const CRectangle& rect)
{
    return rect.m_x + (rect.m_width >> 1);
}

/** Vertical center of a rectangle. */
static inline int RectCenterY(const CRectangle& rect)
{
    return rect.m_y + (rect.m_height >> 1);
}

// Original 0x12c2c4: the blink image is drawn whenever m_blinking is set (no NULL check);
// GetRect() is called (virtually) once per axis for each draw.
void CGPSItem::Paint(ICGraphics2d&, int mapX, int mapY)
{
    if (m_blinking) {
        unsigned int time = CStdUtil_Cocoa::GetUpTimeMS();
        static_cast<App*>(WindowApp::m_instance)->m_helper->DrawImage(
            m_blinkImage, mapX + RectCenterX(GetRect()), mapY + RectCenterY(GetRect()),
            MathLib::Fabs(static_cast<float>(static_cast<int>(time % 800 - 400))) / 800.0f + 0.5f, 0, 0, 0.0f,
            DGHelper::DRAW_HCENTER | DGHelper::DRAW_VCENTER);
    }
    if (m_pressed && m_pressImage != NULL) {
        unsigned int elapsed = CStdUtil_Cocoa::GetUpTimeMS() - m_pressTime;
        float alpha = elapsed / 100.0;
        alpha = alpha < 0.0f ? 0.0f : (1.0f < alpha ? 1.0f : alpha);
        static_cast<App*>(WindowApp::m_instance)->m_helper->DrawImage(
            m_pressImage, mapX + RectCenterX(GetRect()), mapY + RectCenterY(GetRect()), alpha, 0, 0, 0.0f,
            DGHelper::DRAW_HCENTER | DGHelper::DRAW_VCENTER);
    }
}

bool CGPSItem::CheckHit(int x, int y, CGPSItem** item)
{
    if (m_visible) {
        CRectangle rect = GetRect();
        if (rect.m_x < x && x < rect.m_x + rect.m_height &&
            rect.m_y < y && y < rect.m_y + rect.m_height) {
            *item = this;
            return true;
        }
        *item = NULL;
    }
    return false;
}

// Original 0x12c4a8: no NULL check on the created surface.
void CGPSItem::SetImage(const char* name)
{
    if (m_image != NULL) {
        static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(m_image);
    }
    m_image = static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->CreateSurface(name, true);
    unsigned int width = 0;
    unsigned int height = 0;
    m_image->GetWidthAndHeight(width, height);
    SetSize(width, height);
}

void CGPSItem::SetBlinkingImage(const char* name)
{
    if (m_blinkImage != NULL) {
        static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(m_blinkImage);
    }
    m_blinkImage = static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->CreateSurface(name, true);
}

void CGPSItem::SetPressImage(const char* name)
{
    if (m_pressImage != NULL) {
        static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(m_pressImage);
    }
    m_pressImage = static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->CreateSurface(name, true);
}

bool CGPSItem::GetPressed() const
{
    return m_pressed;
}

void CGPSItem::SetPressed(bool pressed)
{
    m_pressed = pressed;
    m_pressTime = CStdUtil_Cocoa::GetUpTimeMS();
}

void CGPSItem::setSubItem(int index, const CRectangle& rect, int, int)
{
    m_subItems.setSize(index + 1);
    m_subItems[index] = rect;
}

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AssertGPSItemSize[sizeof(CGPSItem) == 0x48 ? 1 : -1];
#endif
