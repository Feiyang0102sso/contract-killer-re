// CConflictLogoutPrompt.cpp - shows both connected accounts and offers to log out of one.
#include "CConflictLogoutPrompt.h"
#include "App.h"
#include "BaseDialog.h"
#include "CBH_MenuButton.h"
#include "CFontMgr.h"
#include "CImageButton.h"
#include "CNGS_Platform.h"
#include "CNGSSocialInterface.h"
#include "CStrWChar.h"
#include "DGHelper.h"
#include "ICSystemFont.h"
#include "SScreenAdaptingConstant.h"
#include "ServiceWindows.h"

CConflictLogoutPrompt::CConflictLogoutPrompt()
{
    SetTitle(Window::ResString("IDS_MENU_HELP"));
    Window* layout = new Window(false);
    AddToFront(layout);

    Window* texts = CreateTexts();
    texts->SetPercentWidth(80, 0, 0);
    texts->SetPercentHeight(50, 0, 0);
    texts->SetLayoutType((ELayoutType)1);
    texts->SetAlign(0x24);
    layout->AddToFront(texts);

    Window* buttons = CreateButtons();
    buttons->SetPercentHeight(20, 0, 0);
    buttons->SetPercentWidth(100, 0, 0);
    buttons->SetLayoutType((ELayoutType)1);
    buttons->SetAlign(0x10);
    layout->AddToFront(buttons);

    CBH_MenuButton* help = new CBH_MenuButton();
    help->setImage("IDB_BUTTON_QUESTION");
    help->SetLayoutType((ELayoutType)1);
    help->SetAlign(10);
    help->SetOutsetSpacing(SScreenAdaptingConstant(10, 20)(), SScreenAdaptingConstant(10, 20)(), 0, 0);
    help->m_command = 0x8a79f26d;
    help->m_commandParameter1 = 0;
    help->m_commandParameter2 = 0;
    help->m_commandParameter3 = 0;
    layout->AddToFront(help);
}

Window* CConflictLogoutPrompt::CreateTexts()
{
    Window* texts = new Window(false);
    TextWindow* description = new TextWindow(Window::ResString("IDS_ACCOUNT_SAVE_CONFLICT_DESC"),
                                             CFontMgr::GetInstance()->GetFont(15));
    description->SetPercentWidth(100, 0, 0);
    description->SetLayoutType((ELayoutType)1);
    description->SetAlign(0xc);
    description->SetCellPos(0, 0, 1, 1);
    texts->AddToFront(description);

    WindowTransparent* names = new WindowTransparent(false);
    ICSystemFont* font = ICSystemFont::CreateInstance();
    font->Initialize(SScreenAdaptingConstant(12, 25)(), NULL, 1);

    CStrWChar facebookAlias = CNGS_Platform::GetInstance()->getSocialInterface(2)->getPlayerAlias();
    CStrWChar gameCenterAlias = CNGS_Platform::GetInstance()->getSocialInterface(1)->getPlayerAlias();
    XString facebookName;
    XString gameCenterName;
    if (facebookAlias.GetLength() > 0) {
        facebookName = XString(facebookAlias.GetWCharPtr());
    }
    if (gameCenterAlias.GetLength() > 0) {
        gameCenterName = XString(gameCenterAlias.GetWCharPtr());
    }
    // each name gets a quarter of the prompt width
    DGHelper::CropString(facebookName, font, m_width / 4);
    DGHelper::CropString(gameCenterName, font, m_width / 4);

    TextWindow* facebookText = new TextWindow(facebookName, font);
    facebookText->SetCellPos(0, 1, 1, 1);
    facebookText->SetAlign(0x11);
    facebookText->SetFlags(0x800000);
    names->AddToFront(facebookText);
    TextWindow* gameCenterText = new TextWindow(gameCenterName, font);
    gameCenterText->SetCellPos(1, 1, 1, 1);
    gameCenterText->SetAlign(0x12);
    gameCenterText->SetFlags(0x800000);
    names->SetCellPos(0, 1, 1, 1);
    names->AddToFront(gameCenterText);
    texts->AddToFront(names);
    return texts;
}

Window* CConflictLogoutPrompt::CreateButtons()
{
    Window* buttons = new Window(false);
    buttons->AddToFront(CreateButton("IDB_FACEBOOK_ICON", Window::ResString("IDS_ACCOUNT_LOGOUT"), 0x09f954a7), 0, 0);
    buttons->AddToFront(CreateButton("IDB_GAMECENTER_ICON", Window::ResString("IDS_ACCOUNT_LOGOUT"), 0x194eae3b), 1, 0);
    return buttons;
}

Window* CConflictLogoutPrompt::CreateButton(const char* icon, const XString& text, unsigned int command)
{
    Window* holder = new Window(false);
    LabelWithIcon* label = new LabelWithIcon(icon, text, true, CFontMgr::GetInstance()->GetFont(15), 0);
    label->SetPercentHeight(80, 0, 0);
    label->SetPercentWidth(70, 0, 0);
    label->SetLayoutType((ELayoutType)1);
    label->SetAlign(0xc);
    CImageButton* button = new CImageButton();
    button->AddImage("IDB_SHOP_MONEY_BUTTON", 0, 0);
    button->AddToFront(label);
    button->m_command = command;
    button->m_commandParameter1 = 0;
    button->m_commandParameter2 = 0;
    button->m_commandParameter3 = 0;
    button->SetLayoutType((ELayoutType)1);
    button->SetAlign(0x24);
    holder->AddToFront(button);
    return holder;
}

void CConflictLogoutPrompt::OnStartupEasy()
{
    EasyParams params(this, (EasyLib::EMethod)0x1a);
    params.SetMoving(m_x, m_parent->m_height, m_x, m_y);
    params.SetFullTime(0.7f);
    params.SetDelayTime(0.1f);
    params.SetOpacity(0.0f, 1.0f);
    StartEasy(params, 0x100);
}

void CConflictLogoutPrompt::EasyClose()
{
    EasyParams params(this, (EasyLib::EMethod)0x19);
    params.SetMoving(m_x, m_y, m_x, m_parent->m_height);
    params.SetFullTime(0.7f);
    params.SetDelayTime(0.7f);
    params.SetOpacity(1.0f, 0.0f);
    StartEasy(params, 0x100);
}
