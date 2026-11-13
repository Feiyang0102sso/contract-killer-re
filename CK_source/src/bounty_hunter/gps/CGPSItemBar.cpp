// CGPSItemBar.cpp - unlock the friends marker and refresh its unread count.
#include "CGPSItemBar.h"

#include "App.h"
#include "CBH_Player.h"
#include "CFontMgr.h"
#include "CNewsFeedManager.h"
#include "DGHelper.h"

CGPSBar::CGPSBar() : m_listener(NULL), m_elapsed(2000)
{
    SetImage("IDB_GPS_ICON_BAR");
    m_font = CFontMgr::GetInstance()->GetFont(15);
    m_title = Window::ResString("IDS_GPS_FRIENDS_TITLE");
    {
        DGHelper* helper = static_cast<App*>(WindowApp::m_instance)->m_helper;
        XString::AnsiString title(m_title.c_str());
        m_titleWidth = helper->GetTextWidth(title, m_font);
    }
    SetPressImage("IDB_GPS_ROUND_ICON_ANIM");
    m_visible = false;
}

void CGPSBar::Paint(ICGraphics2d&, int, int)
{
    // The map paints this marker separately.
}

void CGPSBar::Update(long elapsed)
{
    if (CBH_Player::GetInstance()->m_tutorialState >= 13) {
        m_visible = true;
        m_listener->OnBarVisible();
    }
    m_elapsed += elapsed;
    if (m_elapsed > 2000) {
        m_elapsed = 0;
        CNewsFeedManager* news = static_cast<App*>(WindowApp::m_instance)->m_newsFeedManager;
        if (news != NULL) {
            int unread = news->GetUnreadNonEmptyMessageCount();
            unread -= news->GetUnreadNonEmptyMessageCount(NEWS_CHALLENGE_COMPLETED);
            m_unread.SetCount(unread);
        } else {
            m_unread.SetCount(0);
        }
    }
}

void CGPSBar::OnClick()
{
    // The map handles selection of this marker.
}
