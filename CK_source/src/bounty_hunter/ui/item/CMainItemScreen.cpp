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
    int scale = App::Scale();
    int previewWidth = m_content->m_width * 3 / 5;
    int upperHeight = m_content->m_height * 3 / 5;
    const char* title = "IDS_MENU_WEAPON_SLOT2";
    if (slot == 0) {
        title = "IDS_MENU_WEAPON_SLOT1";
    }
    SetTitleText(ResString(title));
    m_weaponName = AddText(m_content, 9, 10 * scale, 5 * scale, previewWidth - 10 * scale, 21 * scale);
    m_weaponImage = new ImageWindow();
    m_weaponImage->SetFillType(ImageWindow::FILL_NONE);
    m_weaponImage->SetLayoutType(LAYOUT_NONE);
    m_weaponImage->SetRect(5 * scale, 28 * scale, previewWidth - 10 * scale, upperHeight / 2, false);
    m_weaponImage->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    m_weaponImage->ClearFlags(FLAG_FOCUSABLE);
    m_content->AddToFront(m_weaponImage);
    int statsY = m_weaponImage->m_y + m_weaponImage->m_height;
    m_stats = AddText(m_content, 8, 10 * scale, statsY, previewWidth - 14 * scale, upperHeight - statsY);
    m_content->AddToFront(CreateStatsButton());
    AddToFront(CreatePlayerStatsWindow());

    CreateChoiseScrolling();
    SetCurrentItem(GetCurrentItemInSlotId());
}

