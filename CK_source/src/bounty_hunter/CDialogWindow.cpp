#include "CDialogWindow.h"

#include "App.h"
#include "AppSettings.h"
#include "BaseDialog.h"
#include "CBH_MenuButton.h"
#include "CBH_Player.h"
#include "CBH_SurfaceManager.h"
#include "CCheckBox.h"
#include "CDH_SoundManager.h"
#include "CEnergy.h"
#include "CFontMgr.h"
#include "CFriendAvatarWindow.h"
#include "CGameAnalytics.h"
#include "CNGS.h"
#include "CNGSLocalUser.h"
#include "CNGSSocialInterface.h"
#include "CNGSUser.h"
#include "CNGS_Platform.h"
#include "CNewsFeedManager.h"
#include "CNotificationHandler.h"
#include "CShopItem.h"
#include "CSocialGiftsManager.h"
#include "CStrWChar.h"
#include "CUtility.h"
#include "SScreenAdaptingConstant.h"
#include "ServiceWindows.h"

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char DialogWindowSizeCheck[sizeof(CDialogWindow) == 0x11c ? 1 : -1];
#endif

// Tunnel commands of the three dialog buttons.
static const int BUTTON_COMMAND_FIRST = (int)0x9f523292;
static const int BUTTON_COMMAND_SECOND = 0x385a651b;
static const int BUTTON_COMMAND_THIRD = 0x385a651a;
static const int BUTTON_FLAGS = 0x2001;

// Screen-adapted layout constants (in this order in __MergedGlobals).
static const SScreenAdaptingConstant EXTRA_X(40, 80);
static const SScreenAdaptingConstant EXTRA_Y(30, 60);
static const SScreenAdaptingConstant TITLE_RIGHT(75, 150);
static const SScreenAdaptingConstant TITLE_TOP(30, 80);
static const SScreenAdaptingConstant CONTENT_X(150, 300);
static const SScreenAdaptingConstant CONTENT_Y(60, 140);
static const SScreenAdaptingConstant BUTTON_BOTTOM(60, 120);
static const SScreenAdaptingConstant DOWN_TEXT_TOP(4, 4);
// Only user dead-stripped in the shipped binary; it keeps its slot at __MergedGlobals+0x80.
static const SScreenAdaptingConstant STRIPPED_X(40, 80);
static const SScreenAdaptingConstant CONTENT_SHIFT(30, 60);
static const SScreenAdaptingConstant TEXT_TITLE_LEFT(0, 0);
static const SScreenAdaptingConstant TEXT_CONTENT_X(0, 0);
static const SScreenAdaptingConstant DELAYED_CONTENT_TOP(50, 110);
static const SScreenAdaptingConstant DELAYED_TITLE_TOP(25, 60);

