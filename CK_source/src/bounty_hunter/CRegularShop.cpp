#include "CRegularShop.h"
#include "CGameAnalytics.h"
#include "ICInAppPurchase.h"
#include "CMiscItemManager.h"
#include "CFont.h"
#include "BaseDialog.h"
#include "CBH_ProgressBar.h"
#include "CDH_Weapon.h"
#include "CEnergyManager.h"
#include "CAutoArrangedWindow.h"
#include "CBH_SurfaceManager.h"
#include "CScrollWindow.h"
#include "SScreenAdaptingConstant.h"

#include "App.h"
#include "CAdColonyWindow.h"
#include "CBH_MenuButton.h"
#include "CAmmo.h"
#include "CAmmoManager.h"
#include "CBH_Player.h"
#include "CDH_WeaponsManager.h"
#include "CFontMgr.h"
#include "CIAPCurrency.h"
#include "CIAPCurrencyManager.h"
#include "CDialogWindow.h"
#include "CPHInterface.h"
#include "CShopButtonWeapon.h"
#include "CShopItemInfoWindow.h"
#include "CTutorialManager.h"
#include "CStdUtil_Cocoa.h"
#include "DGHelper.h"
#include "ICAdManager.h"
#include "TapjoyInterface.h"

namespace {
const int OpenItem = 0x43a30521;
const int ConfirmPurchase = 0x43a30522;
const int CloseStore = (int)0x97204784;
const int CloseButton = 0x43a30524;
const int WatchAd = 0x71f7f6e0;
const int MoreGames = 0x71f7f6e4;
const int OpenTapjoy = static_cast<int>(0x8f577d94u);
const int AmmoChanged = (int)0x9a412292;
const char* const MoreGamesPlacement = "more_games";
}

class CAdColonyButton : public Window {
public:
    CAdColonyButton(const char* image, const char* activeImage, const XString& caption, bool notification,
                    int textSpacing, int inset);
    void Update(bool showNotification);
    virtual ~CAdColonyButton();
    virtual void OnPointerEvent(Event& event);
    void HideNotification();

    // Original layout from CAdColonyButton::Update (0xff508).
    ImageWindow* m_image;           // 0xa4 normal image
    ImageWindow* m_button;          // 0xa8 pressed image (hidden until pressed)
    int m_command;                  // 0xac tunnel command sent on release
    int m_stableFrames;             // 0xb0
    int m_lastX;                    // 0xb4
    int m_lastY;                    // 0xb8
    bool m_notificationShown;       // 0xbc
    bool m_hasNotification;         // 0xbd
    unsigned int m_lastRequestMs;   // 0xc0
};

CAdColonyButton::CAdColonyButton(const char* image, const char* activeImage, const XString& caption,
                                 bool notification, int textSpacing, int inset)
    : Window(false), m_command(0), m_stableFrames(0), m_lastX(-1), m_lastY(-1), m_notificationShown(false),
      m_hasNotification(notification), m_lastRequestMs(0)
{
    SetInsetSpacing(inset, 0, 0, 0);
    Window* images = new Window(false);
    m_image = new ImageWindow();
    m_image->SetFillType(static_cast<ImageWindow::EFillType>(0));
    m_image->SetImageById(image);
    images->AddToFront(m_image);
    m_button = new ImageWindow();
    m_button->SetFillType(static_cast<ImageWindow::EFillType>(0));
    m_button->SetImageById(activeImage);
    m_button->ClearFlags(0x2001);
    images->AddToFront(m_button);
    int width;
    int height;
    if (notification == 1) {
        // Wide button: the caption is drawn on both images.
        AddToFront(images);
        TextWindow* text = new TextWindow(caption, CFontMgr::GetInstance()->GetFont(17));
        text->SetLayoutType(LAYOUT_SIMPLE);
        text->SetAlign(0x14);
        m_image->AddToFront(text);
        TextWindow* activeText = new TextWindow(caption, CFontMgr::GetInstance()->GetFont(17));
        activeText->SetLayoutType(LAYOUT_SIMPLE);
        activeText->SetAlign(0x14);
        m_button->AddToFront(activeText);
        int fontHeight = CFontMgr::GetInstance()->GetFont(17)->GetFontHeight();
        width = ImageWidth(m_image->m_surface);
        height = ImageHeight(m_image->m_surface) + fontHeight;
    } else {
        // Square button: caption below the image.
        width = ImageWidth(m_image->m_surface);
        height = ImageHeight(m_image->m_surface);
        images->SetSize(width, height);
        AddToFront(images, 0, 0);
        TextWindow* text = new TextWindow(caption, CFontMgr::GetInstance()->GetFont(17));
        text->SetOutsetSpacing(textSpacing, 0, 0, 0);
        text->SetLayoutType(LAYOUT_SIMPLE);
        text->SetAlign(0xc);
        AddToFront(text, 0, 1);
        int fontHeight = CFontMgr::GetInstance()->GetFont(17)->GetFontHeight();
        width = ImageWidth(m_image->m_surface);
        int imageHeight = ImageHeight(m_image->m_surface);
        height = fontHeight + imageHeight + SScreenAdaptingConstant(60, 120, 120)();
    }
    SetSize(width, height);
}