int CMainItemScreen::GetCurrentItemInSlotId()
{
    if (m_type != 0) {
        return -1;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_Weapon* weapon = app->m_helper->GetGameData()->GetWeapon(m_slot);
    if (weapon != NULL) {
        return weapon->GetId();
    }
    return -1;
}

int CMainItemScreen::GetAnotherSlotItemId()
{
    if (m_type != 0) {
        return -1;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_Weapon* weapon = app->m_helper->GetGameData()->GetWeapon(1 - m_slot);
    if (weapon != NULL) {
        return weapon->GetId();
    }
    return -1;
}

bool CMainItemScreen::ItemAvailable(int id)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_Weapon* weapon = app->InitWeaponManager()->GetWeaponById(id);
    return m_type == 0 && weapon != NULL && weapon->isAvailable();
}

void CMainItemScreen::CreateChoiseScrolling()
{
    if (m_chooser != NULL) {
        m_chooser->Close();
    }
    int y = m_content->m_height * 3 / 5;
    m_chooser = new CScrollWindow(true);
    m_chooser->SetLayoutType(LAYOUT_NONE);
    m_chooser->SetRect(0, y, m_content->m_width, m_content->m_height - y, false);
    m_content->AddToFront(m_chooser);

    App* app = static_cast<App*>(WindowApp::m_instance);
    Vector<CDH_Weapon*> weapons;
    app->InitWeaponManager()->GetAvailableWeapons(weapons);
    // Owned weapons are listed newest first.
    weapons.revert();
    app->InitWeaponManager()->GetUnavailableWeapons(weapons);
    AddItems(m_chooser, weapons);
}

void CMainItemScreen::AddItems(CScrollWindow* chooser, const Vector<CDH_Weapon*>& weapons)
{
    int other = GetAnotherSlotItemId();
    int scale = App::Scale();
    int cardWidth = chooser->m_width / 4;
    int cardHeight = chooser->m_height - 18 * scale;
    CAutoArrangedWindow* page = NULL;
    for (int index = 0; index < weapons.size(); ++index) {
        CDH_Weapon* weapon = weapons[index];
        int id = weapon->GetId();
        if (id == other) {
            continue;
        }
        CBH_Button* button;
        if (weapon->isAvailable()) {
            CBH_WeaponButton* owned = new CBH_WeaponButton(0, -1, true, false, id);
            owned->ShowName();
            button = owned;
        } else {
            button = new CShopButtonWeapon(weapon->GetName(), weapon->m_unlockLevel, weapon->m_pricePoints,
                id, weapon->getShopImageResId());
            button->SetFlags(FLAG_CLIP_SCREEN);
        }
        button->m_command = ChooseWeapon;
        button->m_commandParameter1 = id;
        button->SetRect(0, 0, cardWidth, cardHeight, false);
        if (page != NULL && page->Add(button)) {
            continue;
        }
        page = new CAutoArrangedWindow(chooser->m_width, chooser->m_height);
        page->SetOneLineMode(true);
        page->Add(button);
        chooser->Add(page);
    }
}

Window* CMainItemScreen::CreateStatsButton()
{
    Window* panel = new Window(false);
    panel->SetLayoutType(LAYOUT_NONE);
    int width = m_content->m_width * 13 / 100;
    panel->SetRect(m_content->m_width - width, 0, width, 30 * App::Scale(), false);
    CImageButton* button = new CImageButton();
    button->AddImage("IDB_STATS_ICON", 0, 0);
    button->SetWidthByContent(0, 0);
    button->SetHeightByContent(0, 0);
    button->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    button->m_command = ShowWeaponStats;
    panel->AddToFront(button);
    return panel;
}

Window* CMainItemScreen::CreatePlayerStatsWindow()
{
    Window* panel = new Window(false);
    panel->SetLayoutType(LAYOUT_NONE);
    int width = m_width * 30 / 100;
    int height = GetTitleBarHeight();
    panel->SetRect(m_width - width, 0, width, height, false);
    int levelWidth = width / 4;
    ImageWindow* badge = new ImageWindow(ImageWindow::FILL_NONE, ImageRes("SUR_SCOPE_LEVEL"));
    badge->SetLayoutType(LAYOUT_NONE);
    badge->SetRect(0, 0, levelWidth, height, false);
    badge->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    panel->AddToFront(badge);
    TextWindow* level = AddText(panel, 9, 0, 0, levelWidth, height);
    level->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    level->SetText(XString(CBH_Player::GetInstance()->GetLevel()));

    int barWidth = width - levelWidth - 5 * App::Scale();
    m_cash = new CBH_ProgressBar(3, barWidth, -1);
    m_cash->SetCorner(levelWidth, 0);
    m_cash->SetDescImage("IDB_HUD_ICON_MONEY");
    m_cash->m_displayFlags = CBH_ProgressBar::DISPLAY_CURRENT | CBH_ProgressBar::HIDE_FILL;
    m_cash->SetTextAlign(TEXT_ALIGN_RIGHT);
    panel->AddToFront(m_cash);

    m_credits = new CBH_ProgressBar(3, barWidth, -1);
    m_credits->SetCorner(levelWidth, height / 2);
    m_credits->SetDescImage("IDB_HUD_ICON_CREDITS");
    m_credits->m_displayFlags = CBH_ProgressBar::DISPLAY_CURRENT | CBH_ProgressBar::HIDE_FILL;
    m_credits->SetTextAlign(TEXT_ALIGN_RIGHT);
    panel->AddToFront(m_credits);
    UpdatePlayerStats();
    return panel;
}

void CMainItemScreen::UpdatePlayerStats()
{
    CBH_Player* player = CBH_Player::GetInstance();
    m_cash->SetCurrValue(player->m_cash);
    m_credits->SetCurrValue(player->m_credits);
}

void CMainItemScreen::HandleStatsButtonPressed()
{
    if (m_weapon != NULL) {
        ShowBuyWeaponWindow(m_weapon->GetId());
    }
}

void CMainItemScreen::SetCurrentItem(int id)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    m_item = id;
    m_weapon = app->InitWeaponManager()->GetWeaponById(id);
    XString name;
    XString stats;
    ICRenderSurface* image = NULL;
    if (m_weapon != NULL) {
        name = m_weapon->GetName();
        stats = m_weapon->GetStatsString();
        image = m_weapon->getShopImage();
        if (image == NULL) {
            image = m_weapon->getMenuImage();
        }
    }
    m_weaponName->SetText(name);
    m_stats->SetText(stats);
    m_weaponImage->SetImage(image);
    CreateItemButtons();
}

void CMainItemScreen::SetCurrentItemToSlot()
{
    if (m_type == 0 && m_weapon != NULL && m_weapon->isAvailable()) {
        App* app = static_cast<App*>(WindowApp::m_instance);
        app->m_helper->m_gamePlayManager.SetWeapon(m_slot, m_weapon);
    }
}

