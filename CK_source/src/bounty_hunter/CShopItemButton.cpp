// CShopItemButton.cpp - render the configured item and its purchase information.
#include "CShopItemButton.h"

#include "App.h"
#include "CAmmo.h"
#include "CAmmoManager.h"
#include "CAmmoPack.h"
#include "CBH_Player.h"
#include "CBH_SurfaceManager.h"
#include "CChallengeWindow.h"
#include "CCurrency.h"
#include "CDailyDealManager.h"
#include "CDH_Weapon.h"
#include "CDH_WeaponsManager.h"
#include "CEnergy.h"
#include "CFont.h"
#include "CFontMgr.h"
#include "CGoPill.h"
#include "CGoPillManager.h"
#include "CIAPCurrency.h"
#include "CIAPCurrencyManager.h"
#include "CImageButton.h"
#include "CMiscItemManager.h"
#include "CStdUtil_Cocoa.h"
#include "CTutorialManager.h"
#include "DGHelper.h"
#include "SScreenAdaptingConstant.h"
#include "ServiceWindows.h"

#include <math.h>

namespace {
enum ItemType {
    Weapon = 0, Ammo = 2, Medkit = 3, Energy = 4, IapCurrency = 5, Cash = 7, GreenWeapon = 8,
    AmmoPack = 9, Pill = 10, Armor = 11, Misc = 12
};
const int UpdateItemLabel = (int)0x81f81622;
/** Exported event id; the original keeps it in __TEXT,__const of this unit (other units read it). */
extern const int EventId_UpdateItemLabel = (int)0x81f81622;
const SScreenAdaptingConstant BarTextMargin(2, 4);
const SScreenAdaptingConstant CardImageOffset(15, 30);
const SScreenAdaptingConstant AmountTop(15, 30);
const SScreenAdaptingConstant AmountRight(5, 10);
const SScreenAdaptingConstant SquareCaptionSpace(37, 60, 60, -1);
const SScreenAdaptingConstant OfferX[] = {
    SScreenAdaptingConstant(50, 107), SScreenAdaptingConstant(8, 16),
    SScreenAdaptingConstant(8, 16), SScreenAdaptingConstant(8, 16)
};
const SScreenAdaptingConstant OfferY[] = {
    SScreenAdaptingConstant(8, 16), SScreenAdaptingConstant(51, 104),
    SScreenAdaptingConstant(52, 104), SScreenAdaptingConstant(52, 104)
};
const SScreenAdaptingConstant WeaponOfferX[] = {
    SScreenAdaptingConstant(151, 314), SScreenAdaptingConstant(8, 17),
    SScreenAdaptingConstant(8, 17), SScreenAdaptingConstant(8, 17)
};
const SScreenAdaptingConstant WeaponOfferY[] = {
    SScreenAdaptingConstant(6, 18), SScreenAdaptingConstant(29, 64),
    SScreenAdaptingConstant(30, 64), SScreenAdaptingConstant(30, 64)
};

bool IsWeaponCard(int type)
{
    return type == Weapon || type == GreenWeapon;
}
}

class CShopItemButton::Label : public FillerBar {
public:
    Label(CItemManager* manager, int itemId, int type);
    void SetAmount();

    int m_itemId;
    int m_type;
    CItemManager* m_manager;
    TextWindow* m_text;
};

