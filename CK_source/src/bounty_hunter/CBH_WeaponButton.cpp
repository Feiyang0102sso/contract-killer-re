#include "CBH_WeaponButton.h"

#include "App.h"
#include "BaseDialog.h"
#include "CAmmo.h"
#include "CBH_Player.h"
#include "CBH_SurfaceManager.h"
#include "CDH_BasicGameData.h"
#include "CDH_Weapon.h"
#include "CDH_WeaponsManager.h"
#include "CFontMgr.h"
#include "CImageButton.h"
#include "CTutorialManager.h"
#include "DGHelper.h"
#include "SScreenAdaptingConstant.h"
#include "ServiceWindows.h"

namespace {

// CK keeps two unused layout constants before these three in one data block.
struct WeaponButtonLayout {
    SScreenAdaptingConstant unusedFirst;
    SScreenAdaptingConstant unusedSecond;
    SScreenAdaptingConstant statsHeight;
    SScreenAdaptingConstant bonusX;
    SScreenAdaptingConstant bonusY;
};

const WeaponButtonLayout LAYOUT = {
    SScreenAdaptingConstant(1, 14),
    SScreenAdaptingConstant(5, 10),
    SScreenAdaptingConstant(22, 60),
    SScreenAdaptingConstant(10, 21),
    SScreenAdaptingConstant(8, 22)
};
const int INFINITE_AMMO = 0xfffffff;

}

CBH_WeaponButton::CBH_WeaponButton(int type, int slot, bool showAmmo, bool blockPointer, int weaponId)
    : m_blockPointer(blockPointer), m_shownAmmo(NULL), m_ammo(NULL), m_weaponOverride(NULL),
      m_reserved10C(false), m_showStats(false), m_wander(NULL), m_range(NULL), m_damage(NULL), m_rpm(NULL)
{
    if (type == 0) {
        m_background = static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->CreateSurface("IDB_BUTTON_WEAPON", true);
    } else {
        m_background = static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->CreateSurface("IDB_BUTTON_GEAR", true);
    }
    int width = static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->GetSurfaceWidth(m_background);
    int height = static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->GetSurfaceHeight(m_background);
    SetSize(width, height);
    m_locked = static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->CreateSurface("IDB_ICON_LOCKED", true);

    m_slot = slot;
    if (slot == -1) {
        m_weaponId = weaponId;
    } else {
        m_weaponId = Weapon()->m_id;
    }

    m_backgroundImage = new ImageWindow();
    m_backgroundImage->SetFillType(ImageWindow::FILL_NONE);
    m_backgroundImage->SetImage(m_background);
    AddToFront(m_backgroundImage);

    m_weaponImage = new ImageWindow();
    m_weaponImage->SetFillType(ImageWindow::FILL_NONE);
    m_weaponImage->SetImage(Weapon()->getMenuImage());
    m_backgroundImage->AddToFront(m_weaponImage);
    m_weaponId = Weapon()->m_id;
    CDH_Weapon* weapon = Weapon();
    if (weapon != NULL) {
        if (showAmmo) {
            const CAmmo* ammo = weapon->GetActiveAmmo();
            m_ammoCount = CBH_Player::GetInstance()->GetAvailableAmmo(ammo, true);
            XString count(m_ammoCount);
            if (ammo == NULL) {
                count = L"\u221e";
            }
            // CK still reads the ammo icon after preparing the infinity label.
            void* labelStorage = ::operator new(sizeof(LabelWithIcon));
            {
                XString iconName(ammo->m_bulletIcon);
                XString::AnsiString icon(iconName.c_str());
                m_ammo = new (labelStorage) LabelWithIcon(icon, count, true, NULL, 0);
            }
            m_ammo->SetAlign(ALIGN_RIGHT | ALIGN_TOP);
            m_ammo->SetOutsetSpacing(SScreenAdaptingConstant(3, 6)(), 0, 0, 0);
            m_backgroundImage->AddToFront(m_ammo);
        }

        m_bonusXp = new CImageButton();
        m_bonusXp->AddImage("IDB_EXP_PLUS_SHOP_ICON", 0, 0);
        m_bonusXp->SetLayoutType(LAYOUT_NONE);
        CImageButton* bonusIcon = m_bonusXp;
        int bonusX = LAYOUT.bonusX();
        int bonusY = LAYOUT.bonusY();
        bonusIcon->SetCorner(bonusX, bonusY);
        m_backgroundImage->AddToFront(m_bonusXp);
        if (weapon->m_bonusXp < 1) {
            m_bonusXp->ClearFlags(FLAG_ENABLED | FLAG_VISIBLE);
        }
    }
}

CBH_WeaponButton::~CBH_WeaponButton()
{
    ReleaseResources();
}

CDH_Weapon* CBH_WeaponButton::Weapon()
{
    if (m_weaponOverride != NULL) {
        return m_weaponOverride;
    }
    switch (m_slot) {
    case 1:
        return static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon02(true);
    case 0:
        return static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon01(true);
    case -1:
        return static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->GetWeaponById(m_weaponId);
    default:
        return NULL;
    }
}

