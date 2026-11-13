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
#include "CFriendsWindow.h"
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
const int OpenFriends = 0x37c37da6;
const int MenuBlocked = -0x2e800a2c;
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
    SetTitleText(ResString("IDS_TAB_NAME_NETWORK"));
    m_notificationLayer = new Window(false);
    m_upperBackground = static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->CreateSurface("IDB_MENU_HONEYCOMD_UP", true);
    SetSize(WindowApp::m_instance->m_screenWidth, WindowApp::m_instance->m_screenHeight);

    Window* profile = new Window(false);
    int profileHeight = 55;
    if (App::IsWVGA()) {
        profileHeight = 50;
    }
    profile->SetPercentHeight(profileHeight, 0, 0);
    m_content->AddToFront(profile, 0, 0);

    Window* avatarColumn = new Window(false);
    avatarColumn->SetLayoutType(LAYOUT_TABLE);
    avatarColumn->SetPercentWidth(25, 0, 0);
    avatarColumn->SetPercentHeight(100, 0, 0);
    avatarColumn->AddToFront(CreateAvatarWindow());
    profile->AddToFront(avatarColumn, 0, 0);

    Window* infoColumn = new Window(false);
    infoColumn->SetPercentWidth(25, 0, 0);
    infoColumn->SetLayoutType(LAYOUT_TABLE);
    infoColumn->SetCellPos(1, 0, 1, 1);
    Window* info = CreateInfoWindow();
    info->SetPercentHeight(100, 0, 0);
    info->SetPercentWidth(100, 0, 0);
    info->SetLayoutType(LAYOUT_SIMPLE);
    info->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    infoColumn->AddToFront(info);
    profile->AddToFront(infoColumn);

    Window* barsColumn = new Window(false);
    barsColumn->SetPercentWidth(50, 0, 0);
    barsColumn->SetLayoutType(LAYOUT_TABLE);
    barsColumn->SetCellPos(2, 0, 1, 1);
    barsColumn->AddToFront(CreateBarsWindow());
    profile->AddToFront(barsColumn);

    Window* weaponRow = new Window(false);
    int weaponHeight = 35;
    if (App::IsWVGA()) {
        weaponHeight = 32;
    }
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

    AddToFront(m_notificationLayer);
    Window* menu = CreateMenuBar();
    m_content->AddToFront(menu, 0, 2);
    m_content->FindCell(0, 2)->SetPercentHeight(30, 0, 0);
    SetTitleText(ResString("IDS_TAB_NAME_NETWORK"));
}

CBH_NetworkWindow::~CBH_NetworkWindow()
{
    ReleaseResources();
    m_pInstance = NULL;
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
    m_avatar = new CBH_MenuButton();
    m_avatar->setImage(CBH_Player::GetInstance()->m_avatar->m_imageResId);
    m_avatar->setPressedImage("IDB_FRAME_SELECTED");
    m_avatar->SetBlinkingImage("IDB_FRAME_SELECTED");
    m_avatar->SetCommandParameters(OpenAvatar, 0, 0, 0);
    panel->AddToFront(m_avatar);
    return panel;
}

Window* CBH_NetworkWindow::CreateInfoWindow()
{
    Window* panel = new Window(false);
    panel->SetPercentHeight(50, 0, 0);

    TextWindow* name = new TextWindow(XString(CBH_Player::GetInstance()->m_playerName),
                                      CFontMgr::GetInstance()->GetFont(11));
    name->SetLayoutType(LAYOUT_SIMPLE);
    name->SetAlign(ALIGN_LEFT | ALIGN_BOTTOM);
    panel->AddToFront(name, 0, 0);

    TextWindow* level = new TextWindow(XString::Format(ResString("IDS_SAFEHOUSE_LEVEL").c_str(),
                                                      CBH_Player::GetInstance()->GetLevel()),
                                       CFontMgr::GetInstance()->GetFont(11));
    level->SetLayoutType(LAYOUT_SIMPLE);
    level->SetAlign(ALIGN_LEFT);
    panel->AddToFront(level, 0, 1);

    TextWindow* contracts = new TextWindow(XString::Format(ResString("IDS_SAFEHOUSE_CONTRACTS").c_str(),
                                                          CBH_Player::GetInstance()->m_gameStats.m_successBounties),
                                           CFontMgr::GetInstance()->GetFont(12));
    contracts->SetLayoutType(LAYOUT_SIMPLE);
    contracts->SetAlign(ALIGN_LEFT | ALIGN_TOP);
    panel->AddToFront(contracts, 0, 2);

    TextWindow* inventory = new TextWindow();
    m_inventory = inventory;
    inventory->SetFont(CFontMgr::GetInstance()->GetFont(13));
    m_inventory->SetLayoutType(LAYOUT_SIMPLE);
    m_inventory->SetAlign(ALIGN_LEFT | ALIGN_TOP);
    panel->AddToFront(m_inventory, 0, 3);
    UpdateOtherText();
    return panel;
}

