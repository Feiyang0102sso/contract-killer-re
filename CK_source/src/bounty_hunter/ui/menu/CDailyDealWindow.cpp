#include "CDailyDealWindow.h"

#include "App.h"
#include "CChallengeWindow.h"
#include "CDailyDealManager.h"
#include "CImageButton.h"
#include "CRegularShop.h"
#include "CShopItem.h"
#include "CShopItemInfoWindow.h"
#include "SScreenAdaptingConstant.h"
#include "TapjoyInterface.h"

namespace {
const int BuyDeal = 0x0027775d;
const int CloseDeal = 0x004004a3;
const int OpenBank = 0x02e7d3e7;
const int OpenTapjoy = 0x02e7d44c;

App* GetApp()
{
    return static_cast<App*>(WindowApp::m_instance);
}
}

CDailyDealWindow::CDailyDealWindow()
{
    CreateUI();
}

void CDailyDealWindow::CreateUI()
{
    CItem* item = GetApp()->InitDailyDealManager()->GetCurrentDealItem();

    ImageWindow* background = new ImageWindow();
    background->m_debugFrame = true;
    background->SetImageById("IDB_SHOP_BACKGROUND");
    background->SetFillType(ImageWindow::FILL_NONE);
    background->SetLayoutType(LAYOUT_SIMPLE);
    background->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
    AddToFront(background);

    CImageButton* closeButton = new CImageButton();
    int closeTop = SScreenAdaptingConstant(12, 24)();
    int closeRight = SScreenAdaptingConstant(35, 70)();
    closeButton->SetOutsetSpacing(closeTop, closeRight, 0, 0);
    closeButton->AddImage("IDB_CLOSE_ICON", 0, 0);
    closeButton->SetLayoutType(LAYOUT_SIMPLE);
    closeButton->SetAlign(ALIGN_RIGHT | ALIGN_TOP);
    closeButton->SetCommandParameters(CloseDeal, 0, 0, 0);
    AddToFront(closeButton);

    Window* content = new Window(false);
    content->m_debugFrame = true;
    content->SetOutsetSpacing(SScreenAdaptingConstant(25, 50)(), 0, 0, 0);
    content->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
    content->SetPercentHeight(80, 0, 0);
    content->SetPercentWidth(100, 0, 0);
    AddToFront(content);

    TextWindow* heading = new TextWindow(ResString("IDS_DAILY_DEAL"), CFontMgr::GetInstance()->GetFont(10));
    heading->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
    content->AddToFront(heading, 0, 0);
    heading->SetPercentHeight(SScreenAdaptingConstant(3, 2)(), 0, 0);
    TextWindow* itemName = new TextWindow(item->GetName(), CFontMgr::GetInstance()->GetFont(9));
    itemName->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
    content->AddToFront(itemName, 0, 1);
    itemName->SetPercentHeight(SScreenAdaptingConstant(3, 2)(), 0, 0);
    Window* spacing = new Window(false);
    spacing->m_debugFrame = true;
    spacing->SetPercentHeight(7, 0, 0);
    content->AddToFront(spacing, 0, 2);

    Window* picture = new Window(false);
    content->AddToFront(picture);
    Window* pictureSpacing = new Window(false);
    picture->AddToFront(pictureSpacing, 0, 0);
    pictureSpacing->SetPercentHeight(9, 0, 0);
    ImageWindow* itemImage = new ImageWindow();
    itemImage->SetFillType(ImageWindow::FILL_NONE);
    itemImage->SetImageById(item->getBigImageResId());
    itemImage->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
    picture->AddToFront(itemImage, 0, 1);

    Window* priceRow = new Window(false);
    priceRow->m_debugFrame = true;
    priceRow->SetPercentHeight(5, 0, 0);
    Window* prices = CreateItemPriceWindow();
    prices->SetPercentWidth(60, 0, 0);
    prices->SetPercentHeight(100, 0, 0);
    prices->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    priceRow->AddToFront(prices);
    content->AddToFront(priceRow, 0, 3);

    Window* timerRow = new Window(false);
    Window* timer = CreateTimerWindow();
    timer->SetPercentWidth(40, 0, 0);
    timer->SetPercentHeight(100, 0, 0);
    timer->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
    timerRow->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
    content->AddToFront(timerRow, 0, 4);
    timerRow->SetPercentHeight(2, 0, 0);
    timerRow->m_debugFrame = true;
    timerRow->AddToFront(timer);
    Window* footerSpacing = new Window(false);
    content->AddToFront(footerSpacing, 0, 5);
    footerSpacing->SetPercentHeight(3, 0, 0);

    Window* footer = new Window(false);
    footer->SetPercentHeight(20, 0, 0);
    footer->SetPercentWidth(100, 0, 0);
    footer->SetAlign(ALIGN_HCENTER | ALIGN_BOTTOM);
    footer->SetOutsetSpacing(0, 0, SScreenAdaptingConstant(40, 80)(), 0);
    AddToFront(footer);
    CImageButton* buyButton = new CImageButton();
    buyButton->m_debugFrame = true;
    buyButton->AddImage("IDB_BUY_NOW_BUTTON", 0, 0);
    buyButton->SetText(XString("BUY NOW!"), ALIGN_HCENTER | ALIGN_VCENTER, NULL, 0, 0);
    buyButton->SetCommandParameters(BuyDeal, 0, 0, 0);
    footer->AddToFront(buyButton);
    buyButton->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    m_debugFrame = true;
}

