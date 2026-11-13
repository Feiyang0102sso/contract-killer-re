#include "CShopItemInfoWindow.h"

#include "App.h"
#include "BaseDialog.h"
#include "CAmmo.h"
#include "CAmmoManager.h"
#include "CAmmoPackManager.h"
#include "CBH_NetworkWindow.h"
#include "CBH_Player.h"
#include "CBH_SurfaceManager.h"
#include "CDH_BasicGameData.h"
#include "CDH_SoundManager.h"
#include "CDH_Weapon.h"
#include "CDailyDealManager.h"
#include "CDialogWindow.h"
#include "CEquipWeaponWindow.h"
#include "CGameAnalytics.h"
#include "CGoPill.h"
#include "CGoPillManager.h"
#include "CImageButton.h"
#include "CIAPCurrency.h"
#include "CMainItemScreen.h"
#include "CRegularShop.h"
#include "CShopItem.h"
#include "CStdUtil_Cocoa.h"
#include "CTutorialManager.h"
#include "DGHelper.h"
#include "SScreenAdaptingConstant.h"
#include "ServiceWindows.h"
#include "TapjoyInterface.h"
#include "CDH_WeaponsManager.h"

// Global event id of the original (defined outside this unit).
extern const int EventId_UpdateItemLabel;

namespace {
const int Back = 0x5f523cd9;
const int BackAlternate = -0x68dfb87c;
const int Buy = -0x37ec9734;
const int BuyIAP = 0x0252dd40;
const int ConfirmArmor = -0x724c8619;
const int Equip = 0x595a14a0;
const int OpenEquipment = -0x1d2d9593;
const int OpenBank = 0x02e7d3e7;
const int OpenBankAlternate = 0x15999961;
const int OpenTapjoy = 0x02e7d44c;
const int IgnoreLowMoney = 0x0257853c;
const int HitechVisorId = 2;
const unsigned int PurchaseDebounceMs = 300;
const unsigned int QuantityRefreshMs = 201;

SScreenAdaptingConstant s_width(375, 750);
SScreenAdaptingConstant s_height(295, 580, 580, 450);
SScreenAdaptingConstant s_top(20, 40, 40, 10);

App* GetApp()
{
    return static_cast<App*>(WindowApp::m_instance);
}

}

