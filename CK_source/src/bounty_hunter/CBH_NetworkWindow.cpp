#include "CBH_NetworkWindow.h"

#include "App.h"
#include "CAchievementsWindow.h"
#include "CAvatar.h"
#include "CBH_GPSMap.h"
#include "CBH_MainMenuBar.h"
#include "CBH_MenuButton.h"
#include "CBH_Player.h"
#include "CBH_ProgressBar.h"
#include "CBH_SurfaceManager.h"
#include "CBH_WeaponButton.h"
#include "CCareerWindow.h"
#include "CChangeAvatarWindow.h"
#include "CDH_BasicGameData.h"
#include "CDH_Weapon.h"
#include "CFactionsWindow.h"
#include "CFontMgr.h"
#include "CGameAnalytics.h"
#include "CGameScene.h"
#include "CImageButton.h"
#include "CMainItemScreen.h"
#include "CRegularShop.h"
#include "CTutorialManager.h"
#include "DGHelper.h"
#include "GameCenter_Facade.h"
#include "SScreenAdaptingConstant.h"

namespace {
const int SelectFirstSlot = 0x69b54947;
const int SelectSecondSlot = 0x69b54944;
const int SwapCommand = 0x51368d85;
const int OpenStore = (int)0x84523dc0;
const int OpenAchievements = 0x2c66b471;
const int OpenAvatar = 0x1e4344a2;
const int OpenFactions = (int)0x9d557067;
const int OpenCareer = 0x265a64dd;
const int OpenLeaderboards = 0x3752d6b8;
const int OpenGlobalNav = (int)0xd3fb7516;

MenuButtonDesc BUTTON_DESCRIPTION[] = {
    { "IDB_BUTTON_SHOP", OpenStore, "IDS_BUTTON_SHOP" },
    { "IDB_BUTTON_TROPHIES", OpenAchievements, "IDS_BUTTON_ACHIEVEMENTS" },
    { "IDB_BUTTON_CAREER", OpenCareer, "IDS_BUTTON_CAREER" },
    { "IDB_BUTTON_STATS", OpenLeaderboards, "IDS_BUTTON_STATS" },
    { "IDB_BUTTON_GLOBALNAV", OpenGlobalNav, "IDS_BUTTON_GLOBALNAV" }
};

}

CBH_NetworkWindow* CBH_NetworkWindow::m_pInstance;

CBH_NetworkWindow::CBH_NetworkWindow()
    : CBaseScreen(false, true, true, false, true, false), m_energy(NULL), m_showingShop(false)
{
    m_pInstance = this;
    App* app = static_cast<App*>(WindowApp::m_instance);
    m_upperBackground = app->m_surfaceManager->CreateSurface("IDB_MENU_HONEYCOMD_UP", true);
    SetTitleText(ResString("IDS_TAB_NAME_NETWORK"));

    int profileHeight = 55;
    int weaponHeight = 35;
    if (App::IsWVGA()) {
        profileHeight = 50;
        weaponHeight = 32;
    }
    Window* profile = new Window(false);
    profile->SetPercentHeight(profileHeight, 0, 0);
    m_content->AddToFront(profile, 0, 0);

    Window* avatarColumn = new Window(false);
    avatarColumn->SetPercentWidth(25, 0, 0);
    avatarColumn->SetPercentHeight(100, 0, 0);
    avatarColumn->AddToFront(CreateAvatarWindow());
    profile->AddToFront(avatarColumn, 0, 0);

    Window* infoColumn = new Window(false);
    infoColumn->SetPercentWidth(25, 0, 0);
    Window* info = CreateInfoWindow();
    info->SetPercentHeight(100, 0, 0);
    info->SetPercentWidth(100, 0, 0);
    info->SetLayoutType(LAYOUT_SIMPLE);
    info->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    infoColumn->AddToFront(info);
    profile->AddToFront(infoColumn, 1, 0);

    Window* barsColumn = new Window(false);
    barsColumn->SetPercentWidth(50, 0, 0);
    barsColumn->AddToFront(CreateBarsWindow());
    profile->AddToFront(barsColumn, 2, 0);

    Window* weaponRow = new Window(false);
    weaponRow->SetPercentHeight(weaponHeight, 0, 0);
    Window* firstColumn = new Window(false);
    firstColumn->SetPercentWidth(35, 0, 0);
    Window* firstSlot = CreateWeaponWindow(0, "IDS_MENU_WEAPON_SLOT1", SelectFirstSlot);
    firstSlot->SetWidthByContent(0, 0);
    firstSlot->SetHeightByContent(0, 0);
    firstSlot->SetLayoutType(LAYOUT_SIMPLE);
    firstSlot->SetAlign(ALIGN_RIGHT | ALIGN_BOTTOM);
    firstColumn->AddToFront(firstSlot);
    weaponRow->AddToFront(firstColumn, 0, 0);

    Window* swapColumn = new Window(false);
    swapColumn->SetPercentWidth(10, 0, 0);
    swapColumn->AddToFront(new Window(false), 0, 0);
    swapColumn->FindCell(0, 0)->SetPercentHeight(25, 0, 0);
    CImageButton* swap = CreateSwapWeaponsWindow(SwapCommand);
    swap->m_debugFrame = true;
    swapColumn->AddToFront(swap, 0, 1);
    weaponRow->AddToFront(swapColumn, 1, 0);

    Window* secondColumn = new Window(false);
    secondColumn->SetPercentWidth(35, 0, 0);
    Window* secondSlot = CreateWeaponWindow(1, "IDS_MENU_WEAPON_SLOT2", SelectSecondSlot);
    secondSlot->SetWidthByContent(0, 0);
    secondSlot->SetHeightByContent(0, 0);
    secondSlot->SetLayoutType(LAYOUT_SIMPLE);
    secondSlot->SetAlign(ALIGN_LEFT | ALIGN_BOTTOM);
    secondColumn->AddToFront(secondSlot);
    weaponRow->AddToFront(secondColumn, 2, 0);
    m_content->AddToFront(weaponRow, 0, 1);

    m_notificationLayer = new Window(false);
    m_notificationLayer->ClearFlags(FLAG_FOCUSABLE | FLAG_ENABLED);
    AddToFront(m_notificationLayer);
    m_content->AddToFront(CreateMenuBar(), 0, 2);
    m_content->FindCell(0, 2)->SetPercentHeight(30, 0, 0);
}

