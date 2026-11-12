#include "CIngameShop.h"
#include "CAutoArrangedWindow.h"
#include "CBH_ProgressBar.h"
#include "CDH_BasicGameData.h"
#include "CIAPCurrencyManager.h"
#include "CMiscItemManager.h"
#include "CScrollWindow.h"
#include "ICInAppPurchase.h"
#include "SScreenAdaptingConstant.h"
#include "CShopItemButton.h"

#include "App.h"
#include "BaseDialog.h"
#include "CAmmo.h"
#include "CAmmoManager.h"
#include "CAmmoPack.h"
#include "CAmmoPackManager.h"
#include "CMedicantManager.h"
#include "CArmorManager.h"
#include "CBH_Player.h"
#include "CDailyDealManager.h"
#include "CDH_SoundManager.h"
#include "CDH_Weapon.h"
#include "CDialogWindow.h"
#include "CFontMgr.h"
#include "CGameAnalytics.h"
#include "CIAPCurrency.h"
#include "CImageButton.h"
#include "CShopWindow.h"
#include "CSaveManager.h"
#include "DGHelper.h"

namespace {
const int ShopFilter = 0x43a30540;
const int ShopItem = 0x43a30541;
const int ShopBuy = 0x43a30542;
const int ShopEquip = 0x43a30543;
const int ShopUnequip = 0x43a30544;
const int ShopReplaceArmor = 0x43a30545;
const int ShopClose = 0x43a30546;
const int ShopCurrency = 0x43a30547;
const int MissionShopClosed = 0x7e4d23f0;
const int CloseSoftKey = static_cast<int>(0x97204784u);
const int OpenBankTab = 0x4c2524f3;
const int UpdateItemLabel = static_cast<int>(0x81f81622u);
const int HitechVisorItem = 2;
const int CyberScopeItem = 4;

CDH_Weapon* GetActiveWeapon()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    return app->m_helper->GetGameData()->GetActiveWeapon();
}

bool IsSilencerItem(int itemId)
{
    return itemId == 0 || itemId == 5 || itemId == 6;
}

int GetEquipAction(CItem* item, CDH_Weapon* weapon, CBH_Player* player)
{
    if (item->getType() == 2 && weapon != NULL) {
        const CAmmo* ammo = static_cast<const CAmmo*>(item);
        if (weapon->GetActiveAmmo() == ammo) {
            if (ammo != weapon->GetDefaultAmmo()) {
                return ShopUnequip;
            }
        } else if (weapon->CanUseAmmo(ammo) && player->GetAvailableAmmo(ammo, true) > 0) {
            return ShopEquip;
        }
    } else if (item->getType() == 12) {
        if (item->m_id == HitechVisorItem && item->isAvailable()) {
            if (player->m_hitechVisorEquipped) {
                return ShopUnequip;
            }
            return ShopEquip;
        }
        if (weapon != NULL && IsSilencerItem(item->m_id) && weapon->m_silencerAvailable && !weapon->m_originallySilent) {
            if (weapon->m_silencerActive) {
                return ShopUnequip;
            }
            if (player->m_silentShotsCount > 0) {
                return ShopEquip;
            }
        }
        if (weapon != NULL && item->m_id == CyberScopeItem && !weapon->m_cyberScopeEquipped
            && player->m_cyberScopesCount > 0) {
            return ShopEquip;
        }
    }
    return -1;
}
}

namespace {
const int UnequipPressed = (int)0x86057e02;
const int BuyItem = (int)0x8db379e7;
const int ItemPressed = (int)0x97479671;
const int EquipPressed = (int)0xb008631b;
const int BuyPressed = (int)0xc5ae691a;
const int ClosePressed = (int)0xe6e4d350;
const int CurrencyUpdated = (int)0xff403571;
const int FilterPressed = 0x05d724e5;
const SScreenAdaptingConstant ItemHeight(220, 290, 550);
}

CIngameShop::CIngameShop()
    : Window(false), m_tab(Bank), m_buyHolder(NULL), m_buyButton(NULL), m_equipHolder(NULL), m_equipButton(NULL),
      m_currentItemButton(NULL), m_waitDialog(NULL)
{
    CreateUI();
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_timeScale = 0.0f;
    app->m_saveManager->ResumeSaving();
}

