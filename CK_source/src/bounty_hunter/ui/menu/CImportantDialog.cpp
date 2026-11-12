// CImportantDialog.cpp - message dialog in the blood frame; OK sends the configured command.
#include "CImportantDialog.h"
#include "App.h"
#include "BaseDialog.h"
#include "CCareerWindow.h"
#include "CFontMgr.h"
#include "CImageButton.h"
#include "SScreenAdaptingConstant.h"
#include "WindowApp.h"

CImportantDialog::CImportantDialog()
{
    SetLayoutType((ELayoutType)1);
    SetAlign(0x24);
    CreateInteface();
}

void CImportantDialog::CreateInteface()
{
    Window* messageArea = new Window(false);
    messageArea->SetPercentHeight(80, 0, 0);

    ScrollingContainer* container = new ScrollingContainer();
    container->SetCellPos(0, 0, 1, 1);
    container->SetOutsetSpacing(10, 10, 10, 10);
    container->SetPercentWidth(100, 0, 0);
    container->SetPercentHeight(100, 0, 0);

    m_message = new TextWindow(CFontMgr::GetInstance()->GetFont(15));
    m_message->SetPercentWidth(100, 0, 0);
    m_message->SetLayoutType((ELayoutType)0);
    container->AddToFront(m_message);
    messageArea->AddToFront(container);

    VertScrollerSimpleRed* scroller = new VertScrollerSimpleRed(container);
    scroller->SetCellPos(1, 0, 1, 1);
    scroller->SetOutsetSpacing(3, 5, 3, 4);
    messageArea->AddToFront(scroller);
    m_content->AddToFront(messageArea, 0, 0);

    Window* button = CreateButton(Window::ResString("IDS_SK_OK"));
    button->SetAlign(0x14);
    m_content->AddToFront(button, 0, 1);
}

Window* CImportantDialog::CreateButton(const XString& text)
{
    Window* holder = new Window(false);
    m_okButton = new CImageButton();
    m_okButton->AddImage("IDB_SHOP_MONEY_BUTTON", 0, 0);
    m_okButton->SetLayoutType((ELayoutType)1);
    m_okButton->SetAlign(0x24);
    m_okButton->SetText(text, 0x24, NULL, 0, 0);
    m_okButton->SetOutsetSpacing(0, 0, SScreenAdaptingConstant(10, 20)(), 0);
    m_okButton->m_command = 0x1fddbe68;
    m_okButton->m_commandParameter1 = 0;
    m_okButton->m_commandParameter2 = 0;
    m_okButton->m_commandParameter3 = 0;
    holder->AddToFront(m_okButton);
    return holder;
}

void CImportantDialog::SetMessageText(const XString& text)
{
    m_message->SetText(text);
}

void CImportantDialog::SetCommand(unsigned int command)
{
    m_command = command;
}

void CImportantDialog::OnCommand(Event& event)
{
    if (event.m_id == 0x1fddbe68) {
        WindowApp::HandleTunnelCommand(m_command, 0, 0, 0);
        Close();
    }
}
