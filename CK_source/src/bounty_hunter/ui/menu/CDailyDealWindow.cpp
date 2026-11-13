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
}

CDailyDealWindow::CDailyDealWindow()
{
    CreateUI();
}

void CDailyDealWindow::CreateUI()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDailyDealManager* manager = app->InitDailyDealManager();
    CItem* item = manager->GetCurrentDealItem();
    if (item == NULL || manager->GetCurrentDailyDeal() == NULL) {
        SetTitleText("IDS_DAILY_DEAL");
        SetButtons(BUTTON_OK);
        return;
    }

    ImageWindow* background = new ImageWindow();
    background->m_debugFrame = true;
    background->SetImageById("IDB_SHOP_BACKGROUND");
    background->SetFillType(ImageWindow::FILL_NONE);
    background->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
    AddToFront(background);

    CImageButton* closeButton = new CImageButton();
    const SScreenAdaptingConstant closeTop(12, 24);
    const SScreenAdaptingConstant closeRight(35, 70);
    closeButton->SetOutsetSpacing(closeTop(), closeRight(), 0, 0);
    closeButton->AddImage("IDB_CLOSE_ICON", 0, 0);
    closeButton->SetAlign(ALIGN_RIGHT | ALIGN_TOP);
    closeButton->m_command = CloseDeal;
    AddToFront(closeButton);

    Window* content = new Window(false);
    content->m_debugFrame = true;
    const SScreenAdaptingConstant contentTop(25, 50);
    content->SetOutsetSpacing(contentTop(), 0, 0, 0);
    content->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
    content->SetPercentHeight(80, 0, 0);
    content->SetPercentWidth(100, 0, 0);
    AddToFront(content);

    CFontMgr* fonts = CFontMgr::GetInstance();
    const SScreenAdaptingConstant headingHeight(3, 2);
    TextWindow* heading = new TextWindow(ResString("IDS_DAILY_DEAL"), fonts->GetFont(10));
    heading->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
    content->AddToFront(heading, 0, 0);
    heading->SetPercentHeight(headingHeight(), 0, 0);
    TextWindow* itemName = new TextWindow(item->GetName(), fonts->GetFont(9));
    itemName->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
    content->AddToFront(itemName, 0, 1);
    itemName->SetPercentHeight(headingHeight(), 0, 0);
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
    const SScreenAdaptingConstant footerBottom(40, 80);
    footer->SetOutsetSpacing(0, 0, footerBottom(), 0);
    AddToFront(footer);
    CImageButton* buyButton = new CImageButton();
    buyButton->m_debugFrame = true;
    buyButton->AddImage("IDB_BUY_NOW_BUTTON", 0, 0);
    buyButton->SetText(XString("BUY NOW!"), ALIGN_HCENTER | ALIGN_VCENTER, NULL, 0, 0);
    buyButton->m_command = BuyDeal;
    footer->AddToFront(buyButton);
    buyButton->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    m_debugFrame = true;
}

Window* CDailyDealWindow::CreateItemPriceWindow()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDailyDealManager* manager = app->InitDailyDealManager();
    CItem* item = manager->GetCurrentDealItem();
    SDailyDeal* deal = manager->GetCurrentDailyDeal();
    Window* price = new Window(false);
    if (item == NULL || deal == NULL) {
        return price;
    }
    CFontMgr* fonts = CFontMgr::GetInstance();
    CFont* captionFont = fonts->GetFont(9);
    Window* priceRows = new Window(false);
    priceRows->SetPercentHeight(100, 0, 0);
    priceRows->SetPercentWidth(65, 0, 0);
    price->AddToFront(priceRows, 0, 0);

    Window* oldPriceRow = new Window(false);
    priceRows->AddToFront(oldPriceRow, 0, 0);
    TextWindow* oldPriceLabel = new TextWindow(ResString("IDS_OLD_PRICE"), captionFont);
    oldPriceLabel->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    oldPriceRow->AddToFront(oldPriceLabel, 0, 0);
    Window* oldPrice = CreatePriceCountWindow(item->m_priceCredits, captionFont);
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
    TextWindow* newPriceLabel = new TextWindow(ResString("IDS_NEW_PRICE"), captionFont);
    newPriceLabel->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    newPriceRow->AddToFront(newPriceLabel, 0, 0);
    Window* newPrice = CreatePriceCountWindow(deal->m_priceCredits, fonts->GetFont(10));
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
    TextWindow* discountText = new TextWindow(deal->m_text, captionFont);
    discountText->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    discount->AddToFront(discountText);
    return price;
}

Window* CDailyDealWindow::CreateTimerWindow()
{
    Window* row = new Window(false);
    CFont* font = CFontMgr::GetInstance()->GetFont(15);
    TextWindow* label = new TextWindow(ResString("IDS_OFFER_ENDS"), font);
    label->SetAlign(ALIGN_RIGHT | ALIGN_VCENTER);
    row->AddToFront(label, 0, 0);
    Window* timerCell = new Window(false);
    App* app = static_cast<App*>(WindowApp::m_instance);
    unsigned int end = app->InitDailyDealManager()->GetNextDealStartTime();
    CChallengeTimerWindow* timer = new CChallengeTimerWindow(end, font);
    timer->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    timer->m_useChallengeTime = false;
    timerCell->AddToFront(timer);
    row->AddToFront(timerCell, 1, 0);
    return row;
}

void CDailyDealWindow::OnCommand(Event& event)
{
    if (event.m_id == BuyDeal) {
        HandleBuyButtonPressed();
    } else if (event.m_id == OpenBank) {
        CRegularShop::Show(CShopWindow::TAB_BANK);
        Close();
    } else if (event.m_id == CloseDeal) {
        Close();
    } else if (event.m_id == OpenTapjoy) {
        if (TapjoyInterface::canDisplayInterface()) {
            TapjoyInterface::openTapjoyInterface();
            Close();
        } else {
            CDialogWindow* warning = new CDialogWindow(true);
            warning->SetTitleText("IDS_DIALOG_TITLE_WARNING");
            warning->SetMessageText(ResString("IDS_DIALOG_CAN_NOT_OPEN_TAPJOY"));
            warning->SetButtons(BUTTON_OK);
            AddModal(warning);
        }
    } else {
        CDialogWindow::OnCommand(event);
        return;
    }
    event.Clear();
}

void CDailyDealWindow::HandleBuyButtonPressed()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CItem* current = app->InitDailyDealManager()->GetCurrentDealItem();
    if (current == NULL) {
        return;
    }
    CItemManager* manager = App::GetItemManagerByType(current->getType());
    CRegularShop* shop = CRegularShop::GetInstance(true);
    CShopWindow::ETab tab = CShopWindow::TAB_BANK;
    switch (current->getType()) {
    case 0:
        tab = CShopWindow::TAB_WEAPONS;
        break;
    case 2:
    case 9:
        tab = CShopWindow::TAB_AMMO;
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
    app->m_mainLayer->AddModal(shop);
    CShopItemInfoWindow* details = new CShopItemInfoWindow(manager, 0, current->m_id, current->GetStatsString(),
                                                          false, CShopItemInfoWindow::SOURCE_SHOP);
    app->m_mainLayer->AddModal(details);
    Close();
}

typedef char CKDailyDealWindowSize[sizeof(CDailyDealWindow) == 0x120 ? 1 : -1];

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