CDialogWindow::CDialogWindow(bool telephone)
    : Window(false)
{
    m_extraImageName = NULL;
    m_downTextCount = 0;
    m_fieldd0 = 0;
    m_fieldd4 = 0;
    m_buttons[0] = NULL;
    m_buttons[1] = NULL;
    m_commands[0] = -1;
    m_commands[1] = -1;
    m_commandParams[0] = 0;
    m_commandParams[1] = 0;
    m_commandParams[2] = 0;
    m_arrangeButtons = true;
    m_telephone = telephone;
    m_closing = false;
    m_animateOpen = true;
    m_animateClose = true;
    m_sound = NULL;
    m_soundPlayed = false;
    m_buttonTypes[0] = BUTTON_NONE;
    m_buttonTypes[1] = BUTTON_NONE;
    m_buttonTypes[2] = BUTTON_NONE;

    m_backgroundSurface = static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->CreateSurface("IDB_DIALOG_WINDOW_BG", true);
    SetLayoutType(LAYOUT_NONE);
    SetSize(ImageWidth(m_backgroundSurface), ImageHeight(m_backgroundSurface));
    SetCorner(static_cast<App*>(WindowApp::m_instance)->m_screenWidth / 2 - m_width / 2,
              static_cast<App*>(WindowApp::m_instance)->m_screenHeight / 2 - m_height / 2 + 20);

    m_backgroundImage = new ImageWindow();
    m_backgroundImage->SetImage(m_backgroundSurface);
    m_backgroundImage->SetAlign(ALIGN_LEFT | ALIGN_TOP);
    m_backgroundImage->SetLayoutType(LAYOUT_NONE);
    m_backgroundImage->SetCorner(0, 0);
    AddToFront(m_backgroundImage);

    m_extraImage = new ImageWindow();
    m_extraImage->SetLayoutType(LAYOUT_NONE);
    m_extraImage->SetAlign(ALIGN_LEFT | ALIGN_TOP);
    m_extraImage->SetCorner(EXTRA_X(), EXTRA_Y());
    AddToFront(m_extraImage);

    m_title = new TextWindow();
    m_title->SetLayoutType(LAYOUT_SIMPLE);
    m_title->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
    m_title->SetOutsetSpacing(TITLE_TOP(), 0, 0, TITLE_RIGHT());
    AddToFront(m_title);

    m_content = new WindowTransparent(false);
    m_content->SetLayoutType(LAYOUT_NONE);
    m_content->SetCorner(CONTENT_X(), CONTENT_Y());
    m_content->SetPercentWidth(55, 0, 0);
    m_content->SetHeightByContent(0, 0);

    m_message = new TextWindow();
    m_message->SetPercentWidth(100, 0, 0);
    m_message->SetCellPos(0, 0, 1, 1);
    m_message->SetHeightByContent(0, 0);
    m_content->AddToFront(m_message);

    m_downText = new WindowTransparent(false);
    m_downText->SetPercentWidth(100, 0, 0);
    m_downText->SetHeightByContent(0, 0);
    m_downText->SetAlign(ALIGN_LEFT);
    Window* cell;
    m_content->AddToFront(cell = createTD(m_downText, 0, 1, 3));
    cell->SetOutsetSpacing(DOWN_TEXT_TOP(), 0, 0, 0);
    AddToFront(m_content);

    m_buttons[0] = new CBH_MenuButton();
    m_buttons[0]->ClearFlags(BUTTON_FLAGS);
    AddToFront(m_buttons[0]);
    m_buttons[1] = new CBH_MenuButton();
    m_buttons[1]->ClearFlags(BUTTON_FLAGS);
    AddToFront(m_buttons[1]);
    m_buttons[2] = new CBH_MenuButton();
    m_buttons[2]->ClearFlags(BUTTON_FLAGS);
    AddToFront(m_buttons[2]);
    m_buttons[0]->SetLayoutType(LAYOUT_NONE);
    m_buttons[1]->SetLayoutType(LAYOUT_NONE);
    m_buttons[2]->SetLayoutType(LAYOUT_NONE);
    if (telephone == true) {
        SetExtraImageFromImageID("IDB_DIALOG_TELEPHONE");
    }
}

void CDialogWindow::SetExtraImageFromImageID(const char* name)
{
    if (name == NULL) {
        return;
    }
    if (m_extraImageName != NULL) {
        static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(m_extraImageName);
    }
    m_extraImageName = name;
    m_extraImage->SetImage(static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->CreateSurface(name, true));
}

CDialogWindow::~CDialogWindow()
{
    static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(m_backgroundSurface);
    if (m_extraImageName != NULL) {
        static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(m_extraImageName);
    }
}

ImageWindow* CDialogWindow::GetBackgroundImage()
{
    return m_backgroundImage;
}

void CDialogWindow::Paint(ICGraphics2d& graphics)
{
    if (!m_soundPlayed && m_sound != NULL) {
        static_cast<App*>(WindowApp::m_instance)->m_soundManager->PlaySound(m_sound, false, false);
        m_soundPlayed = true;
    }
    Window::Paint(graphics);
}

void CDialogWindow::SetSound(const char* name)
{
    m_sound = name;
}

void CDialogWindow::SetExtraImage(ICRenderSurface* surface)
{
    m_extraImage->SetImage(surface);
}

void CDialogWindow::SetTitleText(const char* id)
{
    SetTitleText(XString(CUtility::GetString(id).GetWCharPtr()));
}

void CDialogWindow::SetTitleText(XString text)
{
    m_title->SetText(text.Upper(), CFontMgr::GetInstance()->GetFont(10));
}

void CDialogWindow::SetMessageText(const XString& text, CFont* font)
{
    if (font == NULL) {
        font = CFontMgr::GetInstance()->GetFont(15);
    } else {
        m_message->SetDesiredHeight(SScreenAdaptingConstant(130, 200)());
    }
    m_message->SetText(text, font);
}