Window* CBH_NetworkWindow::CreateBarsWindow()
{
    Window* panel = new Window(false);
    m_experience = new CBH_ProgressBar(3, App::Scale() * 120, -1);
    m_experience->SetDescImage("IDB_HUD_ICON_EXP");
    m_experience->m_displayFlags = CBH_ProgressBar::DISPLAY_CURRENT;
    m_experience->SetOutsetSpacing(SScreenAdaptingConstant(25, 70)(), 0, 0, 0);
    panel->AddToFront(m_experience, 0, 0);
    if (CBH_Player::GetInstance()->IsEnegyEnabled()) {
        m_energy = new CBH_ProgressBar(0, App::Scale() * 120, -1);
        m_energy->SetMaxValue(CBH_Player::GetInstance()->GetMaxEnergy());
        m_energy->SetCurrValue(CBH_Player::GetInstance()->m_energy);
        m_energy->SetDescImage("IDB_HUD_ICON_ENERGY");
        m_energy->SetOutsetSpacing(SScreenAdaptingConstant(25, 70)(), 0, 0, 0);
        panel->AddToFront(m_energy, 1, 0);
    }
    m_money = new CBH_ProgressBar(0, App::Scale() * 120, -1);
    m_money->m_displayFlags = CBH_ProgressBar::DISPLAY_CURRENT | CBH_ProgressBar::HIDE_FILL;
    m_money->SetTextAlign(TEXT_ALIGN_RIGHT);
    m_money->SetCurrValue(CBH_Player::GetInstance()->m_cash);
    m_money->SetDescImage("IDB_HUD_ICON_MONEY");
    m_money->SetOutsetSpacing(0, 0, SScreenAdaptingConstant(25, 50)(), 0);
    panel->AddToFront(m_money, 0, 1);

    m_creditsBar = new CBH_ProgressBar(0, App::Scale() * 120, -1);
    m_creditsBar->m_displayFlags = CBH_ProgressBar::DISPLAY_CURRENT | CBH_ProgressBar::HIDE_FILL;
    m_creditsBar->SetTextAlign(TEXT_ALIGN_RIGHT);
    m_creditsBar->SetCurrValue(CBH_Player::GetInstance()->m_credits);
    m_creditsBar->SetDescImage("IDB_HUD_ICON_CREDITS");
    m_creditsBar->SetOutsetSpacing(0, 0, SScreenAdaptingConstant(25, 50)(), 0);
    panel->AddToFront(m_creditsBar, 1, 1);
    return panel;
}

Window* CBH_NetworkWindow::CreateWeaponWindow(int slot, const char* title, unsigned int command)
{
    Window* panel = new Window(false);
    TextWindow* caption = new TextWindow(ResString(title), CFontMgr::GetInstance()->GetFont(9));
    caption->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    panel->AddToFront(caption, 0, 0);

    CBH_WeaponButton* button = new CBH_WeaponButton(0, slot, true, false, 0);
    button->SetCommandParameters(command, 0, 0, 0);
    button->ShowName();
    panel->AddToFront(button, 0, 1);

    CDH_BasicGameData* data = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData();
    CDH_Weapon* weapon;
    if (slot == 0) {
        weapon = data->GetWeapon01(true);
    } else {
        weapon = data->GetWeapon02(true);
    }
    TextWindow* label = new TextWindow(weapon->GetName(), CFontMgr::GetInstance()->GetFont(15));
    label->SetLayoutType(LAYOUT_SIMPLE);
    label->SetAlign(ALIGN_HCENTER | ALIGN_BOTTOM);
    button->AddToFront(label);
    if (slot == 0) {
        m_weaponButtons[0] = button;
        m_weaponLabels[0] = label;
        m_weapons[0] = weapon;
    } else if (slot == 1) {
        m_weaponButtons[1] = button;
        m_weaponLabels[1] = label;
        m_weapons[1] = weapon;
    }
    return panel;
}

