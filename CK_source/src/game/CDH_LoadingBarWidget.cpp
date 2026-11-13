#include "CDH_LoadingBarWidget.h"
#include "App.h"
#include "CFont.h"
#include "CFontMgr.h"
#include "CUtility.h"
#include "DGHelper.h"

CDH_LoadingBarWidget::CDH_LoadingBarWidget() : Window(false)
{
    m_progress = 0;
    m_blinkTime = 0;
    m_font = CFontMgr::GetInstance()->GetFont(15);
    setState(STATE_LOADING);
}

void CDH_LoadingBarWidget::setState(eState state)
{
    m_state = state;
    switch (state) {
    case STATE_LOADING:
        m_blinking = false;
        CUtility::GetString(m_text, "IDS_LOADING");
        break;
    case STATE_CONNECTING:
        m_blinking = true;
        m_text = "Connecting";
        break;
    case STATE_TAP:
        m_blinking = true;
        m_blinkTime = 0;
        CUtility::GetString(m_text, "IDS_TAP_TO_CONTINUE");
        break;
    }
    m_textLength = m_text.GetLength();
}

void CDH_LoadingBarWidget::Update()
{
    Window::Update();
    if (m_blinking) {
        m_blinkTime += WindowApp::m_instance->m_updateFrameTime;
        if (m_blinkTime > 400) {
            m_blinkTime = -400;
        }
    }
}

void CDH_LoadingBarWidget::Paint(ICGraphics2d& graphics)
{
    DGHelper* helper = static_cast<App*>(WindowApp::m_instance)->m_helper;
    int width = helper->m_screenWidth - 30;
    int y = helper->m_screenHeight - 5;
    int fontHeight = m_font->GetFontHeight();
    FillRect(graphics, 15, y, width, 2, 0xff241008);
    FillRect(graphics, 15, y, width * m_progress / 100, 2, 0xffac0300);
    if (!m_blinking || m_blinkTime < 0) {
        m_font->PaintText(m_text.GetWCharPtr(), m_textLength, 15, y - fontHeight, -1, -1);
    }
}

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AssertLoadingBarSize[sizeof(CDH_LoadingBarWidget) == 0xcc ? 1 : -1];
#endif
