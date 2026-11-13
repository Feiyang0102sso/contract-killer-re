#include "CShopItemInfoWindow.h"

#include "App.h"
#include "BaseDialog.h"
#include "CAmmo.h"
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

XString GetAnalyticsName(CItem* item)
{
    return XString::Format(L"%02d_%s", item->m_id, item->GetName().c_str());
}
}

CShopItemInfoWindow::CShopItemInfoWindow(CItemManager* manager, int type, int id,
                                        const XString& stats, bool hideEquip, ESource source)
    : m_itemImage(NULL), m_extraButton(NULL), m_count(NULL), m_countIncrease(NULL),
      m_refreshQuantity(false), m_displayedCredits(0), m_buyButton(NULL), m_itemId(id),
      m_type(type), m_hideEquip(hideEquip), m_manager(manager), m_quantity(-1),
      m_lastPurchaseMs(0), m_lastWarningTime(0), m_confirmArmor(false), m_source(source)
{
    App* app = GetApp();
    CItem* item = GetItem();
    m_type = item->getType();
    m_displayedCredits = item->getPriceCredits();
    CreateBgEffects();
    SetLayoutType(LAYOUT_NONE);
    SetAlign(0);
    m_debugFrame = true;
    int width = s_width();
    SetRect(app->m_screenWidth / 2 - width / 2, s_top(), width, s_height(), true);
    SetTitle(item->GetName());

    m_redBackground.Create("SUR_SHOP_BG_RED");
    const char* imageName = item->getBigImageResId();
    if (imageName != NULL) {
        m_itemImage = app->m_surfaceManager->CreateSurface(imageName, true);
    }
    Window* imageArea = new Window(false);
    imageArea->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
    ImageWindow* background = new ImageWindow(ImageWindow::FILL_NONE, m_redBackground);
    background->SetLayoutType(LAYOUT_SIMPLE);
    background->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
    imageArea->AddToFront(background);
    if (m_itemImage != NULL) {
        ImageWindow* image = new ImageWindow();
        image->SetLayoutType(LAYOUT_SIMPLE);
        image->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
        image->SetFillType(ImageWindow::FILL_NONE);
        image->SetImage(m_itemImage);
        imageArea->AddToFront(image);
    }
    AddToFront(imageArea, 0, 1);

    int scale = App::Scale();
    SetInsetSpacing(0, scale * 10, scale * 2, scale * 10);
    CImageButton* back = CreateButton("IDB_SHOP_BACK_BUTTON", ResString("IDS_MENU_BACK"),
                                      ALIGN_LEFT | ALIGN_BOTTOM, Back);
    back->SetOutsetSpacing(0, 0, 0, SScreenAdaptingConstant(12, 25)());
    if (item->m_canEquipInSafehouse && !m_hideEquip) {
        SetExtraButton("IDB_SHOP_BACK_BUTTON", XString("EQUIP"), OpenEquipment);
    }
    if (CanShowBuyButton()) {
        const char* caption = "IDS_BTN_BUY";
        if (m_type == 4) {
            caption = "IDS_BTN_BUY_USE";
        }
        int command = Buy;
        if (type == 5) {
            command = BuyIAP;
        }
        m_buyButton = CreateButton("IDB_SHOP_MONEY_BUTTON", ResString(caption), ALIGN_RIGHT | ALIGN_BOTTOM, command);
        m_buyButton->SetOutsetSpacing(0, SScreenAdaptingConstant(12, 25)(), 0, 0);
        LabelWithIcon* price = CreatePriceLabel();
        price->SetLayoutType(LAYOUT_SIMPLE);
        price->SetAlign(ALIGN_HCENTER | ALIGN_BOTTOM);
        price->SetWidthByContent(0, 0);
        price->SetHeightByContent(0, 0);
        price->SetOutsetSpacing(0, 0, scale * 12, 0);
        AddToFront(price);
    } else if (CanShowEquipButton()) {
        CImageButton* equip = CreateButton("IDB_SHOP_BACK_BUTTON", ResString("IDS_EQUIP"),
                                          ALIGN_RIGHT | ALIGN_BOTTOM, Equip);
        equip->SetOutsetSpacing(0, SScreenAdaptingConstant(12, 25)(), 0, 0);
    } else if (m_type == 12 && m_itemId == HitechVisorId && item->isAvailable()
               && !CBH_Player::GetInstance()->m_hitechVisorEquipped && !m_hideEquip) {
        CreateButton("IDB_SHOP_BACK_BUTTON", ResString("IDS_EQUIP"), ALIGN_RIGHT | ALIGN_BOTTOM, Equip);
    }

    int imageHeight = ImageHeight(m_itemImage);
    bool hasStats = stats.Length() != 0;
    if (hasStats) {
        Window* area = CreateTextArea(stats, m_type);
        area->SetPercentWidth(45, 0, 0);
        area->SetPercentHeight(80, 0, 0);
        area->SetAlign(ALIGN_LEFT | ALIGN_TOP);
        area->SetOutsetSpacing(imageHeight + scale * 30, 0, 0, 0);
        AddToFront(area);
    }
    XString description = item->GetDescription();
    if (item->m_unlockLevel > CBH_Player::GetInstance()->GetLevel()) {
        XString format = ResString("IDS_ITEM_UNLOCK_LEVEL");
        description.Append(XString(L"\n"));
        description.Append(XString::Format(format.c_str(), item->m_unlockLevel));
    }
    Window* area = CreateTextArea(description, m_type);
    int descriptionWidth = 62;
    if (hasStats) {
        descriptionWidth = 55;
    }
    area->SetPercentWidth(descriptionWidth, 0, 0);
    area->SetPercentHeight(70, 0, 0);
    area->SetAlign(ALIGN_RIGHT | ALIGN_TOP);
    area->SetOutsetSpacing(imageHeight + scale * SScreenAdaptingConstant(40, 40, 40, 60)(), 0, 0, 0);
    AddToFront(area);
    UpdateAmmoLabel();
    app->m_soundManager->PlaySound("IDM_MENU_WINDOW_OPEN_CLOSE", false, false);
    XString analyticsName = GetAnalyticsName(item);
    if (m_displayedCredits > 0) {
        CGameAnalytics::logItemViewed(analyticsName, m_displayedCredits, CGameAnalytics::CURRENCY_HARD);
    }
    if (item->m_pricePoints > 0) {
        CGameAnalytics::logItemViewed(analyticsName, item->m_pricePoints, CGameAnalytics::CURRENCY_SOFT);
    }
}

