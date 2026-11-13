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

namespace {
const int ChooseWeapon = -0x6a33e64d;
const int ChooseAmmo = 0x15461327;
const int ToggleSilencer = -0x1bdd63db;
const int ToggleHitechVisor = 0x036fcf2c;
const int ToggleCyberScope = 0x05a34149;
const int WeaponPurchased = static_cast<int>(0x9a412292u);
const int RefreshAmmo = -0x44b7c071;
const int RefreshInventory = 0x1e7d8d93;
const int RefreshPlayerStats = -0x777f2883;
const int ShowWeaponStats = 0x029ffc42;
const int SilencerItem = 0;
const int HitechVisorItem = 2;
const int CyberScopeItem = 4;

TextWindow* AddText(Window* parent, int font, int x, int y, int width, int height)
{
    TextWindow* label = new TextWindow(CFontMgr::GetInstance()->GetFont(font));
    label->SetLayoutType(Window::LAYOUT_NONE);
    label->SetRect(x, y, width, height, false);
    label->ClearFlags(Window::FLAG_FOCUSABLE);
    parent->AddToFront(label);
    return label;
}

void UpdateEquipButton(CEquipItemButton* button, bool owned, bool equipped)
{
    if (button == NULL) {
        return;
    }
    button->SetClickable(false);
    button->SetAvailable(equipped);
    if (equipped) {
        button->SetBottomText(Window::ResString("IDS_EQUIPED"), CFontMgr::GetInstance()->GetFont(14));
    } else if (owned) {
        button->SetBottomText(Window::ResString("IDS_OWNED"), CFontMgr::GetInstance()->GetFont(13));
    } else {
        button->SetBottomText(XString(""), NULL);
    }
}

}

CMainItemScreen::CMainItemScreen(int type, int slot)
    : CBaseScreen(false, false, true, false, true, false), m_background("SUR_TAB_BACKGROUND"),
      m_chooser(NULL), m_itemButtons(NULL), m_silencer(NULL), m_hitechVisor(NULL), m_cyberScope(NULL),
      m_weapon(NULL), m_type(type), m_slot(slot)
{
    const char* title = "IDS_MENU_WEAPON_SLOT2";
    if (slot == 0) {
        title = "IDS_MENU_WEAPON_SLOT1";
    }
    SetTitleText(ResString(title));
    Window* preview = createWeaponImage(m_weaponImage);
    m_content->AddToFront(preview);
    preview->SetOutsetSpacing(-GetTitleBarHeight(), 0, 0, 0);

    m_equipmentPanel = new Window(false);
    m_equipmentPanel->SetPercentHeight(65, 0, 0);
    m_equipmentPanel->SetLayoutType(LAYOUT_SIMPLE);
    m_equipmentPanel->SetAlign(ALIGN_TOP);
    ImageWindow* border = new ImageWindow(ImageWindow::FILL_HORIZONTAL, ImageRes("SUR_BG_BORDER"));
    border->SetLayoutType(LAYOUT_SIMPLE);
    border->SetAlign(ALIGN_BOTTOM);
    m_equipmentPanel->AddToFront(border);
    m_content->AddToFront(m_equipmentPanel);

    CreateChoiseScrolling();
    Window* namePanel = new Window(false);
    namePanel->SetPercentHeight(55, 0, 0);
    namePanel->SetPercentWidth(80, 0, 0);
    namePanel->SetLayoutType(LAYOUT_SIMPLE);
    namePanel->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
    m_weaponName = new TextWindow(CFontMgr::GetInstance()->GetFont(15));
    m_weaponName->SetLayoutType(LAYOUT_SIMPLE);
    m_weaponName->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
    namePanel->AddToFront(m_weaponName);

    m_content->AddToFront(CreateStatsButton());
    AddToFront(CreatePlayerStatsWindow());
    m_content->AddToFront(namePanel);
    SetCurrentItem(GetCurrentItemInSlotId());
}

