#include "CGPSShop.h"

#include "App.h"
#include "CFont.h"
#include "CFontMgr.h"
#include "CTutorialManager.h"
#include "CUtility.h"
#include "DGHelper.h"

// Original 0x12d24c: the label width comes from DGHelper::GetTextWidth (no NULL font check).
CGPSShop::CGPSShop() : m_listener(NULL)
{
    SetImage("IDB_SHOP");
    m_font = CFontMgr::GetInstance()->GetFont(15);
    m_label = CUtility::GetString("IDS_MENU_STORE").GetWCharPtr();
    m_textWidth = static_cast<App*>(WindowApp::m_instance)->m_helper->GetTextWidth(
        XString::AnsiString(m_label.c_str()), m_font);
    SetPressImage("IDB_GPS_ROUND_ICON_ANIM");
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
    if (static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial() && static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsMapShopBlink()) {
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
    // Original 0x12d0f0: no NULL checks; the label is centered under the icon.
    static_cast<App*>(WindowApp::m_instance)->m_helper->DrawImage(
        GetImage(), GetX() + mapX, GetY() + mapY, 1.0f, 0, 0, 0.0f, 0);
    CGPSItem::Paint(graphics, mapX, mapY);
    m_font->PaintText(m_label.c_str(), -1, GetX() + mapX + GetWidth() / 2 - m_textWidth / 2,
                      GetY() + mapY + GetHeight(), -1, -1);
    CRectangle label(GetX() + GetWidth() / 2 - m_textWidth / 2, GetY() + GetHeight(), m_textWidth,
                     m_font->GetFontHeight());
    setSubItem(0, label, mapX, mapY);
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