void CAdColonyButton::Update(bool showNotification)
{
    if (!m_hasNotification) {
        return;
    }
    int x = m_button->LocalToGlobalX(0) + static_cast<int>(m_button->m_width * 0.95);
    int y = m_button->LocalToGlobalY(0) + static_cast<int>(m_button->m_height * 0.30);
    if (m_parent != NULL) {
        int top = m_parent->LocalToGlobalY(0);
        showNotification = showNotification && y >= top && y < top + m_parent->m_height;
    }
    if (App::IsVGA()) {
        x /= 2;
        y /= 2;
    }
    if (!showNotification || x != m_lastX || y != m_lastY) {
        HideNotification();
        m_stableFrames = 0;
        m_lastX = x;
        m_lastY = y;
        return;
    }
    if (m_stableFrames < 15) {
        ++m_stableFrames;
        return;
    }
    unsigned int now = CStdUtil_Cocoa::GetUpTimeMS();
    if (!m_notificationShown && now - m_lastRequestMs >= 501) {
        CPHInterface::showNotificationView(MoreGamesPlacement, false, x, y);
        m_notificationShown = true;
        m_lastRequestMs = now;
    }
}

void CAdColonyButton::OnPointerEvent(Event& event)
{
    int type = event.m_type;
    if (type == 0x43a39819) {
        // Pressed: highlight the image.
        m_button->SetFlags(0x2001);
    } else if (type == 0x43a3981b) {
        // Released: restore and fire the command.
        m_button->ClearFlags(0x2001);
        WindowApp::HandleTunnelCommand(m_command, 0, 0, 0);
        event.Clear();
    } else if (type != 0x43a3981a) {
        return;
    }
    event.Clear();
}

CAdColonyButton::~CAdColonyButton()
{
    if (m_notificationShown) {
        CPHInterface::hideNotificationView(MoreGamesPlacement);
    }
}

void CAdColonyButton::HideNotification()
{
    if (m_notificationShown) {
        CPHInterface::hideNotificationView(MoreGamesPlacement);
        m_notificationShown = false;
    }
}

CRegularShop* CRegularShop::m_instance;

namespace {
const int FilterWeapons = 0x1ee3df20;
const int FilterOthers = (int)0x9522a894;
const int TabLockChanged = (int)0xa4d0d121;
const int IAPInfoRequested = (int)0xc24364a4;
const int CurrencyUpdated = (int)0xff403571;
const int CloseRequested = 0x5f523cd9;
const SScreenAdaptingConstant ItemSpacing(200, 440, 550);
// Bridges for the CK_source helpers Show/ShowItem/ShowDeal (not in the original unit).
int g_pendingItemId = -1;
CItem* g_pendingDeal = NULL;
CItem* g_pendingCurrency = NULL;

}