CIngameShop::~CIngameShop()
{
    static_cast<App*>(WindowApp::m_instance)->m_saveManager->PauseSaving();
}

void CIngameShop::CreateUI()
{
    ImageWindow* background = new ImageWindow();
    background->SetFillType(static_cast<ImageWindow::EFillType>(0));
    background->SetImageById("IDB_DIALOG_WINDOW_BG");
    background->SetLayoutType(LAYOUT_SIMPLE);
    background->SetAlign(0x24);
    AddToFront(background);

    Window* content = new Window(false);
    content->SetPercentHeight(80, 0, 0);
    content->SetPercentWidth(80, 0, 0);
    content->SetLayoutType(LAYOUT_SIMPLE);
    content->SetAlign(0x24);
    background->AddToFront(content);

    // Top row: balances and the tab filters.
    Window* topRow = new Window(false);
    Window* topInner = new Window(false);
    topInner->SetLayoutType(LAYOUT_SIMPLE);
    topInner->SetAlign(0x14);
    topInner->SetPercentWidth(95, 0, 0);
    topInner->SetPercentHeight(100, 0, 0);
    topRow->AddToFront(topInner);
    content->AddToFront(topRow, 0, 0);
    Window* stats = CreateStatsWindow();
    stats->SetLayoutType(LAYOUT_SIMPLE);
    stats->SetAlign(0xc);
    stats->SetPercentWidth(25, 0, 0);
    topInner->AddToFront(stats, 0, 0);
    topInner->AddToFront(CreateFilterButtons(), 1, 0);

    // Bottom row: equip and buy buttons.
    Window* buttons = new Window(false);
    buttons->m_debugFrame = true;
    background->AddToFront(buttons);
    Window* leftSpacer = new Window(false);
    buttons->AddToFront(leftSpacer, 0, 0);
    leftSpacer->SetPercentWidth(25, 0, 0);
    m_equipHolder = new Window(false);
    m_equipButton = CreateEquipButton();
    m_equipButton->SetLayoutType(LAYOUT_SIMPLE);
    m_equipButton->SetAlign(0x14);
    m_equipButton->SetOutsetSpacing(0, 0, SScreenAdaptingConstant(35, 90)(), 0);
    m_equipHolder->AddToFront(m_equipButton);
    buttons->AddToFront(m_equipHolder, 1, 0);
    m_equipHolder->SetPercentWidth(0, 0, 0);
    m_buyHolder = new Window(false);
    m_buyButton = CreateBuyButton();
    m_buyButton->SetLayoutType(LAYOUT_SIMPLE);
    m_buyButton->SetAlign(0x14);
    m_buyButton->SetOutsetSpacing(0, 0, SScreenAdaptingConstant(35, 90)(), 0);
    m_buyHolder->AddToFront(m_buyButton);
    buttons->AddToFront(m_buyHolder, 2, 0);
    m_buyHolder->SetPercentWidth(50, 0, 0);
    Window* rightSpacer = new Window(false);
    buttons->AddToFront(rightSpacer, 3, 0);
    rightSpacer->SetPercentWidth(25, 0, 0);

    // Middle: holder of the current tab page.
    Window* middle = new Window(false);
    m_pageHolder = new Window(false);
    middle->AddToFront(m_pageHolder);
    content->AddToFront(middle, 0, 1);
    middle->SetPercentHeight(80, 0, 0);

    CImageButton* close = new CImageButton();
    close->SetOutsetSpacing(SScreenAdaptingConstant(12, 24)(), SScreenAdaptingConstant(35, 70)(), 0, 0);
    close->AddImage("IDB_CLOSE_ICON", 0, 0);
    close->SetLayoutType(LAYOUT_SIMPLE);
    close->SetAlign(10);
    close->m_command = ClosePressed;
    close->m_commandParameter1 = 0;
    close->m_commandParameter2 = 0;
    close->m_commandParameter3 = 0;
    background->AddToFront(close);
    SetTab(Armor);
}