void CBH_WeaponButton::ReleaseResources()
{
    // 原版：锁图按名字归还，背景按表面归还（找不到时 ReleaseSurface 返回 false），两者都不清零
    static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->ReleaseSurface("IDB_ICON_LOCKED");
    static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(m_background);
}

void CBH_WeaponButton::OnPointerEvent(Event& event)
{
    if (!static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial()
        || static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->CanChooseWeapon()) {
        if (!m_blockPointer) {
            CBH_Button::OnPointerEvent(event);
        }
    }
}

void CBH_WeaponButton::SetRect(int x, int y, int, int, bool force)
{
    int statsHeight = 0;
    if (m_showStats) {
        statsHeight = LAYOUT.statsHeight();
    }
    int width = static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->GetSurfaceWidth(m_background);
    int height = static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->GetSurfaceHeight(m_background);
    Window::SetRect(x, y, width, height + statsHeight, force);
}

void CBH_WeaponButton::ShowName()
{
    TextWindow* name = new TextWindow(Weapon()->GetName(), CFontMgr::GetInstance()->GetFont(15));
    SScreenAdaptingConstant height(ImageHeight(m_background));
    name->SetLayoutType(LAYOUT_NONE);
    name->SetCorner(0, height());
    name->SetAlign(ALIGN_HCENTER);
    AddToFront(name);
}

void CBH_WeaponButton::Update()
{
    if (m_pressed) {
        m_weaponImage->SetOpacity(0.7f);
        m_backgroundImage->SetOpacity(0.7f);
    } else {
        m_weaponImage->SetOpacity(1.0f);
        m_backgroundImage->SetOpacity(1.0f);
    }

    CDH_Weapon* weapon = Weapon();
    const CAmmo* ammo = weapon->GetActiveAmmo();
    if (ammo != NULL) {
        if (m_ammo != NULL) {
            m_ammo->SetFlags(FLAG_ENABLED | FLAG_VISIBLE);
        }
        CBH_Player* player = CBH_Player::GetInstance();
        int count = player->GetAvailableAmmo(weapon->GetActiveAmmo(), true);
        if (count == INFINITE_AMMO) {
            // CK writes this label before checking whether one was created.
            m_ammo->SetText(XString(L"\u221e"));
        }
        if (count != m_ammoCount || ammo != m_shownAmmo) {
            m_ammoCount = count;
            m_shownAmmo = ammo;
            XString countText(count);
            if (count == INFINITE_AMMO) {
                countText = L"\u221e";
            }
            if (m_ammo != NULL) {
                {
                    XString iconName(m_shownAmmo->m_bulletIcon);
                    XString::AnsiString icon(iconName.c_str());
                    m_ammo->SetImage(icon);
                }
                m_ammo->SetText(countText);
            }
        }
    }

    if (m_slot == -1) {
        bool blink = static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsWeaponButtonBlink(weapon->m_id);
        m_backgroundImage->m_blinking = blink;
        m_weaponImage->m_blinking = blink;
    }

    if (weapon->m_id != m_weaponId) {
        m_weaponId = Weapon()->m_id;
        if (m_showStats) {
            XString wander(static_cast<int>(weapon->wanderAmount()));
            m_wander->SetText(wander);
            XString range(weapon->m_rangeLevels[0].m_value);
            if (weapon->m_rangeUpgrade != 0) {
                int upgradedRange = static_cast<int>(weapon->effectiveRange());
                range.Append(L"+" + XString(upgradedRange - weapon->m_rangeLevels[0].m_value));
            }
            m_range->SetText(range);

            int baseDamage = 0;
            if (weapon->m_damageLevels.size() != 0) {
                baseDamage = weapon->m_damageLevels[0].m_value;
            }
            XString damage(baseDamage);
            if (weapon->m_damageUpgrade != 0) {
                int upgradedDamage = weapon->damage();
                baseDamage = 0;
                if (weapon->m_damageLevels.size() != 0) {
                    baseDamage = weapon->m_damageLevels[0].m_value;
                }
                damage.Append(L"+" + XString(upgradedDamage - baseDamage));
            }
            m_damage->SetText(damage);
            XString rpm(weapon->getRPM());
            m_rpm->SetText(rpm);
        }
        m_weaponImage->SetImage(weapon->getMenuImage());
        if (weapon->m_bonusXp < 1) {
            m_bonusXp->ClearFlags(FLAG_ENABLED | FLAG_VISIBLE);
        } else {
            m_bonusXp->SetFlags(FLAG_ENABLED | FLAG_VISIBLE);
        }
    }
}

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AssertWeaponButtonSize[sizeof(CBH_WeaponButton) == 0x124 ? 1 : -1];
#endif

void CBH_WeaponButton::Paint(ICGraphics2d& graphics)
{
    CBH_Button::Paint(graphics);
}
