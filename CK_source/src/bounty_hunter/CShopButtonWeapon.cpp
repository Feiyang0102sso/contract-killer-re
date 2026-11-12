// CShopButtonWeapon.cpp - show locked, equipped and owned weapons in the shop.
#include "CShopButtonWeapon.h"

#include "App.h"
#include "BaseDialog.h"
#include "CBH_Player.h"
#include "CDH_BasicGameData.h"
#include "CDH_Weapon.h"
#include "CDH_WeaponsManager.h"
#include "CFontMgr.h"
#include "CStdUtil_Cocoa.h"
#include "CTutorialManager.h"
#include "DGHelper.h"
#include "SScreenAdaptingConstant.h"

#include <math.h>

namespace {
CDH_WeaponsManager* Weapons()
{
    return static_cast<App*>(WindowApp::m_instance)->InitWeaponManager();
}
}

CShopButtonWeapon::CShopButtonWeapon(const XString& name, int unlockLevel, int price, int weaponId, const char* image)
    : CShopItemButton(Weapons(), Weapons()->getItemById(weaponId)->getType(), name, unlockLevel, price, weaponId, image),
      m_weaponId(weaponId), m_tutorialBlink(false)
{
    m_status = new TextWindow();
    m_status->SetFont(CFontMgr::GetInstance()->GetFont(13));
    m_status->SetAlign(ALIGN_RIGHT | ALIGN_VCENTER);
    m_status->SetOutsetSpacing(0, SScreenAdaptingConstant(12, 24, 24, -1)(), 0, 0);
    AddToFront(m_status);
    m_state = GetCurrState();
    UpdateCaption(m_state);
}

CShopButtonWeapon::EWeaponState CShopButtonWeapon::GetCurrState()
{
    XString caption;
    if (IsLocked()) {
        return Locked;
    }
    if (IsEquiped()) {
        return Equipped;
    }
    if (IsOwned()) {
        return Owned;
    }
    return Available;
}

void CShopButtonWeapon::UpdateCaption(EWeaponState state)
{
    if (state == Available) {
        return;
    }
    XString caption;
    int font = 13;
    if (state == Locked) {
        XString format = ResString("IDS_ITEM_UNLOCK_LEVEL");
        caption = XString::Format(format.c_str(), GetUnlockLevel());
        font = 12;
    } else if (state == Equipped) {
        caption = ResString("IDS_EQUIPED");
        font = 14;
    } else if (state == Owned) {
        caption = ResString("IDS_OWNED");
    }
    m_status->SetFont(CFontMgr::GetInstance()->GetFont(font));
    m_status->SetText(caption);
}

void CShopButtonWeapon::Update()
{
    CShopItemButton::Update();
    EWeaponState state = GetCurrState();
    if (m_state != state) {
        UpdateCaption(state);
        m_state = state;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    m_tutorialBlink = app->m_tutorialManager->IsWeaponBlink(m_weaponId);
}

void CShopButtonWeapon::Paint(ICGraphics2d& graphics)
{
    if (m_tutorialBlink) {
        int phase = static_cast<int>(CStdUtil_Cocoa::GetUpTimeMS() % 600) - 300;
        float opacity = 0.4f + fabsf(static_cast<float>(phase)) / 420.0f;
        BeginBlend(graphics, opacity, 1.0f, 1.0f, 1.0f, true, false);
    }
    CShopItemButton::Paint(graphics);
    if (m_tutorialBlink) {
        EndBlend(graphics);
    }
}

bool CShopButtonWeapon::IsOwned()
{
    return Weapons()->getItemById(m_weaponId)->isAvailable();
}

bool CShopButtonWeapon::IsEquiped()
{
    CItem* weapon = Weapons()->getItemById(m_weaponId);
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_BasicGameData* data = app->m_helper->GetGameData();
    return data->GetWeapon01(true) == weapon || data->GetWeapon02(true) == weapon;
}

bool CShopButtonWeapon::IsLocked()
{
    CItem* weapon = Weapons()->getItemById(m_weaponId);
    return CBH_Player::GetInstance()->GetLevel() < weapon->m_unlockLevel;
}