Window* CIngameShop::CreateStatsWindow()
{
    Window* stats = new Window(false);
    m_cashBar = new CBH_ProgressBar(3, SScreenAdaptingConstant(100, 200, 190)(), -1);
    m_cashBar->SetDescImage("IDB_HUD_ICON_MONEY");
    m_cashBar->m_displayFlags = 5;
    m_cashBar->SetTextAlign(TEXT_ALIGN_RIGHT);
    m_cashBar->SetCurrValue(CBH_Player::GetInstance()->m_cash);
    stats->AddToFront(m_cashBar, 0, 0);
    m_creditsBar = new CBH_ProgressBar(3, SScreenAdaptingConstant(100, 200, 190)(), -1);
    m_creditsBar->SetDescImage("IDB_HUD_ICON_CREDITS");
    m_creditsBar->m_displayFlags = 5;
    m_creditsBar->SetTextAlign(TEXT_ALIGN_RIGHT);
    m_creditsBar->SetCurrValue(CBH_Player::GetInstance()->m_credits);
    stats->AddToFront(m_creditsBar, 0, 1);
    return stats;
}

void CIngameShop::UpdateStatsWindow()
{
    m_cashBar->SetCurrValue(CBH_Player::GetInstance()->m_cash);
    m_creditsBar->SetCurrValue(CBH_Player::GetInstance()->m_credits);
}

Window* CIngameShop::CreateFilterButtons()
{
    Window* row = new Window(false);
    const char* const images[] = {
        "IDB_HUD_INGAME_SHOP_FILTER_BANK", "IDB_HUD_INGAME_SHOP_FILTER_AMMO", "IDB_HUD_INGAME_SHOP_FILTER_GO_PILLS",
        "IDB_HUD_INGAME_SHOP_FILTER_MEDKITS", "IDB_HUD_INGAME_SHOP_FILTER_ARMOR"
    };
    for (int tab = 0; tab < TabCount; ++tab) {
        Window* button = CreateFilterButton(images[tab], static_cast<EItemTabs>(tab));
        if (tab == 0) {
            button->SetLayoutType(LAYOUT_SIMPLE);
        }
        row->AddToFront(button, tab, 0);
        m_filterButtons.addElement(button);
    }
    return row;
}

CImageButton* CIngameShop::CreateEquipButton()
{
    CImageButton* button = new CImageButton();
    button->AddImage("IDB_SHOP_MONEY_BUTTON", 0, 0);
    button->SetText(ResString("IDS_EQUIP"), 0x24, NULL, 0, 0);
    button->m_command = EquipPressed;
    button->m_commandParameter1 = 0;
    button->m_commandParameter2 = 0;
    button->m_commandParameter3 = 0;
    return button;
}

CImageButton* CIngameShop::CreateBuyButton()
{
    CImageButton* button = new CImageButton();
    button->AddImage("IDB_SHOP_MONEY_BUTTON", 0, 0);
    button->SetText(ResString("IDS_BTN_BUY"), 0x24, NULL, 0, 0);
    button->m_command = BuyPressed;
    button->m_commandParameter1 = 0;
    button->m_commandParameter2 = 0;
    button->m_commandParameter3 = 0;
    return button;
}

Window* CIngameShop::CreateFilterButton(const char* image, EItemTabs tab)
{
    Window* holder = new Window(false);
    CImageButton* button = new CImageButton();
    button->AddImage(image, 0, 0);
    button->m_command = FilterPressed;
    button->m_commandParameter1 = tab;
    button->m_commandParameter2 = 0;
    button->m_commandParameter3 = 0;
    button->SetLayoutType(LAYOUT_SIMPLE);
    button->SetAlign(0x24);
    holder->AddToFront(button);
    return holder;
}

void CIngameShop::SetTab(EItemTabs tab)
{
    m_currentItemButton = NULL;
    m_pageHolder->DeleteChilds();
    m_scroll = new CScrollWindow(true);
    m_scroll->SetPercentWidth(100, 0, 0);
    m_scroll->SetPercentHeight(100, 0, 0);
    m_scroll->SetInsetSpacing(0, 0, 0, SScreenAdaptingConstant(15, 30)());
    m_scroll->ShowPager(false);
    m_pageHolder->AddToFront(m_scroll);
    m_buyButton->SetFlags(0x2001);
    App* app = static_cast<App*>(WindowApp::m_instance);
    switch (tab) {
    case Bank:
        app->InitIAPCurrencyManager()->UpdateIAPInfo(false);
        CreateBankTab();
        break;
    case Ammo:
        CreateAmmoTab();
        ShowEquipButton();
        break;
    case GoPills:
        CreateTab(reinterpret_cast<CItemManager*>(app->m_goPillManager));
        break;
    case Medkits:
        CreateMedkitsTab();
        break;
    case Armor:
        CreateTab(reinterpret_cast<CItemManager*>(app->m_armorManager));
        break;
    default:
        break;
    }
    if (tab != Ammo) {
        m_equipHolder->SetPercentWidth(0, 0, 0);
        m_buyHolder->SetPercentWidth(50, 0, 0);
    }
    m_tab = tab;
    // Dim every filter except the selected one.
    for (int index = 0; index < m_filterButtons.size(); ++index) {
        if (index == tab) {
            m_filterButtons[index]->SetOpacity(1.0f);
        } else {
            m_filterButtons[index]->SetOpacity(0.5f);
        }
    }
    UpdateEquipUnequipButtonState();
}