CShopItemButton::CShopItemButton(CItemManager* manager, int type, const XString& name, int unlockLevel,
                                 int, int itemId, const char* image)
    : m_label(NULL), m_type(type), m_unlockLevel(unlockLevel), m_itemId(itemId), m_disabled(false),
      m_alwaysActive(false), m_background(NULL), m_manager(manager), m_item(NULL),
      m_saleMarker(NULL), m_dealTimer(NULL), m_blink(false)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CBH_SurfaceManager* surfaces = app->m_surfaceManager;
    m_itemImage = surfaces->CreateSurface(image, true);
    CItem* item = manager->getItemById(itemId);
    m_price = item->m_pricePoints;
    int credits = item->getPriceCredits();
    if (credits > 0) {
        m_price = credits;
    }
    if (item->m_priceIAP > 0) {
        m_price = item->m_priceIAP;
    }
    if (!IsItemPurchased()) {
        m_priceText = DGHelper::FormatMoneyString(m_price);
    }

    int width = ImageWidth(m_itemImage);
    int height = ImageHeight(m_itemImage);
    const char* active = "IDB_SQUARE_SHOP_ICON_ACTIVE";
    if (type == Weapon) {
        active = "IDB_WEAPON_SHOP_ICON_ACTIVE";
        m_background.Create("SUR_SHOP_BUTTON_BACKGROUND");
    } else if (type == GreenWeapon) {
        active = "IDB_WEAPON_SHOP_ICON_ACTIVE_GREEN";
        m_background.Create("SUR_SHOP_BUTTON_BACKGROUND_GREEN");
    } else if (type == IapCurrency && static_cast<CIAPCurrency*>(item)->m_type == XString(L"tapjoy")) {
        active = "IDB_SQUARE_SHOP_ICON_ACTIVE_GREEN";
    }
    m_activeImage = surfaces->CreateSurface(active, true);
    if (IsWeaponCard(type)) {
        width = ImageWidth(m_background.GetSurface());
        height = ImageHeight(m_background.GetSurface()) + CardImageOffset();
    }
    SetSize(width, height);
    const char* currency = "SUR_HUD_ICON_MONEY";
    if (credits > 0) {
        currency = "SUR_HUD_ICON_CREDITS";
    }
    m_currencyIcon.Create(currency);
    m_userData = itemId;
    m_titleFont = CFontMgr::GetInstance()->GetFont(15);
    m_priceFont = CFontMgr::GetInstance()->GetFont(11);

    if (item->m_extraText.Length() > 0) {
        Window* spacer = new Window(false);
        spacer->SetPercentHeight(15, 0, 0);
        AddToFront(spacer, 0, 0);
        TextWindow* extra = new TextWindow(item->m_extraText, CFontMgr::GetInstance()->GetFont(17));
        extra->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
        AddToFront(extra, 0, 1);
    }
    m_name = name;
    if (type == Ammo || type == AmmoPack) {
        int ammoId = itemId;
        if (type == AmmoPack) {
            CAmmo* ammo = app->InitAmmoManager()->GetAmmoByType(static_cast<CAmmoPack*>(item)->m_ammoType);
            if (ammo != NULL) {
                ammoId = ammo->m_id;
            }
        }
        if (!app->InitWeaponManager()->HasWeaponForAmmo(ammoId)) {
            m_name = ResString("IDS_SHOP_AMMO_NO_WEAPON");
            m_disabled = true;
        }
    } else if (type == IapCurrency) {
        CIAPCurrency* pack = static_cast<CIAPCurrency*>(item);
        if (pack->m_type == XString(L"tapjoy")) {
            m_name = pack->m_name;
            m_titleFont = CFontMgr::GetInstance()->GetFont(17);
        } else if (pack->isAvailable()) {
            m_name = XString("IAP");
        } else if (app->InitIAPCurrencyManager()->IsRetryWaiting()) {
            m_name = ResString("IDS_SHOP_IAP_RETRY");
        } else {
            m_name = ResString("IDS_SHOP_IAP_LOADING");
        }
    }

    if (!item->isAvailable()) {
        if (item->m_new) {
            AddOfferMarker("IDB_SHOP_ICON_NEW", 0);
        }
        if (item->m_sale || item->m_field56) {
            m_saleMarker = AddOfferMarker("IDB_SHOP_ICON_SALE", 1);
        }
        if (item->m_limited) {
            AddOfferMarker("IDB_SHOP_ICON_LIMITED", 2);
        }
        if (item->m_featured) {
            AddOfferMarker("IDB_SHOP_ICON_FEATURED", 3);
        }
    }
    bool ammoOrMedkit = type == Ammo || type == AmmoPack || type == Medkit;
    bool miscWithAmount = type == Misc && itemId != 2;
    if (ammoOrMedkit || type == Pill || type == Armor || miscWithAmount) {
        m_label = CreateLabel();
        UpdateLabel();
    }
    m_debugFrame = true;
    if (type == Weapon && static_cast<CDH_Weapon*>(item)->m_bonusXp > 0) {
        CImageButton* bonus = new CImageButton();
        bonus->AddImage("IDB_EXP_PLUS_SHOP_ICON", 0, 0);
        bonus->SetLayoutType(LAYOUT_NONE);
        bonus->SetCorner(SScreenAdaptingConstant(10, 21)(), CardImageOffset() + SScreenAdaptingConstant(8, 22)());
        bonus->ClearFlags(FLAG_ENABLED | FLAG_FOCUSABLE);
        AddToFront(bonus);
    }
    if (type == GreenWeapon) {
        m_titleFont = CFontMgr::GetInstance()->GetFont(17);
    }
    if (type == IapCurrency) {
        height += m_priceFont->GetFontHeight();
    } else if (type != Medkit || !m_disabled) {
        height += m_titleFont->GetFontHeight() + m_priceFont->GetFontHeight();
        if (!IsWeaponCard(type)) {
            height += SquareCaptionSpace();
        }
    }
    SetSize(width, height);
    if (IsWeaponCard(type)) {
        SetDesiredWidth(width, width, width);
        SetDesiredHeight(height, height, height);
    }
    AddPackAmount(item);

    m_title = new TextWindow(m_name, m_titleFont);
    m_title->SetLayoutType(LAYOUT_NONE);
    m_title->SetWidthByContent(0, ImageWidth(m_itemImage));
    m_title->SetCorner(0, ImageHeight(m_itemImage));
    m_title->SetAlign(ALIGN_HCENTER);
    AddToFront(m_title);
    if (app->InitDailyDealManager()->GetCurrentDealItem() == item) {
        AddDealTimer(item);
    }
    m_item = item;
}