Window* CBH_NetworkWindow::CreateMenuBar()
{
    Window* container = new Window(false);
    container->SetPercentHeight(100, 0, 0);
    CBH_MainMenuBar* bar = new CBH_MainMenuBar(-1, -1, -1, -1);
    bar->SetPercentHeight(100, 0, 0);
    bar->SetPercentWidth(100, 0, 0);
    bar->SetLayoutType(LAYOUT_SIMPLE);
    bar->SetAlign(ALIGN_TOP);
    Vector<MenuButtonDesc*> descriptions;
    const int count = sizeof(BUTTON_DESCRIPTION) / sizeof(BUTTON_DESCRIPTION[0]);
    for (int index = 0; index < count; ++index) {
        // The original allocates the copy first; a skipped leaderboard entry leaks it.
        MenuButtonDesc* description = new MenuButtonDesc;
        if (index != 3 || GameCenter_Facade::GetInstance()->isGameCenterSupported() == true) {
            const MenuButtonDesc& source = BUTTON_DESCRIPTION[index];
            description->m_image = source.m_image;
            description->m_command = source.m_command;
            description->m_caption = source.m_caption;
            descriptions.addElement(description);
        }
    }
    bar->SetButtons(descriptions);
    SAFE_DELETE_VECTOR(descriptions);
    container->SetLayoutType(LAYOUT_SIMPLE);
    container->SetAlign(ALIGN_TOP);
    container->AddToFront(bar);
    return container;
}

void CBH_NetworkWindow::Init()
{
    CGameAnalytics::logSafehouseEnter();
}

void CBH_NetworkWindow::OnGetFocus()
{
    m_avatar->setImage(CBH_Player::GetInstance()->m_avatar->m_imageResId);
    m_avatar->SetLayoutType(LAYOUT_SIMPLE);
    m_avatar->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
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
    CDH_Weapon* first = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon01(true);
    if (first != m_weapons[0]) {
        m_weaponLabels[0]->SetText(first->GetName(), CFontMgr::GetInstance()->GetFont(15));
        m_weapons[0] = first;
    }
    CDH_Weapon* second = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon02(true);
    if (second != m_weapons[1]) {
        m_weaponLabels[1]->SetText(second->GetName(), CFontMgr::GetInstance()->GetFont(15));
        m_weapons[1] = second;
    }
}

void CBH_NetworkWindow::UpdateOtherText()
{
    CBH_Player::GetInstance();
    TextWindow* inventory = m_inventory;
    XString armor = ResString("IDS_SAFEHOUSE_ARMOR");
    const wchar_t* armorText = armor.c_str();
    XString medkits = ResString("IDS_SAFEHOUSE_MEDKITS");
    const wchar_t* medkitsText = medkits.c_str();
    XString pills = ResString("IDS_SAFEHOUSE_PILLS");
    const wchar_t* pillsText = pills.c_str();
    XString silencer = ResString("IDS_SAFEHOUSE_SILENCER");
    XString format = XString::Format(L"%s\n%s\n%s\n%s", armorText, medkitsText, pillsText, silencer.c_str());
    const wchar_t* inventoryFormat = format.c_str();
    int armorValue = CBH_Player::GetInstance()->m_armorDamageProof;
    int medkitCount = CBH_Player::GetInstance()->m_medkits;
    int pillsCount = CBH_Player::GetInstance()->GetPillsCount();
    int silentShots = CBH_Player::GetInstance()->m_silentShotsCount;
    inventory->SetText(XString::Format(inventoryFormat, armorValue / 1000, medkitCount, pillsCount, silentShots));
}

void CBH_NetworkWindow::Update()
{
    UpdateWeaponNames();
    UpdateOtherText();
    if (static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial()) {
        int blinkBack = static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsNetworkCloseBlink();
        m_back->SetBlinking(blinkBack == 1);
        bool blinkWeapon = static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsWeaponChoicePressBlink() == 1;
        CBH_WeaponButton* secondSlot = m_weaponButtons[1];
        secondSlot->m_backgroundImage->m_blinking = blinkWeapon;
        secondSlot->m_weaponImage->m_blinking = blinkWeapon;
        m_weaponButtons[0]->m_blockPointer = blinkWeapon;
        int blinkAvatar = static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsAvatarChoicePressBlink();
        m_avatar->SetBlinking(blinkAvatar == 1);
    }
    int experience = CBH_Player::GetInstance()->m_experience;
    int next = CBH_Player::GetInstance()->GetXpForNextLvl();
    int previous = CBH_Player::GetInstance()->GetXpForCurrentLvl();
    m_experience->SetCurrValue(experience);
    if (next > 0 && next > previous) {
        // The original uses 32-bit subtraction and multiplication before signed division.
        int earned = static_cast<int>((static_cast<unsigned int>(experience) - previous) * 100u);
        int span = static_cast<int>(static_cast<unsigned int>(next) - previous);
        m_experience->SetSpecialPercents(earned / span);
    } else {
        m_experience->SetSpecialPercents(100);
    }
    if (m_energy != NULL) {
        m_energy->SetCurrValue(CBH_Player::GetInstance()->m_energy);
        m_energy->SetMaxValue(CBH_Player::GetInstance()->GetMaxEnergy());
    }
    m_money->SetCurrValue(CBH_Player::GetInstance()->m_cash);
    m_creditsBar->SetCurrValue(CBH_Player::GetInstance()->m_credits);
}