CBH_NetworkWindow::~CBH_NetworkWindow()
{
    ReleaseResources();
    if (m_pInstance == this) {
        m_pInstance = NULL;
    }
}

CBH_NetworkWindow* CBH_NetworkWindow::GetInstance()
{
    // the original does not look at the window flags
    if (m_pInstance == NULL) {
        m_pInstance = new CBH_NetworkWindow();
    }
    return m_pInstance;
}

void CBH_NetworkWindow::ReleaseResources()
{
    // the original releases the surface by its resource name and keeps the pointer
    static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->ReleaseSurface("IDB_MENU_HONEYCOMD_UP");
}

void CBH_NetworkWindow::Paint(ICGraphics2d& graphics)
{
    if (!CBH_GPSMap::weaponIsPainting) {
        CBaseScreen::Paint(graphics);
    }
}

Window* CBH_NetworkWindow::CreateAvatarWindow()
{
    Window* panel = new Window(false);
    panel->SetHeightByContent(0, 0);
    panel->SetPercentWidth(100, 0, 0);
    panel->SetLayoutType(LAYOUT_SIMPLE);
    panel->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    const CAvatar* avatar = CBH_Player::GetInstance()->m_avatar;
    const char* resource = "IDB_FRAME_SELECTED";
    if (avatar != NULL && avatar->m_imageResId != NULL) {
        resource = avatar->m_imageResId;
    }
    m_avatar = CreateMenuButton(resource, OpenAvatar);
    m_avatar->SetLayoutType(LAYOUT_SIMPLE);
    m_avatar->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
    m_avatar->setPressedImage("IDB_FRAME_SELECTED");
    m_avatar->SetBlinkingImage("IDB_FRAME_SELECTED");
    panel->AddToFront(m_avatar);
    return panel;
}

Window* CBH_NetworkWindow::CreateInfoWindow()
{
    Window* panel = new Window(false);
    panel->SetPercentHeight(50, 0, 0);
    CBH_Player* player = CBH_Player::GetInstance();
    CFontMgr* fonts = CFontMgr::GetInstance();
    TextWindow* name = new TextWindow(player->m_playerName, fonts->GetFont(11));
    name->SetAlign(ALIGN_LEFT | ALIGN_BOTTOM);
    panel->AddToFront(name, 0, 0);

    XString levelText = XString::Format(ResString("IDS_SAFEHOUSE_LEVEL").c_str(), player->GetLevel());
    TextWindow* level = new TextWindow(levelText, fonts->GetFont(11));
    level->SetAlign(ALIGN_LEFT);
    panel->AddToFront(level, 0, 1);

    XString contractText = XString::Format(ResString("IDS_SAFEHOUSE_CONTRACTS").c_str(),
                                           player->m_gameStats.m_successBounties);
    TextWindow* contracts = new TextWindow(contractText, fonts->GetFont(12));
    contracts->SetAlign(ALIGN_LEFT | ALIGN_TOP);
    panel->AddToFront(contracts, 0, 2);

    m_inventory = new TextWindow(fonts->GetFont(13));
    m_inventory->SetAlign(ALIGN_LEFT | ALIGN_TOP);
    panel->AddToFront(m_inventory, 0, 3);
    UpdateOtherText();
    return panel;
}

