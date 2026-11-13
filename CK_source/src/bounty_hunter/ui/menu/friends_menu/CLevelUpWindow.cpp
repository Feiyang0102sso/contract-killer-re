#include "CLevelUpWindow.h"

#include "App.h"
#include "SScreenAdaptingConstant.h"
#include "CFontMgr.h"
#include "CDH_SoundManager.h"
#include "SG_Instance.h"
#include "SG_Presenter.h"
#include "ServiceWindows.h"

CLevelUpAnimWindow::CLevelUpAnimWindow()
    : Window(false)
{
    SG_Instance* sprites = new SG_Instance();
    m_sprites = sprites;
    const char* resources = "LEVEL_UP_WVGA";
    if (!App::IsWVGA()) {
        if (App::IsHD()) {
            resources = "LEVEL_UP_VGA";
        } else {
            resources = "LEVEL_UP_HVGA";
        }
    }
    sprites->Init(const_cast<char*>(resources));
    sprites = m_sprites;
    unsigned char finished = 0;
    if (!App::IsWVGA()) {
        App::IsHD();
    }
    sprites->QueueArchetypeCharacter(0, 0);
    while (sprites->LoadQueued(1000, finished)) {
    }
    m_animation = new SG_Presenter(0, 0);
    m_animation->SetSGInstance(sprites);
    m_animation->SetAnimation(0);
    m_animation->SetLoop(false);
}

CLevelUpAnimWindow::~CLevelUpAnimWindow()
{
    SG_Instance* sprites = static_cast<App*>(WindowApp::m_instance)->m_sgInstance;
    if (!App::IsWVGA()) {
        App::IsHD();
    }
    sprites->DumpArchetypeCharacter(0, 0);
    if (m_animation != NULL) {
        delete m_animation;
        m_animation = NULL;
    }
    if (m_sprites != NULL) {
        delete m_sprites;
        m_sprites = NULL;
    }
}

void CLevelUpAnimWindow::Paint(ICGraphics2d&)
{
    m_animation->Draw(m_width / 2, m_height / 2);
}

void CLevelUpAnimWindow::Update()
{
    m_animation->Update(WindowApp::m_instance->m_updateFrameTime);
}

CLevelUpWindow::CLevelUpWindow(XString message, bool allowShare)
    : CTextDialog(), m_closeCommand(0)
{
    m_levelMessage = message;
    SetTitleText("");
    Window* controls = new Window(false);
    controls->SetPercentWidth(100, 0, 0);
    controls->SetPercentHeight(100, 0, 0);
    AddToFront(controls);

    ImageWindow* backdrop = new ImageWindow();
    backdrop->SetFillType(ImageWindow::FILL_NONE);
    backdrop->SetImageById("IDB_SHOP_BACKGROUND");
    backdrop->SetLayoutType(LAYOUT_SIMPLE);
    backdrop->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    backdrop->SetOutsetSpacing(SScreenAdaptingConstant(-50, -100)(), 0, 0, 0);
    GetBackgroundImage()->AddToFront(backdrop);
    AddMessageText();

    if (allowShare == true) {
        WindowTransparent* shareSpacing = new WindowTransparent(false);
        shareSpacing->SetCellPos(0, 1, 1, 1);
        TextWindow* spacer = new TextWindow(XString(" "), CFontMgr::GetInstance()->GetFont(22));
        spacer->SetCellPos(0, 1, 1, 1);
        spacer->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
        shareSpacing->AddToFront(spacer);
        int right = SScreenAdaptingConstant(90, 180)();
        int bottom = SScreenAdaptingConstant(70, 140)();
        shareSpacing->SetOutsetSpacing(0, right, bottom, 0);
        AddToFront(shareSpacing);
    }

    Window* buttons = CreateButtons(allowShare);
    buttons->SetPercentHeight(20, 0, 0);
    buttons->SetPercentWidth(40, 0, 0);
    buttons->SetLayoutType(LAYOUT_SIMPLE);
    buttons->SetAlign(ALIGN_HCENTER | ALIGN_BOTTOM);
    buttons->SetOutsetSpacing(0, 0, SScreenAdaptingConstant(40, 80)(), 0);
    controls->AddToFront(buttons);
}

