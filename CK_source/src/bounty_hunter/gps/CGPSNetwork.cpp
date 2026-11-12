#include "CGPSNetwork.h"

#include "App.h"
#include "CFont.h"
#include "CFontMgr.h"
#include "CTutorialManager.h"
#include "DGHelper.h"

CGPSNetwork::CGPSNetwork() : m_listener(NULL), m_labelWidth(0)
{
    SetImage("IDB_HOME");
    m_font = CFontMgr::GetInstance()->GetFont(15);
    m_label = Window::ResString("IDS_MENU_HOME");
    if (m_font != NULL) {
        m_labelWidth = m_font->MeasureTextWidth(m_label.c_str(), -1, -1, NULL);
    }
    SetBlinkingImage("IDB_GPS_ROUND_ICON_ANIM");
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
    if (CTutorialManager::IsPlayerInTutorial()) {
        SetBlinking(CTutorialManager::IsMapNetworkBlink());
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
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (GetImage() != NULL) {
        app->m_helper->DrawImage(GetImage(), GetX() + mapX, GetY() + mapY, 1.0f, 0, 0, 0.0f, 0);
    }
    CGPSItem::Paint(graphics, mapX, mapY);
    if (m_font != NULL) {
        CRectangle label;
        label.m_x = GetX() + (GetWidth() - m_labelWidth) / 2;
        label.m_y = GetY() + GetHeight();
        label.m_width = m_labelWidth;
        label.m_height = m_font->GetFontHeight();
        m_font->PaintText(m_label.c_str(), -1, mapX + label.m_x, mapY + label.m_y, -1, -1);
        setSubItem(0, label, mapX, mapY);
    }
}

typedef char AssertGPSNetworkSize[sizeof(CGPSNetwork) == 0x58 ? 1 : -1];
