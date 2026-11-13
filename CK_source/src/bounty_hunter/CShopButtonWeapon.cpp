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
#include "CUtility.h"
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
      m_weaponId(weaponId), m_state(static_cast<EWeaponState>(0)), m_tutorialBlink(false)
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
    if (state == Locked) {
        caption = CUtility::GetString("IDS_ITEM_UNLOCK_LEVEL").GetWCharPtr();
        const wchar_t* format = caption.c_str();
        caption = XString::Format(format, GetUnlockLevel());
        m_status->SetFont(CFontMgr::GetInstance()->GetFont(12));
    } else if (state == Equipped) {
        caption = CUtility::GetString("IDS_EQUIPED").GetWCharPtr();
        m_status->SetFont(CFontMgr::GetInstance()->GetFont(14));
    } else if (state == Owned) {
        caption = CUtility::GetString("IDS_OWNED").GetWCharPtr();
        m_status->SetFont(CFontMgr::GetInstance()->GetFont(13));
    }
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
        unsigned int uptime = CStdUtil_Cocoa::GetUpTimeMS();
        int phase = static_cast<int>(uptime % 600) - 300;
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
    if (app->m_helper->GetGameData()->GetWeapon01(true) == weapon) {
        return true;
    }
    app = static_cast<App*>(WindowApp::m_instance);
    return app->m_helper->GetGameData()->GetWeapon02(true) == weapon;
}

bool CShopButtonWeapon::IsLocked()
{
    CItem* weapon = Weapons()->getItemById(m_weaponId);
    return weapon->m_unlockLevel > CBH_Player::GetInstance()->GetLevel();
}
