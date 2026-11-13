#include "CGiftsWindow.h"

#include "App.h"
#include "BaseDialog.h"
#include "CAutoArrangedWindow.h"
#include "CBH_Player.h"
#include "CFontMgr.h"
#include "CImageButton.h"
#include "CScrollWindow.h"
#include "CShopItem.h"
#include "CSocialGiftsManager.h"
#include "SScreenAdaptingConstant.h"

static const SScreenAdaptingConstant ARRANGED_WINDOW_HEIGHT(220, 440, 550);

namespace {
const int SelectGift = -0x5bc88ffe;

CSocialGiftManager* GetGiftManager()
{
    return static_cast<App*>(WindowApp::m_instance)->m_socialGiftManager;
}
}

CGiftsWindow::CGiftsWindow()
    : Window(false)
{
    Vector<const SGift*> gifts;
    GetGifts(gifts);
    CreateUserInterface(gifts);
}

void CGiftsWindow::GetGifts(Vector<const SGift*>& gifts)
{
    CSocialGiftManager* manager = GetGiftManager();
    for (int index = 0; index < manager->GetGiftsCount(); ++index) {
        const SGift* gift = manager->GetGift(index);
        if (gift != NULL && !gift->m_hidden) {
            gifts.addElement(gift);
        }
    }
}

void CGiftsWindow::CreateUserInterface(const Vector<const SGift*>& gifts)
{
    CAutoArrangedWindow* page = new CAutoArrangedWindow(WindowApp::m_instance->m_screenWidth, ARRANGED_WINDOW_HEIGHT());
    CScrollWindow* scroll = new CScrollWindow(true);
    scroll->Add(page);
    for (int index = 0; index < gifts.size(); ++index) {
        if (!CBH_Player::GetInstance()->IsEnegyEnabled()) {
            const SGift* gift = gifts[index];
            if (gift == NULL || gift->m_itemManager == NULL) {
                continue;
            }
            CItem* item = gift->GetItem();
            if (item == NULL || item->getType() == 4) {
                continue;
            }
        }
        // CK uses the filtered row index to look up the catalog entry.
        Window* button = CreateGiftButton(index);
        if (!page->Add(button)) {
            page = new CAutoArrangedWindow(WindowApp::m_instance->m_screenWidth, ARRANGED_WINDOW_HEIGHT());
            scroll->Add(page);
            page->Add(button);
        }
    }
    AddToFront(scroll);
}

Window* CGiftsWindow::CreateGiftButton(int catalogIndex)
{
    CItem* item = GetGiftManager()->GetGift(catalogIndex)->GetItem();
    Window* cell = new Window(false);
    cell->SetHeightByContent(0, 0);
    cell->SetWidthByContent(0, 0);
    CImageButton* button = new CImageButton();
    button->AddImage(item->getShopImage(), 0, 0);
    button->m_command = SelectGift;
    button->m_commandParameter1 = catalogIndex;
    button->m_commandParameter2 = 0;
    button->m_commandParameter3 = 0;
    cell->SetSize(button->m_width, button->m_height);
    cell->AddToFront(button, 0, 0);

    TextWindow* name = new TextWindow(item->GetName(), CFontMgr::GetInstance()->GetFont(15));
    name->SetDesiredWidth(button->m_width);
    name->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    cell->AddToFront(name, 0, 1);
    cell->HandleUpdate(false);
    return cell;
}
