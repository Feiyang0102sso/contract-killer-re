#include "CIngameShop.h"
#include "AdBannerSizes.h"
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
#include "CCurrencyManager.h"
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
#include "CSwerveGame.h"
#include "DGHelper.h"
#include "GamePlayHUD.h"

extern const int EventId_UpdateItemLabel;

namespace {
const int MissionShopClosed = 0x7e4d23f0;
const int OpenBankTab = 0x4c2524f3;
const int HitechVisorItem = 2;
const int CyberScopeItem = 4;

App* GetApp()
{
    return static_cast<App*>(WindowApp::m_instance);
}

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
    GetApp()->m_timeScale = 0.0f;
    GetApp()->m_saveManager->ResumeSaving();
    if (CSwerveGame::m_pInstance != NULL) {
        CSwerveGame::m_pInstance->m_hud->m_fireHeld = false;
    }
}

CIngameShop::~CIngameShop()
{
    ReleaseTabResources(m_tab);
    GetApp()->m_saveManager->PauseSaving();
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
    close->SetCommandParameters(ClosePressed, 0, 0, 0);
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
    Window* bank = CreateFilterButton("IDB_HUD_INGAME_SHOP_FILTER_BANK", Bank);
    bank->SetLayoutType(LAYOUT_SIMPLE);
    row->AddToFront(bank, 0, 0);
    m_filterButtons.addElement(bank);

    Window* ammo = CreateFilterButton("IDB_HUD_INGAME_SHOP_FILTER_AMMO", Ammo);
    row->AddToFront(ammo, 1, 0);
    m_filterButtons.addElement(ammo);

    Window* goPills = CreateFilterButton("IDB_HUD_INGAME_SHOP_FILTER_GO_PILLS", GoPills);
    row->AddToFront(goPills, 2, 0);
    m_filterButtons.addElement(goPills);

    Window* medkits = CreateFilterButton("IDB_HUD_INGAME_SHOP_FILTER_MEDKITS", Medkits);
    row->AddToFront(medkits, 3, 0);
    m_filterButtons.addElement(medkits);

    Window* armor = CreateFilterButton("IDB_HUD_INGAME_SHOP_FILTER_ARMOR", Armor);
    row->AddToFront(armor, 4, 0);
    m_filterButtons.addElement(armor);
    return row;
}

CImageButton* CIngameShop::CreateEquipButton()
{
    CImageButton* button = new CImageButton();
    button->AddImage("IDB_SHOP_MONEY_BUTTON", 0, 0);
    button->SetText(ResString("IDS_EQUIP"), 0x24, NULL, 0, 0);
    button->SetCommandParameters(EquipPressed, 0, 0, 0);
    return button;
}

CImageButton* CIngameShop::CreateBuyButton()
{
    CImageButton* button = new CImageButton();
    button->AddImage("IDB_SHOP_MONEY_BUTTON", 0, 0);
    button->SetText(ResString("IDS_BTN_BUY"), 0x24, NULL, 0, 0);
    button->SetCommandParameters(BuyPressed, 0, 0, 0);
    return button;
}