CShopItemInfoWindow::~CShopItemInfoWindow()
{
    if (m_itemImage != NULL) {
        GetApp()->m_surfaceManager->ReleaseSurface(m_itemImage);
    }
}

void CShopItemInfoWindow::Paint(ICGraphics2d&)
{
}

CItem* CShopItemInfoWindow::GetItem() const
{
    return m_manager->getItemById(m_itemId);
}

CImageButton* CShopItemInfoWindow::CreateButton(const char* image, const XString& text, int align, int command)
{
    CImageButton* button = new CImageButton();
    button->AddImage(image, 0, 0);
    button->SetText(text, ALIGN_HCENTER | ALIGN_TOP, CFontMgr::GetInstance()->GetFont(15), App::Scale() * 5, 0);
    button->SetLayoutType(LAYOUT_SIMPLE);
    button->SetAlign(align);
    button->m_command = command;
    AddToFront(button);
    return button;
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
    CItem* item = GetItem();
    return !item->isAvailable() && item->m_unlockLevel <= CBH_Player::GetInstance()->GetLevel();
}

bool CShopItemInfoWindow::CanShowEquipButton()
{
    if (static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial() || m_type != 0 || m_hideEquip || !GetItem()->isAvailable()) {
        return false;
    }
    CDH_BasicGameData* data = GetApp()->m_helper->GetGameData();
    const CDH_Weapon* first = data->GetWeapon01(true);
    const CDH_Weapon* second = data->GetWeapon02(true);
    return (first == NULL || first->m_id != m_itemId) && (second == NULL || second->m_id != m_itemId);
}

Window* CShopItemInfoWindow::CreateTextArea(const XString& text, int)
{
    Window* area = new Window(false);
    ScrollingContainer* content = new ScrollingContainer();
    content->SetPercentWidth(100, 0, 0);
    content->SetPercentHeight(100, 0, 0);
    content->SetOutsetSpacing(10, 10, 10, 10);
    area->AddToFront(content, 0, 0);
    TextWindow* label = new TextWindow(text, CFontMgr::GetInstance()->GetFont(15));
    label->SetPercentWidth(100, 0, 0);
    label->SetLayoutType(LAYOUT_NONE);
    content->AddToFront(label);
    ScrollingContainer::VertScroller* scroller = new ScrollingContainer::VertScroller(content);
    scroller->SetOutsetSpacing(3, 5, 3, 4);
    area->AddToFront(scroller, 1, 0);
    return area;
}

LabelWithIcon* CShopItemInfoWindow::CreatePriceLabel()
{
    CItem* item = GetItem();
    int price = item->getPriceCredits();
    const char* icon = "IDB_HUD_ICON_CREDITS";
    if (price < 1) {
        price = item->m_pricePoints;
        icon = "IDB_HUD_ICON_MONEY";
    }
    return new LabelWithIcon(icon, DGHelper::FormatMoneyString(price), false,
                             CFontMgr::GetInstance()->GetFont(15), 0);
}