Window* CShopItemButton::AddOfferMarker(const char* image, int index)
{
    CImageButton* marker = new CImageButton();
    marker->AddImage(image, 0, 0);
    marker->SetLayoutType(LAYOUT_NONE);
    if (IsWeaponCard(m_type)) {
        marker->SetCorner(WeaponOfferX[index](), CardImageOffset() + WeaponOfferY[index]());
    } else {
        marker->SetCorner(OfferX[index](), OfferY[index]());
    }
    marker->ClearFlags(FLAG_ENABLED | FLAG_FOCUSABLE);
    AddToFront(marker);
    return marker;
}

void CShopItemButton::AddPackAmount(CItem* item)
{
    const char* icon = NULL;
    int amount = 0;
    XString ammoIconName;
    if (m_type == Ammo) {
        ammoIconName = static_cast<CAmmo*>(item)->m_bulletIcon;
    }
    XString::AnsiString ammoIcon(ammoIconName.c_str());
    switch (m_type) {
    case Ammo:
        if (m_disabled) {
            return;
        }
        amount = static_cast<CAmmo*>(item)->m_countInPack;
        icon = ammoIcon;
        break;
    case AmmoPack:
        if (m_disabled) {
            return;
        }
        amount = static_cast<CAmmoPack*>(item)->m_countInPack;
        icon = "IDB_HUD_HANDGUN_BULLET_ICON";
        break;
    case Energy:
        amount = static_cast<CEnergy*>(item)->m_energy;
        icon = "IDB_HUD_ICON_ENERGY";
        break;
    case Cash:
        amount = static_cast<CCurrency*>(item)->m_cashGain;
        icon = "IDB_HUD_ICON_MONEY";
        break;
    case IapCurrency: {
        CIAPCurrency* pack = static_cast<CIAPCurrency*>(item);
        if (!pack->isAvailable() || pack->m_type == XString(L"tapjoy")) {
            return;
        }
        if (pack->m_creditsGain != 0) {
            amount = pack->m_creditsGain;
            icon = "IDB_HUD_ICON_CREDITS";
        } else if (pack->m_cashGain != 0) {
            amount = pack->m_cashGain;
            icon = "IDB_HUD_ICON_MONEY";
        }
        break;
    }
    default:
        return;
    }
    LabelWithIcon* label = new LabelWithIcon(icon, amount);
    label->SetAlign(ALIGN_RIGHT | ALIGN_TOP);
    label->SetOutsetSpacing(AmountTop() + ImageHeight(m_itemImage) / 2, AmountRight(), 0, 0);
    AddToFront(label);
}

