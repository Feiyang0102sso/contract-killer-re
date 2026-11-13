#include "CMainItemScreen.h"
#include "ServiceWindows.h"
#include "CBH_GPSMap.h"

#include "App.h"
#include "CAmmo.h"
#include "CAmmoManager.h"
#include "CAutoArrangedWindow.h"
#include "CBH_MenuButton.h"
#include "CBH_Player.h"
#include "CBH_ProgressBar.h"
#include "CBH_WeaponButton.h"
#include "CChallengeManager.h"
#include "CDH_BasicGameData.h"
#include "CDH_Weapon.h"
#include "CDH_WeaponsManager.h"
#include "CEquipItemButton.h"
#include "CFont.h"
#include "CFontMgr.h"
#include "CImageButton.h"
#include "CMiscItemManager.h"
#include "CScrollWindow.h"
#include "CShopButtonWeapon.h"
#include "CShopItemInfoWindow.h"
#include "CTutorialManager.h"
#include "DGHelper.h"
#include "SScreenAdaptingConstant.h"

static const SScreenAdaptingConstant WEAPON_SELECT_HEIGHT(95, 205, 205);

extern const int EventId_UpdateWeaponImage;
extern const int EventId_UpdateWeaponCaption;
extern const int EventId_UpdateWeaponBar;

namespace {
const int ChooseWeapon = -0x6a33e64d;
const int ChooseAmmo = 0x15461327;
const int ToggleSilencer = -0x1bdd63db;
const int ToggleHitechVisor = 0x036fcf2c;
const int ToggleCyberScope = 0x05a34149;
const int WeaponPurchased = static_cast<int>(0x9a412292u);
const int RefreshPlayerStats = -0x777f2883;
const int ShowWeaponStats = 0x029ffc42;
const int SilencerItem = 0;
const int HitechVisorItem = 2;
const int CyberScopeItem = 4;


}

CMainItemScreen::CMainItemScreen(int type, int slot)
    : CBaseScreen(false, false, true, false, true, false), m_itemButtons(NULL), m_equipmentPanel(NULL),
      m_silencer(NULL), m_hitechVisor(NULL), m_cyberScope(NULL), m_weapon(NULL), m_type(type), m_slot(slot)
{
    m_item = GetCurrentItemInSlotId();
    const char* title = "IDS_MENU_WEAPON_SLOT2";
    if (m_slot == 0) {
        title = "IDS_MENU_WEAPON_SLOT1";
    }
    SetTitleText(ResString(title));
    Window* content = m_content;
    Window* preview = createWeaponImage(m_weaponImage);
    content->AddToFront(preview);
    preview->SetOutsetSpacing(-GetTitleBarHeight(), 0, 0, 0);
    SetImageItem(GetCurrentItemInSlotId());
    m_background.Create("SUR_TAB_BACKGROUND");

    Window* equipmentPanel = new Window(false);
    m_equipmentPanel = equipmentPanel;
    equipmentPanel->SetPercentHeight(65, 0, 0);
    m_equipmentPanel->SetLayoutType(LAYOUT_SIMPLE);
    m_equipmentPanel->SetAlign(ALIGN_TOP);
    ImageWindow* border = new ImageWindow(ImageWindow::FILL_HORIZONTAL, ImageRes("SUR_BG_BORDER"));
    border->SetLayoutType(LAYOUT_SIMPLE);
    border->SetAlign(ALIGN_BOTTOM);
    m_equipmentPanel->AddToFront(border);
    m_content->AddToFront(m_equipmentPanel);

    m_chooser = CreateChoiseScrolling();
    m_content->AddToFront(m_chooser);
    Window* namePanel = new Window(false);
    namePanel->SetPercentHeight(55, 0, 0);
    namePanel->SetPercentWidth(80, 0, 0);
    namePanel->SetLayoutType(LAYOUT_SIMPLE);
    namePanel->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
    m_weaponName = new TextWindow(XString("TEST"), CFontMgr::GetInstance()->GetFont(15));
    m_weaponName->SetLayoutType(LAYOUT_SIMPLE);
    m_weaponName->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
    namePanel->AddToFront(m_weaponName);

    Window* stats = CreateStatsButton();
    stats->m_debugFrame = true;
    stats->SetPercentHeight(20, 0, 0);
    stats->SetPercentWidth(13, 0, 0);
    stats->SetLayoutType(LAYOUT_SIMPLE);
    stats->SetAlign(ALIGN_RIGHT | ALIGN_TOP);
    m_content->AddToFront(stats);
    Window* playerStats = CreatePlayerStatsWindow();
    playerStats->m_debugFrame = true;
    int width = m_width;
    playerStats->SetSize(width, GetTitleBarHeight());
    playerStats->SetPercentWidth(30, 0, 0);
    playerStats->SetLayoutType(LAYOUT_SIMPLE);
    playerStats->SetAlign(ALIGN_RIGHT | ALIGN_TOP);
    AddToFront(playerStats);
    m_content->AddToFront(namePanel);
    SetNameItem(GetCurrentItemInSlotId());
    SetCurrentItem(GetCurrentItemInSlotId());
}