void CLevelUpWindow::OnCommand(Event& event)
{
    if (event.m_id == 0x6e7d5203) {
        WindowApp::HandleTunnelCommand(0x6e7d5204, 0, 0, 0);
    } else {
        if (event.m_id != 0x2074d009 && event.m_id != 0x5749) {
            return;
        }
        if (m_closeCommand != 0) {
            WindowApp::HandleTunnelCommand(m_closeCommand, 0, 0, 0);
        }
        App* app = static_cast<App*>(WindowApp::m_instance);
        app->m_soundManager->Stop("IDM_LEVEL_UP");
    }
    Close();
}

Window* CLevelUpWindow::CreateButtons(bool allowShare)
{
    Window* row = new Window(false);
    Window* okButton = CreateButton(NULL, ResString("IDS_SK_OK"), 0x5749);
    okButton->m_debugFrame = true;
    row->AddToFront(okButton, 0, 0);
    if (allowShare) {
        Window* shareButton = CreateButton("IDB_FACEBOOK_ICON", ResString("IDS_DIALOG_BUTTON_FB_SHARE"), 0x6e7d5203);
        row->AddToFront(shareButton, 1, 0);
    }
    return row;
}

Window* CLevelUpWindow::CreateButton(const char* icon, const XString& text, unsigned int command)
{
    Window* cell = new Window(false);
    // With an icon the text goes into the label next to it, not onto the button.
    XString caption = icon == NULL ? text : XString(L"");
    Window* holder = new Window(false);
    holder->SetWidthByContent(0, 0);
    holder->SetHeightByContent(0, 0);
    holder->m_debugFrame = true;
    holder->SetLayoutType(LAYOUT_SIMPLE);
    holder->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    StretchImageButton* button = new StretchImageButton(caption, "IDB_HUD_TRANSPARENT_BLACK_LEFT",
                                                        "IDB_HUD_TRANSPARENT_BLACK_TILE",
                                                        "IDB_HUD_TRANSPARENT_BLACK_RIGHT");
    button->m_command = command;
    button->m_commandParameter1 = 0;
    button->m_commandParameter2 = 0;
    button->m_commandParameter3 = 0;
    button->ForceSetWidth(SScreenAdaptingConstant(75, 150)());
    holder->AddToFront(button);
    cell->AddToFront(holder);
    if (icon != NULL) {
        LabelWithIcon* label = new LabelWithIcon(icon, text, false, CFontMgr::GetInstance()->GetFont(15), 0);
        label->SetPercentHeight(80, 0, 0);
        label->SetPercentWidth(73, 0, 0);
        label->SetLayoutType(LAYOUT_SIMPLE);
        label->SetAlign(0xc);
        button->AddToFront(label);
    }
    return cell;
}

void CLevelUpWindow::AddMessageText()
{
    TextWindow* text = new TextWindow(m_levelMessage, CFontMgr::GetInstance()->GetFont(15));
    text->SetLayoutType(LAYOUT_SIMPLE);
    text->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    text->SetPercentWidth(80, 0, 0);
    text->SetPercentHeight(50, 0, 0);
    text->SetOutsetSpacing(SScreenAdaptingConstant(-20, -40)(), 0, 0, 0);
    AddToFront(text);
}

void CLevelUpWindow::OnEasyFinished()
{
    AddToFront(new CLevelUpAnimWindow());
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_soundManager->PlaySound("IDM_LEVEL_UP", false, false);
    AddMessageText();
}

void CLevelUpWindow::OnStartupEasy()
{
    EasyParams params(this, (EasyLib::EMethod)0x1a);
    params.SetMoving(m_x, m_parent->m_height, m_x, m_y);
    params.SetFullTime(0.7f);
    params.SetDelayTime(0.1f);
    params.SetOpacity(0.0f, 1.0f);
    StartEasy(params, FLAG_SHOW_AFTER_EASY);
}

void CLevelUpWindow::EasyClose()
{
    EasyParams params(this, (EasyLib::EMethod)0x19);
    params.SetMoving(m_x, m_y, m_x, m_parent->m_height);
    params.SetFullTime(0.7f);
    params.SetDelayTime(0.7f);
    params.SetOpacity(1.0f, 0.0f);
    StartEasy(params, FLAG_SHOW_AFTER_EASY);
}

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char CKLevelUpWindowSize[sizeof(CLevelUpWindow) == 0x128 ? 1 : -1];
#endif
#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char CKLevelUpAnimWindowSize[sizeof(CLevelUpAnimWindow) == 0xac ? 1 : -1];
#endif