CShopItemInfoWindow::CShopItemInfoWindow(CItemManager* manager, int type, int id,
                                        const XString& stats, bool hideEquip, ESource source)
    : m_extraButton(NULL), m_lastWarningTime(0), m_confirmArmor(false)
{
    CreateBgEffects();
    m_itemId = id;
    m_manager = manager;
    m_type = manager->getItemById(id)->getType();
    m_hideEquip = hideEquip;
    m_buyButton = NULL;
    m_count = NULL;
    m_countIncrease = NULL;
    m_refreshQuantity = false;
    m_lastPurchaseMs = 0;
    m_quantity = -1;
    m_source = source;
    m_debugFrame = true;
    SetLayoutType(LAYOUT_NONE);
    SetAlign(0);

    int screenWidth = GetApp()->m_screenWidth;
    int x = screenWidth / 2 - s_width() / 2;
    int y = s_top();
    int width = s_width();
    int height = s_height();
    SetRect(x, y, width, height, true);
    m_redBackground.Create("SUR_SHOP_BG_RED");
    CItem* item = manager->getItemById(id);
    CBH_SurfaceManager* surfaces = GetApp()->m_surfaceManager;
    m_itemImage = surfaces->CreateSurface(item->getBigImageResId(), true);
    XString title = item->m_name;
    SetTitle(title);

    Window* imageArea = new Window(false);
    imageArea->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
    ImageWindow* background = new ImageWindow();
    background->SetLayoutType(LAYOUT_SIMPLE);
    background->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
    background->SetFillType(ImageWindow::FILL_NONE);
    background->SetImage(m_redBackground);
    imageArea->AddToFront(background);
    ImageWindow* image = new ImageWindow();
    image->SetLayoutType(LAYOUT_SIMPLE);
    image->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
    image->SetFillType(ImageWindow::FILL_NONE);
    image->SetImage(m_itemImage);
    imageArea->AddToFront(image);
    AddToFront(imageArea, 0, 1);

    int rightInset = App::Scale() * 10;
    int bottomInset = App::Scale() * 2;
    int leftInset = App::Scale() * 10;
    SetInsetSpacing(0, rightInset, bottomInset, leftInset);
    CFont* font = CFontMgr::GetInstance()->GetFont(15);
    CImageButton* back = new CImageButton();
    back->AddImage("IDB_SHOP_BACK_BUTTON", 0, 0);
    back->SetText(ResString("IDS_MENU_BACK"), ALIGN_HCENTER | ALIGN_TOP, font, App::Scale() * 5, 0);
    back->SetLayoutType(LAYOUT_SIMPLE);
    back->SetAlign(ALIGN_LEFT | ALIGN_BOTTOM);
    back->SetOutsetSpacing(0, 0, 0, SScreenAdaptingConstant(12, 25)());
    back->SetCommandParameters(Back, 0, 0, 0);
    AddToFront(back);
    if (item->m_canEquipInSafehouse && !m_hideEquip) {
        SetExtraButton("IDB_SHOP_BACK_BUTTON", XString("EQUIP"), OpenEquipment);
    }
    if (CanShowBuyButton()) {
        m_buyButton = new CImageButton();
        m_buyButton->AddImage("IDB_SHOP_MONEY_BUTTON", 0, 0);
        XString caption = ResString("IDS_BTN_BUY");
        if (m_type == 4) {
            caption.Assign(ResString("IDS_BTN_BUY_USE"));
        }
        m_buyButton->SetText(caption, ALIGN_HCENTER | ALIGN_TOP, font, App::Scale() * 5, 0);
        m_buyButton->SetLayoutType(LAYOUT_SIMPLE);
        m_buyButton->SetAlign(ALIGN_RIGHT | ALIGN_BOTTOM);
        int command = Buy;
        if (type == 5) {
            command = BuyIAP;
        }
        m_buyButton->SetCommandParameters(command, 0, 0, 0);
        m_buyButton->SetOutsetSpacing(0, SScreenAdaptingConstant(12, 25)(), 0, 0);
        AddToFront(m_buyButton);
    } else if (CanShowEquipButton()) {
        CImageButton* equip = new CImageButton();
        equip->AddImage("IDB_SHOP_BACK_BUTTON", 0, 0);
        equip->SetText(ResString("IDS_EQUIP"), ALIGN_HCENTER | ALIGN_TOP, font, App::Scale() * 5, 0);
        equip->SetLayoutType(LAYOUT_SIMPLE);
        equip->SetAlign(ALIGN_RIGHT | ALIGN_BOTTOM);
        equip->SetCommandParameters(Equip, 0, 0, 0);
        equip->SetOutsetSpacing(0, SScreenAdaptingConstant(12, 25)(), 0, 0);
        AddToFront(equip);
    }

    if (m_type != 0) {
        Window* area = CreateTextArea(stats, m_type);
        area->SetPercentWidth(45, 0, 0);
        area->SetPercentHeight(80, 0, 0);
        area->SetAlign(ALIGN_LEFT | ALIGN_TOP);
        int imageHeight = ImageHeight(m_itemImage);
        area->SetOutsetSpacing(imageHeight + App::Scale() * 30, 0, 0, 0);
        AddToFront(area);
    }
    XString description = item->m_description;
    Window* area = CreateTextArea(description, m_type);
    area->SetPercentWidth(55, 0, 0);
    area->SetPercentHeight(70, 0, 0);
    area->SetAlign(ALIGN_RIGHT | ALIGN_TOP);
    int imageHeight = ImageHeight(m_itemImage);
    int textTop = SScreenAdaptingConstant(40, 40, 40, 60)();
    area->SetOutsetSpacing(imageHeight + App::Scale() * textTop, 0, 0, 0);
    if (stats.ByteSize() == 0) {
        area->SetPercentWidth(62, 0, 0);
    }
    AddToFront(area);

    switch (m_type) {
    case 0: {
        if (!item->isAvailable()) {
            Window* priceArea = new Window(false);
            Window* priceRow = new Window(false);
            LabelWithIcon* price = CreatePriceLabel();
            price->SetHeightByContent(0, 0);
            price->SetWidthByContent(0, 0);
            price->SetLayoutType(LAYOUT_SIMPLE);
            price->SetAlign(ALIGN_HCENTER | ALIGN_BOTTOM);
            priceRow->AddToFront(price);
            priceArea->AddToFront(priceRow, 1, 0);
            priceArea->SetPercentHeight(50, 0, 0);
            AddToFront(priceArea);
        }
        CDH_Weapon* weapon = static_cast<CDH_Weapon*>(manager->getItemById(id));
        Window* weaponStats = CRegularShop::CreateWeaponStats(weapon);
        weaponStats->SetPercentWidth(50, 0, 0);
        weaponStats->SetPercentHeight(80, 0, 0);
        weaponStats->SetAlign(ALIGN_LEFT | ALIGN_TOP);
        int weaponImageHeight = ImageHeight(m_itemImage);
        int scale = App::Scale();
        int left = SScreenAdaptingConstant(-12, -25)();
        weaponStats->SetOutsetSpacing(weaponImageHeight + scale * 30, 0, 0, left);
        AddToFront(weaponStats);
        break;
    }
    case 2:
    case 3:
    case 9:
    case 10:
        UpdateAmmoLabel();
        break;
    case 12:
        if (m_itemId != HitechVisorId) {
            UpdateAmmoLabel();
        }
        break;
    }

    GetApp()->m_soundManager->PlaySound("IDM_MENU_WINDOW_OPEN_CLOSE", false, false);
    int analyticsId = m_itemId;
    XString analyticsName = XString::Format(L"%02d_%s", analyticsId, GetItem()->m_name.c_str());
    if (GetItem()->getPriceCredits() > 0) {
        CGameAnalytics::logItemViewed(analyticsName, GetItem()->getPriceCredits(), CGameAnalytics::CURRENCY_HARD);
    }
    if (GetItem()->m_pricePoints > 0) {
        CGameAnalytics::logItemViewed(analyticsName, GetItem()->m_pricePoints, CGameAnalytics::CURRENCY_SOFT);
    }
}

