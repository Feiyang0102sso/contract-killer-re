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

TextWindow* AddLabel(Window* parent, const XString& text, int font, int x, int y, int width, int height)
{
    TextWindow* label = new TextWindow(CFontMgr::GetInstance()->GetFont(font));
    label->SetLayoutType(Window::LAYOUT_NONE);
    label->SetRect(x, y, width, height, false);
    label->SetText(text);
    label->ClearFlags(Window::FLAG_FOCUSABLE);
    parent->AddToFront(label);
    return label;
}

CBH_ProgressBar* AddProfileBar(Window* parent, int color, const char* icon, int x, int y, int width)
{
    CBH_ProgressBar* bar = new CBH_ProgressBar(color, width, -1);
    bar->SetDescImage(icon);
    bar->SetCorner(x, y);
    bar->ClearFlags(Window::FLAG_FOCUSABLE);
    parent->AddToFront(bar);
    return bar;
}
}

CBH_NetworkWindow* CBH_NetworkWindow::m_pInstance;

CBH_NetworkWindow::CBH_NetworkWindow()
    : CBaseScreen(false, true, true, false, true, false), m_energy(NULL), m_showingShop(false)
{
    m_pInstance = this;
    App* app = static_cast<App*>(WindowApp::m_instance);
    m_upperBackground = app->m_surfaceManager->CreateSurface("IDB_MENU_HONEYCOMD_UP", true);
    SetTitleText(ResString("IDS_TAB_NAME_NETWORK"));
    CreateAvatarWindow();
    CreateInfoWindow();
    CreateBarsWindow();
    CreateWeaponWindow(0, "IDS_MENU_WEAPON_SLOT1", SelectFirstSlot);
    CreateWeaponWindow(1, "IDS_MENU_WEAPON_SLOT2", SelectSecondSlot);
    m_swapButton = CreateMenuButton("IDB_HUD_WEAPON_SWITCH_BUTTON", SwapCommand);
    m_swapButton->SetCorner((m_content->m_width - m_swapButton->m_width) / 2, m_content->m_height * 3 / 5);
    m_content->AddToFront(m_swapButton);
    CreateMenuBar();
    m_notificationLayer = new Window(false);
    m_notificationLayer->ClearFlags(FLAG_FOCUSABLE | FLAG_ENABLED);
    AddToFront(m_notificationLayer);
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
    if (m_pInstance == NULL || (m_pInstance->m_flags & (FLAG_CLOSING | FLAG_DEAD)) != 0) {
        return new CBH_NetworkWindow();
    }
    return m_pInstance;
}

void CBH_NetworkWindow::ReleaseResources()
{
    if (m_upperBackground != NULL) {
        App* app = static_cast<App*>(WindowApp::m_instance);
        app->m_surfaceManager->ReleaseSurface(m_upperBackground);
        m_upperBackground = NULL;
    }
}

void CBH_NetworkWindow::Paint(ICGraphics2d& graphics)
{
    CBaseScreen::Paint(graphics);
    DrawTileImageHoriz(graphics, m_upperBackground, 0, GetTitleBarHeight(), m_width);
}

void CBH_NetworkWindow::CreateAvatarWindow()
{
    const CAvatar* avatar = CBH_Player::GetInstance()->m_avatar;
    const char* resource = "IDB_FRAME_SELECTED";
    if (avatar != NULL) {
        resource = avatar->m_imageResId;
    }
    m_avatar = CreateMenuButton(resource, OpenAvatar);
    m_avatar->SetCorner((m_content->m_width / 4 - m_avatar->m_width) / 2, 6 * App::Scale());
    m_avatar->setPressedImage("IDB_FRAME_SELECTED");
    m_avatar->SetBlinkingImage("IDB_FRAME_SELECTED");
    m_content->AddToFront(m_avatar);
}

void CBH_NetworkWindow::CreateInfoWindow()
{
    int width = m_content->m_width / 4;
    int scale = App::Scale();
    m_profileInfo = AddLabel(m_content, XString(), 15, width, 8 * scale, width, 45 * scale);
    m_inventory = AddLabel(m_content, XString(), 15, width, 55 * scale, width, 62 * scale);
}

void CBH_NetworkWindow::CreateBarsWindow()
{
    int scale = App::Scale();
    int x = m_content->m_width / 2 + 12 * scale;
    int width = m_content->m_width - x - 18 * scale;
    int y = 3 * scale;
    m_experience = AddProfileBar(m_content, 3, "IDB_HUD_ICON_EXP", x, y, width);
    m_experience->m_displayFlags |= CBH_ProgressBar::MANUAL_PERCENT;
    y += m_experience->m_height;
    if (CBH_Player::GetInstance()->IsEnegyEnabled()) {
        m_energy = AddProfileBar(m_content, 4, "IDB_HUD_ICON_ENERGY", x, y, width);
        y += m_energy->m_height;
    }
    m_money = AddProfileBar(m_content, 3, "IDB_HUD_ICON_MONEY", x, y, width);
    m_money->m_displayFlags = CBH_ProgressBar::DISPLAY_CURRENT | CBH_ProgressBar::HIDE_FILL;
    y += m_money->m_height;
    m_creditsBar = AddProfileBar(m_content, 3, "IDB_HUD_ICON_CREDITS", x, y, width);
    m_creditsBar->m_displayFlags = CBH_ProgressBar::DISPLAY_CURRENT | CBH_ProgressBar::HIDE_FILL;
}