void CDialogWindow::SetButtons(EDialogButton first, EDialogButton second, EDialogButton third)
{
    m_buttonTypes[0] = first;
    m_buttonTypes[1] = second;
    m_buttonTypes[2] = third;
    if (first != BUTTON_NONE) {
        CreateButton(m_buttons[0], first);
        CBH_MenuButton* button0 = m_buttons[0];
        button0->m_command = BUTTON_COMMAND_FIRST;
        button0->m_commandParameter1 = 0;
        button0->m_commandParameter2 = 0;
        button0->m_commandParameter3 = 0;
        m_buttons[0]->SetFlags(BUTTON_FLAGS);
    }
    if (second != BUTTON_NONE) {
        CreateButton(m_buttons[1], second);
        CBH_MenuButton* button1 = m_buttons[1];
        button1->m_command = BUTTON_COMMAND_SECOND;
        button1->m_commandParameter1 = 0;
        button1->m_commandParameter2 = 0;
        button1->m_commandParameter3 = 0;
        m_buttons[1]->SetFlags(BUTTON_FLAGS);
    }
    if (third != BUTTON_NONE) {
        CreateButton(m_buttons[2], third);
        CBH_MenuButton* button2 = m_buttons[2];
        button2->m_command = BUTTON_COMMAND_THIRD;
        button2->m_commandParameter1 = 0;
        button2->m_commandParameter2 = 0;
        button2->m_commandParameter3 = 0;
        m_buttons[2]->SetFlags(BUTTON_FLAGS);
    }
    if (!m_arrangeButtons) {
        return;
    }
    int start = 120;
    if (m_telephone) {
        start = 180;
    }
    bool both = first != BUTTON_NONE && second != BUTTON_NONE;
    if (both && third != BUTTON_NONE) {
        m_buttons[0]->SetCorner(start * App::Scale(), m_height - m_buttons[0]->m_height - BUTTON_BOTTOM());
        m_buttons[1]->SetCorner(m_buttons[0]->Left() + m_buttons[0]->m_width + 5 * App::Scale(),
                                m_height - m_buttons[1]->m_height - BUTTON_BOTTOM());
        m_buttons[2]->SetCorner(m_buttons[1]->Left() + m_buttons[1]->m_width + 5 * App::Scale(),
                                m_height - m_buttons[2]->m_height - BUTTON_BOTTOM());
    } else if (both) {
        m_buttons[0]->SetCorner(start * App::Scale(), m_height - m_buttons[0]->m_height - BUTTON_BOTTOM());
        m_buttons[1]->SetCorner(m_buttons[0]->Left() + m_buttons[0]->m_width + 20 * App::Scale(),
                                m_height - m_buttons[1]->m_height - BUTTON_BOTTOM());
    } else if (first != BUTTON_NONE) {
        m_buttons[0]->SetCorner(m_width / 2 - m_buttons[0]->m_width / 2, m_height - m_buttons[0]->m_height - BUTTON_BOTTOM());
    } else if (second != BUTTON_NONE) {
        m_buttons[1]->SetCorner(m_width / 2 - m_buttons[1]->m_width / 2, m_height - m_buttons[1]->m_height - BUTTON_BOTTOM());
    }
}