void CShopItemButton::AddDealTimer(CItem* item)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    m_dealTimer = new Window(false);
    m_dealTimer->SetPercentHeight(100, 0, 0);
    m_dealTimer->SetPercentWidth(100, 0, 0);
    AddToFront(m_dealTimer);
    Window* spacer = new Window(false);
    m_dealTimer->AddToFront(spacer, 0, 0);
    spacer->SetPercentHeight(15, 0, 0);
    CChallengeTimerWindow* timer = new CChallengeTimerWindow(app->InitDailyDealManager()->GetNextDealStartTime(),
                                                              CFontMgr::GetInstance()->GetFont(8));
    timer->SetTimeFormat(XString("%02d:%02d"));
    timer->m_useChallengeTime = false;
    if (item->getType() != Weapon) {
        timer->SetAlign(ALIGN_LEFT | ALIGN_TOP);
        Window* row = new Window(false);
        row->SetPercentWidth(100, 0, 0);
        m_dealTimer->AddToFront(row);
        Window* margin = new Window(false);
        row->AddToFront(margin, 0, 0);
        margin->SetPercentWidth(5, 0, 0);
        row->AddToFront(timer, 1, 0);
    } else {
        timer->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
        m_dealTimer->AddToFront(timer, 0, 1);
    }
}

bool CShopItemButton::IsItemPurchased()
{
    if (m_type == IapCurrency) {
        return false;
    }
    return m_manager->getItemById(m_itemId)->isAvailable();
}

CShopItemButton::Label* CShopItemButton::CreateLabel()
{
    Label* label = new Label(m_manager, m_itemId, m_type);
    label->SetAlign(ALIGN_RIGHT | ALIGN_TOP);
    SetSizesByContent(label, 3);
    AddToFront(label);
    return label;
}

void CShopItemButton::UpdateLabel()
{
    if (m_label == NULL) {
        return;
    }
    switch (m_type) {
    case Ammo:
    case AmmoPack:
        m_label->SetAmount();
        break;
    case Medkit:
        m_label->m_text->SetText(XString(CBH_Player::GetInstance()->m_medkits));
        break;
    case Pill:
        UpdateGoPills();
        break;
    case Armor: {
        int amount = 0;
        CBH_Player* player = CBH_Player::GetInstance();
        if (player->m_armorId == m_itemId) {
            amount = player->m_armorDamageProof / 1000;
        }
        m_label->m_text->SetText(XString(amount));
        break;
    }
    case Misc:
        UpdateMisc();
        break;
    }
}

void CShopItemButton::Paint(ICGraphics2d& graphics)
{
    int y = 0;
    if (IsWeaponCard(m_type)) {
        y = CardImageOffset();
    }
    ICRenderSurface* background = m_background.GetSurface();
    if (background != NULL) {
        DrawImage(graphics, background, 0, y, 0, 0);
    }
    bool blend = m_disabled || m_blink;
    if (blend) {
        float opacity = 0.5f;
        if (!m_disabled) {
            int phase = static_cast<int>(CStdUtil_Cocoa::GetUpTimeMS() % 600) - 300;
            opacity = 0.2f + fabsf(static_cast<float>(phase)) / 360.0f;
        }
        int margin = App::Scale() * 7;
        FillRect(graphics, margin, margin + y, ImageWidth(m_itemImage) - 2 * margin,
                 ImageHeight(m_itemImage) - 2 * margin, 0xff000000);
        BeginBlend(graphics, opacity, 1.0f, 1.0f, 1.0f, true, false);
    }
    DrawImage(graphics, m_itemImage, 0, y, 0, 0);
    if ((m_pressed && !m_disabled) || m_alwaysActive) {
        DrawImage(graphics, m_activeImage, 0, y, 0, 0);
    }
    if (m_type != IapCurrency && !IsItemPurchased()) {
        int priceY = ImageHeight(m_itemImage) + m_title->m_height;
        ICRenderSurface* currency = m_currencyIcon.GetSurface();
        int center = m_width / 2;
        DrawImage(graphics, currency, center - ImageWidth(currency), priceY, 0, 0);
        DrawFontText(graphics, m_priceFont, m_priceText.c_str(), center, priceY, 0);
    }
    if (blend) {
        EndBlend(graphics);
    }
    if (m_type != IapCurrency) {
        Window::Paint(graphics);
    }
}