CAutoArrangedWindow* CIngameShop::CreateItemsContainer()
{
    int width = SScreenAdaptingConstant(400, 800, 800)();
    CAutoArrangedWindow* container = new CAutoArrangedWindow(width, ItemHeight());
    container->SetOneLineMode(true);
    container->m_maxColumns = 4;
    container->SetItemSpacing(0);
    return container;
}

CAutoArrangedWindow* CIngameShop::CreatePage(CScrollWindow* scroll, CItemManager* manager, CItem* item,
                                             CAutoArrangedWindow* container)
{
    if (container == NULL) {
        container = CreateItemsContainer();
    }
    if (item->isHidden()) {
        scroll->Add(container);
        return container;
    }
    int unlockLevel = item->m_unlockLevel;
    int price = item->m_pricePoints;
    if (item->getPriceCredits() > 0) {
        price = item->getPriceCredits();
    }
    XString name = item->m_name;
    int priceIAP = item->m_priceIAP;
    item->loadResources();
    item->getShopImage();
    if (priceIAP > 0) {
        price = priceIAP;
    }
    CShopItemButton* button = new CShopItemButton(manager, item->getType(), name, unlockLevel, price, item->m_id,
                                                  item->getShopImageResId());
    button->SetActiveImage("IDB_BUTTON_INGAME_SHOP_ACTIVE");
    button->m_command = ItemPressed;
    button->m_commandParameter1 = reinterpret_cast<int>(button);
    button->m_commandParameter2 = 0;
    button->m_commandParameter3 = 0;
    if (!container->Add(button)) {
        // Page full: flush it and start the next one.
        scroll->Add(container);
        container = CreateItemsContainer();
        container->Add(button);
    }
    if (m_currentItemButton == NULL) {
        SetCurrentItemButton(button);
    }
    scroll->Add(container);
    return container;
}

void CIngameShop::SetCurrentItemButton(CShopItemButton* button)
{
    if (m_currentItemButton != NULL) {
        m_currentItemButton->SetAlwaysActive(false);
    }
    if (button != NULL) {
        button->SetAlwaysActive(true);
    }
    m_currentItemButton = button;
}

void CIngameShop::ReleaseTabResources(EItemTabs tab)
{
    CItemManager* manager;
    switch (tab) {
    case Ammo: {
        WindowApp** instance = &WindowApp::m_instance;
        App* app = static_cast<App*>(*instance);
        app->InitAmmoManager()->freeResources();
        app = static_cast<App*>(*instance);
        manager = reinterpret_cast<CItemManager*>(app->m_ammoPackManager);
        break;
    }
    case Armor:
        manager = reinterpret_cast<CItemManager*>(static_cast<App*>(WindowApp::m_instance)->m_armorManager);
        break;
    case GoPills:
        manager = reinterpret_cast<CItemManager*>(static_cast<App*>(WindowApp::m_instance)->m_armorManager);
        break;
    case Medkits:
        manager = reinterpret_cast<CItemManager*>(static_cast<App*>(WindowApp::m_instance)->m_medicantManager);
        break;
    default:
        return;
    }
    manager->freeResources();
}