int CMainItemScreen::GetCurrentItemInSlotId()
{
    // the original returns 0 for other screens and does not check the weapon
    int id = 0;
    if (m_type == 0) {
        int slot = m_slot;
        CDH_BasicGameData* data = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData();
        if (slot == 0) {
            id = data->GetWeapon01(true)->GetId();
        } else {
            id = data->GetWeapon02(true)->GetId();
        }
    }
    return id;
}

int CMainItemScreen::GetAnotherSlotItemId()
{
    // the original returns 0 for other screens and does not check the weapon
    int id = 0;
    if (m_type == 0) {
        int slot = m_slot;
        id = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon(1 - slot)->GetId();
    }
    return id;
}

bool CMainItemScreen::ItemAvailable(int id)
{
    // the original tests the screen type first and does not check the weapon
    if (m_type != 0) {
        return false;
    }
    return static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->getItemById(id)->isAvailable();
}

Window* CMainItemScreen::CreateChoiseScrolling()
{
    Window* chooser = new Window(false);
    chooser->SetPercentHeight(100, 0, 0);
    chooser->SetLayoutType(LAYOUT_SIMPLE);
    chooser->SetAlign(ALIGN_BOTTOM);
    chooser->m_debugFrame = true;

    Window* scrollArea = new Window(false);
    CScrollWindow* scroll = new CScrollWindow(true);
    scroll->SetLayoutType(LAYOUT_SIMPLE);
    scroll->SetAlign(ALIGN_TOP);
    scroll->SetPercentHeight(100, 0, 0);

    Vector<CDH_Weapon*> weapons;
    static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->GetAvailableWeapons(weapons);
    // Owned weapons are listed newest first.
    weapons.revert();
    static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->GetUnavailableWeapons(weapons);
    AddItems(scroll, weapons);

    scrollArea->SetPercentHeight(100, 0, 0);
    scrollArea->SetPercentWidth(100, 0, 0);
    scrollArea->AddToFront(scroll);
    scrollArea->SetPercentHeight(40, 0, 0);
    scrollArea->SetLayoutType(LAYOUT_SIMPLE);
    scrollArea->SetAlign(ALIGN_BOTTOM);
    scrollArea->m_debugFrame = true;
    chooser->AddToFront(scrollArea);
    return chooser;
}

// Original 0x131c40: owned weapons get a name label (font 15); the page is handed to the chooser after
// every button (as in the original), and a full page starts a new one.
void CMainItemScreen::AddItems(CScrollWindow* chooser, const Vector<CDH_Weapon*>& weapons)
{
    CAutoArrangedWindow* page = new CAutoArrangedWindow(m_width, WEAPON_SELECT_HEIGHT());
    page->SetPercentHeight(100, 0, 0);
    page->SetOneLineMode(true);
    for (int index = 0; index < weapons.size(); ++index) {
        CDH_Weapon* weapon = weapons[index];
        if (GetAnotherSlotItemId() == weapon->GetId()) {
            continue;
        }
        CBH_Button* button;
        if (weapon->isAvailable()) {
            CBH_WeaponButton* owned = new CBH_WeaponButton(0, -1, true, false, weapon->GetId());
            owned->ShowName();
            owned->SetLayoutType(LAYOUT_SIMPLE);
            owned->SetAlign(ALIGN_VCENTER);
            owned->SetPercentHeight(100, 0, 0);
            TextWindow* name = new TextWindow(weapon->m_name, CFontMgr::GetInstance()->GetFont(15));
            name->SetLayoutType(LAYOUT_SIMPLE);
            name->SetAlign(ALIGN_HCENTER | ALIGN_BOTTOM);
            owned->AddToFront(name);
            button = owned;
        } else {
            button = new CShopButtonWeapon(weapon->m_name, weapon->m_unlockLevel, weapon->m_pricePoints,
                weapon->GetId(), weapon->getShopImageResId());
            button->SetFlags(FLAG_CLIP_SCREEN);
        }
        button->m_userData = weapon->GetId();
        button->SetCommandParameters(ChooseWeapon, weapon->GetId(), 0, 0);
        if (!page->Add(button)) {
            chooser->Add(page);
            page = new CAutoArrangedWindow(m_width, WEAPON_SELECT_HEIGHT());
            page->SetPercentHeight(100, 0, 0);
            page->SetOneLineMode(true);
            page->Add(button);
        }
        chooser->Add(page);
    }
}

