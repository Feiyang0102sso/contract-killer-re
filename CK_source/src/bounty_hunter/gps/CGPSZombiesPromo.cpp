// CGPSZombiesPromo.cpp - draw the promotion marker and dispatch a tap.
#include "CGPSZombiesPromo.h"

#include "App.h"
#include "DGHelper.h"

CGPSZombiesPromo::CGPSZombiesPromo() : m_listener(NULL)
{
    CGPSItem::SetImage("IDB_ZOMBIES_ICON");
    SetPressImage("IDB_ZOMBIES_ICON_GLOW");
}

void CGPSZombiesPromo::OnClick()
{
    if (m_visible && m_listener != NULL) {
        m_listener->OnZombiesItemSelected(this);
    }
}

void CGPSZombiesPromo::Update(long)
{
    // This marker has no per-frame state.
}

void CGPSZombiesPromo::Save(TiXmlElement*)
{
    // This marker has no persistent state.
}

void CGPSZombiesPromo::Load(TiXmlElement*)
{
    // This marker has no persistent state.
}

void CGPSZombiesPromo::Paint(ICGraphics2d& graphics, int mapX, int mapY)
{
    CGPSItem::Paint(graphics, mapX, mapY);
    if (m_visible) {
        static_cast<App*>(WindowApp::m_instance)->m_helper->DrawImage(
            GetImage(), GetX() + mapX, GetY() + mapY, 1.0f, 0, 0, 0.0f, 0);
    }
}