CShopItemInfoWindow::~CShopItemInfoWindow()
{
}

void CShopItemInfoWindow::Paint(ICGraphics2d&)
{
}

CItem* CShopItemInfoWindow::GetItem() const
{
    return m_manager->getItemById(m_itemId);
}



void CShopItemInfoWindow::SetExtraButton(const char* image, const XString& text, unsigned int command)
{
    if (m_extraButton != NULL) {
        m_extraButton->Close();
    }
    m_extraButton = new CImageButton();
    m_extraButton->AddImage(image, 0, 0);
    m_extraButton->SetText(text, ALIGN_HCENTER | ALIGN_TOP, CFontMgr::GetInstance()->GetFont(15), App::Scale() * 5, 0);
    m_extraButton->SetLayoutType(LAYOUT_SIMPLE);
    m_extraButton->SetAlign(ALIGN_HCENTER | ALIGN_BOTTOM);
    m_extraButton->SetCommandParameters(command, 0, 0, 0);
    AddToFront(m_extraButton);
}

bool CShopItemInfoWindow::CanShowBuyButton()
{
    bool available = GetItem()->isAvailable();
    int level = CBH_Player::GetInstance()->GetLevel();
    bool unlocked = level >= GetItem()->m_unlockLevel;
    bool canBuy = !available & unlocked;
    return canBuy;
}

bool CShopItemInfoWindow::CanShowEquipButton()
{
    if (GetApp()->m_tutorialManager->IsPlayerInTutorial()) {
        return false;
    }
    int type = GetItem()->getType();
    bool available = GetItem()->isAvailable();
    const CDH_Weapon* first = GetApp()->m_helper->GetGameData()->GetWeapon01(true);
    if (first->m_id == m_itemId) {
        return false;
    }
    const CDH_Weapon* second = GetApp()->m_helper->GetGameData()->GetWeapon02(true);
    if (type != 0 || !available) {
        return false;
    }
    int secondId = second->m_id;
    if (secondId == m_itemId) {
        return false;
    }
    return !m_hideEquip;
}