Window* CMainItemScreen::CreateStatsButton()
{
    // the original sizes and aligns neither the panel nor the button here
    Window* panel = new Window(false);
    CImageButton* button = new CImageButton();
    button->AddImage("IDB_STATS_ICON", 0, 0);
    button->SetWidthByContent(0, 0);
    button->SetHeightByContent(0, 0);
    button->m_command = ShowWeaponStats;
    button->m_commandParameter1 = 0;
    button->m_commandParameter2 = 0;
    button->m_commandParameter3 = 0;
    panel->AddToFront(button);
    return panel;
}

Window* CMainItemScreen::CreatePlayerStatsWindow()
{
    // Original: level badge in cell (0,0), cash/credits bars at fixed scaled corners in cell (1,0).
    Window* panel = new Window(false);
    Window* levelCell = new Window(false);
    levelCell->SetPercentWidth(25, 0, 0);
    panel->AddToFront(levelCell, 0, 0);
    ImageWindow* badge = new ImageWindow();
    badge->SetFillType(ImageWindow::FILL_NONE);
    badge->SetImage(ImageRes("SUR_SCOPE_LEVEL"));
    levelCell->AddToFront(badge);
    XString levelText(CBH_Player::GetInstance()->m_level);
    TextWindow* level = new TextWindow(levelText, CFontMgr::GetInstance()->GetFont(9));
    level->SetLayoutType(LAYOUT_SIMPLE);
    level->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    levelCell->AddToFront(level);
    Window* bars = new Window(false);
    panel->AddToFront(bars, 1, 0);
    m_cash = new CBH_ProgressBar(3, App::Scale() * 100, -1);
    m_cash->SetCorner(App::Scale() * 50, App::Scale() * 42);
    m_cash->SetDescImage("IDB_HUD_ICON_MONEY");
    m_cash->m_displayFlags = CBH_ProgressBar::DISPLAY_CURRENT | CBH_ProgressBar::HIDE_FILL;
    m_cash->SetTextAlign(TEXT_ALIGN_RIGHT);
    m_cash->SetCurrValue(CBH_Player::GetInstance()->m_cash);
    bars->AddToFront(m_cash, 0, 0);
    m_credits = new CBH_ProgressBar(3, App::Scale() * 100, -1);
    m_credits->SetCorner(App::Scale() * 50, App::Scale() * 62);
    m_credits->SetDescImage("IDB_HUD_ICON_CREDITS");
    m_credits->m_displayFlags = CBH_ProgressBar::DISPLAY_CURRENT | CBH_ProgressBar::HIDE_FILL;
    m_credits->SetTextAlign(TEXT_ALIGN_RIGHT);
    m_credits->SetCurrValue(CBH_Player::GetInstance()->m_credits);
    bars->AddToFront(m_credits, 0, 1);
    return panel;
}

void CMainItemScreen::UpdatePlayerStats()
{
    // the original asks CBH_Player::GetInstance() for each value
    m_cash->SetCurrValue(CBH_Player::GetInstance()->m_cash);
    m_credits->SetCurrValue(CBH_Player::GetInstance()->m_credits);
}

__attribute__((noinline)) void CMainItemScreen::HandleStatsButtonPressed()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CItem* weapon = app->InitWeaponManager()->getItemById(m_item);
    CShopItemInfoWindow* details = new CShopItemInfoWindow(
        static_cast<App*>(WindowApp::m_instance)->InitWeaponManager(), 0, m_item,
        weapon->GetStatsString(), true, CShopItemInfoWindow::SOURCE_SHOP);
    Window::AddModal(details);
}