int CMainItemScreen::GetCurrentItemInSlotId()
{
    // the original returns 0 for other screens and does not check the weapon
    int id = 0;
    if (m_type == 0) {
        int slot = m_slot;
        id = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon(slot)->GetId();
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

void CMainItemScreen::CreateChoiseScrolling()
{
    if (m_chooser != NULL) {
        m_chooser->Close();
    }
    m_chooser = new Window(false);
    m_chooser->SetPercentHeight(100, 0, 0);
    m_chooser->SetLayoutType(LAYOUT_SIMPLE);
    m_chooser->SetAlign(ALIGN_BOTTOM);
    m_chooser->m_debugFrame = true;

    CScrollWindow* scroll = new CScrollWindow(true);
    scroll->SetLayoutType(LAYOUT_SIMPLE);
    scroll->SetAlign(ALIGN_TOP);
    scroll->SetPercentHeight(100, 0, 0);

    App* app = static_cast<App*>(WindowApp::m_instance);
    Vector<CDH_Weapon*> weapons;
    app->InitWeaponManager()->GetAvailableWeapons(weapons);
    // Owned weapons are listed newest first.
    weapons.revert();
    app->InitWeaponManager()->GetUnavailableWeapons(weapons);
    AddItems(scroll, weapons);

    Window* scrollArea = new Window(false);
    scrollArea->SetPercentHeight(40, 0, 0);
    scrollArea->SetPercentWidth(100, 0, 0);
    scrollArea->SetLayoutType(LAYOUT_SIMPLE);
    scrollArea->SetAlign(ALIGN_BOTTOM);
    scrollArea->m_debugFrame = true;
    scrollArea->AddToFront(scroll);
    m_chooser->AddToFront(scrollArea);
    m_content->AddToFront(m_chooser);
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
    if (weapon == NULL) {
        return;
    }
    CShopItemInfoWindow* details = new CShopItemInfoWindow(
        static_cast<App*>(WindowApp::m_instance)->InitWeaponManager(), 0, m_item,
        weapon->GetStatsString(), true, CShopItemInfoWindow::SOURCE_SHOP);
    Window::AddModal(details);
}

void CMainItemScreen::SetCurrentItem(int id)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    m_item = id;
    m_weapon = app->InitWeaponManager()->GetWeaponById(id);
    if (m_weapon != NULL) {
        const char* imageId = m_weapon->getBigImageResId();
        if (imageId != NULL && imageId[0] != '\0') {
            SetImageItem(id);
        } else {
            imageId = m_weapon->getShopImageResId();
            if (imageId == NULL) {
                imageId = "";
            }
            m_weaponImage->SetImageById(imageId);
        }
        SetNameItem(id);
    } else {
        m_weaponImage->SetImageById("");
        m_weaponName->SetText(XString());
    }
    CreateItemButtons();
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

void CMainItemScreen::CreateItemButtons()
{
    if (m_itemButtons != NULL) {
        m_itemButtons->Close();
    }
    // The closing window owns the old buttons. Only keep pointers to the new row.
    m_ammoButtons.removeAllElements();
    m_silencer = NULL;
    m_hitechVisor = NULL;
    m_cyberScope = NULL;
    m_itemButtons = new Window(false);
    m_itemButtons->SetPercentHeight(30, 0, 0);
    m_itemButtons->SetPercentWidth(90, 0, 0);
    m_itemButtons->SetLayoutType(LAYOUT_SIMPLE);
    m_itemButtons->SetAlign(ALIGN_HCENTER | ALIGN_BOTTOM);
    m_itemButtons->m_debugFrame = true;
    m_equipmentPanel->AddToFront(m_itemButtons);
    if (m_weapon == NULL || static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsMenuBarBlocked()) {
        return;
    }

    App* app = static_cast<App*>(WindowApp::m_instance);
    CAmmoManager* ammoManager = app->InitAmmoManager();
    const CAmmo* ammunition[] = {
        m_weapon->GetDefaultAmmo(),
        ammoManager->GetAmmoByType(XString("explosive")),
        ammoManager->GetAmmoByType(XString("incendiary"))
    };
    int column = 0;
    for (int index = 0; index < 3; ++index) {
        const CAmmo* ammo = ammunition[index];
        if (ammo != NULL && m_weapon->CanUseAmmo(ammo)) {
            m_itemButtons->AddToFront(CreateButton(ammo), column++, 0);
        }
    }
    if (m_weapon->m_silencerAvailable && !m_weapon->m_originallySilent) {
        m_silencer = CreateSilencerButton();
        m_itemButtons->AddToFront(m_silencer, column++, 0);
        UpdateSilencerButton();
    }
    m_hitechVisor = CreateHitechVisorButton();
    m_itemButtons->AddToFront(m_hitechVisor, column++, 0);
    UpdateHitechVisorButton();
    m_cyberScope = CreateCyberScope();
    m_itemButtons->AddToFront(m_cyberScope, column, 0);
    UpdateCyberScopeButton();
}

CEquipItemButton* CMainItemScreen::CreateSilencerButton()
{
    CFont* font = CFontMgr::GetInstance()->GetFont(13);
    CEquipItemButton* button = new CEquipItemButton("IDB_BULLETS_SILENCER_BUTTON", XString(""), font, NULL,
        CBH_Player::GetInstance()->m_silentShotsCount);
    button->m_command = ToggleSilencer;
    return button;
}

CEquipItemButton* CMainItemScreen::CreateHitechVisorButton()
{
    CEquipItemButton* button = new CEquipItemButton("IDB_BULLETS_HITECH_VISOR_BUTTON", XString(""), NULL, NULL, -1);
    button->m_command = ToggleHitechVisor;
    return button;
}

CEquipItemButton* CMainItemScreen::CreateCyberScope()
{
    CFont* font = CFontMgr::GetInstance()->GetFont(13);
    CEquipItemButton* button = new CEquipItemButton("IDB_BULLETS_CYBER_SCOPE_BUTTON", XString(""), font, NULL,
        CBH_Player::GetInstance()->m_cyberScopesCount);
    button->m_command = ToggleCyberScope;
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
    if (m_silencer == NULL || m_weapon == NULL || !m_weapon->m_silencerAvailable) {
        return;
    }
    int count = CBH_Player::GetInstance()->m_silentShotsCount;
    if (count < 1) {
        m_silencer->SetClickable(false);
        m_silencer->SetAvailable(false);
        m_silencer->SetBottomText(XString(""), NULL);
    } else if (m_weapon->isSilent()) {
        m_silencer->SetAvailable(true);
        m_silencer->SetBottomText(ResString("IDS_EQUIPED"), CFontMgr::GetInstance()->GetFont(14));
    } else {
        m_silencer->SetAvailable(false);
        m_silencer->SetBottomText(ResString("IDS_OWNED"), CFontMgr::GetInstance()->GetFont(13));
    }
    m_silencer->SetItemCount(count);
}

void CMainItemScreen::UpdateHitechVisorButton()
{
    if (m_hitechVisor == NULL) {
        return;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    CItem* visor = app->InitMiscItemManager()->getItemById(HitechVisorItem);
    if (visor == NULL || !visor->isAvailable()) {
        m_hitechVisor->SetClickable(true);
        m_hitechVisor->SetAvailable(false);
        m_hitechVisor->SetBottomText(XString(""), NULL);
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
    if (m_cyberScope != NULL) {
        int count = CBH_Player::GetInstance()->m_cyberScopesCount;
        bool equipped = m_weapon->m_cyberScopeEquipped;
        UpdateEquipButton(m_cyberScope, count > 0 || equipped, equipped);
        m_cyberScope->SetItemCount(count);
    }
}

void CMainItemScreen::ShowMiscItemDetails(int itemId)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CMiscItemManager* manager = app->InitMiscItemManager();
    CItem* item = manager->getItemById(itemId);
    if (item != NULL) {
        AddModal(new CShopItemInfoWindow(manager, 0, itemId, item->GetStatsString(), true,
            CShopItemInfoWindow::SOURCE_SHOP));
    }
}

void CMainItemScreen::HandleBulletButtonPressed(const CAmmo* ammo)
{
    if (m_weapon == NULL || ammo == NULL || !m_weapon->CanUseAmmo(ammo)) {
        return;
    }
    int available = CBH_Player::GetInstance()->GetAvailableAmmo(ammo, true);
    if (available == 0 || (available != 0xfffffff && m_weapon->GetActiveAmmo() == ammo)) {
        App* app = static_cast<App*>(WindowApp::m_instance);
        AddModal(new CShopItemInfoWindow(app->InitAmmoManager(), 0, ammo->m_id, ammo->GetStatsString(), true,
            CShopItemInfoWindow::SOURCE_SHOP));
        return;
    }
    m_weapon->m_silencerActive = false;
    m_weapon->SetActiveAmmo(ammo);
    m_weapon->Save(true);
    UpdateAmmoButtons();
    UpdateSilencerButton();
}

void CMainItemScreen::HandleSilencerButtonPressed()
{
    if (m_weapon == NULL || !m_weapon->m_silencerAvailable || m_weapon->m_originallySilent) {
        return;
    }
    if (CBH_Player::GetInstance()->m_silentShotsCount < 1) {
        ShowMiscItemDetails(SilencerItem);
        return;
    }
    m_weapon->m_silencerActive = !m_weapon->m_silencerActive;
    m_weapon->SetActiveAmmo(m_weapon->GetDefaultAmmo());
    m_weapon->Save(true);
    UpdateAmmoButtons();
    UpdateSilencerButton();
}

void CMainItemScreen::HandleHitechVisorButtonPressed()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CItem* visor = app->InitMiscItemManager()->getItemById(HitechVisorItem);
    if (visor != NULL && visor->isAvailable()) {
        CBH_Player* player = CBH_Player::GetInstance();
        player->m_hitechVisorEquipped = !player->m_hitechVisorEquipped;
        player->Save();
        UpdateHitechVisorButton();
    } else {
        ShowMiscItemDetails(HitechVisorItem);
    }
}

void CMainItemScreen::HandleCyberScopeButtonPressed()
{
    if (m_weapon == NULL) {
        return;
    }
    CBH_Player* player = CBH_Player::GetInstance();
    if (m_weapon->m_cyberScopeEquipped) {
        m_weapon->SetCyberScopeEquiped(false);
        player->AddCyberScopeCount(1);
    } else if (player->m_cyberScopesCount > 0) {
        m_weapon->SetCyberScopeEquiped(true);
        player->ReduceCyberScopeCount(1);
    } else {
        ShowMiscItemDetails(CyberScopeItem);
        return;
    }
    m_weapon->Save(true);
    UpdateCyberScopeButton();
}

void CMainItemScreen::ShowBuyWeaponWindow(int id)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_WeaponsManager* manager = app->InitWeaponManager();
    CDH_Weapon* weapon = manager->GetWeaponById(id);
    if (weapon != NULL) {
        AddModal(new CShopItemInfoWindow(manager, 0, id, weapon->GetStatsString(), true,
            CShopItemInfoWindow::SOURCE_SHOP));
    }
}

void CMainItemScreen::OnCommand(Event& event)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (event.m_id == ChooseWeapon) {
        if (ItemAvailable(event.m_param1)) {
            SetCurrentItem(event.m_param1);
            WindowApp::HandleTunnelCommand(0x0df6b618, 0, 0, 0);
        } else if (static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->CanCloseWeaponChoice()) {
            ShowBuyWeaponWindow(event.m_param1);
        }
    } else if (event.m_id == ChooseAmmo) {
        const CAmmo* ammo = static_cast<const CAmmo*>(app->InitAmmoManager()->getItemById(event.m_param1));
        HandleBulletButtonPressed(ammo);
    } else if (event.m_id == ToggleSilencer) {
        HandleSilencerButtonPressed();
    } else if (event.m_id == ToggleHitechVisor) {
        HandleHitechVisorButtonPressed();
    } else if (event.m_id == ToggleCyberScope) {
        HandleCyberScopeButtonPressed();
    } else if (event.m_id == ShowWeaponStats) {
        HandleStatsButtonPressed();
    } else if (event.m_id == RefreshAmmo || event.m_id == RefreshInventory || event.m_id == RefreshPlayerStats) {
        UpdateAmmoButtons();
        UpdateSilencerButton();
        UpdateHitechVisorButton();
        UpdateCyberScopeButton();
        UpdatePlayerStats();
        return;
    } else if (event.m_id == WeaponPurchased) {
        CreateChoiseScrolling();
        SetCurrentItem(event.m_param1);
        return;
    } else if (event.m_id == -0x68dfb87c || event.m_id == 0x5f523cd9) {
        WindowApp::HandleTunnelCommand(0x6e6446f5, 0, 0, 0);
        if (static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->CanCloseWeaponChoice()) {
            SetCurrentItemToSlot();
            ReverseEasy(FLAG_CLOSE_AFTER_EASY);
        }
    } else {
        CBaseScreen::OnCommand(event);
        return;
    }
    event.Clear();
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

typedef char AssertMainItemScreenSize[sizeof(CMainItemScreen) == 0x120 ? 1 : -1];

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