void CDialogWindow::CreateButton(CBH_MenuButton* button, EDialogButton type)
{
    button->m_buttonType = type;
    switch (type) {
    case BUTTON_BUY:
        button->setImage("IDB_DIALOG_BUY_BUTTON");
        button->SetCaption("IDS_DIALOG_BUTTON_BUY");
        break;
    case BUTTON_ACCEPT:
        button->setImage("IDB_DIALOG_BUY_BUTTON");
        button->SetCaption("IDS_DIALOG_BUTTON_ACCEPT");
        break;
    case BUTTON_IGNORE:
        button->setImage("IDB_DIALOG_IGNORE_BUTTON");
        button->SetCaption("IDS_DIALOG_BUTTON_IGNORE");
        break;
    case BUTTON_OK:
        button->setImage("IDB_DIALOG_OK_BUTTON");
        button->SetCaption("IDS_SK_OK");
        break;
    case BUTTON_YES:
        button->setImage("IDB_DIALOG_OK_BUTTON");
        button->SetCaption("IDS_YES");
        break;
    case BUTTON_NO:
        button->setImage("IDB_DIALOG_IGNORE_BUTTON");
        button->SetCaption("IDS_NO");
        break;
    case BUTTON_GET:
        button->setImage("IDB_DIALOG_BUY_BUTTON");
        button->SetCaption("IDS_DIALOG_BUTTON_GET");
        break;
    case BUTTON_LATER:
        button->setImage("IDB_DIALOG_IGNORE_BUTTON");
        button->SetCaption("IDS_DIALOG_BUTTON_LATER");
        break;
    case BUTTON_AMMO:
        button->setImage("IDB_DIALOG_BUY_BUTTON");
        button->SetCaption("IDS_DIALOG_BUTTON_AMMO");
        break;
    case BUTTON_WEAPON:
        button->setImage("IDB_DIALOG_BUY_BUTTON");
        button->SetCaption("IDS_DIALOG_BUTTON_WEAPON");
        break;
    case BUTTON_EQUIP:
        button->setImage("IDB_BUTTON_SET_BOUNTY");
        button->SetCaption("IDS_EQUIP");
        break;
    case BUTTON_LOCAL_SAVE:
        button->setImage("IDB_DIALOG_OK_BUTTON");
        button->SetCaption("IDS_CHOOSE_LOCAL_SAVE");
        break;
    case BUTTON_SERVER_SAVE:
        button->setImage("IDB_DIALOG_OK_BUTTON");
        button->SetCaption("IDS_CHOOSE_SERVER_SAVE");
        break;
    case BUTTON_RETRY:
        button->setImage("IDB_DIALOG_OK_BUTTON");
        button->SetCaption("IDS_RETRY");
        break;
    case BUTTON_ACKNOWLEDGE:
        button->setImage("IDB_DIALOG_OK_BUTTON");
        button->SetCaption("IDS_SK_OK");
        break;
    case BUTTON_HIRE:
        button->setImage("IDB_DIALOG_OK_BUTTON");
        button->SetCaption("IDS_FRIENDS_HIRE_FRIEND");
        break;
    case BUTTON_HELP:
        button->setImage("IDB_DIALOG_OK_BUTTON");
        button->SetCaption("IDS_MENU_HELP");
        break;
    case BUTTON_FACEBOOK_SHARE:
        button->setImage("IDB_DIALOG_OK_BUTTON");
        button->SetCaption("IDS_DIALOG_BUTTON_FB_SHARE");
        break;
    case BUTTON_GAME_CENTER_LOGOUT:
        button->setImage("IDB_DIALOG_OK_BUTTON");
        button->SetCaption("IDS_ACCOUNT_LOGOUT_GC");
        break;
    case BUTTON_FACEBOOK_LOGOUT:
        button->setImage("IDB_DIALOG_OK_BUTTON");
        button->SetCaption("IDS_ACCOUNT_LOGOUT_FB");
        break;
    }
}

void CDialogWindow::SetButtonsCommands(int first, int second, int third)
{
    m_commands[0] = first;
    m_commands[1] = second;
    m_commands[2] = third;
}

void CDialogWindow::SetButtonsCommandFirstParam(int first, int second, int third)
{
    m_commandParams[0] = first;
    m_commandParams[1] = second;
    m_commandParams[2] = third;
}

void CDialogWindow::OnCommand(Event& event)
{
    switch (event.m_id) {
    case BUTTON_COMMAND_FIRST:
        if (event.m_handler != NULL) {
            return;
        }
        if (m_commands[0] != -1) {
            WindowApp::HandleTunnelCommand(m_commands[0], m_commandParams[0], 0, 0);
        }
        EasyClose();
        event.m_handler = this;
        break;
    case BUTTON_COMMAND_SECOND:
        if (event.m_handler != NULL) {
            return;
        }
        if (m_commands[1] != -1) {
            WindowApp::HandleTunnelCommand(m_commands[1], m_commandParams[1], 0, 0);
        }
        EasyClose();
        event.m_handler = this;
        break;
    case BUTTON_COMMAND_THIRD:
        if (event.m_handler != NULL) {
            return;
        }
        if (m_commands[2] != -1) {
            WindowApp::HandleTunnelCommand(m_commands[2], m_commandParams[2], 0, 0);
        }
        EasyClose();
        event.m_handler = this;
        break;
    }
}

void CDialogWindow::EasyClose()
{
    if (!m_animateClose) {
        Close();
        return;
    }
    m_closing = true;
    EasyParams params(this, (EasyLib::EMethod)25);
    params.SetMoving(m_x, m_y, m_x, m_parent->m_height);
    params.SetFullTime(0.4f);
    params.SetDelayTime(0.4f);
    params.SetOpacity(1.0f, 0.0f);
    StartEasy(params, 0x100);
}

