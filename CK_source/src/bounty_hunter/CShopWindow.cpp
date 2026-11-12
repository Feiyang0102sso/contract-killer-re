#include "CShopWindow.h"
#include "CBH_GPSMap.h"
#include "CBH_SurfaceManager.h"
#include "CPHInterface.h"
#include "CTabWindow.h"
#include "CTutorialManager.h"
#include "ICAdManager.h"
#include "SScreenAdaptingConstant.h"
#include <string.h>

#include "App.h"
#include "CDialogWindow.h"
#include "CFontMgr.h"
#include "CGameAnalytics.h"
#include "CIAPCurrencyManager.h"
#include "ICInAppPurchase.h"
#include "ServiceWindows.h"

namespace {
const int ShopClosedTunnel = 0x703ba476;
const SScreenAdaptingConstant TabsHeight(290, 585, 710, 435);
const SScreenAdaptingConstant UnusedConstant50(220, 440, 550, 345);
const SScreenAdaptingConstant UnusedConstant60(40, 80);
}

CShopWindow::CShopWindow() : CBaseScreen(false, false, true, false, true, false), m_unknownEc(false)
{
    SetTitleText(ResString("IDS_TITLE_SHOP"));
    m_tabs = new CTabWindow(true);
    WindowApp* app = WindowApp::m_instance;
    int screenHeight = app->m_screenHeight;
    int tabsHeight = TabsHeight();
    int screenWidth = app->m_screenWidth;
    m_tabs->SetRect(0, screenHeight - tabsHeight, screenWidth, TabsHeight(), true);
    AddToFront(m_tabs);
    m_honeycombTop.Create("SUR_MENU_HONEYCOMD_UP");
    m_honeycombBottom.Create("SUR_MENU_HONEYCOMD_BOTTOM");
    memset(m_tabIndices, 0xff, sizeof(m_tabIndices));
}

CShopWindow::~CShopWindow()
{
    static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->ReleaseSurface("IDB_SHOP_ICOM_TAPJOY_WEAPON");
    CBH_GPSMap::shopIsPainting = false;
}

void CShopWindow::OnClosed()
{
    m_unknownEc = false;
    WindowApp::HandleTunnelCommand(ShopClosedTunnel, 0, 0, 0);
}

void CShopWindow::Paint(ICGraphics2d& graphics)
{
    DrawTileImageHoriz(graphics, m_honeycombTop.GetSurface(), 0, 0, m_width);
}

void CShopWindow::AddModal(Window* window)
{
    ICAdManager::GetInstance()->HideAd();
    m_unknownEc = false;
    Window::AddModal(window);
}

void CShopWindow::OnEnterForeground()
{
    static_cast<App*>(WindowApp::m_instance)->InitIAPCurrencyManager()->UpdateIAPInfo(true);
    CBH_GPSMap::shopIsPainting = true;
}

int CShopWindow::AddTab(XString name, Window* window, const char* image, bool locked, int padding)
{
    int index = m_tabs->GetTabCount();
    m_tabs->AddTab(name, window, image, locked, padding);
    return index;
}

Window* CShopWindow::AddShopButton(Window* parent, const XString& text, int x, int y,
                                  int width, int height, int command, int argument)
{
    StretchImageButton* button = new StretchImageButton(text, "IDB_HUD_TRANSPARENT_BLACK_LEFT",
                                                        "IDB_HUD_TRANSPARENT_BLACK_TILE", "IDB_HUD_TRANSPARENT_BLACK_RIGHT");
    button->ForceSetWidth(width);
    button->SetLayoutType(LAYOUT_NONE);
    button->SetRect(x, y, width, height, false);
    button->m_command = command;
    button->m_commandParameter1 = argument;
    button->m_commandParameter2 = 0;
    button->m_commandParameter3 = 0;
    parent->AddToFront(button);
    return button;
}

void CShopWindow::SetTab(int tab)
{
    if ((unsigned int)tab > TAB_COUNT) {
        return;
    }
    int index = m_tabIndices[tab];
    if (index == -1) {
        return;
    }
    // Opening the bank tab asks PlayHaven for its placement (not during the tutorial).
    if (index == 0 && !CTutorialManager::IsPlayerInTutorial()) {
        CPHInterface::startPublisherContentRequest("bank_launch", false, false);
    }
    m_tabs->SetCurrTab(index, false);
}

