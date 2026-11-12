#include "CBH_WeaponButton.h"

#include "App.h"
#include "BaseDialog.h"
#include "CBH_Player.h"
#include "CBH_SurfaceManager.h"
#include "CDH_BasicGameData.h"
#include "CDH_Weapon.h"
#include "CDH_WeaponsManager.h"
#include "CFontMgr.h"
#include "DGHelper.h"

CBH_WeaponButton::CBH_WeaponButton(int type, int slot, bool showAmmo, bool blink, int weaponId)
    : m_blinking(blink), m_showAmmo(showAmmo), m_type(type), m_scale(App::Scale()), m_weaponId(weaponId),
      m_slot(slot), m_ammoCount(-1), m_name(NULL), m_ammo(NULL), m_lockImage(NULL), m_shownWeapon(NULL)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    const char* background = "IDB_BUTTON_WEAPON";
    if (type != 0) {
        background = "IDB_BUTTON_GEAR";
    }
    m_background = app->m_surfaceManager->CreateSurface(background, true);
    m_locked = app->m_surfaceManager->CreateSurface("IDB_ICON_LOCKED", true);
    m_backgroundImage = new ImageWindow();
    m_backgroundImage->SetImage(m_background);
    m_backgroundImage->ClearFlags(FLAG_FOCUSABLE);
    AddToFront(m_backgroundImage);

    m_weaponImage = new ImageWindow();
    m_weaponImage->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    m_weaponImage->ClearFlags(FLAG_FOCUSABLE);
    AddToFront(m_weaponImage);

    m_lockImage = new ImageWindow();
    m_lockImage->SetImage(m_locked);
    m_lockImage->SetAlign(ALIGN_RIGHT | ALIGN_TOP);
    m_lockImage->ClearFlags(FLAG_VISIBLE | FLAG_FOCUSABLE);
    AddToFront(m_lockImage);

    if (showAmmo) {
        m_ammo = new TextWindow(CFontMgr::GetInstance()->GetFont(8));
        m_ammo->SetAlign(ALIGN_RIGHT | ALIGN_BOTTOM);
        m_ammo->SetOutsetSpacing(0, 3 * m_scale, 4 * m_scale, 2 * m_scale);
        m_ammo->ClearFlags(FLAG_FOCUSABLE);
        AddToFront(m_ammo);
    }
    SetSize(ImageWidth(m_background), ImageHeight(m_background));
    Update();
}

CBH_WeaponButton::~CBH_WeaponButton()
{
    ReleaseResources();
}

CDH_Weapon* CBH_WeaponButton::Weapon()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (m_type != 0) {
        return NULL;
    }
    if (m_slot == 0) {
        return app->m_helper->GetGameData()->GetWeapon01(true);
    }
    if (m_slot == 1) {
        return app->m_helper->GetGameData()->GetWeapon02(true);
    }
    return app->InitWeaponManager()->GetWeaponById(m_weaponId);
}

void CBH_WeaponButton::ReleaseResources()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (m_background != NULL) {
        app->m_surfaceManager->ReleaseSurface(m_background);
        m_background = NULL;
    }
    if (m_locked != NULL) {
        app->m_surfaceManager->ReleaseSurface(m_locked);
        m_locked = NULL;
    }
}

void CBH_WeaponButton::OnPointerEvent(Event& event)
{
    CBH_Button::OnPointerEvent(event);
}

void CBH_WeaponButton::SetRect(int x, int y, int width, int height, bool force)
{
    Window::SetRect(x, y, width, height, force);
    if (m_backgroundImage != NULL) {
        m_backgroundImage->SetSize(width, height);
    }
}

void CBH_WeaponButton::ShowName()
{
    if (m_name == NULL) {
        m_name = new TextWindow(CFontMgr::GetInstance()->GetFont(15));
        m_name->SetAlign(ALIGN_HCENTER | ALIGN_BOTTOM);
        m_name->ClearFlags(FLAG_FOCUSABLE);
        AddToFront(m_name);
    }
    if (m_shownWeapon != NULL) {
        m_name->SetText(m_shownWeapon->GetName());
    }
}

void CBH_WeaponButton::SetBlinking(bool enabled)
{
    m_blinking = enabled;
    m_backgroundImage->m_blinking = enabled;
    m_weaponImage->m_blinking = enabled;
}

void CBH_WeaponButton::Update()
{
    CDH_Weapon* weapon = Weapon();
    if (weapon != m_shownWeapon) {
        ICRenderSurface* surface = NULL;
        if (weapon != NULL) {
            surface = weapon->getMenuImage();
        }
        m_weaponImage->SetImage(surface);
        m_shownWeapon = weapon;
        m_ammoCount = -1;
        if (m_name != NULL) {
            XString name;
            if (weapon != NULL) {
                name = weapon->GetName();
            }
            m_name->SetText(name);
        }
    }
    if (weapon != NULL && !weapon->isAvailable()) {
        m_lockImage->SetFlags(FLAG_VISIBLE);
    } else {
        m_lockImage->ClearFlags(FLAG_VISIBLE);
    }
    if (m_showAmmo && weapon != NULL) {
        int ammo = CBH_Player::GetInstance()->GetAvailableAmmo(weapon->GetActiveAmmo(), true);
        if (ammo != m_ammoCount) {
            m_ammoCount = ammo;
            XString text(L"\u221e");
            if (ammo != 0xfffffff) {
                text = XString(ammo);
            }
            m_ammo->SetText(text);
        }
    }
    SetBlinking(m_blinking);
}

typedef char AssertWeaponButtonSize[sizeof(CBH_WeaponButton) == 0x124 ? 1 : -1];

void CBH_WeaponButton::Paint(ICGraphics2d& graphics)
{
    CBH_Button::Paint(graphics);
}
