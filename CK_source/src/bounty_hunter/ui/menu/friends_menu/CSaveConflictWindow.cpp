// CSaveConflictWindow.cpp - lets the player keep the device save or take the server save.
#include "CSaveConflictWindow.h"
#include "App.h"
#include "BaseDialog.h"
#include "CFontMgr.h"
#include "CNGS.h"
#include "CNGSLocalUser.h"
#include "CNotificationHandler.h"
#include "CStrWChar.h"
#include "SScreenAdaptingConstant.h"
#include "ServiceWindows.h"

CSaveConflictWindow::CSaveConflictWindow(SConflictInfo& info)
{
    // debug summary of both saves; built but never shown
    CStrWChar summary;
    summary.Concatenate("Use Local?\nLocal:\n Server:\n");
    CNotificationHandler* handler = CNGS::GetInstance()->GetLocalUser()->m_notificationHandler;
    if (handler->m_localDataText.GetLength() >= 1 && handler->m_serverDataText.GetLength() >= 1) {
        CStrWChar joined = "Use Local?\nLocal:\n" + handler->m_localDataText + "Server:\n" + handler->m_serverDataText;
        if (joined.GetWCharPtr() != summary.GetWCharPtr()) {
            summary.ReleaseMemory();
            summary.Concatenate(joined.GetWCharPtr());
        }
    }

    SetTitleText("IDS_SAVE_CONFLICT_TITLE");
    Window* fill = new Window(false);
    fill->SetPercentWidth(100, 0, 0);
    fill->SetPercentHeight(100, 0, 0);
    AddToFront(fill);

    ImageWindow* background = new ImageWindow();
    background->SetFillType(ImageWindow::FILL_NONE);
    background->SetImageById("IDB_SHOP_BACKGROUND");
    background->SetLayoutType((ELayoutType)1);
    background->SetAlign(0x24);
    background->SetOutsetSpacing(SScreenAdaptingConstant(-50, -100)(), 0, 0, 0);
    GetBackgroundImage()->AddToFront(background);

    TextWindow* message = new TextWindow(Window::ResString("IDS_SAVE_CONFLICT_MESSAGE"), CFontMgr::GetInstance()->GetFont(15));
    message->m_debugFrame = true;
    message->SetWidthByContent(0, 0);
    message->SetHeightByContent(0, 0);
    message->SetLayoutType((ELayoutType)1);
    message->SetAlign(0xc);
    message->SetOutsetSpacing(SScreenAdaptingConstant(65, 130)(), 0, 0, 0);
    AddToFront(message);

    Window* saves = new Window(false);
    saves->SetPercentHeight(50, 0, 0);
    saves->SetPercentWidth(60, 0, 0);
    saves->SetLayoutType((ELayoutType)1);
    saves->SetAlign(0x14);
    saves->SetOutsetSpacing(0, 0, SScreenAdaptingConstant(75, 150)(), 0);
    saves->AddToFront(CreateSaveInfoWindow(Window::ResString("IDS_SAVE_CONFLICT_LOCAL_TITLE"),
                                           Window::ResString("IDS_SAVE_CONFLICT_LOCAL_BUTTON"), info.m_localLevel,
                                           info.m_localSoftCurrency, info.m_localHardCurrency, 0x7b7b47f1),
                      0, 0);
    saves->AddToFront(CreateSaveInfoWindow(Window::ResString("IDS_SAVE_CONFLICT_SERVER_TITLE"),
                                           Window::ResString("IDS_SAVE_CONFLICT_SERVER_BUTTON"), info.m_serverLevel,
                                           info.m_serverSoftCurrency, info.m_serverHardCurrency, 0xb64f7bf5),
                      1, 0);
    AddToFront(saves);
}

Window* CSaveConflictWindow::CreateSaveInfoWindow(const XString& title, const XString& buttonText, int level,
                                                  int softCurrency, int hardCurrency, unsigned int command)
{
    Window* column = new Window(false);
    TextWindow* titleText = new TextWindow(title, CFontMgr::GetInstance()->GetFont(15));
    titleText->SetAlign(0x24);
    column->AddToFront(titleText, 0, 0);

    Window* label = CreateLabelWithIcon("IDB_HUD_ICON_MONEY", XString(softCurrency));
    label->SetHeightByContent(0, 0);
    column->AddToFront(label, 0, 1);
    label = CreateLabelWithIcon("IDB_HUD_ICON_CREDITS", XString(hardCurrency));
    label->SetHeightByContent(0, 0);
    column->AddToFront(label, 0, 2);
    label = CreateLabelWithIcon("IDB_HUD_ICON_EXP", XString("lvl. ").Append(XString(level)));
    label->SetHeightByContent(0, 0);
    column->AddToFront(label, 0, 3);

    Window* buttonRow = new Window(false);
    buttonRow->SetHeightByContent(0, 0);
    StretchImageButton* button = new StretchImageButton(buttonText, "IDB_HUD_TRANSPARENT_BLACK_LEFT",
                                                        "IDB_HUD_TRANSPARENT_BLACK_TILE",
                                                        "IDB_HUD_TRANSPARENT_BLACK_RIGHT");
    button->SetAlign(0x24);
    button->m_command = command;
    button->m_commandParameter1 = 0;
    button->m_commandParameter2 = 0;
    button->m_commandParameter3 = 0;
    buttonRow->AddToFront(button);
    column->AddToFront(buttonRow, 0, 4);
    return column;
}

void CSaveConflictWindow::OnStartupEasy()
{
    EasyParams params(this, (EasyLib::EMethod)0x1a);
    params.SetMoving(m_x, m_parent->m_height, m_x, m_y);
    params.SetFullTime(0.7f);
    params.SetDelayTime(0.1f);
    params.SetOpacity(0.0f, 1.0f);
    StartEasy(params, 0x100);
}

void CSaveConflictWindow::EasyClose()
{
    EasyParams params(this, (EasyLib::EMethod)0x19);
    params.SetMoving(m_x, m_y, m_x, m_parent->m_height);
    params.SetFullTime(0.7f);
    params.SetDelayTime(0.7f);
    params.SetOpacity(1.0f, 0.0f);
    StartEasy(params, 0x100);
}

void CSaveConflictWindow::OnCommand(Event& event)
{
    int command = event.m_id;
    if (command == (int)0xb64f7bf5 || command == 0x7b7b47f1) {
        Close();
        return;
    }
    if (command == 0x7d8ce451) {
        Close();
    }
}

Window* CSaveConflictWindow::CreateLabelWithIcon(const char* icon, const XString& text)
{
    Window* holder = new Window(false);
    Window* row = new Window(false);
    row->SetLayoutType((ELayoutType)1);
    row->SetAlign(0x24);
    row->SetHeightByContent(0, 0);
    row->SetPercentWidth(50, 0, 0);
    holder->AddToFront(row);
    ImageWindow* image = new ImageWindow();
    image->SetFillType(ImageWindow::FILL_NONE);
    image->SetImageById(icon);
    row->AddToFront(image, 0, 0);
    row->AddToFront(new TextWindow(text, CFontMgr::GetInstance()->GetFont(15)), 1, 0);
    return holder;
}
