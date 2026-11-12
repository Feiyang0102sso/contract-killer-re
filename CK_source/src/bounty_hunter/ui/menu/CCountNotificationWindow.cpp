// CCountNotificationWindow.cpp - a numbered badge drawn over a tab.
#include "CCountNotificationWindow.h"

#include "App.h"
#include "CFont.h"
#include "CFontMgr.h"
#include "DGHelper.h"

CCountNotificationWindow::CCountNotificationWindow()
    : Window(false), m_leftWidth(0), m_middleWidth(0), m_rightWidth(0), m_barHeight(0),
      m_font(NULL), m_posX(0), m_posY(0), m_textWidth(0), m_textHeight(0), m_count(0)
{
    m_left.Create("SUR_HUD_COUNT_NOTIFICATION_BAR_LEFT");
    m_middle.Create("SUR_HUD_COUNT_NOTIFICATION_BAR");
    m_right.Create("SUR_HUD_COUNT_NOTIFICATION_BAR_RIGHT");
    int fontIndex = 8;
    if (App::IsHD()) {
        fontIndex = 5;
    }
    m_font = CFontMgr::GetInstance()->GetFont(fontIndex);
    if (m_font != NULL) {
        m_leftWidth = ImageWidth(m_left.GetSurface());
        m_middleWidth = ImageWidth(m_middle.GetSurface());
        m_rightWidth = ImageWidth(m_right.GetSurface());
        m_barHeight = ImageHeight(m_middle.GetSurface());
        UpdateCount();
    }
}

void CCountNotificationWindow::UpdateCount()
{
    m_text = XString::ToString(m_count);
    if (m_font != NULL) {
        App* app = static_cast<App*>(WindowApp::m_instance);
        XString::AnsiString number(m_text.c_str());
        m_textWidth = app->m_helper->GetTextWidth(number, m_font);
        m_textHeight = m_font->GetFontHeight();
    }
}

void CCountNotificationWindow::SetCount(int count)
{
    m_count = count;
    UpdateCount();
}

void CCountNotificationWindow::SetPos(int x, int y)
{
    m_posX = x;
    m_posY = y;
}

void CCountNotificationWindow::Paint(ICGraphics2d& graphics)
{
    if (m_count <= 0) {
        return;
    }
    int width = m_leftWidth + m_rightWidth;
    int x = m_posX;
    DrawImage(graphics, m_left.GetSurface(), x, m_posY, 0, 0);
    x += m_leftWidth;
    if (width < m_textWidth * 2) {
        DrawImage(graphics, m_middle.GetSurface(), x, m_posY, 0, 0);
        x += m_middleWidth;
        width += m_middleWidth;
    }
    DrawImage(graphics, m_right.GetSurface(), x, m_posY, 0, 0);
    if (m_font != NULL) {
        m_font->PaintText(m_text.c_str(), -1, m_posX + (width - m_textWidth) / 2,
                          m_posY + (m_barHeight - m_textHeight) / 2, -1, -1);
    }
}