void CMainItemScreen::SetCurrentItem(int id)
{
    SetImageItem(id);
    m_weapon = static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->GetWeaponById(id);
    m_item = id;
    if (m_itemButtons != NULL) {
        m_itemButtons->Close();
    }
    m_itemButtons = CreateItemButtons();
    m_equipmentPanel->AddToFront(m_itemButtons);
    WindowApp::HandleTunnelCommand(EventId_UpdateWeaponCaption, id, 0, 0);
    WindowApp::HandleTunnelCommand(EventId_UpdateWeaponBar, 0, 0, 0);
    WindowApp::HandleTunnelCommand(EventId_UpdateWeaponImage, 0, 0, 0);
}

void CMainItemScreen::SetCurrentItemToSlot()
{
    // the original equips the selected item id into the chosen slot without further checks
    if (m_slot == 0) {
        static_cast<App*>(WindowApp::m_instance)->m_helper->m_gamePlayManager.SetWeapon01(m_item);
    } else {
        static_cast<App*>(WindowApp::m_instance)->m_helper->m_gamePlayManager.SetWeapon02(m_item);
    }
}

Window* CMainItemScreen::CreateItemButtons()
{
    // The closing window owns the old buttons. Only keep pointers to the new row.
    m_ammoButtons.removeAllElements();
    Window* buttons = new Window(false);
    buttons->SetPercentHeight(30, 0, 0);
    buttons->SetPercentWidth(90, 0, 0);
    buttons->SetLayoutType(LAYOUT_SIMPLE);
    buttons->SetAlign(ALIGN_HCENTER | ALIGN_BOTTOM);
    buttons->m_debugFrame = true;

    int column = 0;
    const CAmmo* ammo = m_weapon->GetDefaultAmmo();
    if (m_weapon->CanUseAmmo(ammo)) {
        buttons->AddToFront(CreateButton(ammo), column, 0);
        ++column;
    }
    ammo = static_cast<App*>(WindowApp::m_instance)->InitAmmoManager()->GetAmmoByType(XString("explosive"));
    if (m_weapon->CanUseAmmo(ammo)) {
        buttons->AddToFront(CreateButton(ammo), column, 0);
        ++column;
    }
    ammo = static_cast<App*>(WindowApp::m_instance)->InitAmmoManager()->GetAmmoByType(XString("incendiary"));
    if (m_weapon->CanUseAmmo(ammo)) {
        buttons->AddToFront(CreateButton(ammo), column, 0);
        ++column;
    }
    if (m_weapon->m_silencerAvailable) {
        m_silencer = CreateSilencerButton();
        buttons->AddToFront(m_silencer, column, 0);
        ++column;
        UpdateSilencerButton();
    }
    m_hitechVisor = CreateHitechVisorButton();
    buttons->AddToFront(m_hitechVisor, column, 0);
    UpdateHitechVisorButton();
    m_cyberScope = CreateCyberScope();
    buttons->AddToFront(m_cyberScope, column + 1, 0);
    UpdateCyberScopeButton();
    return buttons;
}

CEquipItemButton* CMainItemScreen::CreateSilencerButton()
{
    CEquipItemButton* button = new CEquipItemButton("IDB_BULLETS_SILENCER_BUTTON", XString(""),
        CFontMgr::GetInstance()->GetFont(13), NULL,
        CBH_Player::GetInstance()->m_silentShotsCount);
    button->SetCommandParameters(ToggleSilencer, 0, 0, 0);
    return button;
}

CEquipItemButton* CMainItemScreen::CreateHitechVisorButton()
{
    CEquipItemButton* button = new CEquipItemButton("IDB_BULLETS_HITECH_VISOR_BUTTON", XString(""), NULL, NULL, -1);
    button->SetCommandParameters(ToggleHitechVisor, 0, 0, 0);
    return button;
}

CEquipItemButton* CMainItemScreen::CreateCyberScope()
{
    CEquipItemButton* button = new CEquipItemButton("IDB_BULLETS_CYBER_SCOPE_BUTTON", XString(""),
        CFontMgr::GetInstance()->GetFont(13), NULL,
        CBH_Player::GetInstance()->m_cyberScopesCount);
    button->SetCommandParameters(ToggleCyberScope, 0, 0, 0);
    return button;
}