Window* CDailyDealWindow::CreateItemPriceWindow()
{
    CItem* item = GetApp()->InitDailyDealManager()->GetCurrentDealItem();
    Window* price = new Window(false);
    Window* priceRows = new Window(false);
    priceRows->SetPercentHeight(100, 0, 0);
    priceRows->SetPercentWidth(65, 0, 0);
    price->AddToFront(priceRows, 0, 0);

    Window* oldPriceRow = new Window(false);
    priceRows->AddToFront(oldPriceRow, 0, 0);
    TextWindow* oldPriceLabel = new TextWindow(ResString("IDS_OLD_PRICE"), CFontMgr::GetInstance()->GetFont(9));
    oldPriceLabel->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    oldPriceRow->AddToFront(oldPriceLabel, 0, 0);
    int oldCredits = item->m_priceCredits;
    Window* oldPrice = CreatePriceCountWindow(oldCredits, CFontMgr::GetInstance()->GetFont(9));
    oldPrice->SetPercentWidth(35, 0, 0);
    oldPrice->SetAlign(ALIGN_LEFT | ALIGN_VCENTER);
    ImageWindow* strike = new ImageWindow();
    strike->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    strike->SetFillType(ImageWindow::FILL_NONE);
    strike->SetImageById("IDB_OLD_PRICE");
    oldPrice->AddToFront(strike);
    oldPriceRow->AddToFront(oldPrice, 1, 0);

    Window* newPriceRow = new Window(false);
    priceRows->AddToFront(newPriceRow, 0, 1);
    TextWindow* newPriceLabel = new TextWindow(ResString("IDS_NEW_PRICE"), CFontMgr::GetInstance()->GetFont(9));
    newPriceLabel->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    newPriceRow->AddToFront(newPriceLabel, 0, 0);
    SDailyDeal* deal = GetApp()->InitDailyDealManager()->GetCurrentDailyDeal();
    int newCredits = deal->m_priceCredits;
    Window* newPrice = CreatePriceCountWindow(newCredits, CFontMgr::GetInstance()->GetFont(10));
    newPrice->SetPercentWidth(35, 0, 0);
    newPrice->SetAlign(ALIGN_LEFT | ALIGN_VCENTER);
    newPriceRow->AddToFront(newPrice, 1, 0);

    Window* discount = new Window(false);
    discount->SetPercentWidth(35, 0, 0);
    discount->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    price->AddToFront(discount, 1, 0);
    ImageWindow* discountBackground = new ImageWindow();
    discountBackground->SetImageById("IDB_GREEN_FIELD");
    discountBackground->SetFillType(ImageWindow::FILL_NONE);
    discountBackground->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    discount->AddToFront(discountBackground);
    TextWindow* discountText = new TextWindow(deal->m_text, CFontMgr::GetInstance()->GetFont(9));
    discountText->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    discount->AddToFront(discountText);
    return price;
}