CRegularShop::CRegularShop()
    : m_weaponFilter(0), m_otherFilter(0), m_waitDialog(NULL), m_bankPage(NULL)
{
    m_unknownEc = false;
    SetTitleText(ResString("IDS_TITLE_SHOP"));
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->InitIAPCurrencyManager()->UpdateIAPInfo(true);

    // Bank tab.
    m_bankPage = new CScrollWindow(true);
    m_bankPage->SetCorner(GetTabLeft(), GetTabTop());
    m_bankPage->SetLayoutType(LAYOUT_NONE);
    UpdateCurrenciesPage();
    m_tabIndices[TAB_BANK] = AddTab(ResString("IDS_TAB_BANK"), m_bankPage, "SUR_TAB_MONEY", false, -1);

    // Weapons tab: filter column on the left, scroll page on the right.
    Window* weaponsTab = new Window(false);
    weaponsTab->m_debugFrame = true;
    Window* filters = new Window(false);
    filters->SetPercentWidth(20, 0, 0);
    filters->SetPercentHeight(100, 0, 0);
    filters->SetLayoutType(LAYOUT_SIMPLE);
    filters->SetAlign(9);
    filters->SetHeightByContent(0, 0);
    const char* const filterTexts[] = {
        "IDS_FILTER_HANDGUN", "IDS_FILTER_SNIPER", "IDS_FILTER_AUTOMATIC", "IDS_FILTER_SHOTGUN", "IDS_FILTER_EXOTIC"
    };
    const char* const filterImages[] = {
        "IDB_SHOP_FILTER_HANDGUNS", "IDB_SHOP_FILTER_SNIPER", "IDB_SHOP_FILTER_AUTOMATIC",
        "IDB_SHOP_FILTER_SHOTGUNS", "IDB_SHOP_FILTER_EXOTIC"
    };
    const int filterBits[] = { 1, 8, 4, 0x10, 0x20 };
    for (int index = 0; index < 5; ++index) {
        CFilterButton* button = new CFilterButton(ResString(filterTexts[index]), filterImages[index], m_weaponFilter);
        button->m_command = FilterWeapons;
        button->m_commandParameter1 = filterBits[index];
        button->m_commandParameter2 = 0;
        button->m_commandParameter3 = 0;
        filters->AddToFront(button, 0, index);
        if (index > 0) {
            button->SetOutsetSpacing(SScreenAdaptingConstant(-2, 0)(), 0, 0, 0);
        }
    }
    weaponsTab->AddToFront(filters);
    m_weaponsPage = new CScrollWindow(true);
    weaponsTab->m_debugFrame = true;
    weaponsTab->AddToFront(m_weaponsPage);
    m_weaponsPage->SetLayoutType(LAYOUT_SIMPLE);
    m_weaponsPage->SetAlign(0x12);
    int pageWidth = SScreenAdaptingConstant(390, 790, 840)();
    int pageHeight = m_height - GetTabTop() - SScreenAdaptingConstant(51, 125)();
    m_weaponsPage->SetSize(pageWidth, pageHeight);
    m_weaponsPage->HandleUpdate(false);
    CreateWeaponsPage(m_weaponsPage, NULL, 0);
    m_tabIndices[TAB_WEAPONS] = AddTab(ResString("IDS_TAB_WEAPONS"), weaponsTab, "SUR_TAB_WEAPONS", false, -1);

    // Energy tab (credits video + currency packs, energy items when enabled).
    CScrollWindow* energyPage = new CScrollWindow(true);
    energyPage->SetCorner(GetTabLeft(), GetTabTop());
    energyPage->SetLayoutType(LAYOUT_NONE);
    int margin = SScreenAdaptingConstant(50, 100, 100)();
    CAutoArrangedWindow* container = CreateItemsContainer(m_width - margin);
    CreateCustomButton(container, false, "IDB_SHOP_ICON_ADCOLONY", "IDB_SQUARE_SHOP_ICON_ACTIVE_GREEN",
                       ResString("IDS_AWARD_CREDITS").Upper(), WatchAd);
    CAutoArrangedWindow* energyContainer = CreatePage(energyPage, app->InitIAPCurrencyManager(), container, 1);
    if (CBH_Player::GetInstance()->IsEnegyEnabled() == 1) {
        CreatePage(energyPage, app->m_energyManager, energyContainer, -1);
        m_tabIndices[2] = AddTab(  // 0xf8 energy slot
            ResString("IDS_TAB_ENERGY"), energyPage, "SUR_TAB_ENERGY", false, -1);
    }

    // Ammo tab.
    m_ammoPage = new CScrollWindow(true);
    m_ammoPage->SetCorner(GetTabLeft(), GetTabTop());
    m_ammoPage->SetLayoutType(LAYOUT_NONE);
    UpdateAmmoPage();
    m_tabIndices[TAB_AMMO] = AddTab(ResString("IDS_TAB_AMMO"), m_ammoPage, "SUR_TAB_AMMO", false, -1);

    CreateOtherTab();
    UpdateTabLock();
    SetTab(TAB_WEAPONS);
    if (!CTutorialManager::IsPlayerInTutorial()) {
        CPHInterface::startPublisherContentRequest("store_launch", false, false);
    }
}
CRegularShop::~CRegularShop()
{
    m_instance = NULL;
    static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->ReleaseSurface("IDB_SHOP_ICOM_TAPJOY_WEAPON");
}

CRegularShop* CRegularShop::GetInstance(bool create)
{
    if (m_instance == NULL && create) {
        m_instance = new CRegularShop();
    }
    return m_instance;
}

void CRegularShop::Show(ETab tab)
{
    CRegularShop* existing = GetInstance(false);
    if (existing != NULL) {
        existing->SetTab(tab);
        return;
    }
    CRegularShop* shop = GetInstance(true);
    shop->SetTab(tab);
    WindowApp::HandleTunnelCommand(0x718a0676, 0, 0, 0);
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_mainLayer->AddModal(shop);
}

void CRegularShop::ShowItem(ETab tab, int id)
{
    Show(tab);
    g_pendingItemId = id;
}

void CRegularShop::ShowDeal(CItem* item)
{
    ETab tab = TAB_OTHER;
    if (item->getType() == 0) {
        tab = TAB_WEAPONS;
    } else if (item->getType() == 2) {
        tab = TAB_AMMO;
    }
    Show(tab);
    g_pendingDeal = item;
}

void CRegularShop::Init()
{
    if (CTutorialManager::IsPlayerInTutorial() == 1) {
        WindowApp::HandleTunnelCommand(-0x7b79c896, 0, 0, 0);
    }
}

void CRegularShop::Update()
{
    bool showBadge = false;
    if (GetTab() == TAB_WEAPONS && !HasModalChild() && !CTutorialManager::IsPlayerInTutorial()) {
        if ((m_flags & 0x200) == 0) {
            showBadge = ((m_flags >> 0x18) & 0x20) != 0;
        }
    }
    m_moreGamesButton->Update(showBadge);
    if (CTutorialManager::IsPlayerInTutorial() == 1 && CTutorialManager::IsShopCloseBlink() == 1) {
        m_back->SetBlinking(true);
    }
    // Top banner: hidden while a modal child is open, shown again when the shop is active.
    if (HasModalChild() && m_unknownEc) {
        ICAdManager::GetInstance()->HideAd();
        m_unknownEc = false;
    } else if (!HasModalChild() && !m_unknownEc && (m_flags & 0x20000000) != 0) {
        ICAdManager::GetInstance()->ShowAd(ICAdManager::Type_AdMarvel, ICAdManager::Position_TopRight,
                                          -1, -1, NULL, NULL);
        m_unknownEc = true;
    }
    if (g_pendingDeal != NULL) {
        CItem* deal = g_pendingDeal;
        g_pendingDeal = NULL;
        if (deal->m_unlockLevel <= CBH_Player::GetInstance()->GetLevel()) {
            ShowItem(deal);
        }
    }
    if (g_pendingItemId >= 0) {
        int itemId = g_pendingItemId;
        g_pendingItemId = -1;
        CItem* item = static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->getItemById(itemId);
        if (item != NULL && item->m_unlockLevel <= CBH_Player::GetInstance()->GetLevel()) {
            ShowItem(item);
        }
    }
    CBaseScreen::Update();
}