Window* CBH_NetworkWindow::CreateBarsWindow()
{
    Window* panel = new Window(false);
    int width = App::Scale() * 120;
    int topSpacing = SScreenAdaptingConstant(25, 70)();
    int bottomSpacing = SScreenAdaptingConstant(25, 50)();
    CBH_Player* player = CBH_Player::GetInstance();

    m_experience = new CBH_ProgressBar(3, width, -1);
    m_experience->SetDescImage("IDB_HUD_ICON_EXP");
    m_experience->m_displayFlags = CBH_ProgressBar::DISPLAY_CURRENT | CBH_ProgressBar::MANUAL_PERCENT;
    m_experience->SetOutsetSpacing(topSpacing, 0, 0, 0);
    panel->AddToFront(m_experience, 0, 0);
    if (player->IsEnegyEnabled()) {
        m_energy = new CBH_ProgressBar(0, width, -1);
        m_energy->SetMaxValue(player->GetMaxEnergy());
        m_energy->SetCurrValue(player->m_energy);
        m_energy->SetDescImage("IDB_HUD_ICON_ENERGY");
        m_energy->SetOutsetSpacing(topSpacing, 0, 0, 0);
        panel->AddToFront(m_energy, 1, 0);
    }
    m_money = new CBH_ProgressBar(0, width, -1);
    m_money->m_displayFlags = CBH_ProgressBar::DISPLAY_CURRENT | CBH_ProgressBar::HIDE_FILL;
    m_money->SetTextAlign(TEXT_ALIGN_RIGHT);
    m_money->SetCurrValue(player->m_cash);
    m_money->SetDescImage("IDB_HUD_ICON_MONEY");
    m_money->SetOutsetSpacing(0, 0, bottomSpacing, 0);
    panel->AddToFront(m_money, 0, 1);

    m_creditsBar = new CBH_ProgressBar(0, width, -1);
    m_creditsBar->m_displayFlags = CBH_ProgressBar::DISPLAY_CURRENT | CBH_ProgressBar::HIDE_FILL;
    m_creditsBar->SetTextAlign(TEXT_ALIGN_RIGHT);
    m_creditsBar->SetCurrValue(player->m_credits);
    m_creditsBar->SetDescImage("IDB_HUD_ICON_CREDITS");
    m_creditsBar->SetOutsetSpacing(0, 0, bottomSpacing, 0);
    panel->AddToFront(m_creditsBar, 1, 1);
    return panel;
}

Window* CBH_NetworkWindow::CreateWeaponWindow(int slot, const char* title, unsigned int command)
{
    Window* panel = new Window(false);
    CFontMgr* fonts = CFontMgr::GetInstance();
    TextWindow* caption = new TextWindow(ResString(title), fonts->GetFont(9));
    caption->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    panel->AddToFront(caption, 0, 0);

    CBH_WeaponButton* button = new CBH_WeaponButton(0, slot, true, false, 0);
    button->m_command = command;
    button->m_commandParameter1 = 0;
    button->m_commandParameter2 = 0;
    button->m_commandParameter3 = 0;
    panel->AddToFront(button, 0, 1);

    CDH_Weapon* weapon = button->Weapon();
    XString weaponName;
    if (weapon != NULL) {
        weaponName = weapon->GetName();
    }
    TextWindow* label = new TextWindow(weaponName, fonts->GetFont(15));
    label->SetLayoutType(LAYOUT_SIMPLE);
    label->SetAlign(ALIGN_HCENTER | ALIGN_BOTTOM);
    button->AddToFront(label);
    m_weaponButtons[slot] = button;
    m_weaponLabels[slot] = label;
    m_weapons[slot] = weapon;
    return panel;
}

