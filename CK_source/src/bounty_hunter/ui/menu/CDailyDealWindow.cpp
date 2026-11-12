#include "CDailyDealWindow.h"

#include "App.h"
#include "CBH_Player.h"
#include "CDailyDealManager.h"
#include "CRegularShop.h"
#include "CShopItem.h"
#include "CStdUtil_Cocoa.h"

namespace {
const int BuyDeal = 0x0027775d;
const int OpenBank = 0x02e7d3e7;

class DailyDealTimer : public TextWindow {
public:
    explicit DailyDealTimer(unsigned int finishTime)
        : TextWindow(CFontMgr::GetInstance()->GetFont(8)), m_finishTime(finishTime), m_lastSeconds(-1)
    {
        SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
        Update();
    }

    virtual void Update()
    {
        unsigned int now = CStdUtil_Cocoa::GetTimeSeconds();
        int remaining = 0;
        if (m_finishTime > now) {
            remaining = m_finishTime - now;
        }
        if (remaining != m_lastSeconds) {
            m_lastSeconds = remaining;
            XString title = ResString("IDS_OFFER_ENDS");
            SetText(XString::Format(L"%ls  %02d:%02d:%02d", title.c_str(),
                remaining / 3600, (remaining / 60) % 60, remaining % 60));
        }
    }

private:
    unsigned int m_finishTime;
    int m_lastSeconds;
};
}

CDailyDealWindow::CDailyDealWindow() : CDialogWindow(false), m_item(NULL)
{
    CreateUI();
}

void CDailyDealWindow::CreateUI()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDailyDealManager* manager = app->InitDailyDealManager();
    m_item = manager->GetCurrentDealItem();
    SetTitleText("IDS_DAILY_DEAL");
    if (m_item == NULL) {
        SetButtons(BUTTON_OK);
        return;
    }
    SDailyDeal* deal = manager->GetCurrentDailyDeal();
    XString message = m_item->GetName() + L"\n" + m_item->GetDescription();
    if (deal != NULL) {
        message.Append(XString(L"\n"));
        message.Append(deal->m_text);
    }
    SetMessageText(message);
    SetExtraImageFromImageID(m_item->getBigImageResId());
    CreateItemPriceWindow();
    CreateTimerWindow();
    if (CBH_Player::GetInstance()->m_credits < m_item->getPriceCredits()) {
        SetButtons(BUTTON_GET, BUTTON_IGNORE);
        SetButtonsCommands(OpenBank, -1, -1);
    } else {
        SetButtons(BUTTON_BUY, BUTTON_IGNORE);
        SetButtonsCommands(BuyDeal, -1, -1);
    }
}

void CDailyDealWindow::CreateItemPriceWindow()
{
    XString oldLabel = ResString("IDS_OLD_PRICE");
    XString newLabel = ResString("IDS_NEW_PRICE");
    XString text = XString::Format(L"%ls %d    %ls %d", oldLabel.c_str(), m_item->m_priceCredits,
                                  newLabel.c_str(), m_item->getPriceCredits());
    AddDownTextWithIcon(text, "IDB_HUD_ICON_CREDITS", false, false, static_cast<eGameFont>(8));
}

void CDailyDealWindow::CreateTimerWindow()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    unsigned int end = app->InitDailyDealManager()->GetNextDealStartTime();
    m_downText->AddToFront(new DailyDealTimer(end), 0, 1);
}

void CDailyDealWindow::OnCommand(Event& event)
{
    if (event.m_id == BuyDeal) {
        HandleBuyButtonPressed();
    } else if (event.m_id == OpenBank) {
        CRegularShop::Show(CShopWindow::TAB_BANK);
    } else {
        CDialogWindow::OnCommand(event);
        return;
    }
    event.Clear();
}

void CDailyDealWindow::HandleBuyButtonPressed()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDailyDealManager* manager = app->InitDailyDealManager();
    if (manager->GetCurrentDailyDeal() == NULL) {
        return;
    }
    CItem* current = manager->GetCurrentDealItem();
    if (current != NULL && current == m_item) {
        CRegularShop::ShowDeal(current);
    }
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