void CRegularShop::OnGetFocus()
{
}

void CRegularShop::OnLostFocus()
{
    m_unknownEc = false;
    ICAdManager::GetInstance()->HideAd();
    m_moreGamesButton->HideNotification();
}

void CRegularShop::UpdateTabLock()
{
    CTutorialManager* tutorial = static_cast<App*>(WindowApp::m_instance)->m_tutorialManager;
    SetTabLock(3, tutorial->IsTabLocked(static_cast<ETab>(3)));
    SetTabLock(0, tutorial->IsTabLocked(static_cast<ETab>(0)));
    SetTabLock(2, tutorial->IsTabLocked(static_cast<ETab>(2)));
    SetTabLock(4, tutorial->IsTabLocked(static_cast<ETab>(4)));
    SetTabLock(1, tutorial->IsTabLocked(static_cast<ETab>(1)));
}

void CRegularShop::UpdateCurrenciesPage()
{
    if (m_bankPage != NULL) {
        m_bankPage->FlushPages();
    }
    int margin = SScreenAdaptingConstant(50, 100, 100)();
    CAutoArrangedWindow* container = CreateItemsContainer(m_width - margin);
    CreateCustomButton(container, false, "IDB_SHOP_ICON_ADCOLONY", "IDB_SQUARE_SHOP_ICON_ACTIVE_GREEN",
                       ResString("IDS_AWARD_CREDITS").Upper(), WatchAd);
    App* app = static_cast<App*>(WindowApp::m_instance);
    container = CreatePage(m_bankPage, app->InitIAPCurrencyManager(), container, -1);
    CreatePage(m_bankPage, reinterpret_cast<CItemManager*>(app->m_currencyManager), container, -1);
}

void CRegularShop::CreateWeaponsPage(CScrollWindow* scroll, CAutoArrangedWindow* container, unsigned int filter)
{
    if (container == NULL) {
        int margin = SScreenAdaptingConstant(90, 200, 180)();
        int height = SScreenAdaptingConstant(230, 460, 550)();
        container = new CAutoArrangedWindow(m_width - margin, height);
        container->m_maxRows = 2;
    }
    // First cell: "more games" banner above the Tapjoy weapon offer.
    Window* offers = new Window(false);
    offers->SetLayoutType(LAYOUT_NONE);
    offers->SetSize(container->m_width / 2, container->m_height);
    m_moreGamesButton = CreateCustomButton(offers, true, "IDB_SHOP_ICON_PLAYHAVEN_WIDE",
        "IDB_WEAPON_SHOP_ICON_ACTIVE_GREEN", ResString("IDS_MENU_MORE_GAMES"), MoreGames);
    offers->AddToFront(CreateTapjoyWeaponButton(), 0, 1);
    container->Add(offers);
    App* app = static_cast<App*>(WindowApp::m_instance);
    int presentId = app->m_helper->m_config.GetIntegerValueByToken("GAME_UPDATE_WEAPON_PRESENT_ID", true);
    CDH_Weapon* weapon = static_cast<CDH_Weapon*>(app->InitWeaponManager()->firstItem());
    while (weapon != NULL) {
        bool listed = !weapon->IsPriceless() && !weapon->isHidden();
        if (listed && !((weapon->m_category & filter) != 0 || filter == 0 || filter == 0x3d)) {
            listed = false;
        }
        // The update present weapon only shows up once it is owned.
        if (listed && weapon->m_id == presentId && !weapon->isAvailable()) {
            listed = false;
        }
        if (listed) {
            Window* description = CreateWeaponDescription(weapon);
            description->SetPercentHeight(90, 0, 0);
            if (!container->Add(description)) {
                scroll->Add(container);
                int margin = SScreenAdaptingConstant(90, 180, 180)();
                int height = SScreenAdaptingConstant(230, 460, 550)();
                container = new CAutoArrangedWindow(m_width - margin, height);
                container->m_maxRows = 2;
                container->Add(description);
            }
        }
        weapon = static_cast<CDH_Weapon*>(app->InitWeaponManager()->nextItem());
    }
    scroll->Add(container);
}

