#include "CGPSItem.h"

#include "App.h"
#include "CBH_SurfaceManager.h"
#include "CStdUtil_Cocoa.h"
#include "DGHelper.h"
#include "ICRenderSurface.h"

#include <math.h>

CGPSItem::CGPSItem()
    : m_x(0), m_y(0), m_width(0), m_height(0), m_attractSpeed(0), m_blinking(false),
      m_pressed(false), m_pressTime(0), m_visible(true), m_image(NULL), m_blinkImage(NULL), m_pressImage(NULL)
{
    float scale = 1.0f;
    if (App::IsHD() && (App::IsXGA() || App::IsVGA() || App::IsWVGA())) {
        scale = 2.0f;
    }
    m_scaleX = scale;
    m_scaleY = scale;
}

CGPSItem::~CGPSItem()
{
    CBH_SurfaceManager* manager = static_cast<App*>(WindowApp::m_instance)->m_surfaceManager;
    if (m_image != NULL) {
        manager->ReleaseSurface(m_image);
    }
    if (m_blinkImage != NULL) {
        manager->ReleaseSurface(m_blinkImage);
    }
    if (m_pressImage != NULL) {
        manager->ReleaseSurface(m_pressImage);
    }
}

CRectangle CGPSItem::GetRect() const
{
    CRectangle rect;
    rect.m_x = GetX();
    rect.m_y = GetY();
    rect.m_width = m_width;
    rect.m_height = m_height;
    return rect;
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

void CGPSItem::Paint(ICGraphics2d&, int mapX, int mapY)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CRectangle rect = GetRect();
    int x = mapX + rect.m_x + rect.m_width / 2;
    int y = mapY + rect.m_y + rect.m_height / 2;
    if (m_blinking && m_blinkImage != NULL) {
        int phase = CStdUtil_Cocoa::GetUpTimeMS() % 800 - 400;
        float alpha = fabsf(static_cast<float>(phase)) / 800.0f + 0.5f;
        app->m_helper->DrawImage(m_blinkImage, x, y, alpha, 0, 0, 0.0f,
                                 DGHelper::DRAW_HCENTER | DGHelper::DRAW_VCENTER);
    }
    if (m_pressed && m_pressImage != NULL) {
        unsigned int elapsed = CStdUtil_Cocoa::GetUpTimeMS() - m_pressTime;
        float alpha = elapsed / 100.0f;
        if (alpha > 1.0f) {
            alpha = 1.0f;
        }
        app->m_helper->DrawImage(m_pressImage, x, y, alpha, 0, 0, 0.0f,
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

void CGPSItem::SetImage(const char* name)
{
    CBH_SurfaceManager* manager = static_cast<App*>(WindowApp::m_instance)->m_surfaceManager;
    if (m_image != NULL) {
        manager->ReleaseSurface(m_image);
    }
    m_image = manager->CreateSurface(name, true);
    unsigned int width = 0;
    unsigned int height = 0;
    if (m_image != NULL) {
        m_image->GetWidthAndHeight(width, height);
    }
    SetSize(width, height);
}

void CGPSItem::SetBlinkingImage(const char* name)
{
    CBH_SurfaceManager* manager = static_cast<App*>(WindowApp::m_instance)->m_surfaceManager;
    if (m_blinkImage != NULL) {
        manager->ReleaseSurface(m_blinkImage);
    }
    m_blinkImage = manager->CreateSurface(name, true);
}

void CGPSItem::SetPressImage(const char* name)
{
    CBH_SurfaceManager* manager = static_cast<App*>(WindowApp::m_instance)->m_surfaceManager;
    if (m_pressImage != NULL) {
        manager->ReleaseSurface(m_pressImage);
    }
    m_pressImage = manager->CreateSurface(name, true);
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

typedef char AssertGPSItemSize[sizeof(CGPSItem) == 0x48 ? 1 : -1];