Window* CIngameShop::CreateFilterButton(const char* image, EItemTabs tab)
{
    Window* holder = new Window(false);
    CImageButton* button = new CImageButton();
    button->AddImage(image, 0, 0);
    button->SetCommandParameters(FilterPressed, tab, 0, 0);
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
    ReleaseTabResources(m_tab);
    CItemManager* manager;
    switch (tab) {
    case Bank:
        GetApp()->InitIAPCurrencyManager()->UpdateIAPInfo(false);
        CreateBankTab();
        goto hideEquipHolder;
    case Armor:
        manager = reinterpret_cast<CItemManager*>(GetApp()->m_armorManager);
    addTab:
        CreateTab(manager);
    hideEquipHolder:
        m_equipHolder->SetPercentWidth(0, 0, 0);
        m_buyHolder->SetPercentWidth(50, 0, 0);
        break;
    case Medkits:
        CreateMedkitsTab();
        goto hideEquipHolder;
    case GoPills:
        manager = reinterpret_cast<CItemManager*>(GetApp()->m_goPillManager);
        goto addTab;
    case Ammo:
        CreateAmmoTab();
        ShowEquipButton();
        break;
    default:
        break;
    }

    m_tab = tab;
    // Dim the filters before highlighting the selected one.
    for (int index = 0; index < m_filterButtons.size(); ++index) {
        m_filterButtons[index]->SetOpacity(0.5f);
    }
    m_filterButtons[tab]->SetOpacity(1.0f);
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
    int priceIAP = item->m_priceIAP;
    XString name = item->m_name;
    item->loadResources();
    item->getShopImage();
    if (priceIAP > 0) {
        price = priceIAP;
    }
    CShopItemButton* button = new CShopItemButton(manager, item->getType(), name, unlockLevel, price, item->m_id,
                                                  item->getShopImageResId());
    button->SetActiveImage("IDB_BUTTON_INGAME_SHOP_ACTIVE");
    button->SetCommandParameters(ItemPressed, reinterpret_cast<int>(button), 0, 0);
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
    CAutoArrangedWindow* container = NULL;
    for (CItem* item = GetApp()->m_currencyManager->firstItem(); item != NULL;
         item = GetApp()->m_currencyManager->nextItem()) {
        container = CreatePage(m_scroll, GetApp()->m_currencyManager, item, container);
    }
    // In-app currency packs (all but the first, hidden ones skipped), as many as the catalog lists.
    int shown = 0;
    CItem* item = GetApp()->InitIAPCurrencyManager()->firstItem();
    while (true) {
        if (item == NULL) {
            return;
        }
        if (item->m_id != 0 && !item->isHidden()) {
            int count = GetApp()->InitIAPCurrencyManager()->getItemsCount();
            if (shown >= count) {
                return;
            }
            CScrollWindow* scroll = m_scroll;
            ++shown;
            CItemManager* currencies = GetApp()->InitIAPCurrencyManager();
            container = CreatePage(scroll, currencies, item, container);
        }
        item = GetApp()->InitIAPCurrencyManager()->nextItem();
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
    CItem* medkit = GetApp()->InitMiscItemManager()->getItemById(1);
    CScrollWindow* scroll = m_scroll;
    CItemManager* misc = GetApp()->InitMiscItemManager();
    CAutoArrangedWindow* container = CreatePage(scroll, misc, medkit, NULL);
    CItemManager* medicants = reinterpret_cast<CItemManager*>(GetApp()->m_medicantManager);
    for (CItem* item = medicants->firstItem(); item != NULL; item = medicants->nextItem()) {
        container = CreatePage(m_scroll, medicants, item, container);
    }
}

void CIngameShop::CreateAmmoTab()
{
    CScrollWindow* scroll = m_scroll;
    CItemManager* ammo = GetApp()->InitAmmoManager();
    CAmmoManager* ammoCatalog = GetApp()->InitAmmoManager();
    CAutoArrangedWindow* container = CreatePage(scroll, ammo, ammoCatalog->GetAmmoByType(XString("incendiary")), NULL);

    scroll = m_scroll;
    ammo = GetApp()->InitAmmoManager();
    ammoCatalog = GetApp()->InitAmmoManager();
    container = CreatePage(scroll, ammo, ammoCatalog->GetAmmoByType(XString("explosive")), container);

    // Misc items in the original order: silencers, visor, revive, cyber scope.
    CItem* item = GetApp()->InitMiscItemManager()->getItemById(0);
    scroll = m_scroll;
    CItemManager* misc = GetApp()->InitMiscItemManager();
    container = CreatePage(scroll, misc, item, container);

    item = GetApp()->InitMiscItemManager()->getItemById(5);
    scroll = m_scroll;
    misc = GetApp()->InitMiscItemManager();
    container = CreatePage(scroll, misc, item, container);

    item = GetApp()->InitMiscItemManager()->getItemById(6);
    scroll = m_scroll;
    misc = GetApp()->InitMiscItemManager();
    container = CreatePage(scroll, misc, item, container);

    item = GetApp()->InitMiscItemManager()->getItemById(HitechVisorItem);
    scroll = m_scroll;
    misc = GetApp()->InitMiscItemManager();
    container = CreatePage(scroll, misc, item, container);

    item = GetApp()->InitMiscItemManager()->getItemById(3);
    scroll = m_scroll;
    misc = GetApp()->InitMiscItemManager();
    container = CreatePage(scroll, misc, item, container);

    item = GetApp()->InitMiscItemManager()->getItemById(CyberScopeItem);
    scroll = m_scroll;
    misc = GetApp()->InitMiscItemManager();
    container = CreatePage(scroll, misc, item, container);

    CDH_Weapon* first = GetApp()->m_helper->GetGameData()->GetWeapon01(true);
    container = GetAmmoForWeapon(container, first);
    CDH_Weapon* second = GetApp()->m_helper->GetGameData()->GetWeapon02(true);
    const CAmmo* firstAmmo = first->GetDefaultAmmo();
    const CAmmo* secondAmmo = second->GetDefaultAmmo();
    if (first != second && firstAmmo != secondAmmo) {
        container = GetAmmoForWeapon(container, GetApp()->m_helper->GetGameData()->GetWeapon02(true));
    }
    if (container->m_items.size() == 0) {
        m_buyButton->ClearFlags(0x2001);
    }
}

CAutoArrangedWindow* CIngameShop::GetAmmoForWeapon(CAutoArrangedWindow* container, const CDH_Weapon* weapon)
{
    Vector<const CAmmo*> ammo = weapon->m_compatibleAmmo;
    for (int index = 0; index < ammo.size(); ++index) {
        const CAmmo* current = ammo[index];
        if (CBH_Player::GetInstance()->m_estimatedWeapon != NULL &&
            CBH_Player::GetInstance()->m_estimatedAmmo != -1 &&
            CBH_Player::GetInstance()->m_estimatedWeapon->GetDefaultAmmo()->m_id == current->m_id) {
            // The estimate already covers its weapon's default ammo.
            continue;
        }
        if (!(XString(current->m_ammoType) == XString("incendiary")) &&
            !(XString(current->m_ammoType) == XString("explosive"))) {
            CScrollWindow* scroll = m_scroll;
            CItemManager* manager = GetApp()->InitAmmoManager();
            CItem* item = GetApp()->InitAmmoManager()->getItemById(current->m_id);
            container = CreatePage(scroll, manager, item, container);
        }
        Vector<CAmmoPack*> packs = GetApp()->m_ammoPackManager->getAmmoPacksByType(XString(current->m_ammoType));
        for (int pack = 0; pack < packs.size(); ++pack) {
            container = CreatePage(m_scroll, GetApp()->m_ammoPackManager, packs[pack], container);
        }
    }
    return container;
}

void CIngameShop::ShowEquipButton()
{
    m_equipHolder->SetPercentWidth(25, 0, 0);
    m_buyHolder->SetPercentWidth(25, 0, 0);
    m_equipButton->SetText(ResString("IDS_EQUIP"), 0x24, NULL, 0, 0);
    m_equipButton->SetCommandParameters(EquipPressed, 0, 0, 0);
    m_equipButton->Enable();
}

void CIngameShop::ShowUneqipButton()
{
    m_equipHolder->SetPercentWidth(25, 0, 0);
    m_buyHolder->SetPercentWidth(25, 0, 0);
    m_equipButton->SetText(ResString("IDS_UNEQUIP"), 0x24, NULL, 0, 0);
    m_equipButton->SetCommandParameters(UnequipPressed, 0, 0, 0);
    m_equipButton->Enable();
}

void CIngameShop::UpdateEquipUnequipButtonState()
{
    m_buyButton->Enable();
    CItemManager* manager = App::GetItemManagerByType(m_currentItemButton->m_type);
    CItem* item = manager->getItemById(m_currentItemButton->m_itemId);
    m_equipButton->Enable();

    int type = item->getType();
    if (type == 12) {
        goto miscItem;
    }
    if (type == 9) {
        goto disableEquip;
    }
    if (type != 2) {
        return;
    }

    {
        m_equipButton->Disable();
        const CAmmo* ammo = static_cast<const CAmmo*>(item);
        int available = CBH_Player::GetInstance()->GetAvailableAmmo(ammo, true);
        CDH_Weapon* weapon = GetApp()->m_helper->GetGameData()->GetActiveWeapon();
        const CAmmo* active = weapon->GetActiveAmmo();
        const CAmmo* standard = weapon->GetDefaultAmmo();
        bool compatible = weapon->CanUseAmmo(ammo);
        bool different = active != ammo;
        if (standard == ammo || different) {
            if (available < 1 || !compatible || active == ammo) {
                return;
            }
            goto showEquip;
        }
    }

showUnequip:
    ShowUneqipButton();
    return;

showEquip:
    ShowEquipButton();
    return;

miscItem:
    switch (item->m_id) {
    case HitechVisorItem:
        if (!CBH_Player::GetInstance()->m_hitechVisorEquipped) {
            goto visorUnavailable;
        }
        ShowUneqipButton();
    disableBuy:
        m_buyButton->Disable();
        return;
    case 0:
    case 5:
    case 6: {
        CDH_Weapon* weapon = GetApp()->m_helper->GetGameData()->GetActiveWeapon();
        bool silent = weapon->isSilent();
        bool supportsSilencer = weapon->m_silencerAvailable;
        CBH_Player* player = CBH_Player::GetInstance();
        if (supportsSilencer) {
            if (silent) {
                goto showUnequip;
            }
        }
        if (supportsSilencer && !silent && player->m_silentShotsCount > 0) {
            goto showEquip;
        }
    }
    disableEquip:
        m_equipButton->Disable();
        return;
    visorUnavailable:
        if (item->isAvailable()) {
            ShowEquipButton();
            goto disableBuy;
        }
        goto disableEquip;
    default:
        return;
    case 3:
    case CyberScopeItem: {
        CDH_Weapon* weapon = GetApp()->m_helper->GetGameData()->GetActiveWeapon();
        m_equipButton->Disable();
        if (CBH_Player::GetInstance()->m_cyberScopesCount > 0 && !weapon->m_cyberScopeEquipped) {
            m_equipButton->Enable();
        }
        return;
    }
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
        XString message = ResString("IDS_SHOP_IAP_SUCCESS") + item->m_name.c_str();
        dialog->SetMessageText(message);
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
        GetApp()->m_mainLayer->AddModal(dialog);
    }
}

void CIngameShop::TryToBuyItem(CItemManager* manager, CItem* item)
{
    int priceCredits = item->getPriceCredits();
    int priceCash = item->m_pricePoints;
    int itemId = m_currentItemButton->m_itemId;
    XString analyticsName = XString::Format(L"%02d_%s", itemId, manager->getItemById(itemId)->m_name.c_str());
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
            GetApp()->m_soundManager->PlaySound("IDM_MENU_PURCHASE_COMPLETE", false, false);
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
            GetApp()->m_soundManager->PlaySound("IDM_MENU_PURCHASE_COMPLETE", false, false);
            GetApp()->InitDailyDealManager()->ItemPurchased(item);
        }
    }
    UpdateStatsWindow();
    GetApp()->m_saveManager->Save(true);
    WindowApp::HandleTunnelCommand(EventId_UpdateItemLabel, m_currentItemButton->m_itemId, 0, 0);
}

