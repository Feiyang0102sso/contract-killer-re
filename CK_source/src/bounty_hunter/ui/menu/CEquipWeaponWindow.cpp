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

App* GetApp()
{
    return static_cast<App*>(WindowApp::m_instance);
}
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
    prompt->SetLayoutType(LAYOUT_TABLE);
    prompt->SetCellPos(0, 0, 1, 1);
    slots->AddToFront(prompt);

    Window* firstLabel = CreateTextWindow("IDS_MENU_WEAPON_SLOT1");
    firstLabel->SetPercentHeight(15, 0, 0);
    firstLabel->SetLayoutType(LAYOUT_TABLE);
    firstLabel->SetCellPos(0, 1, 1, 1);
    slots->AddToFront(firstLabel);

    Window* firstSlot = CreateSlotButton(0, PreviewFirst);
    firstSlot->SetLayoutType(LAYOUT_TABLE);
    firstSlot->SetCellPos(0, 2, 1, 1);
    slots->AddToFront(firstSlot);

    Window* secondLabel = CreateTextWindow("IDS_MENU_WEAPON_SLOT2");
    secondLabel->SetPercentHeight(15, 0, 0);
    secondLabel->SetLayoutType(LAYOUT_TABLE);
    secondLabel->SetCellPos(0, 3, 1, 1);
    slots->AddToFront(secondLabel);

    Window* secondSlot = CreateSlotButton(1, PreviewSecond);
    secondSlot->SetLayoutType(LAYOUT_TABLE);
    secondSlot->SetCellPos(0, 4, 1, 1);
    slots->AddToFront(secondSlot);

    CImageButton* back = new CImageButton();
    back->AddImage("IDB_SHOP_BACK_BUTTON", 0, 0);
    back->SetText(ResString("IDS_MENU_BACK"), ALIGN_HCENTER | ALIGN_TOP,
                  CFontMgr::GetInstance()->GetFont(15), App::Scale() * 5, 0);
    back->SetLayoutType(LAYOUT_SIMPLE);
    back->SetAlign(ALIGN_LEFT | ALIGN_BOTTOM);
    back->SetCommandParameters(CancelEquip, 0, 0, 0);
    int backRight = App::Scale() * 10;
    int backBottom = App::Scale() * 2;
    int backLeft = App::Scale() * 10;
    back->SetOutsetSpacing(0, backRight, backBottom, backLeft);
    m_content->AddToFront(back);

    CImageButton* confirm = new CImageButton();
    confirm->AddImage("IDB_SHOP_BACK_BUTTON", 0, 0);
    confirm->SetText(ResString("IDS_SK_OK"), ALIGN_HCENTER | ALIGN_TOP,
                     CFontMgr::GetInstance()->GetFont(15), App::Scale() * 5, 0);
    confirm->SetLayoutType(LAYOUT_SIMPLE);
    confirm->SetAlign(ALIGN_RIGHT | ALIGN_BOTTOM);
    confirm->SetCommandParameters(ConfirmEquip, 0, 0, 0);
    int confirmRight = App::Scale() * 10;
    int confirmBottom = App::Scale() * 2;
    int confirmLeft = App::Scale() * 10;
    confirm->SetOutsetSpacing(0, confirmRight, confirmBottom, confirmLeft);
    m_content->AddToFront(confirm);

    // Original: the equipped weapons are not checked for NULL.
    m_originalFirst = GetApp()->m_helper->GetGameData()->GetWeapon01(true)->m_id;
    m_originalSecond = GetApp()->m_helper->GetGameData()->GetWeapon02(true)->m_id;
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
    switch (event.m_id) {
    case CancelEquip: {
        int second = m_originalSecond;
        GetApp()->m_helper->m_gamePlayManager.SetWeapon01(m_originalFirst);
        GetApp()->m_helper->m_gamePlayManager.SetWeapon02(second);
        Close();
        return;
    }
    case ConfirmEquip:
    case CloseShop:
        Close();
        return;
    }

    int first;
    int second;
    WindowApp** instance = &WindowApp::m_instance;
    App* app;
    switch (event.m_id) {
    case PreviewSecond:
        first = m_originalFirst;
        second = m_weaponId;
        app = static_cast<App*>(*instance);
        break;
    case PreviewFirst:
        app = static_cast<App*>(*instance);
        first = m_weaponId;
        second = m_originalSecond;
        break;
    default:
        return;
    }
    app->m_helper->m_gamePlayManager.SetWeapon01(first);
    static_cast<App*>(*instance)->m_helper->m_gamePlayManager.SetWeapon02(second);
}

void CEquipWeaponWindow::OnClosed()
{
}