Window* CDailyDealWindow::CreateTimerWindow()
{
    Window* row = new Window(false);
    TextWindow* label = new TextWindow(ResString("IDS_OFFER_ENDS"), CFontMgr::GetInstance()->GetFont(15));
    label->SetAlign(ALIGN_RIGHT | ALIGN_VCENTER);
    row->AddToFront(label, 0, 0);
    Window* timerCell = new Window(false);
    CChallengeTimerWindow* timer = new CChallengeTimerWindow(
        GetApp()->InitDailyDealManager()->GetNextDealStartTime(), CFontMgr::GetInstance()->GetFont(15));
    timer->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    timer->m_useChallengeTime = false;
    timerCell->AddToFront(timer);
    row->AddToFront(timerCell, 1, 0);
    return row;
}

void CDailyDealWindow::OnCommand(Event& event)
{
    int command = event.m_id;
    if (command < CloseDeal) {
        if (command == BuyDeal) {
            HandleBuyButtonPressed();
        }
        return;
    }

    if (command == OpenTapjoy) {
        if (TapjoyInterface::canDisplayInterface()) {
            TapjoyInterface::openTapjoyInterface();
        } else {
            CDialogWindow* warning = new CDialogWindow(true);
            warning->SetTitleText(ResString("IDS_DIALOG_TITLE_WARNING"));
            warning->SetMessageText(ResString("IDS_DIALOG_CAN_NOT_OPEN_TAPJOY"));
            warning->SetButtons(BUTTON_OK);
            AddModal(warning);
        }
    } else if (command == OpenBank) {
        CRegularShop* shop = CRegularShop::GetInstance(true);
        shop->SetTab(CShopWindow::TAB_BANK);
        Window* parent = shop->m_parent;
        if (parent != NULL) {
            parent->Remove(shop);
        }
        GetApp()->m_mainLayer->AddToFront(shop);
    } else {
        if (command == CloseDeal) {
            Close();
        }
        return;
    }
    event.Clear();
    Close();
}

void CDailyDealWindow::HandleBuyButtonPressed()
{
    CItem* current = GetApp()->InitDailyDealManager()->GetCurrentDealItem();
    if (current == NULL) {
        return;
    }
    CItemManager* manager = App::GetItemManagerByType(current->getType());
    CRegularShop* shop = CRegularShop::GetInstance(true);
    CShopWindow::ETab tab = CShopWindow::TAB_BANK;
    switch (current->getType()) {
    case 2:
    case 9:
        tab = CShopWindow::TAB_AMMO;
        break;
    case 0:
        tab = CShopWindow::TAB_WEAPONS;
        break;
    case 1:
    case 3:
    case 10:
    case 11:
    case 12:
        tab = CShopWindow::TAB_OTHER;
        break;
    case 4:
        tab = CShopWindow::TAB_ENERGY;
        break;
    }
    shop->SetTab(tab);
    GetApp()->m_mainLayer->AddModal(shop);
    CShopItemInfoWindow* details = new CShopItemInfoWindow(manager, 0, current->m_id, current->GetStatsString(),
                                                          false, CShopItemInfoWindow::SOURCE_SHOP);
    GetApp()->m_mainLayer->AddModal(details);
    Close();
}

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char CKDailyDealWindowSize[sizeof(CDailyDealWindow) == 0x120 ? 1 : -1];
#endif

Window* CDailyDealWindow::CreatePriceCountWindow(int price, CFont* font)
{
    Window* row = new Window(false);
    ImageWindow* icon = new ImageWindow();
    icon->SetFillType(ImageWindow::FILL_NONE);
    icon->SetImageById("IDB_HUD_ICON_CREDITS");
    icon->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    row->AddToFront(icon, 0, 0);
    TextWindow* count = new TextWindow(XString(price), font);
    count->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    row->AddToFront(count, 1, 0);
    return row;
}