void CIngameShop::HandleBuyButtonPressed()
{
    CItemManager* manager = App::GetItemManagerByType(m_currentItemButton->m_type);
    CItem* item = manager->getItemById(m_currentItemButton->m_itemId);
    if (item->getType() == 11 && CBH_Player::GetInstance()->m_armorDamageProof > 0) {
        ShowArmorDialog(item);
        return;
    }
    if (item->getType() == 5) {
        int currencyId = item->m_id;
        CIAPCurrency* currency = static_cast<CIAPCurrency*>(GetApp()->InitIAPCurrencyManager()->getItemById(currencyId));
        if (currency->isAvailable()) {
            ICInAppPurchase* store = ICInAppPurchase::GetInstance();
            CDialogWindow* wait = new CDialogWindow(true);
            wait->SetTitleText(ResString("IDS_SHOP_IAP_LOADING_TITLE"));
            wait->SetMessageText(ResString("IDS_SHOP_IAP_LOADING_MESSAGE"));
            GetApp()->m_mainLayer->AddModal(wait);
            m_waitDialog = wait;
            XString analyticsName = XString::Format(L"%02d_%s", currencyId,
                GetApp()->InitIAPCurrencyManager()->getItemById(currencyId)->m_name.c_str());
            CGameAnalytics::logIAPBuyClicked(analyticsName);
            store->BuyProduct(CStrWChar(XString(static_cast<CIAPCurrency*>(
                GetApp()->InitIAPCurrencyManager()->getItemById(currencyId))->m_appleId).c_str()));
        } else {
            GetApp()->InitIAPCurrencyManager()->UpdateIAPInfo(true);
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
    CItemManager* manager = App::GetItemManagerByType(m_currentItemButton->m_type);
    CItem* item = manager->getItemById(m_currentItemButton->m_itemId);
    int type = item->getType();
    if (type != 12) {
        if (type == 2) {
            const CAmmo* ammo = static_cast<const CAmmo*>(item);
            CDH_Weapon* weapon = GetApp()->m_helper->GetGameData()->GetActiveWeapon();
            weapon->SetActiveAmmo(ammo);
            if (XString(ammo->m_ammoType) == XString("incendiary") ||
                XString(ammo->m_ammoType) == XString("explosive")) {
                weapon->m_silencerActive = false;
            }
        }
    } else {
        switch (item->m_id) {
        case 0:
        case 5:
        case 6: {
            CDH_Weapon* weapon = GetApp()->m_helper->GetGameData()->GetActiveWeapon();
            weapon->m_silencerActive = true;
            weapon->SetActiveAmmo(weapon->GetDefaultAmmo());
            break;
        }
        case HitechVisorItem:
            CBH_Player::GetInstance()->m_hitechVisorEquipped = true;
            break;
        case CyberScopeItem: {
            CDH_Weapon* weapon = GetApp()->m_helper->GetGameData()->GetActiveWeapon();
            weapon->SetCyberScopeEquiped(true);
            CBH_Player::GetInstance()->ReduceCyberScopeCount(1);
            GetApp()->m_saveManager->Save(true);
            WindowApp::HandleTunnelCommand(EventId_UpdateItemLabel, m_currentItemButton->m_itemId, 0, 0);
            m_equipButton->Disable();
            break;
        }
        default:
            break;
        }
    }
    UpdateEquipUnequipButtonState();
}

void CIngameShop::HandleUnequipButtonPressed()
{
    CItemManager* manager = App::GetItemManagerByType(m_currentItemButton->m_type);
    CItem* item = manager->getItemById(m_currentItemButton->m_itemId);
    int type = item->getType();
    if (type != 12) {
        if (type == 2) {
            CDH_Weapon* weapon = GetApp()->m_helper->GetGameData()->GetActiveWeapon();
            weapon->SetActiveAmmo(weapon->GetDefaultAmmo());
        }
    } else {
        int id = item->m_id;
        if (id != 0) {
            if (id == HitechVisorItem) {
                CBH_Player::GetInstance()->m_hitechVisorEquipped = false;
                goto updateButtons;
            }
            if (static_cast<unsigned int>(id) - 5 > 1) {
                goto updateButtons;
            }
        }
        GetApp()->m_helper->GetGameData()->GetActiveWeapon()->m_silencerActive = false;
    }
updateButtons:
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