CAutoArrangedWindow* CRegularShop::CreatePage(CScrollWindow* scroll, CItemManager* manager,
                                              CAutoArrangedWindow* container, int maxItems)
{
    if (manager->getItemsCount() == 0) {
        return NULL;
    }
    if (container == NULL) {
        int margin = SScreenAdaptingConstant(50, 100, 100)();
        container = CreateItemsContainer(m_width - margin);
    }
    int shown = 0;
    for (CItem* item = manager->firstItem(); item != NULL; item = manager->nextItem()) {
        if (item->isHidden()) {
            continue;
        }
        // maxItems > 0 limits the number of cards (e.g. one IAP pack beside the video offer).
        if (maxItems > 0 && shown == maxItems) {
            break;
        }
        if (manager->getItemsCount() <= shown) {
            break;
        }
        int price = item->m_pricePoints;
        int unlockLevel = item->m_unlockLevel;
        if (item->getPriceCredits() > 0) {
            price = item->getPriceCredits();
        }
        XString name = item->m_name;
        shown += 1;
        int priceIAP = item->m_priceIAP;
        item->getShopImage();
        int type = item->getType();
        if (priceIAP > 0) {
            price = priceIAP;
        }
        CShopItemButton* card;
        if (type == 0) {
            card = new CShopButtonWeapon(name, unlockLevel, price, item->m_id, item->getShopImageResId());
        } else {
            card = new CShopItemButton(manager, item->getType(), name, unlockLevel, price, item->m_id,
                                       item->getShopImageResId());
        }
        card->m_command = OpenTapjoy;
        card->m_commandParameter1 = reinterpret_cast<int>(card);
        card->m_commandParameter2 = 0;
        card->m_commandParameter3 = 0;
        if (!container->Add(card)) {
            int width = container->m_width;
            scroll->Add(container);
            container = CreateItemsContainer(width);
            container->Add(card);
        }
    }
    scroll->Add(container);
    return container;
}

void CRegularShop::UpdateAmmoPage()
{
    if (m_ammoPage != NULL) {
        m_ammoPage->FlushPages();
    }
    int margin = SScreenAdaptingConstant(50, 100, 100)();
    CAutoArrangedWindow* container = CreateItemsContainer(m_width - margin);
    CreateCustomButton(container, false, "IDB_SHOP_ICON_ADCOLONY", "IDB_SQUARE_SHOP_ICON_ACTIVE_GREEN",
                       ResString("IDS_AWARD_CREDITS").Upper(), WatchAd);
    App* app = static_cast<App*>(WindowApp::m_instance);
    container = CreatePage(m_ammoPage, app->InitIAPCurrencyManager(), container, 1);
    container = CreatePage(m_ammoPage, app->InitAmmoManager(), container, -1);
    CreatePage(m_ammoPage, reinterpret_cast<CItemManager*>(app->m_ammoPackManager), container, -1);
}

void CRegularShop::CreateOtherTab()
{
    Window* othersTab = new Window(false);
    Window* filters = new Window(false);
    filters->SetPercentWidth(20, 0, 0);
    filters->SetPercentHeight(100, 0, 0);
    filters->SetLayoutType(LAYOUT_SIMPLE);
    filters->SetAlign(9);
    const char* const filterTexts[] = { "IDS_FILTER_HEALTH", "IDS_FILTER_ARMOR", "IDS_FILTER_PILLS" };
    const char* const filterImages[] = { "IDB_SHOP_FILTER_HEALTH", "IDB_SHOP_FILTER_ARMOR", "IDB_SHOP_FILTER_PILLS" };
    const int filterBits[] = { 2, 4, 8 };
    for (int index = 0; index < 3; ++index) {
        CFilterButton* button = new CFilterButton(ResString(filterTexts[index]), filterImages[index], m_otherFilter);
        button->m_command = FilterOthers;
        button->m_commandParameter1 = filterBits[index];
        button->m_commandParameter2 = 0;
        button->m_commandParameter3 = 0;
        filters->AddToFront(button, 0, index);
    }
    othersTab->AddToFront(filters);
    m_othersPage = new CScrollWindow(true);
    othersTab->AddToFront(m_othersPage);
    m_othersPage->SetLayoutType(LAYOUT_SIMPLE);
    m_othersPage->SetAlign(0x12);
    m_othersPage->SetSize(SScreenAdaptingConstant(390, 790, 840)(), m_height - GetTabTop() - SScreenAdaptingConstant(51, 125)());
    m_othersPage->HandleUpdate(false);
    CreateOthersPage();
    m_tabIndices[4] = AddTab(ResString("IDS_TAB_OTHER"), othersTab, "SUR_TAB_OTHER", false, -1);  // 0x100
}

CAutoArrangedWindow* CRegularShop::CreateItemsContainer(int width)
{
    CAutoArrangedWindow* container = new CAutoArrangedWindow(width, ItemSpacing());
    container->m_maxRows = 2;
    container->SetLayoutType(LAYOUT_SIMPLE);
    container->SetAlign(0x24);
    return container;
}