Window* CShopItemInfoWindow::CreateTextArea(const XString& text, int)
{
    Window* area = new Window(false);
    ScrollingContainer* content = new ScrollingContainer();
    content->SetPercentWidth(100, 0, 0);
    content->SetPercentHeight(100, 0, 0);
    content->SetCellPos(0, 0, 1, 1);
    content->SetOutsetSpacing(10, 10, 10, 10);
    TextWindow* label = new TextWindow(text, CFontMgr::GetInstance()->GetFont(15));
    label->SetPercentWidth(100, 0, 0);
    label->SetLayoutType(LAYOUT_NONE);
    content->AddToFront(label);
    area->AddToFront(content);
    ScrollingContainer::VertScroller* scroller = new ScrollingContainer::VertScroller(content);
    scroller->SetCellPos(1, 0, 1, 1);
    scroller->SetOutsetSpacing(3, 5, 3, 4);
    area->AddToFront(scroller);
    return area;
}

LabelWithIcon* CShopItemInfoWindow::CreatePriceLabel()
{
    CItem* item = GetItem();
    int price;
    const char* icon;
    if (item->getPriceCredits() >= 1) {
        price = item->getPriceCredits();
        icon = "IDB_HUD_ICON_CREDITS";
    } else {
        price = item->m_pricePoints;
        icon = "IDB_HUD_ICON_MONEY";
    }
    XString text = DGHelper::FormatMoneyString(price);
    return new LabelWithIcon(icon, text, false, CFontMgr::GetInstance()->GetFont(15), 0);
}

void CShopItemInfoWindow::UpdateAmmoLabel()
{
    int quantity;
    switch (m_type) {
    case 2: {
        CBH_Player* player = CBH_Player::GetInstance();
        CAmmo* ammo = static_cast<CAmmo*>(GetApp()->InitAmmoManager()->getItemById(m_itemId));
        quantity = player->GetAvailableAmmo(ammo, true);
        UpdateAmmoLabel(quantity, "IDB_HUD_HANDGUN_BULLET_ICON");
        return;
    }
    case 9:
        quantity = GetApp()->m_ammoPackManager->getAmount(m_itemId);
        UpdateAmmoLabel(quantity, "IDB_HUD_HANDGUN_BULLET_ICON");
        return;
    case 12:
        switch (m_itemId) {
        case 0:
        case 5:
        case 6:
            quantity = CBH_Player::GetInstance()->m_silentShotsCount;
            break;
        case 1:
            quantity = CBH_Player::GetInstance()->m_reviveCount;
            break;
        case 3:
            quantity = CBH_Player::GetInstance()->m_ultrasoundAttemptCount;
            break;
        case 4:
            quantity = CBH_Player::GetInstance()->m_cyberScopesCount;
            break;
        default:
            return;
        }
        break;
    case 10: {
        CGoPill* pill = GetApp()->m_goPillManager->GetPillById(m_itemId);
        if (pill == NULL || pill->m_pills < 1) {
            return;
        }
        quantity = CBH_Player::GetInstance()->GetPillsCount();
        break;
    }
    case 3:
        quantity = CBH_Player::GetInstance()->m_medkits;
        break;
    default:
        return;
    }
    UpdateAmmoLabel(quantity, NULL);
}