Window* CBH_NetworkWindow::CreateMenuBar()
{
    Window* container = new Window(false);
    CBH_MainMenuBar* bar = new CBH_MainMenuBar(-1, -1, -1, -1);
    bar->SetPercentHeight(100, 0, 0);
    bar->SetPercentWidth(100, 0, 0);
    bar->SetLayoutType(LAYOUT_SIMPLE);
    bar->SetAlign(ALIGN_TOP);
    Vector<MenuButtonDesc*> descriptions;
    const int count = sizeof(BUTTON_DESCRIPTION) / sizeof(BUTTON_DESCRIPTION[0]);
    for (int index = 0; index < count; ++index) {
        // The original allocates the copy first; a skipped leaderboard entry leaks it.
        if (index != 3 || GameCenter_Facade::GetInstance()->isGameCenterSupported() == true) {
            MenuButtonDesc* description = new MenuButtonDesc;
            *description = BUTTON_DESCRIPTION[index];
            descriptions.addElement(description);
        }
    }
    bar->SetButtons(descriptions);
    SAFE_DELETE_VECTOR(descriptions);
    container->AddToFront(bar);
    return container;
}

void CBH_NetworkWindow::Init()
{
    CGameAnalytics::logSafehouseEnter();
}

void CBH_NetworkWindow::OnGetFocus()
{
    const CAvatar* avatar = CBH_Player::GetInstance()->m_avatar;
    if (avatar != NULL && avatar->m_imageResId != NULL) {
        m_avatar->setImage(avatar->m_imageResId);
    }
    if (m_showingShop) {
        m_showingShop = false;
        CGameAnalytics::logSafehouseEnter();
    }
    if (static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial()) {
        WindowApp::HandleTunnelCommand(-0x4b9aa0d3, 0, 0, 0);
    }
}

void CBH_NetworkWindow::UpdateWeaponNames()
{
    for (int slot = 0; slot < 2; ++slot) {
        CDH_Weapon* weapon = m_weaponButtons[slot]->Weapon();
        if (m_weapons[slot] != weapon) {
            m_weaponButtons[slot]->Update();
            XString name;
            if (weapon != NULL) {
                name = weapon->GetName();
            }
            m_weaponLabels[slot]->SetText(name);
            m_weapons[slot] = weapon;
        }
    }
}

void CBH_NetworkWindow::UpdateOtherText()
{
    CBH_Player* player = CBH_Player::GetInstance();
    XString format = XString::Format(L"%s\n%s\n%s\n%s",
        ResString("IDS_SAFEHOUSE_ARMOR").c_str(), ResString("IDS_SAFEHOUSE_MEDKITS").c_str(),
        ResString("IDS_SAFEHOUSE_PILLS").c_str(), ResString("IDS_SAFEHOUSE_SILENCER").c_str());
    XString inventory = XString::Format(format.c_str(), player->m_armorDamageProof / 1000,
                                        player->m_medkits, player->GetPillsCount(),
                                        player->m_silentShotsCount);
    if (!(inventory == m_inventory->m_text)) {
        m_inventory->SetText(inventory);
    }
}

void CBH_NetworkWindow::Update()
{
    UpdateWeaponNames();
    UpdateOtherText();
    m_back->SetBlinking(static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsNetworkCloseBlink());
    m_avatar->SetBlinking(static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsAvatarChoicePressBlink());
    m_weaponButtons[1]->SetBlinking(static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsWeaponChoicePressBlink());
    CBH_Player* player = CBH_Player::GetInstance();
    int next = player->GetXpForNextLvl();
    int previous = player->GetXpForCurrentLvl();
    int percent = 100;
    if (next > previous) {
        percent = (player->m_experience - previous) * 100 / (next - previous);
    }
    m_experience->SetCurrValue(player->m_experience);
    m_experience->SetMaxValue(next);
    m_experience->SetSpecialPercents(percent);
    if (m_energy != NULL) {
        m_energy->SetCurrValue(player->m_energy);
        m_energy->SetMaxValue(player->GetMaxEnergy());
    }
    m_money->SetCurrValue(player->m_cash);
    m_creditsBar->SetCurrValue(player->m_credits);
}

void CBH_NetworkWindow::SwapWeapons()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_BasicGameData* data = app->m_helper->GetGameData();
    int first = data->m_weaponIds[0];
    int second = data->m_weaponIds[1];
    if (first >= 0 && second >= 0 && first != second) {
        app->m_helper->m_gamePlayManager.SetWeapon01(second);
        app->m_helper->m_gamePlayManager.SetWeapon02(first);
        CBH_Player::GetInstance()->Save();
    }
}

