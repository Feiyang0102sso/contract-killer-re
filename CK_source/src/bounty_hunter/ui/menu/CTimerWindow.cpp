// CTimerWindow.cpp - refresh the countdown when its displayed value changes.
#include "CTimerWindow.h"

#include "App.h"

CTimerWindow::CTimerWindow(unsigned int endTime, CFont* font)
    : TextWindow(font), m_endTime(endTime), m_displayedSeconds(-1)
{
}

void CTimerWindow::Update()
{
    int seconds = static_cast<int>(m_endTime - App::GetTime());
    if (seconds < 0) {
        seconds = 0;
    }
    if (seconds != m_displayedSeconds) {
        m_displayedSeconds = seconds;
        int minutes = seconds / 60;
        SetText(XString::Format(L"%02d:%02d:%02d", seconds / 3600, minutes % 60, seconds % 60));
        Window::Update();
    }
}