void CIngameShop::CreateBankTab()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CAutoArrangedWindow* container = NULL;
    CItemManager* currencies = reinterpret_cast<CItemManager*>(app->m_currencyManager);
    for (CItem* item = currencies->firstItem(); item != NULL; item = currencies->nextItem()) {
        container = CreatePage(m_scroll, currencies, item, container);
    }
    // In-app currency packs (all but the first, hidden ones skipped), as many as the catalog lists.
    int shown = 0;
    for (CItem* item = app->InitIAPCurrencyManager()->firstItem(); item != NULL;
         item = app->InitIAPCurrencyManager()->nextItem()) {
        if (item->m_id == 0 || item->isHidden()) {
            continue;
        }
        if (app->InitIAPCurrencyManager()->getItemsCount() <= shown) {
            break;
        }
        shown += 1;
        container = CreatePage(m_scroll, app->InitIAPCurrencyManager(), item, container);
    }
}

void CIngameShop::UpdateBankTab()
{
    m_currentItemButton = NULL;
    m_pageHolder->DeleteChilds();
    m_scroll = new CScrollWindow(true);
    m_scroll->SetPercentWidth(100, 0, 0);
    m_scroll->SetPercentHeight(100, 0, 0);
    m_scroll->SetInsetSpacing(0, 0, 0, SScreenAdaptingConstant(15, 30)());
    m_scroll->ShowPager(false);
    m_pageHolder->AddToFront(m_scroll);
    CreateBankTab();
}

void CIngameShop::CreateTab(CItemManager* manager)
{
    CItem* item = manager->firstItem();
    if (item == NULL) {
        return;
    }
    CAutoArrangedWindow* container = NULL;
    while (item != NULL) {
        container = CreatePage(m_scroll, manager, item, container);
        item = manager->nextItem();
    }
}

void CIngameShop::CreateMedkitsTab()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CItem* medkit = app->InitMiscItemManager()->getItemById(1);
    CAutoArrangedWindow* container = CreatePage(m_scroll, app->InitMiscItemManager(), medkit, NULL);
    CItemManager* medicants = reinterpret_cast<CItemManager*>(app->m_medicantManager);
    for (CItem* item = medicants->firstItem(); item != NULL; item = medicants->nextItem()) {
        container = CreatePage(m_scroll, medicants, item, container);
    }
}

void CIngameShop::CreateAmmoTab()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CAutoArrangedWindow* container = CreatePage(m_scroll, app->InitAmmoManager(),
        app->InitAmmoManager()->GetAmmoByType(XString("incendiary")), NULL);
    container = CreatePage(m_scroll, app->InitAmmoManager(),
        app->InitAmmoManager()->GetAmmoByType(XString("explosive")), container);
    // Misc items in the original order: silencer, ..., cyber scope.
    const int miscIds[] = { 0, 5, 6, 2, 3, 4 };
    for (int index = 0; index < 6; ++index) {
        CItem* item = app->InitMiscItemManager()->getItemById(miscIds[index]);
        container = CreatePage(m_scroll, app->InitMiscItemManager(), item, container);
    }
    CDH_Weapon* first = app->m_helper->GetGameData()->GetWeapon01(true);
    container = GetAmmoForWeapon(container, first);
    CDH_Weapon* second = app->m_helper->GetGameData()->GetWeapon02(true);
    if (first != second && first->GetDefaultAmmo() != second->GetDefaultAmmo()) {
        container = GetAmmoForWeapon(container, app->m_helper->GetGameData()->GetWeapon02(true));
    }
    if (container->m_items.size() == 0) {
        m_buyButton->ClearFlags(0x2001);
    }
}

CAutoArrangedWindow* CIngameShop::GetAmmoForWeapon(CAutoArrangedWindow* container, const CDH_Weapon* weapon)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    Vector<const CAmmo*> ammo = weapon->m_compatibleAmmo;
    for (int index = 0; index < ammo.size(); ++index) {
        const CAmmo* current = ammo[index];
        CBH_Player* player = CBH_Player::GetInstance();
        if (player->m_estimatedWeapon != NULL && CBH_Player::GetInstance()->m_estimatedAmmo != -1) {
            // While an estimate is pending only the estimated weapon's default ammo is listed.
            if (CBH_Player::GetInstance()->m_estimatedWeapon->GetDefaultAmmo()->m_id == current->m_id) {
                continue;
            }
        }
        if (!(current->m_ammoType == XString("incendiary")) && !(current->m_ammoType == XString("explosive"))) {
            CItem* item = app->InitAmmoManager()->getItemById(current->m_id);
            container = CreatePage(m_scroll, app->InitAmmoManager(), item, container);
        }
        Vector<CAmmoPack*> packs = app->m_ammoPackManager->getAmmoPacksByType(current->m_ammoType);
        for (int pack = 0; pack < packs.size(); ++pack) {
            container = CreatePage(m_scroll, reinterpret_cast<CItemManager*>(app->m_ammoPackManager),
                                   reinterpret_cast<CItem*>(packs[pack]), container);
        }
    }
    return container;
}