void CRegularShop::CreateOthersPage()
{
    int margin = SScreenAdaptingConstant(175, 250, 250)();
    CAutoArrangedWindow* container = CreateItemsContainer(m_width - margin);
    CreateCustomButton(container, false, "IDB_SHOP_ICON_ADCOLONY", "IDB_SQUARE_SHOP_ICON_ACTIVE_GREEN",
                       ResString("IDS_AWARD_CREDITS").Upper(), WatchAd);
    App* app = static_cast<App*>(WindowApp::m_instance);
    container = CreatePage(m_othersPage, app->InitIAPCurrencyManager(), container, 1);
    // Each filter bit adds one item family; no bit set shows everything.
    if ((m_otherFilter & 0x10) != 0 || m_otherFilter == 0) {
        container = CreatePage(m_othersPage, app->InitMiscItemManager(), container, -1);
    }
    if ((m_otherFilter & 8) != 0 || m_otherFilter == 0) {
        container = CreatePage(m_othersPage, reinterpret_cast<CItemManager*>(app->m_goPillManager), container, -1);
    }
    if ((m_otherFilter & 2) != 0 || m_otherFilter == 0) {
        container = CreatePage(m_othersPage, reinterpret_cast<CItemManager*>(app->m_medicantManager), container, -1);
    }
    if ((m_otherFilter & 4) != 0 || m_otherFilter == 0) {
        CreatePage(m_othersPage, reinterpret_cast<CItemManager*>(app->m_armorManager), container, -1);
    }
}

void CRegularShop::SwitchWeaponFilter(int filter)
{
    m_weaponsPage->FlushPages();
    if ((m_weaponFilter & filter) == 0) {
        m_weaponFilter = m_weaponFilter | filter;
    } else {
        m_weaponFilter = m_weaponFilter & ~filter;
    }
    CreateWeaponsPage(m_weaponsPage, NULL, m_weaponFilter);
}

CAdColonyButton* CRegularShop::CreateCustomButton(Window* parent, bool notification, const char* image,
                                               const char* activeImage, const XString& caption, unsigned int command)
{
    int textSpacing;
    if (notification) {
        textSpacing = SScreenAdaptingConstant(-17, -35)();
    } else {
        textSpacing = SScreenAdaptingConstant(2, 5, 5)();
    }
    CAdColonyButton* button = new CAdColonyButton(image, activeImage, caption, notification, textSpacing, 0);
    button->m_command = command;
    if (notification) {
        // Wide "more games" banner fills a whole row.
        parent->AddToFront(button, 0, 0);
        button->SetPercentHeight(SScreenAdaptingConstant(75, 75, 20)(), 0, 0);
    } else {
        static_cast<CAutoArrangedWindow*>(parent)->Add(button);
    }
    return button;
}

CShopItemButton* CRegularShop::CreateTapjoyWeaponButton()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CIAPCurrencyManager* manager = app->InitIAPCurrencyManager();
    CIAPCurrency* pack = static_cast<CIAPCurrency*>(manager->getItemById(0));
    if (pack == NULL || !(pack->m_type == L"tapjoy") || pack->isHidden()) {
        return NULL;
    }
    CShopItemButton* button = new CShopItemButton(manager, 8, pack->GetName(), pack->m_unlockLevel,
                                                  0, pack->m_id, "IDB_SHOP_ICOM_TAPJOY_WEAPON");
    button->m_command = OpenTapjoy;
    button->SetAlwaysActive(true);
    return button;
}

void CRegularShop::TryOpenTapJoy()
{
    if (TapjoyInterface::canDisplayInterface()) {
        TapjoyInterface::openTapjoyInterface();
        return;
    }
    CDialogWindow* warning = new CDialogWindow(true);
    warning->SetTitleText(ResString("IDS_DIALOG_TITLE_WARNING"));
    warning->SetMessageText(ResString("IDS_DIALOG_CAN_NOT_OPEN_TAPJOY"));
    warning->SetButtons(CDialogWindow::BUTTON_OK);
    AddModal(warning);
}