void CShopItemButton::Update()
{
    if (m_disabled && m_type == IapCurrency) {
        int phase = static_cast<int>(CStdUtil_Cocoa::GetUpTimeMS() % 1100) - 550;
        SetOpacity(0.2f + fabsf(static_cast<float>(phase)) / 660.0f);
    }
    if (m_type == Ammo) {
        m_blink = CTutorialManager::IsAmmoBlink(m_itemId);
    }
    if (m_dealTimer != NULL) {
        App* app = static_cast<App*>(WindowApp::m_instance);
        if (app->InitDailyDealManager()->GetCurrentDealItem() != m_item) {
            m_dealTimer->Close();
            m_dealTimer = NULL;
            if (m_saleMarker != NULL) {
                m_saleMarker->Close();
                m_saleMarker = NULL;
            }
            m_item->m_sale = false;
        }
    }
}

CShopItemButton::Label::Label(CItemManager* manager, int itemId, int type)
    : m_itemId(itemId), m_type(type), m_manager(manager), m_text(NULL)
{
    m_text = new TextWindow(L"", CFontMgr::GetInstance()->GetFont(8));
    m_text->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    m_text->SetOutsetSpacing(BarTextMargin(), 0, 0, 0);
    AddToFront(m_text);
    SetAmount();
}

void CShopItemButton::Label::SetAmount()
{
    XString amount(m_manager->getAmount(m_itemId));
    if (m_text != NULL) {
        m_text->SetText(amount);
    }
}

CShopItemButton::~CShopItemButton()
{
    CBH_SurfaceManager* surfaces = static_cast<App*>(WindowApp::m_instance)->m_surfaceManager;
    surfaces->ReleaseSurface(m_activeImage);
    surfaces->ReleaseSurface(m_itemImage);
}

int CShopItemButton::GetUnlockLevel()
{
    return m_unlockLevel;
}

void CShopItemButton::SetAlwaysActive(bool active)
{
    m_alwaysActive = active;
    m_title->SetFont(m_titleFont);
}

void CShopItemButton::UpdateGoPills()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    const CGoPill* pill = app->m_goPillManager->GetPillById(m_itemId);
    if (pill == NULL) {
        return;
    }
    CBH_Player* player = CBH_Player::GetInstance();
    if (pill->m_pills == 0) {
        int seconds = player->GetReactixControlTimeSec();
        m_label->m_text->SetText(XString::Format(L"%02d:%02d", seconds / 60, seconds % 60));
    } else {
        m_label->m_text->SetText(XString(player->GetPillsCount()));
    }
}

void CShopItemButton::UpdateMisc()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CItem* item = app->InitMiscItemManager()->getItemById(m_itemId);
    if (item != NULL) {
        m_label->m_text->SetText(XString(item->getCurrentAmount()));
    }
}

void CShopItemButton::OnCommand(Event& event)
{
    if (event.m_id == UpdateItemLabel) {
        UpdateLabel();
    }
}

void CShopItemButton::SetActiveImage(const char* image)
{
    CBH_SurfaceManager* surfaces = static_cast<App*>(WindowApp::m_instance)->m_surfaceManager;
    surfaces->ReleaseSurface(m_activeImage);
    m_activeImage = surfaces->CreateSurface(image, true);
}