void CShopItemInfoWindow::UpdateAmmoLabel(int quantity, const char* icon)
{
    m_refreshQuantity = false;
    if (m_count != NULL) {
        m_count->Close();
    }
    m_count = new AnimatedWindow();
    CFont* font = CFontMgr::GetInstance()->GetFont(9);
    LabelWithIcon* label = new LabelWithIcon("IDB_HUD_HANDGUN_BULLET_ICON", XString(quantity), true, font, 0);
    m_count->AddToFront(label);
    m_count->SetAlign(ALIGN_RIGHT | ALIGN_TOP);
    Window* count = m_count;
    int height = ImageHeight(m_itemImage);
    int top = SScreenAdaptingConstant(30, 60)();
    int right = SScreenAdaptingConstant(50, 100)();
    count->SetOutsetSpacing(top + height / 2, right, 0, 0);
    AddToFront(m_count);

    if (m_countIncrease == NULL && m_quantity >= 0) {
        m_countIncrease = new AnimatedWindow();
        CFont* deltaFont = CFontMgr::GetInstance()->GetFont(9);
        LabelWithIcon* delta = new LabelWithIcon(icon, XString("+") + XString(quantity - m_quantity),
                                                 true, deltaFont, 0);
        m_countIncrease->AddToFront(delta);
        m_countIncrease->SetAlign(ALIGN_RIGHT | ALIGN_TOP);
        Window* countIncrease = m_countIncrease;
        int imageHeight = ImageHeight(m_itemImage);
        int topInset = SScreenAdaptingConstant(30, 60)();
        int rightInset = SScreenAdaptingConstant(50, 100)();
        countIncrease->SetOutsetSpacing(topInset + imageHeight / 2, rightInset, 0, 0);
        m_countIncrease->SetStartEasy(0, -35 * App::Scale(), 0.3f, 1.0f, 0.2f, 0.0f, static_cast<EasyLib::EMethod>(0));
        AddToFront(m_countIncrease);
    }
    m_quantity = quantity;
}

void CShopItemInfoWindow::HandleRedurectToMainItemScreen()
{
    CBH_NetworkWindow* safehouse = CBH_NetworkWindow::GetInstance();
    safehouse->BringToFrontModal(GetApp()->m_mainLayer);
    CMainItemScreen* items = new CMainItemScreen(0, 0);
    GetApp()->m_mainLayer->AddModal(items);
    Close();
}

void CShopItemInfoWindow::ShowEquipWindow(int unused)
{
    CEquipWeaponWindow* equip = new CEquipWeaponWindow(m_itemId);
    equip->SetPercentHeight(100, 0, 0);
    equip->SetPercentWidth(80, 0, 0);
    equip->SetLayoutType(LAYOUT_SIMPLE);
    equip->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    m_parent->AddModal(equip);
}

void CShopItemInfoWindow::ShowArmorDialog()
{
    unsigned int now = App::GetTime();
    if (now != m_lastWarningTime) {
        m_lastWarningTime = now;
        CTextDialog* warning = new CTextDialog();
        warning->SetButtons(CDialogWindow::BUTTON_OK, CDialogWindow::BUTTON_IGNORE);
        warning->SetMessageText(ResString("IDS_DIALOG_HAVE_ALREADY_HAD_ARMOR"));
        warning->SetTitleText("IDS_DIALOG_TITLE_WARNING");
        warning->SetButtonsCommands(ConfirmArmor, -1, -1);
        GetApp()->m_mainLayer->AddModal(warning);
    }
}

void CShopItemInfoWindow::ShowFullEnergyDialog()
{
    unsigned int now = App::GetTime();
    if (now != m_lastWarningTime) {
        m_lastWarningTime = now;
        CDialogWindow* warning = new CDialogWindow(true);
        warning->SetButtons(CDialogWindow::BUTTON_OK);
        warning->SetMessageText(ResString("IDS_MENU_FULL_ENERGY_DIALOG"));
        warning->SetTitleText("IDS_DIALOG_TITLE_WARNING");
        GetApp()->m_mainLayer->AddModal(warning);
    }
}

void CShopItemInfoWindow::ShowLowMoneyDialog()
{
    unsigned int now = App::GetTime();
    if (now != m_lastWarningTime) {
        m_lastWarningTime = now;
        CDialogWindow* warning = new CDialogWindow(true);
        warning->SetButtons(CDialogWindow::BUTTON_BUY, CDialogWindow::BUTTON_GET, CDialogWindow::BUTTON_IGNORE);
        warning->SetMessageText(ResString("IDS_MENU_LOW_MONEY_DIALOG"));
        warning->SetTitleText("IDS_DIALOG_TITLE_WARNING");
        warning->SetButtonsCommands(OpenBank, OpenTapjoy, IgnoreLowMoney);
        GetApp()->m_mainLayer->AddModal(warning);
    }
}