void CBH_NetworkWindow::OnCommand(Event& event)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (event.m_id == SelectFirstSlot || event.m_id == SelectSecondSlot) {
        int slot = 0;
        if (event.m_id == SelectSecondSlot) {
            slot = 1;
        }
        if (static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->CanChooseWeapon() && (!static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsWeaponChoicePressBlink() || slot == 1)) {
            app->m_mainLayer->AddModal(new CMainItemScreen(0, slot));
            if (slot == 1) {
                WindowApp::HandleTunnelCommand(0x21d908a0, 0, 0, 0);
            }
        }
    } else if (event.m_id == SwapCommand) {
        if (!static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsMenuBarBlocked()) {
            SwapWeapons();
        }
    } else if (event.m_id == OpenStore) {
        if (!static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsMenuBarBlocked()) {
            m_showingShop = true;
            CRegularShop::Show(CShopWindow::TAB_WEAPONS);
        }
    } else if (event.m_id == OpenAvatar) {
        app->m_mainLayer->AddModal(new CChangeAvatarWindow());
        WindowApp::HandleTunnelCommand(0x33e070cb, 0, 0, 0);
    } else if (event.m_id == OpenFactions) {
        app->m_mainLayer->AddModal(new CFactionsWindow());
        WindowApp::HandleTunnelCommand(0x718a0676, 0, 0, 0);
    } else if (event.m_id == CChangeAvatarWindow::AvatarChanged) {
        const CAvatar* avatar = CBH_Player::GetInstance()->m_avatar;
        if (avatar != NULL && avatar->m_imageResId != NULL) {
            m_avatar->setImage(avatar->m_imageResId);
        }
    } else if (event.m_id == OpenAchievements) {
        if (!static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsMenuBarBlocked()) {
            app->m_mainLayer->AddModal(new CAchievementsWindow());
        }
    } else if (event.m_id == OpenCareer) {
        if (!static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsMenuBarBlocked()) {
            app->m_mainLayer->AddModal(new CCareerWindow());
        }
    } else if (event.m_id == OpenLeaderboards) {
        if (!static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsMenuBarBlocked()) {
            GameCenter_Facade* gameCenter = GameCenter_Facade::GetInstance();
            if (gameCenter->isGameCenterSupported() && gameCenter->isAuthenticated()) {
                gameCenter->RunLeaderboardsView(NULL);
                CGameAnalytics::logLeaderboards();
            }
        }
    } else if (event.m_id == OpenGlobalNav) {
        CGameScene::ShowGGN();
        CGameAnalytics::logGMGEntered();
    } else if (event.m_id == -0x68dfb87c || event.m_id == 0x5f523cd9) {
        WindowApp::HandleTunnelCommand(-0x64191a07, 0, 0, 0);
        if (static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->CanCloseNetwork()) {
            if (CBH_Player::GetInstance()->GetTutorialState() == 11) {
                CGameAnalytics::logTutorialEvents(CGameAnalytics::TUTORIAL_EVENT_LEFT_SAFEHOUSE);
            }
            ReverseEasy(FLAG_CLOSE_AFTER_EASY);
            WindowApp::PutEvent(Event::TUNNEL_COMMAND, 0x40067282, 0, 0, 0);
        }
    } else {
        CBaseScreen::OnCommand(event);
        return;
    }
    event.Clear();
}

void CBH_NetworkWindow::OnPointerEvent(Event& event)
{
}

__attribute__((noinline)) CImageButton* CBH_NetworkWindow::CreateSwapWeaponsWindow(unsigned int command)
{
    CImageButton* button = new CImageButton();
    button->AddImage("IDB_HUD_WEAPON_SWITCH_BUTTON", 0, 0);
    button->m_command = command;
    button->m_commandParameter1 = 0;
    button->m_commandParameter2 = 0;
    button->m_commandParameter3 = 0;
    return button;
}

void CBH_NetworkWindow::OnStartupEasy()
{
    EasyParams params(this, (EasyLib::EMethod)5, (EasyLib::EMethod)5);
    params.SetFullTime(0.5f);
    params.SetOpacity(0.0f, 1.0f);
    StartEasy(params, FLAG_SHOW_AFTER_EASY);
}

void CBH_NetworkWindow::OnStateInterrupt(int state, Window* window)
{
    // The original reads the interrupt twice; the first value is unused.
    window->GetInterrupt();
    if (window->GetInterrupt() == 2) {
        PopState();
    }
}

void CBH_NetworkWindow::OnEasyStarted()
{
    CBH_GPSMap::safehouseIsPainting = false;
}

void CBH_NetworkWindow::Shutdown()
{
}

void CBH_NetworkWindow::OnLostFocus()
{
}

void CBH_NetworkWindow::OnEnterForeground()
{
    CBH_GPSMap::safehouseIsPainting = true;
}

typedef char AssertNetworkWindowSize[sizeof(CBH_NetworkWindow) == 0x124 ? 1 : -1];