void CMainItemScreen::UpdateAmmoButtons()
{
    for (int index = 0; index < m_ammoButtons.size(); ++index) {
        m_ammoButtons[index]->UpdateStatus();
    }
}

void CMainItemScreen::UpdateSilencerButton()
{
    if (m_weapon->m_silencerAvailable) {
        if (CBH_Player::GetInstance()->m_silentShotsCount < 1) {
            m_silencer->SetClickable(false);
            m_silencer->SetAvailable(false);
        } else if (m_weapon->isSilent()) {
            m_silencer->SetAvailable(true);
            m_silencer->SetBottomText(ResString("IDS_EQUIPED"), CFontMgr::GetInstance()->GetFont(14));
        } else {
            m_silencer->SetAvailable(false);
            m_silencer->SetBottomText(ResString("IDS_OWNED"), CFontMgr::GetInstance()->GetFont(13));
        }
        m_silencer->SetItemCount(CBH_Player::GetInstance()->m_silentShotsCount);
    }
}

void CMainItemScreen::UpdateHitechVisorButton()
{
    CItem* visor = static_cast<App*>(WindowApp::m_instance)->InitMiscItemManager()->getItemById(HitechVisorItem);
    if (!visor->isAvailable()) {
        m_hitechVisor->SetClickable(true);
        m_hitechVisor->SetAvailable(false);
    } else if (CBH_Player::GetInstance()->m_hitechVisorEquipped) {
        m_hitechVisor->SetAvailable(true);
        m_hitechVisor->SetBottomText(ResString("IDS_EQUIPED"), CFontMgr::GetInstance()->GetFont(14));
    } else {
        m_hitechVisor->SetAvailable(false);
        m_hitechVisor->SetBottomText(ResString("IDS_OWNED"), CFontMgr::GetInstance()->GetFont(13));
    }
}

void CMainItemScreen::UpdateCyberScopeButton()
{
    if (m_weapon->m_cyberScopeEquipped) {
        m_cyberScope->SetAvailable(true);
        m_cyberScope->SetBottomText(ResString("IDS_EQUIPED"), CFontMgr::GetInstance()->GetFont(14));
    } else if (CBH_Player::GetInstance()->m_cyberScopesCount > 0) {
        m_cyberScope->SetAvailable(false);
        m_cyberScope->SetBottomText(ResString("IDS_OWNED"), CFontMgr::GetInstance()->GetFont(13));
    } else {
        m_cyberScope->SetClickable(true);
        m_cyberScope->SetAvailable(false);
        m_cyberScope->SetBottomText(XString(""), NULL);
    }
    m_cyberScope->SetItemCount(CBH_Player::GetInstance()->m_cyberScopesCount);
}

void CMainItemScreen::HandleBulletButtonPressed(const CAmmo* ammo)
{
    if (CBH_Player::GetInstance()->GetAvailableAmmo(ammo, true) != 0xfffffff
        && (CBH_Player::GetInstance()->GetAvailableAmmo(ammo, true) == 0 || m_weapon->GetActiveAmmo() == ammo)) {
        CShopItemInfoWindow* details = new CShopItemInfoWindow(
            static_cast<App*>(WindowApp::m_instance)->InitAmmoManager(), 0, ammo->m_id,
            ammo->GetStatsString(), true, CShopItemInfoWindow::SOURCE_SHOP);
        AddModal(details);
    } else {
        if (CBH_Player::GetInstance()->GetAvailableAmmo(ammo, true) < 1) {
            goto saveWeapon;
        }
        m_weapon->m_silencerActive = false;
        UpdateSilencerButton();
        m_weapon->SetActiveAmmo(ammo);
    }
    UpdateAmmoButtons();
    UpdateSilencerButton();
saveWeapon:
    m_weapon->Save(true);
}

