#include "CDH_XPWidget.h"

#include "BaseDialog.h"
#include "CFontMgr.h"

CDH_XPWidget::CDH_XPWidget(int amount) : Window(false)
{
    ImageWindow* icon = new ImageWindow(ImageWindow::FILL_NONE, ImageRes("SUR_XP"));
    icon->SetCellPos(0, 0, 1, 1);
    AddToFront(icon);
    CFont* font = CFontMgr::GetInstance()->GetFont(5);
    Window* label = new Window(false);
    label->SetWidthByContent(0, 0);
    label->SetHeightByContent(0, 0);
    AddToFront(label, 1, 0);
    TextWindow* text = new TextWindow(XString::Format(L"+%dXP", amount), font);
    text->SetAlign(ALIGN_LEFT | ALIGN_VCENTER);
    label->AddToFront(text);
    SetWidthByContent(0, 0);
    SetHeightByContent(0, 0);
}
