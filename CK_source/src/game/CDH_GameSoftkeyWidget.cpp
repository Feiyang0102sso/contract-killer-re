// CDH_GameSoftkeyWidget.cpp - open the pause screen on touch-down.
#include "CDH_GameSoftkeyWidget.h"

#include "WindowApp.h"

CDH_GameSofkeyWidget::CDH_GameSofkeyWidget()
{
    ImageRes image("SUR_HUD_PAUSE_BUTTON");
    SetImage(image);
    SetDesiredWidth(ImageWidth(image.GetSurface()));
    SetDesiredHeight(ImageHeight(image.GetSurface()));
}

CDH_GameSofkeyWidget::~CDH_GameSofkeyWidget()
{
}

int CDH_GameSofkeyWidget::GetPauseButtonHeight()
{
    ImageRes image("SUR_HUD_PAUSE_BUTTON");
    return ImageHeight(image.GetSurface());
}

void CDH_GameSofkeyWidget::OnPointerEvent(Event& event)
{
    if (event.m_type == Event::POINTER_PRESSED) {
        WindowApp::HandleEvent(Event::COMMAND, 0x121683a7, 0, 0, 0);
        event.Clear();
    }
}
