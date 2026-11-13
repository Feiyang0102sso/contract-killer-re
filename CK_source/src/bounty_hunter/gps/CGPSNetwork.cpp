#include "CGPSNetwork.h"

#include "App.h"
#include "CFont.h"
#include "CFontMgr.h"
#include "CTutorialManager.h"
#include "CUtility.h"
#include "DGHelper.h"

// Original 0x12cefc: the label width comes from DGHelper::GetTextWidth (no NULL font check).
CGPSNetwork::CGPSNetwork() : m_listener(NULL)
{
    SetImage("IDB_HOME");
    m_font = CFontMgr::GetInstance()->GetFont(15);
    m_label = CUtility::GetString("IDS_MENU_HOME").GetWCharPtr();
    m_labelWidth = static_cast<App*>(WindowApp::m_instance)->m_helper->GetTextWidth(
        XString::AnsiString(m_label.c_str()), m_font);
    SetPressImage("IDB_GPS_ROUND_ICON_ANIM");
}

CGPSNetwork::~CGPSNetwork()
{
}

void CGPSNetwork::SetListener(CGPSNetworkListener* listener)
{
    m_listener = listener;
}

void CGPSNetwork::SetBlinking(bool enabled)
{
    if (m_blinking != enabled) {
        m_blinking = enabled;
        if (enabled) {
            SetBlinkingImage("IDB_GPS_ROUND_ICON_ANIM_TUTORIAL");
        }
    }
}

void CGPSNetwork::Update(long)
{
    if (static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial()) {
        bool enabled = static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsMapNetworkBlink() == 1;
        SetBlinking(enabled);
    }
}

void CGPSNetwork::Save(TiXmlElement*)
{
}

void CGPSNetwork::Load(TiXmlElement*)
{
}

void CGPSNetwork::OnClick()
{
    if (m_listener != NULL) {
        m_listener->OnGPSNetworkSelected(this);
    }
}

void CGPSNetwork::Paint(ICGraphics2d& graphics, int mapX, int mapY)
{
    // Original 0x12cda0: no visibility / NULL checks; the label is centered under the icon.
    static_cast<App*>(WindowApp::m_instance)->m_helper->DrawImage(
        GetImage(), GetX() + mapX, GetY() + mapY, 1.0f, 0, 0, 0.0f, 0);
    CGPSItem::Paint(graphics, mapX, mapY);
    m_font->PaintText(m_label.c_str(), -1, GetX() + mapX + GetWidth() / 2 - m_labelWidth / 2,
                      GetY() + mapY + GetHeight(), -1, -1);
    CRectangle label(GetX() + GetWidth() / 2 - m_labelWidth / 2, GetY() + GetHeight(), m_labelWidth,
                     m_font->GetFontHeight());
    setSubItem(0, label, mapX, mapY);
}

typedef char AssertGPSNetworkSize[sizeof(CGPSNetwork) == 0x58 ? 1 : -1];