void CDialogWindow::AddDownTextWithIcon(const XString& text, const char* image, bool right, bool spacing, eGameFont font)
{
    LabelWithIcon* label = new LabelWithIcon(image, text, right, CFontMgr::GetInstance()->GetFont(font), 0);
    if (spacing) {
        if (m_downTextCount != 0) {
            label->SetOutsetSpacing(0, App::Scale() * 4);
        } else {
            label->SetOutsetSpacing(0, App::Scale() * 4, 0, 0);
        }
    }
    m_downText->AddToFront(label, m_downTextCount++, 0);
}

void CDialogWindow::OnStartupEasy()
{
    if (m_animateOpen) {
        EasyParams params(this, (EasyLib::EMethod)26);
        params.SetMoving(m_x, m_parent->m_height, m_x, m_y);
        params.SetFullTime(0.4f);
        params.SetDelayTime(0.1f);
        params.SetOpacity(0.0f, 1.0f);
        StartEasy(params, 0x100);
    }
}

void CDialogWindow::OnEasyFinished()
{
    if (m_closing) {
        Close();
    }
}

void CDialogWindow::SetEasyStyle(bool start, bool close)
{
    m_animateClose = close;
    m_animateOpen = start;
}

void CDialogWindow::SetMissionRewards(int money, int experience, int energy, bool targeting)
{
    AddDownTextWithIcon(XString::Format(L"+%d", money), "IDB_HUD_ICON_MONEY", false, true, GAME_FONT_DEFAULT);
    AddDownTextWithIcon(XString::Format(L"+%d", experience), "IDB_HUD_ICON_EXP", false, true, GAME_FONT_DEFAULT);
    if (!CBH_Player::GetInstance()->IsEnegyEnabled()) {
        return;
    }
    if (targeting) {
        AddDownTextWithIcon(XString::Format(L"-%d", energy), "IDB_HUD_ICON_ENERGY", false, false, GAME_FONT_DEFAULT);
    } else {
        XString cost = ResString("IDS_DIALOG_COST");
        AddDownTextWithIcon(XString::Format(L"   %ls:", cost.c_str()), NULL, false, false, GAME_FONT_DEFAULT);
        AddDownTextWithIcon(XString(energy), "IDB_HUD_ICON_ENERGY", false, false, GAME_FONT_DEFAULT);
    }
}

CBeforeHuntDialog::CBeforeHuntDialog()
    : CDialogWindow(true)
{
    m_content->SetPercentWidth(48, 0, 0);
    m_content->SetCorner(m_content->m_x + CONTENT_SHIFT(), m_content->m_y);
    m_downText->SetWidthByContent(0, 0);
}

CTextDialog::CTextDialog()
    : CDialogWindow(false), m_buttonOffsetY(0)
{
    m_message->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    m_content->SetLayoutType(LAYOUT_SIMPLE);
    m_content->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
    m_content->SetOutsetSpacing(CONTENT_Y(), 0, 0, 0);
    m_content->SetPercentWidth(60, 0, 0);
    m_title->SetOutsetSpacing(TITLE_TOP(), 0, 0, TEXT_TITLE_LEFT());
    m_content->SetCorner(TEXT_CONTENT_X(), CONTENT_Y());
    m_downText->SetPercentWidth(50, 0, 0);
    m_arrangeButtons = false;
}

void CTextDialog::SetButtons(EDialogButton first, EDialogButton second, EDialogButton third)
{
    CDialogWindow::SetButtons(first, second, third);
    if (m_buttons[0]->m_buttonType >= BUTTON_NONE && m_buttons[1]->m_buttonType >= BUTTON_NONE) {
        m_buttons[0]->SetCorner(m_width / 2 - m_buttons[0]->m_width - 10 * App::Scale(),
                                m_height - m_buttons[0]->m_height - BUTTON_BOTTOM());
        m_buttons[1]->SetCorner(m_width / 2 + 10 * App::Scale(), m_height - m_buttons[1]->m_height - BUTTON_BOTTOM());
    } else {
        m_buttons[0]->SetCorner(m_width / 2 - m_buttons[0]->m_width / 2,
                                m_height - m_buttons[0]->m_height - BUTTON_BOTTOM() + m_buttonOffsetY);
    }
}

