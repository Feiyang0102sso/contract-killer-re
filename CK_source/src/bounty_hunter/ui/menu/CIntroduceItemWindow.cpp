// CIntroduceItemWindow.cpp - display the item and continue the owning flow on close.
#include "CIntroduceItemWindow.h"

#include "App.h"
#include "BaseDialog.h"
#include "CBH_MenuButton.h"
#include "CDH_WeaponsManager.h"
#include "CRegularShop.h"
#include "CShopItem.h"
#include "CShopItemInfoWindow.h"
#include "SScreenAdaptingConstant.h"

namespace {
const int IntroduceItemContinue = -0x1b2b7ba;
const SScreenAdaptingConstant BUTTON_OFFSET_DOWN(20, 40);
}

CIntroduceItemWindow::CIntroduceItemWindow(CItem* item, XString message, bool showShop, bool pauseTime)
    : CTextDialog(), m_item(NULL), m_closeCommand(0), m_introMessage(message),
      m_showShop(showShop), m_pauseTime(pauseTime)
{
    if (m_introMessage.Length() == 0) {
        XString format = ResString("IDS_ITEM_INTRODUCE_TEXT");
        m_introMessage = XString::Format(format.c_str(), item->GetName().c_str());
    }
    CreateUI(item);
    m_item = item;
}

void CIntroduceItemWindow::CreateUI(CItem* item)
{
    Window* panel = new Window(false);
    panel->SetLayoutType(LAYOUT_SIMPLE);
    panel->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    panel->m_debugFrame = true;
    panel->SetPercentHeight(50, 0, 0);
    panel->SetPercentWidth(80, 0, 0);
    AddToFront(panel);

    ImageWindow* background = new ImageWindow();
    background->m_debugFrame = true;
    background->SetImageById("IDB_SHOP_BG_RED");
    background->SetFillType(ImageWindow::FILL_NONE);
    background->SetLayoutType(LAYOUT_SIMPLE);
    background->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
    panel->AddToFront(background);

    ImageWindow* image = new ImageWindow();
    image->SetFillType(ImageWindow::FILL_NONE);
    image->SetImageById(item->getBigImageResId());
    image->SetLayoutType(LAYOUT_SIMPLE);
    image->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
    image->SetHeightByContent(0, 0);
    panel->AddToFront(image, 0, 0);
    image->SetHeightByContent(0, 0);

    TextWindow* message = new TextWindow(m_introMessage, CFontMgr::GetInstance()->GetFont(15));
    message->SetLayoutType(LAYOUT_SIMPLE);
    message->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
    panel->AddToFront(message, 0, 1);
    message->SetPercentWidth(30, 0, 0);
    message->SetPercentHeight(40, 0, 0);

    SetTitleText(item->GetName());
    EDialogButton button = BUTTON_OK;
    if (m_showShop) {
        button = BUTTON_EQUIP;
    }
    SetButtons(button);
    SetButtonsCommands(IntroduceItemContinue, -1, -1);
    CBH_MenuButton* confirm = m_buttons[0];
    confirm->SetCorner(m_width / 2 - confirm->m_width / 2,
                       m_height - BUTTON_OFFSET_DOWN() - confirm->m_height + m_buttonOffsetY);
    if (m_pauseTime) {
        static_cast<App*>(WindowApp::m_instance)->m_timeScale = 0.0f;
    }
}

void CIntroduceItemWindow::OnCommand(Event& event)
{
    CDialogWindow::OnCommand(event);
    if (event.m_id == IntroduceItemContinue) {
        HandleButtonPressed();
        event.Clear();
    }
}

void CIntroduceItemWindow::HandleButtonPressed()
{
    if (m_pauseTime) {
        static_cast<App*>(WindowApp::m_instance)->m_timeScale = 1.0f;
        m_pauseTime = false;
    }
    if (m_showShop) {
        CRegularShop* shop = CRegularShop::GetInstance(true);
        if (shop->m_parent == NULL) {
            App* app = static_cast<App*>(WindowApp::m_instance);
            app->m_mainLayer->AddModal(shop);
            shop->SetTab(CShopWindow::TAB_WEAPONS);
            CShopItemInfoWindow* details = new CShopItemInfoWindow(app->InitWeaponManager(), m_item->getType(),
                m_item->m_id, m_item->GetStatsString(), false, CShopItemInfoWindow::SOURCE_SHOP);
            shop->AddModal(details);
            Close();
        }
    }
}

void CIntroduceItemWindow::SetOnCloseCommand(unsigned int command)
{
    m_closeCommand = command;
}

void CIntroduceItemWindow::OnClosed()
{
    // the original does not restore the time scale here
    if (m_closeCommand != 0) {
        WindowApp::HandleTunnelCommand(m_closeCommand, 0, 0, 0);
    }
}