void CShopItemInfoWindow::UpdateAmmoLabel()
{
    CBH_Player* player = CBH_Player::GetInstance();
    int quantity;
    const char* icon = NULL;
    switch (m_type) {
    case 2:
        quantity = player->GetAvailableAmmo(static_cast<CAmmo*>(GetItem()), true);
        icon = "IDB_HUD_HANDGUN_BULLET_ICON";
        break;
    case 3:
        quantity = player->m_medkits;
        break;
    case 9:
        quantity = m_manager->getAmount(m_itemId);
        icon = "IDB_HUD_HANDGUN_BULLET_ICON";
        break;
    case 10:
        if (static_cast<CGoPill*>(GetItem())->m_pills < 1) {
            m_refreshQuantity = false;
            return;
        }
        quantity = player->GetPillsCount();
        break;
    case 12:
        switch (m_itemId) {
        case 0:
        case 5:
        case 6:
            quantity = player->m_silentShotsCount;
            break;
        case 1:
            quantity = player->m_reviveCount;
            break;
        case 3:
            quantity = player->m_ultrasoundAttemptCount;
            break;
        case 4:
            quantity = player->m_cyberScopesCount;
            break;
        default:
            m_refreshQuantity = false;
            return;
        }
        break;
    default:
        m_refreshQuantity = false;
        return;
    }
    UpdateAmmoLabel(quantity, icon);
}

void CShopItemInfoWindow::UpdateAmmoLabel(int quantity, const char* icon)
{
    m_refreshQuantity = false;
    if (m_count != NULL) {
        m_count->Close();
    }
    CFont* font = CFontMgr::GetInstance()->GetFont(9);
    int top = SScreenAdaptingConstant(30, 60)() + ImageHeight(m_itemImage) / 2;
    int right = SScreenAdaptingConstant(50, 100)();
    m_count = new AnimatedWindow();
    m_count->AddToFront(new LabelWithIcon("IDB_HUD_HANDGUN_BULLET_ICON", XString(quantity), true, font, 0));
    m_count->SetAlign(ALIGN_RIGHT | ALIGN_TOP);
    m_count->SetOutsetSpacing(top, right, 0, 0);
    AddToFront(m_count);
    if (m_countIncrease == NULL && m_quantity >= 0) {
        m_countIncrease = new AnimatedWindow();
        XString delta = XString::Format(L"+%d", quantity - m_quantity);
        m_countIncrease->AddToFront(new LabelWithIcon(icon, delta, true, font, 0));
        m_countIncrease->SetAlign(ALIGN_RIGHT | ALIGN_TOP);
        m_countIncrease->SetOutsetSpacing(top, right, 0, 0);
        m_countIncrease->SetStartEasy(0, -35 * App::Scale(), 0.3f, 1.0f, 0.2f, 0.0f, (EasyLib::EMethod)0);
        AddToFront(m_countIncrease);
    }
    m_quantity = quantity;
}

void CShopItemInfoWindow::HandleRedurectToMainItemScreen()
{
    Window* layer = GetApp()->m_mainLayer;
    CBH_NetworkWindow::GetInstance()->BringToFrontModal(layer);
    layer->AddModal(new CMainItemScreen(0, 0));
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
        warning->SetButtonsCommands(OpenBank, OpenTapjoy, -1);
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
    App* app = static_cast<App*>(WindowApp::m_instance);
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
        app->m_mainLayer->AddToFront(shop);
        event.Clear();
        Close();
        return;
    }
    case Equip:
        if (app->m_tutorialManager->IsPlayerInTutorial()) {
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
        if (m_type == 11 && CBH_Player::GetInstance()->m_armorDamageProof > 0 && !m_confirmArmor) {
            ShowArmorDialog();
            event.Clear();
            return;
        }
        if (m_type == 4 && CBH_Player::GetInstance()->m_energy >= CBH_Player::GetInstance()->GetMaxEnergy()) {
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
        app->InitDailyDealManager()->ItemPurchased(item);
        if (m_source == SOURCE_RANDOM_WEAPON_MISSION) {
            int itemId = m_itemId;
            XString name = XString::Format(L"%02d_%s", itemId, m_manager->getItemById(itemId)->m_name.c_str());
            CGameAnalytics::logRandomWeaponMissionPurchased(name, true);
        }
        int refresh = 0x1e7d8d93;
        if (m_type == 0) {
            CBH_Player* player = CBH_Player::GetInstance();
            CDH_Weapon* weapon = static_cast<CDH_Weapon*>(app->InitWeaponManager()->getItemById(m_itemId));
            const CAmmo* ammo = weapon->GetActiveAmmo();
            CDH_Weapon* bought = static_cast<CDH_Weapon*>(app->InitWeaponManager()->getItemById(m_itemId));
            player->IncAmmoByBulletsCount(ammo, bought->m_clipSize);
            CBH_Player::GetInstance()->m_gameStats.AddWeaponsEarned(1);
            WindowApp::HandleTunnelCommand(-0x65bedd6e, m_itemId, 0, 0);
            // Original: closes here and once more below (weapons are not repeatable).
            if (!app->m_tutorialManager->IsPlayerInTutorial() && CanShowEquipButton() == true) {
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
            if (app->m_tutorialManager->IsPlayerInTutorial() != true) {
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
    if (m_itemImage != NULL) {
        GetApp()->m_surfaceManager->ReleaseSurface(m_itemImage);
        m_itemImage = NULL;
    }
    WindowApp::HandleTunnelCommand(-0x777f2883, 0, 0, 0);
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