CDialogWithDelayedButtons::CDialogWithDelayedButtons(int delay)
    : CTextDialog()
{
    m_delay = delay;
    m_content->SetPercentWidth(78, 0, 0);
    m_buttonOffsetY = SScreenAdaptingConstant(7, 14)();
    m_content->SetOutsetSpacing(DELAYED_CONTENT_TOP(), 0, 0, SScreenAdaptingConstant(10, 20)());
    m_message->SetAlign(ALIGN_LEFT);
    m_title->SetOutsetSpacing(DELAYED_TITLE_TOP(), 0, 0, TEXT_TITLE_LEFT());
}

void CDialogWithDelayedButtons::Update()
{
    if (m_delay > 0) {
        m_delay -= WindowApp::m_instance->m_updateFrameTime;
    }
    if (m_delay > 0) {
        if (m_buttons[0] != NULL) {
            m_buttons[0]->ClearFlags(BUTTON_FLAGS);
        }
        if (m_buttons[1] != NULL) {
            m_buttons[1]->ClearFlags(BUTTON_FLAGS);
        }
        if (m_buttons[2] != NULL) {
            m_buttons[2]->ClearFlags(BUTTON_FLAGS);
        }
    } else {
        if (m_buttons[0] != NULL && !m_buttons[0]->IsVisible() && m_buttonTypes[0] != BUTTON_NONE) {
            m_buttons[0]->SetFlags(BUTTON_FLAGS);
        }
        if (m_buttons[1] != NULL && !m_buttons[1]->IsVisible() && m_buttonTypes[1] != BUTTON_NONE) {
            m_buttons[1]->SetFlags(BUTTON_FLAGS);
        }
        if (m_buttons[2] != NULL && !m_buttons[2]->IsVisible() && m_buttonTypes[2] != BUTTON_NONE) {
            m_buttons[2]->SetFlags(BUTTON_FLAGS);
        }
    }
}

CBeforeTargettingDialog::CBeforeTargettingDialog(CStrWChar friendId)
    : CDialogWindow(true)
{
    m_loadPicture = false;
    m_user = NULL;
    m_avatar = new CFriendAvatarWindow(friendId);
    m_avatar->SetLayoutType(LAYOUT_SIMPLE);
    m_avatar->SetAlign(ALIGN_LEFT | ALIGN_TOP);
    m_avatar->SetDesiredWidth(SScreenAdaptingConstant(50, 100)());
    m_avatar->SetDesiredHeight(SScreenAdaptingConstant(50, 100)());
    m_avatar->SetOutsetSpacing(SScreenAdaptingConstant(75, 150)(), 0, 0, SScreenAdaptingConstant(90, 180)());
    m_avatar->m_debugFrame = true;
    m_avatar->SetDefaultImage("IDB_TARGETTING_DEFAULT_AVATAR");
    AddToFront(m_avatar);
    m_content->SetPercentWidth(48, 0, 0);
    m_content->SetCorner(m_content->m_x + CONTENT_SHIFT(), m_content->m_y);
    m_downText->SetWidthByContent(0, 0);
}

CBeforeTargettingDialog::CBeforeTargettingDialog(int avatarId)
    : CDialogWindow(true)
{
    m_loadPicture = false;
    m_user = NULL;
    m_avatar = new CFriendAvatarWindow(avatarId);
    m_avatar->SetLayoutType(LAYOUT_SIMPLE);
    m_avatar->SetAlign(ALIGN_LEFT | ALIGN_TOP);
    m_avatar->SetDesiredWidth(SScreenAdaptingConstant(50, 100)());
    m_avatar->SetDesiredHeight(SScreenAdaptingConstant(50, 100)());
    m_avatar->SetOutsetSpacing(SScreenAdaptingConstant(75, 150)(), 0, 0, SScreenAdaptingConstant(90, 180)());
    m_avatar->m_debugFrame = true;
    m_avatar->SetDefaultImage("IDB_TARGETTING_DEFAULT_AVATAR");
    AddToFront(m_avatar);
    m_content->SetPercentWidth(48, 0, 0);
    m_content->SetCorner(m_content->m_x + CONTENT_SHIFT(), m_content->m_y);
    m_downText->SetWidthByContent(0, 0);
}