bool CShopItemInfoWindow::purchaseForPoints()
{
    XString analyticsName = XString::Format(L"%02d_%s", m_itemId, m_manager->getItemById(m_itemId)->GetName().c_str());
    int price = m_manager->getItemById(m_itemId)->m_pricePoints;
    CGameAnalytics::logBuyItemClicked(analyticsName, price, CGameAnalytics::CURRENCY_SOFT);
    bool purchased;
    if (CBH_Player::GetInstance()->m_cash >= price) {
        purchased = true;
        m_manager->getItemById(m_itemId)->setAvailable(true);
        CBH_Player::GetInstance()->ReduceMoney(price);
        CGameAnalytics::logCurrencyChange(analyticsName, -price, CGameAnalytics::CURRENCY_SOFT);
        GetApp()->m_soundManager->PlaySound("IDM_MENU_PURCHASE_COMPLETE", false, false);
        WindowApp::HandleTunnelCommand(EventId_UpdateItemLabel, m_itemId, 0, 0);
        CGameAnalytics::logItemPurchased(analyticsName, price, CGameAnalytics::CURRENCY_SOFT);
    } else {
        ShowLowMoneyDialog();
        purchased = false;
    }
    return purchased;
}

bool CShopItemInfoWindow::purchaseForCredits()
{
    XString analyticsName = XString::Format(L"%02d_%s", m_itemId, m_manager->getItemById(m_itemId)->GetName().c_str());
    int price = m_manager->getItemById(m_itemId)->getPriceCredits();
    CGameAnalytics::logBuyItemClicked(analyticsName, price, CGameAnalytics::CURRENCY_HARD);
    bool purchased;
    if (CBH_Player::GetInstance()->m_credits >= price) {
        purchased = true;
        m_manager->getItemById(m_itemId)->setAvailable(true);
        CBH_Player::GetInstance()->ReduceCredits(price);
        CGameAnalytics::logCurrencyChange(analyticsName, -price, CGameAnalytics::CURRENCY_HARD);
        GetApp()->m_soundManager->PlaySound("IDM_MENU_PURCHASE_COMPLETE", false, false);
        WindowApp::HandleTunnelCommand(EventId_UpdateItemLabel, m_itemId, 0, 0);
        CGameAnalytics::logItemPurchased(analyticsName, price, CGameAnalytics::CURRENCY_HARD);
    } else {
        ShowLowMoneyDialog();
        purchased = false;
    }
    return purchased;
}

