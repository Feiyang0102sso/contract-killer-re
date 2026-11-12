#include "CGPSShop.h"

#include "App.h"
#include "CFont.h"
#include "CFontMgr.h"
#include "CTutorialManager.h"
#include "DGHelper.h"

CGPSShop::CGPSShop() : m_listener(NULL), m_font(NULL), m_textWidth(0)
{
    SetImage("IDB_SHOP");
    m_font = CFontMgr::GetInstance()->GetFont(15);
    m_label = Window::ResString("IDS_MENU_STORE");
    if (m_font != NULL) {
        m_textWidth = m_font->MeasureTextWidth(m_label.c_str(), -1, -1, NULL);
    }
    SetBlinkingImage("IDB_GPS_ROUND_ICON_ANIM");
}

CGPSShop::~CGPSShop()
{
}

void CGPSShop::SetListener(CGPSShopListener* listener)
{
    m_listener = listener;
}

void CGPSShop::Update(long)
{
    if (CTutorialManager::IsPlayerInTutorial() && CTutorialManager::IsMapShopBlink()) {
        SetBlinking(true);
    }
}

void CGPSShop::Save(TiXmlElement*)
{
}

void CGPSShop::Load(TiXmlElement*)
{
}

void CGPSShop::OnClick()
{
    if (m_listener != NULL) {
        m_listener->OnShopItemSelected(this);
    }
}

void CGPSShop::Paint(ICGraphics2d& graphics, int mapX, int mapY)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (GetImage() != NULL) {
        app->m_helper->DrawImage(GetImage(), GetX() + mapX, GetY() + mapY, 1.0f, 0, 0, 0.0f, 0);
    }
    CGPSItem::Paint(graphics, mapX, mapY);
    if (m_font != NULL) {
        CRectangle label;
        label.m_x = GetX() + (GetWidth() - m_textWidth) / 2;
        label.m_y = GetY() + GetHeight();
        label.m_width = m_textWidth;
        label.m_height = m_font->GetFontHeight();
        m_font->PaintText(m_label.c_str(), -1, mapX + label.m_x, mapY + label.m_y, -1, -1);
        setSubItem(0, label, mapX, mapY);
    }
}

void CGPSShop::SetBlinking(bool blinking)
{
    if (m_blinking != blinking) {
        m_blinking = blinking;
        if (blinking) {
            SetBlinkingImage("IDB_GPS_ROUND_ICON_ANIM_TUTORIAL");
        }
    }
}

typedef char AssertGPSShopSize[sizeof(CGPSShop) == 0x58 ? 1 : -1];
