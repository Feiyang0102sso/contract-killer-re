// CGPSCooperation.cpp - refresh the unread completed-challenge badge.
#include "CGPSCooperation.h"

#include "App.h"
#include "CBH_Player.h"
#include "CFontMgr.h"
#include "CNewsFeedManager.h"
#include "CUtility.h"
#include "DGHelper.h"

CCountNotificationWindow::~CCountNotificationWindow()
{
    // The three image resources and text belong to the badge.
}

CGPSCooperation::CGPSCooperation()
    : m_listener(NULL), m_font(NULL), m_titleWidth(0), m_elapsed(2000)
{
    m_visible = false;
    SetImage("IDB_COOPERATION");
    SetPressImage("IDB_GPS_ROUND_ICON_ANIM");
    CStrWChar title = CUtility::GetString("IDS_MENU_COOPERATION");
    m_title = title.GetWCharPtr();
    m_font = CFontMgr::GetInstance()->GetFont(15);
    if (m_font != NULL) {
        App* app = static_cast<App*>(WindowApp::m_instance);
        XString::AnsiString titleText(m_title.c_str());
        m_titleWidth = app->m_helper->GetTextWidth(titleText, m_font);
    }
}

void CGPSCooperation::OnClick()
{
    // The map handles selection of this marker.
}

void CGPSCooperation::Update(long elapsed)
{
    CBH_Player::GetInstance();
    if (CBH_Player::GetInstance()->m_tutorialState >= 13) {
        m_visible = true;
        if (m_listener != NULL) {
            m_listener->OnCoopVisible();
        }
    }
    m_elapsed += elapsed * 2;
    if (m_elapsed > 2000) {
        m_elapsed = 0;
        App* app = static_cast<App*>(WindowApp::m_instance);
        int unread = 0;
        if (app->m_newsFeedManager != NULL) {
            unread = app->m_newsFeedManager->GetUnreadNonEmptyMessageCount(NEWS_CHALLENGE_COMPLETED);
        }
        m_unread.SetCount(unread);
    }
}

void CGPSCooperation::Paint(ICGraphics2d&, int, int)
{
    // The map paints this marker separately.
}

void CGPSCooperation::SetListener(CGPSCGPSCooperationListener* listener)
{
    m_listener = listener;
}