void CBeforeTargettingDialog::SetMessageText(const XString& text, CFont* font)
{
    CDialogWindow::SetMessageText(text, font);
    m_message->SetDesiredHeight(SScreenAdaptingConstant(80, 130)());
    m_message->SetOutsetSpacing(0, 0, 0, SScreenAdaptingConstant(25, 50)());
    m_message->m_debugFrame = true;
}

void CBeforeTargettingDialog::Update()
{
    if (m_loadPicture) {
        CStrWChar facebookId = m_user->GetIDForSocialNetwork((eSocialNetworkType)2);
        CNGS_Platform::GetInstance()->getSocialInterface(2)->getFriendPicWithID(facebookId, m_pictureData, m_pictureSize);
        if ((m_pictureSize != 0 || m_pictureData != NULL) && *(const unsigned char*)m_pictureData == 0xff) {
            SetExtraImage(static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->CreateSurface(
                m_pictureData, m_pictureSize, 0x7179dacc));
            m_loadPicture = false;
        }
    }
}

CServerGiftDialog::CServerGiftDialog(const SServerAwardMessage* award)
    : CDialogWindow(false)
{
    m_message->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    m_message->m_debugFrame = true;
    m_content->SetLayoutType(LAYOUT_SIMPLE);
    m_content->m_debugFrame = true;
    m_content->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
    m_content->SetOutsetSpacing(TITLE_TOP() * 2, 0, 0, TEXT_TITLE_LEFT());
    m_content->SetPercentWidth(80, 0, 0);
    m_title->SetOutsetSpacing(TITLE_TOP(), 0, 0, TEXT_TITLE_LEFT());
    m_content->SetCorner(TEXT_CONTENT_X(), CONTENT_Y());
    m_downText->SetPercentWidth(80, 0, 0);
    m_award = award;
    if (award->m_itemIndex >= 0) {
        SGift* gift = static_cast<App*>(WindowApp::m_instance)->m_socialGiftManager->GetGift(award->m_itemIndex);
        if (gift != NULL) {
            CItem* item = gift->m_itemManager->getItemById(gift->m_itemId);
            if (item != NULL) {
                m_extraImage->SetImage(item->getShopImage());
                m_extraImage->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
            }
        }
    }
}

void CServerGiftDialog::OnCommand(Event& event)
{
    if (event.m_id != BUTTON_COMMAND_FIRST) {
        return;
    }
    CBH_Player::GetInstance()->AddMoney(m_award->m_commonCurrency, GAIN_SOURCE_IAP);
    CBH_Player::GetInstance()->AddCredits(m_award->m_rareCurrency);
    if (m_award->m_experience > 0) {
        CBH_Player::GetInstance()->AddXp(m_award->m_experience, GAIN_SOURCE_IAP);
        WindowApp::HandleTunnelCommand((int)0xbd680582, 0, 0, 0);
    }
    if (m_award->m_commonCurrency > 0 || m_award->m_rareCurrency > 0) {
        CGameAnalytics::logCurrencyRecieved(m_award->m_commonCurrency, m_award->m_rareCurrency);
    }
    if (m_award->m_itemIndex >= 0) {
        SGift* gift = static_cast<App*>(WindowApp::m_instance)->m_socialGiftManager->GetGift(m_award->m_itemIndex);
        if (gift != NULL) {
            CItem* item = gift->m_itemManager->getItemById(gift->m_itemId);
            if (item != NULL) {
                if (item->getType() == 4) {
                    int energy = static_cast<CEnergy*>(item)->m_energy;
                    CBH_Player::GetInstance()->AddEnergy(energy, true);
                    CGameAnalytics::logGiftAccepted(0, item->m_name);
                } else {
                    item->setAvailable(true);
                    CGameAnalytics::logGiftAccepted(0, item->m_name);
                }
            }
        }
    }
    static_cast<App*>(WindowApp::m_instance)->m_newsFeedManager->RemoveMessage(m_award);
    CNGS::GetInstance()->GetLocalUser()->m_notificationHandler->RemoveGameSpecificMessage(m_award->m_messageId);
    event.Clear();
    Close();
}