void CMainItemScreen::HandleSilencerButtonPressed()
{
    if (CBH_Player::GetInstance()->m_silentShotsCount < 1) {
        CItem* silencer = static_cast<App*>(WindowApp::m_instance)->InitMiscItemManager()->getItemById(SilencerItem);
        CShopItemInfoWindow* details = new CShopItemInfoWindow(
            static_cast<App*>(WindowApp::m_instance)->InitMiscItemManager(), 0, silencer->m_id,
            XString(), true, CShopItemInfoWindow::SOURCE_SHOP);
        AddModal(details);
        UpdateAmmoButtons();
        UpdateSilencerButton();
    } else if (m_weapon->m_silencerAvailable) {
        bool wasSilent = m_weapon->isSilent();
        m_weapon->m_silencerActive = !wasSilent;
    }
    UpdateSilencerButton();
    const CAmmo* ammo = m_weapon->GetDefaultAmmo();
    m_weapon->SetActiveAmmo(ammo);
    UpdateAmmoButtons();
    m_weapon->Save(true);
}

void CMainItemScreen::HandleHitechVisorButtonPressed()
{
    CItem* visor = static_cast<App*>(WindowApp::m_instance)->InitMiscItemManager()->getItemById(HitechVisorItem);
    if (visor->isAvailable()) {
        bool equipped = CBH_Player::GetInstance()->m_hitechVisorEquipped;
        CBH_Player::GetInstance()->m_hitechVisorEquipped = !equipped;
        UpdateHitechVisorButton();
    } else {
        CShopItemInfoWindow* details = new CShopItemInfoWindow(
            static_cast<App*>(WindowApp::m_instance)->InitMiscItemManager(), 0, HitechVisorItem,
            visor->GetStatsString(), true, CShopItemInfoWindow::SOURCE_SHOP);
        AddModal(details);
    }
}

void CMainItemScreen::HandleCyberScopeButtonPressed()
{
    CBH_Player* player = CBH_Player::GetInstance();
    CDH_Weapon* weapon = m_weapon;
    bool equipped = weapon->m_cyberScopeEquipped;
    if (player->m_cyberScopesCount == 0 && !equipped) {
        CItem* scope = static_cast<App*>(WindowApp::m_instance)->InitMiscItemManager()->getItemById(CyberScopeItem);
        CShopItemInfoWindow* details = new CShopItemInfoWindow(
            static_cast<App*>(WindowApp::m_instance)->InitMiscItemManager(), 0, scope->m_id,
            scope->GetStatsString(), true, CShopItemInfoWindow::SOURCE_SHOP);
        AddModal(details);
    } else {
        weapon->SetCyberScopeEquiped(!weapon->m_cyberScopeEquipped);
        bool nowEquipped = m_weapon->m_cyberScopeEquipped;
        CBH_Player* inventory = CBH_Player::GetInstance();
        if (nowEquipped) {
            inventory->ReduceCyberScopeCount(1);
        } else {
            inventory->AddCyberScopeCount(1);
        }
    }
    UpdateCyberScopeButton();
    m_weapon->Save(true);
}

void CMainItemScreen::ShowBuyWeaponWindow(int id)
{
    CDH_WeaponsManager* manager = static_cast<App*>(WindowApp::m_instance)->InitWeaponManager();
    manager->getItemById(id);
    manager->getItemById(id);
    if (manager->getItemById(id)->getPriceCredits() > 0) {
        manager->getItemById(id)->getPriceCredits();
    }
    if (manager->getItemById(id)->m_priceIAP > 0) {
        manager->getItemById(id);
    }
    // The original keeps the name alive while it opens the item information.
    XString name(manager->getItemById(id)->GetName());
    manager->getItemById(id)->getShopImage();
    CItem* weapon = static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->getItemById(id);
    CShopItemInfoWindow* details = new CShopItemInfoWindow(
        static_cast<App*>(WindowApp::m_instance)->InitWeaponManager(), 0, id,
        weapon->GetStatsString(), true, CShopItemInfoWindow::SOURCE_SHOP);
    AddModal(details);
}