void CRegularShop::ShowItem(CItem* item)
{
    if (item == NULL) {
        return;
    }
    int type = item->getType();
    if (type == 5) {
        g_pendingCurrency = item;
        ShowCurrencyDetails(static_cast<CIAPCurrency*>(item), ConfirmPurchase);
        return;
    }
    CItemManager* manager = App::GetItemManagerByType(type);
    if (manager == NULL) {
        return;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    CShopItemInfoWindow* details = new CShopItemInfoWindow(manager, type, item->m_id, item->GetStatsString(),
                                                           false, CShopItemInfoWindow::SOURCE_SHOP);
    app->m_mainLayer->AddModal(details);
}

void CRegularShop::CloseShop()
{
    if (CTutorialManager::CanCloseShop()) {
        OnLostFocus();
        Close();
        WindowApp::HandleTunnelCommand(0x4d5de318, 0, 0, 0);
        WindowApp::HandleTunnelCommand(0x703ba476, 0, 0, 0);
    } else {
        WindowApp::HandleTunnelCommand(-0x21e08bf9, 0, 0, 0);
    }
}

void CRegularShop::OnCommand(Event& event)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    int id = event.m_id;
    if (id == FilterOthers) {
        m_othersPage->FlushPages();
        if ((m_otherFilter & event.m_param1) == 0) {
            m_otherFilter = m_otherFilter | event.m_param1;
        } else {
            m_otherFilter = m_otherFilter & ~event.m_param1;
        }
        CreateOthersPage();
    } else if (id == AmmoChanged) {
        UpdateAmmoPage();
    } else if (id == TabLockChanged) {
        UpdateTabLock();
    } else if (id == IAPPurchaseComplete) {
        if (m_waitDialog != NULL) {
            m_waitDialog->Close();
        }
        CIAPCurrency* item = reinterpret_cast<CIAPCurrency*>(event.m_param1);
        CDialogWindow* dialog = new CDialogWindow(true);
        dialog->SetMessageText(ResString("IDS_SHOP_IAP_SUCCESS") + item->GetName());
        dialog->SetButtons(CDialogWindow::BUTTON_OK);
        app->m_mainLayer->AddModal(dialog);
    } else if (id == IAPInfoRequested) {
        if (app->InitIAPCurrencyManager()->IsRetryWaiting()) {
            UpdateCurrenciesPage();
        } else {
            app->InitIAPCurrencyManager()->UpdateIAPInfo(false);
            event.Clear();
        }
    } else if (id == CurrencyUpdated) {
        UpdateCurrenciesPage();
    } else if (id == FilterWeapons) {
        SwitchWeaponFilter(event.m_param1);
    } else if (id == CloseRequested) {
        CloseShop();
    } else if (id == IAPPurchaseCancelled) {
        if (m_waitDialog != NULL) {
            m_waitDialog->Close();
        }
        CDialogWindow* dialog = new CDialogWindow(true);
        dialog->SetTitleText(ResString("IDS_SHOP_IAP_ERROR_TITLE"));
        dialog->SetMessageText(ResString("IDS_SHOP_IAP_ERROR"));
        dialog->SetButtons(CDialogWindow::BUTTON_OK);
        app->m_mainLayer->AddModal(dialog);
    } else if (id == IAPPurchaseFailed) {
        CDialogWindow* dialog = new CDialogWindow(true);
        dialog->SetTitleText(ResString("IDS_SHOP_IAP_ERROR_TITLE"));
        dialog->SetMessageText(ResString("IDS_SHOP_IAP_ERROR"));
        dialog->SetButtons(CDialogWindow::BUTTON_OK);
        app->m_mainLayer->AddModal(dialog);
        if (m_waitDialog != NULL) {
            m_waitDialog->Close();
        }
    } else if (id == WatchAd) {
        AddModal(new CAdColonyWindow(false));
    } else if (id == MoreGames) {
        CPHInterface::startPublisherContentRequest(MoreGamesPlacement, false, true);
    } else if (id == OpenTapjoy) {
        // A shop card was pressed: event.m_param1 is the card itself.
        CShopItemButton* card = reinterpret_cast<CShopItemButton*>(event.m_param1);
        if (!card->m_disabled) {
            OpenCard(card);
        }
    }
    CBaseScreen::OnCommand(event);
}

void CRegularShop::OpenCard(CShopItemButton* card)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    int type = card->m_type;
    int itemId = card->m_itemId;
    if (type == 5) {
        CIAPCurrency* currency = static_cast<CIAPCurrency*>(app->InitIAPCurrencyManager()->getItemById(itemId));
        if (currency->m_type == XString("tapjoy")) {
            TryOpenTapJoy();
            return;
        }
        if (!currency->isAvailable()) {
            app->InitIAPCurrencyManager()->UpdateIAPInfo(true);
            UpdateCurrenciesPage();
            return;
        }
        ICInAppPurchase* store = ICInAppPurchase::GetInstance();
        CDialogWindow* wait = new CDialogWindow(true);
        wait->SetTitleText(ResString("IDS_SHOP_IAP_LOADING_TITLE"));
        wait->SetMessageText(ResString("IDS_SHOP_IAP_LOADING_MESSAGE"));
        app->m_mainLayer->AddModal(wait);
        m_waitDialog = wait;
        CGameAnalytics::logIAPBuyClicked(XString::Format(L"%02d_%ls", itemId,
            app->InitIAPCurrencyManager()->getItemById(itemId)->m_name.c_str()));
        CIAPCurrency* product = static_cast<CIAPCurrency*>(app->InitIAPCurrencyManager()->getItemById(itemId));
        store->BuyProduct(CStrWChar(product->m_appleId.c_str()));
        return;
    }
    CItemManager* manager = App::GetItemManagerByType(type);
    if (manager == NULL) {
        return;
    }
    CItem* item = manager->getItemById(itemId);
    CShopItemInfoWindow* details = new CShopItemInfoWindow(manager, type, itemId, item->GetStatsString(),
                                                           false, CShopItemInfoWindow::SOURCE_SHOP);
    AddModal(details);
}

Window* CRegularShop::CreateWeaponDescription(CDH_Weapon* weapon)
{
    Window* description = new Window(false);
    // Price shown on the card: points, overridden by credits, overridden by IAP.
    int price = weapon->m_pricePoints;
    if (weapon->getPriceCredits() > 0) {
        price = weapon->getPriceCredits();
    }
    XString name = weapon->m_name;
    int priceIAP = weapon->m_priceIAP;
    weapon->getShopImage();
    if (priceIAP > 0) {
        price = priceIAP;
    }
    CShopButtonWeapon* card = new CShopButtonWeapon(name, weapon->m_unlockLevel, price, weapon->m_id,
                                                    weapon->getShopImageResId());
    card->m_command = OpenTapjoy;
    card->m_commandParameter1 = reinterpret_cast<int>(card);
    card->m_commandParameter2 = 0;
    card->m_commandParameter3 = 0;
    description->AddToFront(card, 0, 0);
    description->AddToFront(CreateWeaponStats(weapon), 0, 1);
    description->SetSize(card->m_width, ItemSpacing());
    return description;
}

