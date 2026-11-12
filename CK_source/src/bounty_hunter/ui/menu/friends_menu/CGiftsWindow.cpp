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

namespace {
const int SelectGift = -0x5bc88ffe;
const SScreenAdaptingConstant s_arrangedWindowHeight(220, 440, 550);

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
    int width = WindowApp::m_instance->m_screenWidth - 12 * App::Scale();
    int height = s_arrangedWindowHeight();
    CAutoArrangedWindow* page = new CAutoArrangedWindow(width, height);
    CScrollWindow* scroll = new CScrollWindow(true);
    scroll->Add(page);
    CSocialGiftManager* manager = GetGiftManager();
    bool energyEnabled = CBH_Player::GetInstance()->IsEnegyEnabled();
    int catalogIndex = 0;
    for (int index = 0; index < gifts.size(); ++index) {
        const SGift* gift = gifts[index];
        // The wire ID is the XML position, including entries hidden from the gift browser.
        while (manager->GetGift(catalogIndex) != gift) {
            ++catalogIndex;
        }
        CItem* item = gift->GetItem();
        if (item == NULL || (!energyEnabled && item->getType() == 4)) {
            continue;
        }
        Window* button = CreateGiftButton(catalogIndex);
        if (!page->Add(button)) {
            page = new CAutoArrangedWindow(width, height);
            scroll->Add(page);
            if (!page->Add(button)) {
                delete button;
            }
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
    ICRenderSurface* image = item->getShopImage();
    if (image != NULL) {
        button->AddImage(image, 0, 0);
    } else if (item->getShopImageResId() != NULL) {
        button->AddImage(item->getShopImageResId(), 0, 0);
    } else {
        button->SetSize(90 * App::Scale(), 24 * App::Scale());
        button->SetText(item->GetName(), ALIGN_HCENTER | ALIGN_VCENTER, NULL, 0, 0);
    }
    button->m_command = SelectGift;
    button->m_commandParameter1 = catalogIndex;
    cell->SetSize(button->m_width, button->m_height);
    cell->AddToFront(button, 0, 0);

    if (image != NULL || item->getShopImageResId() != NULL) {
        TextWindow* name = new TextWindow(item->GetName(), CFontMgr::GetInstance()->GetFont(15));
        name->SetDesiredWidth(button->m_width);
        name->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
        cell->AddToFront(name, 0, 1);
    }
    cell->HandleUpdate(false);
    return cell;
}