void CMainItemScreen::OnCommand(Event& event)
{
    switch (event.m_id) {
    case RefreshPlayerStats:
        UpdateHitechVisorButton();
        UpdateSilencerButton();
        UpdateAmmoButtons();
        UpdatePlayerStats();
        UpdateCyberScopeButton();
        break;
    case ToggleSilencer:
        HandleSilencerButtonPressed();
        break;
    case ToggleHitechVisor:
        HandleHitechVisorButtonPressed();
        break;
    case ShowWeaponStats:
        HandleStatsButtonPressed();
        break;
    case ToggleCyberScope:
        HandleCyberScopeButtonPressed();
        break;
    case ChooseAmmo:
        HandleBulletButtonPressed(static_cast<const CAmmo*>(
            static_cast<App*>(WindowApp::m_instance)->InitAmmoManager()->getItemById(event.m_param1)));
        break;
    }

    if (event.m_id == WeaponPurchased) {
        m_chooser->Close();
        m_chooser = CreateChoiseScrolling();
        m_content->AddToFront(m_chooser);
        SetCurrentItem(event.m_param1);
    }
    if (event.m_id == -0x68dfb87c || event.m_id == 0x5f523cd9) {
        WindowApp::HandleTunnelCommand(0x6e6446f5, 0, 0, 0);
        if (static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->CanCloseWeaponChoice()) {
            SetCurrentItemToSlot();
            ReverseEasy(FLAG_CLOSE_AFTER_EASY);
            event.Clear();
        }
        return;
    }
    if (EventId_UpdateWeaponImage == event.m_id) {
        SetImageItem(m_item);
        SetNameItem(m_item);
    } else if (event.m_id == ChooseWeapon) {
        if (ItemAvailable(event.m_param1)) {
            WindowApp::HandleTunnelCommand(0x0df6b618, 0, 0, 0);
            SetCurrentItem(event.m_param1);
            CBH_Player::GetInstance()->Save();
            static_cast<App*>(WindowApp::m_instance)->m_challengeManager->Save();
        } else {
            ShowBuyWeaponWindow(event.m_param1);
        }
    }
    CBaseScreen::OnCommand(event);
}

void CMainItemScreen::Update()
{
    // The original calls both tutorial checks on App::m_tutorialManager (+0x2b0).
    if (static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial() == true && static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsWeaponChoiceCloseBlink() == true) {
        m_back->SetBlinking(true);
    }
    CBaseScreen::Update();
}

void CMainItemScreen::Paint(ICGraphics2d& graphics)
{
    DrawTileImageRect(graphics, m_background.GetSurface(), 0, 0, m_width, m_height);
    CBaseScreen::Paint(graphics);
}

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AssertMainItemScreenSize[sizeof(CMainItemScreen) == 0x120 ? 1 : -1];
#endif

void CMainItemScreen::OnEnterForeground()
{
    CBH_GPSMap::weaponIsPainting = true;
}

void CMainItemScreen::OnLostFocus()
{
}

void CMainItemScreen::OnEasyStarted()
{
    CBH_GPSMap::weaponIsPainting = false;
}

Window* CMainItemScreen::CreateButton(const CAmmo* ammo)
{
    Window* cell = new Window(false);
    cell->SetPercentHeight(100, 0, 0);
    CEquipAmmoButton* button = new CEquipAmmoButton(m_weapon, ammo);
    const int ammoId = ammo->m_id;
    button->m_command = ChooseAmmo;
    button->m_commandParameter1 = ammoId;
    button->m_commandParameter2 = 0;
    button->m_commandParameter3 = 0;
    cell->AddToFront(button);
    button->SetLayoutType(LAYOUT_SIMPLE);
    button->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    button->SetPercentHeight(100, 0, 0);
    button->SetPercentWidth(100, 0, 0);
    m_ammoButtons.addElement(button);
    return cell;
}

Window* CMainItemScreen::createWeaponImage(ImageWindow*& weaponImage)
{
    Window* cell = createTD(NULL, 3);
    ImageWindow* background = NULL;
    // The temporary ImageRes lives until AddToFront returns (one full expression, as in the original).
    cell->AddToFront(background = new ImageWindow(ImageWindow::FILL_NONE, ImageRes("SUR_SHOP_BACKGROUND")));
    background->SetAlign(0xc);
    weaponImage = new ImageWindow();
    cell->AddToFront(weaponImage);
    weaponImage->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    cell->SetAlign(0xc);
    return cell;
}

void CMainItemScreen::SetImageItem(int id)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_Weapon* weapon = app->InitWeaponManager()->GetWeaponById(id);
    m_weaponImage->SetImageById(weapon->getBigImageResId());
}

void CMainItemScreen::SetNameItem(int id)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_Weapon* weapon = app->InitWeaponManager()->GetWeaponById(id);
    m_weaponName->SetText(weapon->m_name.Upper());
}