void CIngameShop::ShowEquipButton()
{
    m_equipHolder->SetPercentWidth(25, 0, 0);
    m_buyHolder->SetPercentWidth(25, 0, 0);
    m_equipButton->SetText(ResString("IDS_EQUIP"), 0x24, NULL, 0, 0);
    m_equipButton->m_command = EquipPressed;
    m_equipButton->m_commandParameter1 = 0;
    m_equipButton->m_commandParameter2 = 0;
    m_equipButton->m_commandParameter3 = 0;
    m_equipButton->Enable();
}

void CIngameShop::ShowUneqipButton()
{
    m_equipHolder->SetPercentWidth(25, 0, 0);
    m_buyHolder->SetPercentWidth(25, 0, 0);
    m_equipButton->SetText(ResString("IDS_UNEQUIP"), 0x24, NULL, 0, 0);
    m_equipButton->m_command = UnequipPressed;
    m_equipButton->m_commandParameter1 = 0;
    m_equipButton->m_commandParameter2 = 0;
    m_equipButton->m_commandParameter3 = 0;
    m_equipButton->Enable();
}

void CIngameShop::UpdateEquipUnequipButtonState()
{
    if (m_currentItemButton == NULL) {
        m_equipButton->Disable();
        return;
    }
    m_buyButton->Enable();
    CItemManager* manager = App::GetItemManagerByType(m_currentItemButton->m_type);
    CItem* item = manager->getItemById(m_currentItemButton->m_itemId);
    m_equipButton->Enable();
    CBH_Player* player = CBH_Player::GetInstance();
    int action = GetEquipAction(item, GetActiveWeapon(), player);
    if (action == ShopEquip) {
        ShowEquipButton();
    } else if (action == ShopUnequip) {
        ShowUneqipButton();
    } else {
        m_equipButton->Disable();
    }
}

void CIngameShop::OnCommand(Event& event)
{
    int id = event.m_id;
    if (id == UnequipPressed) {
        HandleUnequipButtonPressed();
    } else if (id == BuyItem) {
        CItem* item = reinterpret_cast<CItem*>(event.m_param1);
        TryToBuyItem(App::GetItemManagerByType(item->getType()), item);
    } else if (id == ItemPressed) {
        SetCurrentItemButton(reinterpret_cast<CShopItemButton*>(event.m_param1));
        UpdateEquipUnequipButtonState();
    } else if (id == EquipPressed) {
        HandleEquipButtonPressed();
    } else if (id == CShopWindow::IAPPurchaseComplete) {
        if (m_waitDialog != NULL) {
            m_waitDialog->Close();
        }
        CIAPCurrency* item = reinterpret_cast<CIAPCurrency*>(event.m_param1);
        CDialogWindow* dialog = new CDialogWindow(true);
        dialog->SetMessageText(ResString("IDS_SHOP_IAP_SUCCESS") + item->GetName());
        dialog->SetButtons(CDialogWindow::BUTTON_OK);
        static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddModal(dialog);
        UpdateStatsWindow();
    } else if (id == BuyPressed) {
        HandleBuyButtonPressed();
    } else if (id == ClosePressed) {
        Close();
    } else if (id == CurrencyUpdated) {
        if (m_tab == Bank) {
            UpdateBankTab();
        }
    } else if (id == FilterPressed) {
        SetTab(static_cast<EItemTabs>(event.m_param1));
    } else if (id == OpenBankTab) {
        SetTab(Bank);
    } else if (id == CShopWindow::IAPPurchaseCancelled || id == CShopWindow::IAPPurchaseFailed) {
        if (m_waitDialog != NULL) {
            m_waitDialog->Close();
        }
        CDialogWindow* dialog = new CDialogWindow(true);
        dialog->SetTitleText(ResString("IDS_SHOP_IAP_ERROR_TITLE"));
        dialog->SetMessageText(ResString("IDS_SHOP_IAP_ERROR"));
        dialog->SetButtons(CDialogWindow::BUTTON_OK);
        static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddModal(dialog);
        SetTab(Bank);
    }
}