void CShopItemInfoWindow::OnCommand(Event& event)
{
    switch (event.m_id) {
    case ConfirmArmor:
        // Re-send Buy; m_confirmArmor lets it pass the armor check (the original never resets it).
        m_confirmArmor = true;
        WindowApp::HandleTunnelCommand(Buy, 0, 0, 0);
        event.Clear();
        return;
    case BackAlternate:
    case Back:
        if (m_source == SOURCE_RANDOM_WEAPON_MISSION) {
            int itemId = m_itemId;
            XString name = XString::Format(L"%02d_%s", itemId, m_manager->getItemById(itemId)->m_name.c_str());
            CGameAnalytics::logRandomWeaponMissionPurchased(name, false);
        }
        break;
    case OpenEquipment:
        HandleRedurectToMainItemScreen();
        event.Clear();
        return;
    case OpenTapjoy:
        if (TapjoyInterface::canDisplayInterface()) {
            TapjoyInterface::openTapjoyInterface();
        } else {
            CDialogWindow* warning = new CDialogWindow(true);
            warning->SetTitleText(ResString("IDS_DIALOG_TITLE_WARNING"));
            warning->SetMessageText(ResString("IDS_DIALOG_CAN_NOT_OPEN_TAPJOY"));
            warning->SetButtons(CDialogWindow::BUTTON_OK);
            AddModal(warning);
        }
        event.Clear();
        Close();
        return;
    case OpenBank:
    case OpenBankAlternate: {
        CShopWindow* shop = CRegularShop::GetInstance(true);
        shop->SetTab(CShopWindow::TAB_BANK);
        if (shop->m_parent != NULL) {
            shop->m_parent->Remove(shop);
        }
        GetApp()->m_mainLayer->AddToFront(shop);
        event.Clear();
        Close();
        return;
    }
    case Equip:
        if (GetApp()->m_tutorialManager->IsPlayerInTutorial()) {
            return;
        }
        if (CanShowEquipButton() != true) {
            return;
        }
        ShowEquipWindow(m_itemId);
        event.Clear();
        Close();
        return;
    case Buy: {
        if (m_lastPurchaseMs != 0 && CStdUtil_Cocoa::GetUpTimeMS() - m_lastPurchaseMs < PurchaseDebounceMs) {
            return;
        }
        m_lastPurchaseMs = CStdUtil_Cocoa::GetUpTimeMS();
        int type = m_type;
        if (type == 11) {
            if (CBH_Player::GetInstance()->m_armorDamageProof > 0 && !m_confirmArmor) {
                ShowArmorDialog();
                event.Clear();
                return;
            }
            type = m_type;
        }
        if (type == 4 && CBH_Player::GetInstance()->m_energy >= CBH_Player::GetInstance()->GetMaxEnergy()) {
            ShowFullEnergyDialog();
            event.Clear();
            Close();
            return;
        }
        CItem* item = m_manager->getItemById(m_itemId);
        if (item->getPriceCredits() < 1) {
            if (purchaseForPoints() != true) {
                event.Clear();
                return;
            }
        } else {
            if (!purchaseForCredits()) {
                event.Clear();
                return;
            }
        }
        GetApp()->InitDailyDealManager()->ItemPurchased(item);
        if (m_source == SOURCE_RANDOM_WEAPON_MISSION) {
            int itemId = m_itemId;
            XString name = XString::Format(L"%02d_%s", itemId, m_manager->getItemById(itemId)->m_name.c_str());
            CGameAnalytics::logRandomWeaponMissionPurchased(name, true);
        }
        int refresh = 0x1e7d8d93;
        if (m_type == 0) {
            CBH_Player* player = CBH_Player::GetInstance();
            CDH_Weapon* weapon = static_cast<CDH_Weapon*>(GetApp()->InitWeaponManager()->getItemById(m_itemId));
            const CAmmo* ammo = weapon->GetActiveAmmo();
            CDH_Weapon* bought = static_cast<CDH_Weapon*>(GetApp()->InitWeaponManager()->getItemById(m_itemId));
            player->IncAmmoByBulletsCount(ammo, bought->m_clipSize);
            CBH_Player::GetInstance()->m_gameStats.AddWeaponsEarned(1);
            WindowApp::HandleTunnelCommand(-0x65bedd6e, m_itemId, 0, 0);
            // Original: closes here and once more below (weapons are not repeatable).
            if (!GetApp()->m_tutorialManager->IsPlayerInTutorial() && CanShowEquipButton() == true) {
                ShowEquipWindow(m_itemId);
                Close();
            }
        } else if (m_type == 2 || m_type == 9) {
            refresh = -0x44b7c071;
        } else if (m_type == 12) {
            CBH_Player::GetInstance()->m_gameStats.RegisterMiscPurchase(m_itemId);
        }
        WindowApp::HandleTunnelCommand(refresh, 0, 0, 0);
        // Consumables stay open to buy again (outside the tutorial).
        bool repeatable = m_type == 2 || m_type == 3 || m_type == 9 || m_type == 10
            || (m_type == 12 && m_itemId != HitechVisorId);
        if (repeatable) {
            m_refreshQuantity = true;
            if (GetApp()->m_tutorialManager->IsPlayerInTutorial() != true) {
                event.Clear();
                return;
            }
        }
        break;
    }
    default:
        return;
    }
    Close();
    event.Clear();
}

void CShopItemInfoWindow::OnClosed()
{
    if (m_manager != NULL && GetItem() != NULL) {
        GetApp()->m_surfaceManager->ReleaseSurface(m_itemImage);
        WindowApp::HandleTunnelCommand(-0x777f2883, 0, 0, 0);
    }
}

void CShopItemInfoWindow::Update()
{
    if (m_countIncrease != NULL && m_countIncrease->IsWaitingState()) {
        m_countIncrease->Close();
        m_countIncrease = NULL;
    }
    if (CStdUtil_Cocoa::GetUpTimeMS() - m_lastPurchaseMs >= QuantityRefreshMs && m_refreshQuantity) {
        UpdateAmmoLabel();
    }
}