void CMainItemScreen::CreateItemButtons()
{
    if (m_itemButtons != NULL) {
        m_itemButtons->Close();
    }
    m_silencer = NULL;
    m_hitechVisor = NULL;
    m_cyberScope = NULL;
    int margin = 10 * App::Scale();
    int y = 30 * App::Scale();
    int x = m_content->m_width * 3 / 5;
    m_itemButtons = new ScrollingContainer();
    m_itemButtons->SetLayoutType(LAYOUT_NONE);
    m_itemButtons->SetRect(x + margin, y, m_content->m_width - x - 2 * margin,
        m_content->m_height * 3 / 5 - y, false);
    m_content->AddToFront(m_itemButtons);
    if (m_weapon == NULL || CTutorialManager::IsMenuBarBlocked()) {
        return;
    }
    y = 0;
    int scale = App::Scale();
    int captionHeight = CFontMgr::GetInstance()->GetFont(14)->GetFontHeight();
    for (int index = 0; index < m_weapon->m_compatibleAmmo.size(); ++index) {
        const CAmmo* ammo = m_weapon->m_compatibleAmmo[index];
        CEquipAmmoButton* button = new CEquipAmmoButton(m_weapon, ammo);
        int height = button->GetIconHeight() + captionHeight;
        if (height < 25 * scale) {
            height = 25 * scale;
        }
        button->SetLayoutType(LAYOUT_NONE);
        button->SetRect(0, y, m_itemButtons->m_width, height, false);
        button->m_command = ChooseAmmo;
        button->m_commandParameter1 = ammo->m_id;
        m_itemButtons->AddToFront(button);
        m_ammoButtons.addElement(button);
        y += height + 2 * scale;
    }
    if (m_weapon->m_silencerAvailable && !m_weapon->m_originallySilent) {
        m_silencer = CreateSilencerButton();
        UpdateSilencerButton();
    }
    m_hitechVisor = CreateHitechVisorButton();
    m_cyberScope = CreateCyberScope();
    UpdateHitechVisorButton();
    UpdateCyberScopeButton();
    CEquipItemButton* accessories[] = { m_silencer, m_hitechVisor, m_cyberScope };
    for (unsigned int index = 0; index < sizeof(accessories) / sizeof(accessories[0]); ++index) {
        CEquipItemButton* button = accessories[index];
        if (button != NULL) {
            int height = button->GetIconHeight() + captionHeight;
            if (height < 25 * scale) {
                height = 25 * scale;
            }
            button->SetLayoutType(LAYOUT_NONE);
            button->SetRect(0, y, m_itemButtons->m_width, height, false);
            m_itemButtons->AddToFront(button);
            y += height + 2 * scale;
        }
    }
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
    if (m_silencer != NULL) {
        int count = CBH_Player::GetInstance()->m_silentShotsCount;
        UpdateEquipButton(m_silencer, count > 0, count > 0 && m_weapon->m_silencerActive);
        m_silencer->SetItemCount(count);
    }
}

void CMainItemScreen::UpdateHitechVisorButton()
{
    if (m_hitechVisor != NULL) {
        App* app = static_cast<App*>(WindowApp::m_instance);
        CItem* visor = app->InitMiscItemManager()->getItemById(HitechVisorItem);
        bool owned = visor != NULL && visor->isAvailable();
        UpdateEquipButton(m_hitechVisor, owned, owned && CBH_Player::GetInstance()->m_hitechVisorEquipped);
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
        } else if (CTutorialManager::CanCloseWeaponChoice()) {
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
        if (CTutorialManager::CanCloseWeaponChoice()) {
            SetCurrentItemToSlot();
            Close();
        } else {
            WindowApp::HandleTunnelCommand(0x6e6446f5, 0, 0, 0);
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
    if (CTutorialManager::IsPlayerInTutorial() == true && CTutorialManager::IsWeaponChoiceCloseBlink() == true) {
        m_back->SetBlinking(true);
    }
    CBaseScreen::Update();
}

void CMainItemScreen::Paint(ICGraphics2d& graphics)
{
    DrawTileImageRect(graphics, m_background.GetSurface(), 0, GetTitleBarHeight(), m_width, m_height - GetTitleBarHeight());
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
    button->m_command = ChooseAmmo;
    button->m_commandParameter1 = ammo->m_id;
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