int CShopWindow::GetTab()
{
    int current = m_tabs->m_currentTab;
    for (int tab = 0; tab <= TAB_COUNT; ++tab) {
        if (current == m_tabIndices[tab]) {
            return tab;
        }
    }
    return -1;
}

void CShopWindow::SetTabLock(int tab, bool locked)
{
    if ((unsigned int)tab > TAB_COUNT) {
        return;
    }
    if (m_tabIndices[tab] != -1) {
        m_tabs->SetTabLock(m_tabIndices[tab], locked);
    }
}

int CShopWindow::GetTabLeft()
{
    return m_tabs->m_x;
}

int CShopWindow::GetTabTop()
{
    return m_tabs->m_y;
}

void CShopWindow::RefreshCurrencyProducts()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    ICInAppPurchase* store = ICInAppPurchase::GetInstance();
    CIAPCurrencyManager* catalog = app->InitIAPCurrencyManager();
    if (catalog->GetAvailableItemsCount() == 0 && store->IsAvailable() && !store->IsTransactionActive()) {
        catalog->UpdateIAPInfo(true);
    }
}

CIAPCurrency* CShopWindow::FindCurrency(int catalogIndex)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CIAPCurrency* item = static_cast<CIAPCurrency*>(
        app->InitIAPCurrencyManager()->getItemByInternalId(catalogIndex));
    if (item != NULL && item->m_type == L"iap" && !item->isHidden()) {
        return item;
    }
    return NULL;
}

void CShopWindow::AddCurrencyRows(Window* parent, int y, int command)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CIAPCurrencyManager* catalog = app->InitIAPCurrencyManager();
    int scale = App::Scale();
    for (int index = 0; catalog->getItemByInternalId(index) != NULL; ++index) {
        CIAPCurrency* item = FindCurrency(index);
        if (item != NULL) {
            AddShopButton(parent, item->GetName(), 0, y, parent->m_width, 30 * scale, command, index);
            y += 32 * scale;
        }
    }
}

void CShopWindow::ShowCurrencyDetails(CIAPCurrency* item, int buyCommand)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDialogWindow* dialog = new CDialogWindow(false);
    dialog->SetTitleText(item->GetName());
    XString message = item->GetDescription();
    if (item->isAvailable()) {
        dialog->SetButtons(CDialogWindow::BUTTON_BUY, CDialogWindow::BUTTON_NO);
        dialog->SetButtonsCommands(buyCommand, -1, -1);
    } else {
        const char* status = "IDS_SHOP_IAP_ERROR";
        if (ICInAppPurchase::GetInstance()->IsTransactionActive()) {
            status = "IDS_SHOP_IAP_LOADING_MESSAGE";
        }
        message.Append(XString(L"\n"));
        message.Append(ResString(status));
        dialog->SetButtons(CDialogWindow::BUTTON_OK);
    }
    dialog->SetMessageText(message);
    dialog->SetExtraImageFromImageID(item->getBigImageResId());
    app->m_mainLayer->AddModal(dialog);
}

void CShopWindow::BuyCurrency(CIAPCurrency* item)
{
    ICInAppPurchase* store = ICInAppPurchase::GetInstance();
    if (store->IsTransactionActive()) {
        return;
    }
    if (!item->isAvailable() || !store->IsAvailable()) {
        ShowCurrencyResult(IAPPurchaseFailed, NULL);
        return;
    }
    store->BuyProduct(CStrWChar(item->m_appleId.c_str()));
    CGameAnalytics::logIAPBuyClicked(XString::Format(L"%02d_iap", item->m_id));
}

void CShopWindow::ShowCurrencyResult(int command, CIAPCurrency* item)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDialogWindow* dialog = new CDialogWindow(true);
    if (command == IAPPurchaseComplete && item != NULL) {
        dialog->SetMessageText(ResString("IDS_SHOP_IAP_SUCCESS") + item->GetName());
    } else {
        dialog->SetTitleText("IDS_SHOP_IAP_ERROR_TITLE");
        dialog->SetMessageText(ResString("IDS_SHOP_IAP_ERROR"));
    }
    dialog->SetButtons(CDialogWindow::BUTTON_OK);
    app->m_mainLayer->AddModal(dialog);
}

void CShopWindow::OnLostFocus()
{
    CBH_GPSMap::shopIsPainting = false;
}

void CShopWindow::OnGetFocus()
{
    CBH_GPSMap::shopIsPainting = true;
}

void CShopWindow::OnEasyStarted()
{
    CBH_GPSMap::shopIsPainting = false;
}