void CBH_NetworkWindow::CreateWeaponWindow(int slot, const char* title, unsigned int command)
{
    int width = m_content->m_width * 35 / 100;
    int x = m_content->m_width / 10 + slot * (m_content->m_width * 45 / 100);
    int y = m_content->m_height / 2;
    int titleHeight = 17 * App::Scale();
    AddLabel(m_content, ResString(title), 9, x, y, width, titleHeight);
    CBH_WeaponButton* button = new CBH_WeaponButton(0, slot, true, false, 0);
    button->m_command = command;
    button->SetCorner(x, y + titleHeight);
    button->ShowName();
    m_weaponButtons[slot] = button;
    m_weaponLabels[slot] = NULL;
    m_weapons[slot] = NULL;
    m_content->AddToFront(button);
}

void CBH_NetworkWindow::CreateMenuBar()
{
    Window* container = new Window(false);
    int height = 30 * m_content->m_height / 100;
    container->SetLayoutType(LAYOUT_NONE);
    container->SetRect(0, m_content->m_height - height, m_content->m_width, height, false);
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
            *description = BUTTON_DESCRIPTION[index];
            descriptions.addElement(description);
        }
    }
    bar->SetButtons(descriptions);
    SAFE_DELETE_VECTOR(descriptions);
    container->AddToFront(bar);
    m_content->AddToFront(container);
}

void CBH_NetworkWindow::Init()
{
    CBaseScreen::Init();
    Update();
}

void CBH_NetworkWindow::OnGetFocus()
{
    m_showingShop = false;
    if (CTutorialManager::IsPlayerInTutorial()) {
        WindowApp::HandleTunnelCommand(-0x4b9aa0d3, 0, 0, 0);
    }
}

void CBH_NetworkWindow::UpdateWeaponNames()
{
    for (int slot = 0; slot < 2; ++slot) {
        CDH_Weapon* weapon = m_weaponButtons[slot]->Weapon();
        if (m_weapons[slot] != weapon) {
            m_weaponButtons[slot]->Update();
            m_weapons[slot] = weapon;
        }
    }
}

void CBH_NetworkWindow::UpdateOtherText()
{
    CBH_Player* player = CBH_Player::GetInstance();
    XString profile = XString::Format(L"%ls\n%ls %d\n%ls %d", player->m_playerName.c_str(),
        ResString("IDS_SAFEHOUSE_LEVEL").c_str(), player->GetLevel(),
        ResString("IDS_SAFEHOUSE_CONTRACTS").c_str(), player->m_completedMissionCount);
    if (!(profile == m_profileInfo->m_text)) {
        m_profileInfo->SetText(profile);
    }
    XString inventory = XString::Format(L"%ls %d\n%ls %d\n%ls %d\n%ls %d",
        ResString("IDS_SAFEHOUSE_ARMOR").c_str(), player->m_armorDamageProof / 1000,
        ResString("IDS_SAFEHOUSE_MEDKITS").c_str(), player->m_medkits,
        ResString("IDS_SAFEHOUSE_PILLS").c_str(), player->GetPillsCount(),
        ResString("IDS_SAFEHOUSE_SILENCER").c_str(), player->m_silentShotsCount);
    if (!(inventory == m_inventory->m_text)) {
        m_inventory->SetText(inventory);
    }
}

void CBH_NetworkWindow::Update()
{
    UpdateWeaponNames();
    UpdateOtherText();
    m_back->SetBlinking(CTutorialManager::IsNetworkCloseBlink());
    m_avatar->SetBlinking(CTutorialManager::IsAvatarChoicePressBlink());
    m_weaponButtons[1]->SetBlinking(CTutorialManager::IsWeaponChoicePressBlink());
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
        if (CTutorialManager::CanChooseWeapon() && (!CTutorialManager::IsWeaponChoicePressBlink() || slot == 1)) {
            app->m_mainLayer->AddModal(new CMainItemScreen(0, slot));
            if (slot == 1) {
                WindowApp::HandleTunnelCommand(0x21d908a0, 0, 0, 0);
            }
        }
    } else if (event.m_id == SwapCommand) {
        if (!CTutorialManager::IsMenuBarBlocked()) {
            SwapWeapons();
        }
    } else if (event.m_id == OpenStore) {
        if (!CTutorialManager::IsMenuBarBlocked()) {
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
            m_avatar->SetCorner((m_content->m_width / 4 - m_avatar->m_width) / 2, 6 * App::Scale());
        }
    } else if (event.m_id == OpenAchievements) {
        if (!CTutorialManager::IsMenuBarBlocked()) {
            app->m_mainLayer->AddModal(new CAchievementsWindow());
        }
    } else if (event.m_id == OpenCareer) {
        if (!CTutorialManager::IsMenuBarBlocked()) {
            app->m_mainLayer->AddModal(new CCareerWindow());
        }
    } else if (event.m_id == OpenLeaderboards) {
        if (!CTutorialManager::IsMenuBarBlocked()) {
            GameCenter_Facade* gameCenter = GameCenter_Facade::GetInstance();
            if (gameCenter->isGameCenterSupported() && gameCenter->isAuthenticated()) {
                gameCenter->RunLeaderboardsView(NULL);
            }
        }
    } else if (event.m_id == OpenGlobalNav) {
        CGameScene::ShowGGN();
        CGameAnalytics::logGMGEntered();
    } else if (event.m_id == -0x68dfb87c || event.m_id == 0x5f523cd9) {
        if (CTutorialManager::CanCloseNetwork()) {
            Close();
            WindowApp::PutEvent(Event::TUNNEL_COMMAND, 0x40067282, 0, 0, 0);
        } else {
            WindowApp::HandleTunnelCommand(-0x64191a07, 0, 0, 0);
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

CImageButton* CBH_NetworkWindow::CreateSwapWeaponsWindow(unsigned int command)
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