CChallengeCompletedDialog::CChallengeCompletedDialog()
    : CTextDialog()
{
    Window* table = new Window(false);
    m_backgroundImage->AddToFront(table);
    Window* top = new Window(false);
    table->AddToFront(top, 0, 0);
    top->SetPercentHeight(8, 0, 0);
    Window* middle = new Window(false);
    table->AddToFront(middle, 0, 1);
    middle->SetHeightByContent(0, 0);
    ImageWindow* shade = new ImageWindow();
    shade->SetFillType(ImageWindow::FILL_NONE);
    shade->SetImageById("IDB_SHOP_BACKGROUND");
    shade->SetLayoutType(LAYOUT_SIMPLE);
    shade->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    shade->SetOpacity(0.2f);
    middle->AddToFront(shade);
    ImageWindow* image = new ImageWindow();
    image->SetFillType(ImageWindow::FILL_NONE);
    image->SetImageById("IDB_COOP_COMPLETED_BG");
    image->SetLayoutType(LAYOUT_SIMPLE);
    image->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    middle->AddToFront(image);
    Window* bottom = new Window(false);
    table->AddToFront(bottom, 0, 2);
    bottom->SetPercentHeight(10, 0, 0);
}

void CChallengeCompletedDialog::OnCommand(Event& event)
{
    CDialogWindow::OnCommand(event);
    if (event.m_id == BUTTON_COMMAND_FIRST) {
        Close();
    }
}

CServerGiftNotifierDialog::CServerGiftNotifierDialog()
    : CDialogWindow(true)
{
    static_cast<App*>(WindowApp::m_instance)->m_timeScale = 0.0f;
}

void CServerGiftNotifierDialog::OnCommand(Event& event)
{
    CDialogWindow::OnCommand(event);
    if (event.m_id != BUTTON_COMMAND_FIRST) {
        return;
    }
    static_cast<App*>(WindowApp::m_instance)->m_timeScale = 1.0f;
    event.Clear();
    Close();
}

CFacebookLoginDialog::CFacebookLoginDialog()
    : CDialogWindow(true)
{
    m_content->SetCorner(CONTENT_X(), SScreenAdaptingConstant(40, 90)());
    Window* row = new Window(false);
    row->SetPercentHeight(10, 0, 0);
    row->SetPercentWidth(55, 0, 0);
    row->SetOutsetSpacing(SScreenAdaptingConstant(35, 70)(), 0, 0, SScreenAdaptingConstant(150, 300)());
    TextWindow* text = new TextWindow(ResString("IDS_GSERV_DONT_SHOW_FB_LOGIN_PROMPT"), CFontMgr::GetInstance()->GetFont(15));
    text->SetOutsetSpacing(SScreenAdaptingConstant(5, 10)(), 0, 0, 0);
    row->AddToFront(text, 1, 0);
    m_reminderCheckBox = new CCheckBox(0);
    row->AddToFront(m_reminderCheckBox, 0, 0);
    row->SetLayoutType(LAYOUT_SIMPLE);
    row->SetAlign(ALIGN_LEFT | ALIGN_VCENTER);
    AddToFront(row);
}

void CFacebookLoginDialog::OnCommand(Event& event)
{
    switch (event.m_id) {
    case BUTTON_COMMAND_FIRST:
        if (event.m_handler != NULL) {
            return;
        }
        if (m_commands[0] != -1) {
            WindowApp::HandleTunnelCommand(m_commands[0], m_commandParams[0], 0, 0);
            if (m_reminderCheckBox->IsCheched() == true) {
                static_cast<App*>(WindowApp::m_instance)->m_settings->SetFBReminderEnabled(false);
                static_cast<App*>(WindowApp::m_instance)->m_settings->Save();
            }
        }
        EasyClose();
        event.m_handler = this;
        break;
    case BUTTON_COMMAND_SECOND:
        if (event.m_handler != NULL) {
            return;
        }
        if (m_commands[1] != -1) {
            WindowApp::HandleTunnelCommand(m_commands[1], m_commandParams[1], 0, 0);
            if (m_reminderCheckBox->IsCheched() == true) {
                static_cast<App*>(WindowApp::m_instance)->m_settings->SetFBReminderEnabled(false);
                static_cast<App*>(WindowApp::m_instance)->m_settings->Save();
            }
        }
        EasyClose();
        event.m_handler = this;
        break;
    }
}

// Dead-stripped in the shipped binary: some function of this unit read STRIPPED_X (its value stays at
// __MergedGlobals+0x80). The name and the body are guesses; only the reference matters.
int CDialogWindow_StrippedLayoutX()
{
    return STRIPPED_X();
}

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char CKTextDialogSize[sizeof(CTextDialog) == 0x120 ? 1 : -1];
#endif