Window* CRegularShop::CreateWeaponStats(CDH_Weapon* weapon)
{
    Window* stats = new Window(false);
    Window* rows = new Window(false);
    rows->SetLayoutType(LAYOUT_SIMPLE);
    rows->SetAlign(0xc);
    rows->SetPercentHeight(100, 0, 0);
    rows->SetPercentWidth(80, 0, 0);
    App* app = static_cast<App*>(WindowApp::m_instance);

    AddIconDescription(rows, "SUR_ICON_DRIFT", ResString("IDS_ACCURACY_NAME"), 0, 0);
    float wander = weapon->wanderAmount();
    float maxWander = (float)app->InitWeaponManager()->m_maxWanderRounded;
    int accuracy = (int)(wander / maxWander * -100.0f + 104.0f);
    AddStatsBar(rows, accuracy, 100, 1, 0, false);

    AddIconDescription(rows, "SUR_ICON_ACCURACY", ResString("IDS_RANGE_NAME"), 0, 1);
    int range = (int)weapon->effectiveRange();
    AddStatsBar(rows, range, app->InitWeaponManager()->m_maxRange, 1, 1, false);

    AddIconDescription(rows, "SUR_ICON_DAMAGE", ResString("IDS_DAMAGE_NAME"), 0, 2);
    int damage = weapon->damage();
    AddStatsBar(rows, damage, app->InitWeaponManager()->m_maxDamage, 1, 2, false);

    AddIconDescription(rows, "SUR_ICON_RATE_OF_FIRE", ResString("IDS_RATE_NAME"), 0, 3);
    int rpm = weapon->getRPM();
    AddStatsBar(rows, rpm, app->InitWeaponManager()->m_maxRPM, 1, 3, false);

    AddIconDescription(rows, "SUR_ICON_CLIP", ResString("IDS_CLIP_NAME"), 0, 4);
    // Infinite clip when any compatible ammo is "infinite" or nothing is loaded.
    Vector<const CAmmo*> ammo = weapon->m_compatibleAmmo;
    bool infinite = false;
    for (int index = 0; index < ammo.size(); ++index) {
        if (ammo[index]->m_ammoType == XString("infinite")) {
            infinite = true;
        }
    }
    const CAmmo* active = weapon->GetActiveAmmo();
    int clip = weapon->m_clipSize;
    int maxClip = app->InitWeaponManager()->m_maxClip;
    bool showInfinite = false;
    if (active == NULL || infinite) {
        showInfinite = true;
    }
    AddStatsBar(rows, clip, maxClip, 1, 4, showInfinite);
    stats->AddToFront(rows);
    return stats;
}

void CRegularShop::AddIconDescription(Window* parent, const char* icon, const XString& text, int column, int row)
{
    Window* cell = new Window(false);
    cell->SetPercentHeight(100, 0, 0);
    cell->SetPercentWidth(100, 0, 0);
    ImageWindow* image = new ImageWindow(static_cast<ImageWindow::EFillType>(0), ImageRes(icon));
    image->SetLayoutType(LAYOUT_SIMPLE);
    image->SetAlign(0x24);
    cell->AddToFront(image, 0, 0);
    TextWindow* label = new TextWindow(text, CFontMgr::GetInstance()->GetFont(15));
    label->SetLayoutType(LAYOUT_SIMPLE);
    label->SetAlign(0x21);
    cell->AddToFront(label, 1, 0);
    parent->AddToFront(cell, column, row);
}

void CRegularShop::AddStatsBar(Window* parent, int value, int maximum, int column, int row, bool infinite)
{
    CBH_ProgressBar* bar = new CBH_ProgressBar(3, App::Scale() * 90, -1);
    bar->m_displayFlags = 1;
    bar->SetMaxValue(maximum);
    bar->SetCurrValue(value);
    bar->SetPercentWidth(80, 0, 0);
    bar->SetOutsetSpacing(App::Scale() * 4, 0, 0, 0);
    if (infinite == 1) {
        bar->SetText(XString(L"\u221e"));
    }
    parent->AddToFront(bar, column, row);
}

CFilterButton::CFilterButton(const XString& text, const char* image, const int& filterMask)
    : CImageButton(), m_filterMask(&filterMask)
{
    AddImage(image, 0, 0);
    SetText(text, 0x24, CFontMgr::GetInstance()->GetFont(15), 0, 0);
}

void CFilterButton::Paint(ICGraphics2d& graphics)
{
    WindowApp* app = WindowApp::m_instance;
    app->PopOpacity();
    // Full opacity when this button's bit (command parameter 1) is in the mask.
    if ((*m_filterMask & m_commandParameter1) != 0) {
        app->PushOpacity(1.0f);
    } else {
        app->PushOpacity(0.5f);
    }
}