void CIngameShop::TryToBuyItem(CItemManager* manager, CItem* item)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    int priceCredits = item->getPriceCredits();
    int priceCash = item->m_pricePoints;
    int itemId = m_currentItemButton->m_itemId;
    XString analyticsName = XString::Format(L"%02d_%ls", itemId, manager->getItemById(itemId)->m_name.c_str());
    if (priceCredits > 0) {
        if (CBH_Player::GetInstance()->m_credits - priceCredits < 0) {
            CTextDialog* dialog = new CTextDialog();
            dialog->SetTitleText(ResString("IDS_SHOP_NOT_ENOUGH_CREDITS_TITLE"));
            dialog->SetMessageText(ResString("IDS_SHOP_NOT_ENOUGH_CREDITS_TEXT"));
            dialog->SetButtons(CDialogWindow::BUTTON_OK, static_cast<CDialogWindow::EDialogButton>(5));
            dialog->SetButtonsCommands(0, OpenBankTab, -1);
            AddModal(dialog);
        } else {
            item->setAvailable(true);
            RegisterPurchasingForAchievement(item);
            CBH_Player::GetInstance()->ReduceCredits(priceCredits);
            CGameAnalytics::logItemPurchased(analyticsName, priceCredits, static_cast<CGameAnalytics::ECurrencyType>(1));
            CGameAnalytics::logIngameItemPurchased(analyticsName, priceCredits, static_cast<CGameAnalytics::ECurrencyType>(1));
            app->m_soundManager->PlaySound("IDM_MENU_PURCHASE_COMPLETE", false, false);
        }
    }
    if (priceCash > 0) {
        if (CBH_Player::GetInstance()->m_cash - priceCash < 0) {
            CTextDialog* dialog = new CTextDialog();
            dialog->SetTitleText(ResString("IDS_SHOP_NOT_ENOUGH_CASH_TITLE"));
            dialog->SetMessageText(ResString("IDS_SHOP_NOT_ENOUGH_CASH_TEXT"));
            dialog->SetButtons(CDialogWindow::BUTTON_OK, static_cast<CDialogWindow::EDialogButton>(5));
            dialog->SetButtonsCommands(0, OpenBankTab, -1);
            AddModal(dialog);
        } else {
            item->setAvailable(true);
            RegisterPurchasingForAchievement(item);
            CBH_Player::GetInstance()->ReduceMoney(priceCash);
            CGameAnalytics::logItemPurchased(analyticsName, priceCash, static_cast<CGameAnalytics::ECurrencyType>(0));
            CGameAnalytics::logIngameItemPurchased(analyticsName, priceCash, static_cast<CGameAnalytics::ECurrencyType>(0));
            app->m_soundManager->PlaySound("IDM_MENU_PURCHASE_COMPLETE", false, false);
            app->InitDailyDealManager()->ItemPurchased(item);
        }
    }
    UpdateStatsWindow();
    app->m_saveManager->Save(true);
    WindowApp::HandleTunnelCommand(UpdateItemLabel, m_currentItemButton->m_itemId, 0, 0);
}

void CIngameShop::HandleBuyButtonPressed()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CItemManager* manager = App::GetItemManagerByType(m_currentItemButton->m_type);
    CItem* item = manager->getItemById(m_currentItemButton->m_itemId);
    if (item->getType() == 11 && CBH_Player::GetInstance()->m_armorId > 0) {
        ShowArmorDialog(item);
    } else if (item->getType() == 5) {
        int currencyId = item->m_id;
        CIAPCurrency* currency = static_cast<CIAPCurrency*>(app->InitIAPCurrencyManager()->getItemById(currencyId));
        if (currency->isAvailable()) {
            ICInAppPurchase* store = ICInAppPurchase::GetInstance();
            CDialogWindow* wait = new CDialogWindow(true);
            wait->SetTitleText(ResString("IDS_SHOP_IAP_LOADING_TITLE"));
            wait->SetMessageText(ResString("IDS_SHOP_IAP_LOADING_MESSAGE"));
            app->m_mainLayer->AddModal(wait);
            m_waitDialog = wait;
            CGameAnalytics::logIAPBuyClicked(XString::Format(L"%02d_%ls", currencyId,
                app->InitIAPCurrencyManager()->getItemById(currencyId)->m_name.c_str()));
            CIAPCurrency* product = static_cast<CIAPCurrency*>(app->InitIAPCurrencyManager()->getItemById(currencyId));
            store->BuyProduct(CStrWChar(product->m_appleId.c_str()));
        } else {
            app->InitIAPCurrencyManager()->UpdateIAPInfo(true);
        }
    } else {
        TryToBuyItem(manager, item);
    }
    UpdateEquipUnequipButtonState();
}