void CBH_NetworkWindow::SwapWeapons()
{
    int first = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon01(true)->m_id;
    int second = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon02(true)->m_id;
    static_cast<App*>(WindowApp::m_instance)->m_helper->m_gamePlayManager.SetWeapon01(second);
    static_cast<App*>(WindowApp::m_instance)->m_helper->m_gamePlayManager.SetWeapon02(first);
}

void CBH_NetworkWindow::OnCommand(Event& event)
{
    switch (event.m_id) {
    case OpenStore: {
        CRegularShop* shop = CRegularShop::GetInstance(true);
        if (static_cast<App*>(WindowApp::m_instance)->m_mainLayer->IsWindowExisting(shop) == 1) {
            shop->m_parent->Remove(shop);
        }
        if (static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsMenuBarBlocked()) {
            WindowApp::HandleTunnelCommand(MenuBlocked, 0, 0, 0);
        } else {
            shop = CRegularShop::GetInstance(true);
            CGameAnalytics::logStoreEnter(CGameAnalytics::SHOP_ENTER_NETWORK);
            static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddModal(shop);
            m_showingShop = true;
            shop->SetTab(static_cast<CShopWindow::ETab>(event.m_param1));
            WindowApp::HandleTunnelCommand(0x718a0676, 0, 0, 0);
        }
        event.Clear();
        return;
    }
    case -0x68dfb87c:
    case 0x5f523cd9:
        WindowApp::HandleTunnelCommand(-0x64191a07, 0, 0, 0);
        if (static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->CanCloseNetwork()) {
            if (CBH_Player::GetInstance()->GetTutorialState() == 11) {
                CGameAnalytics::logTutorialEvents(CGameAnalytics::TUTORIAL_EVENT_LEFT_SAFEHOUSE);
            }
            ReverseEasy(FLAG_CLOSE_AFTER_EASY);
            event.Clear();
        }
        return;
    case OpenFactions: {
        CFactionsWindow* window = new CFactionsWindow();
        static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddModal(window);
        WindowApp::HandleTunnelCommand(0x718a0676, 0, 0, 0);
        event.Clear();
        return;
    }
    case OpenFriends: {
        CFriendsWindow* window = new CFriendsWindow();
        static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddModal(window);
        WindowApp::HandleTunnelCommand(0x718a0676, 0, 0, 0);
        event.Clear();
        return;
    }
    case OpenGlobalNav:
        CGameScene::ShowGGN();
        CGameAnalytics::logGMGEntered();
        event.Clear();
        return;
    case OpenAvatar: {
        CChangeAvatarWindow* window = new CChangeAvatarWindow();
        static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddModal(window);
        WindowApp::HandleTunnelCommand(0x33e070cb, 0, 0, 0);
        event.Clear();
        return;
    }
    case OpenCareer:
        if (static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsMenuBarBlocked()) {
            WindowApp::HandleTunnelCommand(MenuBlocked, 0, 0, 0);
        } else {
            CCareerWindow* window = new CCareerWindow();
            static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddModal(window);
        }
        event.Clear();
        return;
    case OpenAchievements:
        if (static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsMenuBarBlocked()) {
            WindowApp::HandleTunnelCommand(MenuBlocked, 0, 0, 0);
        } else {
            CAchievementsWindow* window = new CAchievementsWindow();
            static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddModal(window);
        }
        event.Clear();
        return;
    case OpenLeaderboards:
        if (static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsMenuBarBlocked()) {
            WindowApp::HandleTunnelCommand(MenuBlocked, 0, 0, 0);
        } else if (GameCenter_Facade::GetInstance()->isGameCenterSupported()
                   && GameCenter_Facade::GetInstance()->isAuthenticated()) {
            GameCenter_Facade::GetInstance()->RunLeaderboardsView(NULL);
            CGameAnalytics::logLeaderboards();
        }
        event.Clear();
        return;
    case SwapCommand:
        SwapWeapons();
        return;
    case SelectSecondSlot: {
        CMainItemScreen* window = new CMainItemScreen(0, 1);
        static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddModal(window);
        WindowApp::HandleTunnelCommand(0x21d908a0, 0, 0, 0);
        event.Clear();
        return;
    }
    case SelectFirstSlot: {
        CMainItemScreen* window = new CMainItemScreen(0, 0);
        static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddModal(window);
        event.Clear();
        return;
    }
    }
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

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AssertNetworkWindowSize[sizeof(CBH_NetworkWindow) == 0x124 ? 1 : -1];
#endif
