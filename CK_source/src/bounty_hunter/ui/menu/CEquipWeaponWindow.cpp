// CEquipWeaponWindow.cpp - restore the saved pair when the player cancels.
#include "CEquipWeaponWindow.h"

#include "App.h"
#include "BaseDialog.h"
#include "CBH_WeaponButton.h"
#include "CDH_BasicGameData.h"
#include "CDH_Weapon.h"
#include "CFontMgr.h"
#include "CImageButton.h"
#include "DGHelper.h"

namespace {
const int PreviewFirst = 0x7552d736;
const int PreviewSecond = 0x7551d736;
const int CancelEquip = (int)0x81051b34;
const int ConfirmEquip = (int)0xbaba08c5;
const int CloseShop = (int)0x97204784;
}

CEquipWeaponWindow::CEquipWeaponWindow(int weaponId)
    : m_weaponId(weaponId)
{
    CreateBgEffects();
    SetTitle(ResString("IDS_EQUIP"));

    Window* slots = new Window(false);
    slots->SetLayoutType(LAYOUT_SIMPLE);
    slots->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
    slots->SetPercentWidth(50, 0, 0);
    slots->SetPercentHeight(80, 0, 0);
    m_content->AddToFront(slots);
    Window* prompt = CreateTextWindow("IDS_SELECT_SLOT");
    prompt->SetPercentHeight(15, 0, 0);
    slots->AddToFront(prompt, 0, 0);
    Window* firstLabel = CreateTextWindow("IDS_MENU_WEAPON_SLOT1");
    firstLabel->SetPercentHeight(15, 0, 0);
    slots->AddToFront(firstLabel, 0, 1);
    slots->AddToFront(CreateSlotButton(0, PreviewFirst), 0, 2);
    Window* secondLabel = CreateTextWindow("IDS_MENU_WEAPON_SLOT2");
    secondLabel->SetPercentHeight(15, 0, 0);
    slots->AddToFront(secondLabel, 0, 3);
    slots->AddToFront(CreateSlotButton(1, PreviewSecond), 0, 4);

    CFont* font = CFontMgr::GetInstance()->GetFont(15);
    int scale = App::Scale();
    CImageButton* back = new CImageButton();
    back->AddImage("IDB_SHOP_BACK_BUTTON", 0, 0);
    back->SetText(ResString("IDS_MENU_BACK"), ALIGN_HCENTER | ALIGN_TOP, font, scale * 5, 0);
    back->SetLayoutType(LAYOUT_SIMPLE);
    back->SetAlign(ALIGN_LEFT | ALIGN_BOTTOM);
    back->m_command = CancelEquip;
    back->SetOutsetSpacing(0, scale * 10, scale * 2, scale * 10);
    m_content->AddToFront(back);
    CImageButton* confirm = new CImageButton();
    confirm->AddImage("IDB_SHOP_BACK_BUTTON", 0, 0);
    confirm->SetText(ResString("IDS_SK_OK"), ALIGN_HCENTER | ALIGN_TOP, font, scale * 5, 0);
    confirm->SetLayoutType(LAYOUT_SIMPLE);
    confirm->SetAlign(ALIGN_RIGHT | ALIGN_BOTTOM);
    confirm->m_command = ConfirmEquip;
    confirm->SetOutsetSpacing(0, scale * 10, scale * 2, scale * 10);
    m_content->AddToFront(confirm);

    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_BasicGameData* data = app->m_helper->GetGameData();
    const CDH_Weapon* first = data->GetWeapon01(true);
    const CDH_Weapon* second = data->GetWeapon02(true);
    m_originalFirst = data->m_weaponIds[0];
    m_originalSecond = data->m_weaponIds[1];
    if (first != NULL) {
        m_originalFirst = first->m_id;
    }
    if (second != NULL) {
        m_originalSecond = second->m_id;
    }
}

Window* CEquipWeaponWindow::CreateTextWindow(const char* resource)
{
    Window* cell = new Window(false);
    TextWindow* text = new TextWindow(ResString(resource), CFontMgr::GetInstance()->GetFont(15));
    text->SetLayoutType(LAYOUT_SIMPLE);
    text->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    cell->AddToFront(text);
    return cell;
}

Window* CEquipWeaponWindow::CreateSlotButton(int slot, int command)
{
    Window* cell = new Window(false);
    CBH_WeaponButton* button = new CBH_WeaponButton(0, slot, true, false, 0);
    button->SetLayoutType(LAYOUT_SIMPLE);
    button->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    button->m_command = command;
    button->m_commandParameter1 = 0;
    button->m_commandParameter2 = 0;
    button->m_commandParameter3 = 0;
    cell->AddToFront(button);
    return cell;
}

void CEquipWeaponWindow::Paint(ICGraphics2d& graphics)
{
    Window::Paint(graphics);
}

void CEquipWeaponWindow::OnCommand(Event& event)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    GamePlayManager& game = app->m_helper->m_gamePlayManager;
    switch (event.m_id) {
    case PreviewFirst:
        game.SetWeapon01(m_weaponId);
        game.SetWeapon02(m_originalSecond);
        break;
    case PreviewSecond:
        game.SetWeapon01(m_originalFirst);
        game.SetWeapon02(m_weaponId);
        break;
    case CancelEquip:
        game.SetWeapon01(m_originalFirst);
        game.SetWeapon02(m_originalSecond);
        Close();
        break;
    case ConfirmEquip:
    case CloseShop:
        Close();
        break;
    }
}

void CEquipWeaponWindow::OnClosed()
{
}