void CIngameShop::RegisterPurchasingForAchievement(CItem* item)
{
    if (item != NULL && item->getType() == 12) {
        CBH_Player::GetInstance()->m_gameStats.RegisterMiscPurchase(item->m_id);
    }
}

void CIngameShop::HandleEquipButtonPressed()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CItemManager* manager = App::GetItemManagerByType(m_currentItemButton->m_type);
    CItem* item = manager->getItemById(m_currentItemButton->m_itemId);
    int type = item->getType();
    if (type == 12) {
        switch (item->m_id) {
        case 0:
        case 5:
        case 6: {
            CDH_Weapon* weapon = app->m_helper->GetGameData()->GetActiveWeapon();
            weapon->m_silencerActive = true;
            weapon->SetActiveAmmo(weapon->GetDefaultAmmo());
            break;
        }
        case HitechVisorItem:
            CBH_Player::GetInstance()->m_hitechVisorEquipped = true;
            break;
        case CyberScopeItem: {
            CDH_Weapon* weapon = app->m_helper->GetGameData()->GetActiveWeapon();
            weapon->SetCyberScopeEquiped(true);
            CBH_Player::GetInstance()->ReduceCyberScopeCount(1);
            app->m_saveManager->Save(true);
            WindowApp::HandleTunnelCommand(UpdateItemLabel, m_currentItemButton->m_itemId, 0, 0);
            m_equipButton->Disable();
            break;
        }
        default:
            break;
        }
    } else if (type == 2) {
        const CAmmo* ammo = static_cast<const CAmmo*>(item);
        CDH_Weapon* weapon = app->m_helper->GetGameData()->GetActiveWeapon();
        weapon->SetActiveAmmo(ammo);
        if (ammo->m_ammoType == XString("incendiary") || ammo->m_ammoType == XString("explosive")) {
            weapon->m_silencerActive = false;
        }
    }
    UpdateEquipUnequipButtonState();
}

void CIngameShop::HandleUnequipButtonPressed()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CItemManager* manager = App::GetItemManagerByType(m_currentItemButton->m_type);
    CItem* item = manager->getItemById(m_currentItemButton->m_itemId);
    int type = item->getType();
    if (type == 2) {
        CDH_Weapon* weapon = app->m_helper->GetGameData()->GetActiveWeapon();
        weapon->SetActiveAmmo(weapon->GetDefaultAmmo());
    }
    int id = item->m_id;
    if (id == HitechVisorItem) {
        CBH_Player::GetInstance()->m_hitechVisorEquipped = false;
    }
    if (id == 0 || (unsigned int)(id - 5) <= 1) {
        app->m_helper->GetGameData()->GetActiveWeapon()->m_silencerActive = false;
    }
    UpdateEquipUnequipButtonState();
}

void CIngameShop::ShowArmorDialog(CItem* item)
{
    CTextDialog* dialog = new CTextDialog();
    dialog->SetButtons(CDialogWindow::BUTTON_OK, CDialogWindow::BUTTON_IGNORE);
    dialog->SetMessageText(ResString("IDS_DIALOG_HAVE_ALREADY_HAD_ARMOR"));
    dialog->SetTitleText("IDS_DIALOG_TITLE_WARNING");
    dialog->SetButtonsCommands(BuyItem, -1, -1);
    dialog->SetButtonsCommandFirstParam(reinterpret_cast<int>(item), 0, 0);
    static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddModal(dialog);
}

void CIngameShop::OnClosed()
{
    WindowApp::HandleTunnelCommand(MissionShopClosed, 0, 0, 0);
    static_cast<App*>(WindowApp::m_instance)->m_timeScale = 1.0f;
}
