// CShopItemButton.cpp - render the configured item and its purchase information.
#include "CShopItemButton.h"

#include "App.h"
#include "CAmmo.h"
#include "CAmmoManager.h"
#include "CAmmoPack.h"
#include "CAmmoPackManager.h"
#include "CBH_Player.h"
#include "CBH_SurfaceManager.h"
#include "CChallengeWindow.h"
#include "CColor.h"
#include "CCurrencyManager.h"
#include "CCurrency.h"
#include "CDailyDealManager.h"
#include "CDH_Weapon.h"
#include "CDH_WeaponsManager.h"
#include "CEnergy.h"
#include "CEnergyManager.h"
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

/** Exported event id; the original keeps it in __TEXT,__const of this unit (other units read it). */
extern const int EventId_UpdateItemLabel = (int)0x81f81622;

namespace {
enum ItemType {
    Weapon = 0, Ammo = 2, Medkit = 3, Energy = 4, IapCurrency = 5, Cash = 7, GreenWeapon = 8,
    AmmoPack = 9, Pill = 10, Armor = 11, Misc = 12
};
const int UpdateItemLabel = (int)0x81f81622;
const SScreenAdaptingConstant AmountTop(15, 30);
const SScreenAdaptingConstant CardImageOffset(15, 30);
const SScreenAdaptingConstant AmountRight(5, 10);
const SScreenAdaptingConstant XPBonusX(10, 21);
const SScreenAdaptingConstant XPBonusY(8, 22);

struct OfferMarkerPosition {
    SScreenAdaptingConstant x;
    SScreenAdaptingConstant y;
};

const OfferMarkerPosition OfferPositions[] = {
    { SScreenAdaptingConstant(50, 107), SScreenAdaptingConstant(8, 16) },
    { SScreenAdaptingConstant(8, 16), SScreenAdaptingConstant(51, 104) },
    { SScreenAdaptingConstant(8, 16), SScreenAdaptingConstant(52, 104) },
    { SScreenAdaptingConstant(8, 16), SScreenAdaptingConstant(52, 104) }
};
const OfferMarkerPosition WeaponOfferPositions[] = {
    { SScreenAdaptingConstant(151, 314), SScreenAdaptingConstant(6, 18) },
    { SScreenAdaptingConstant(8, 17), SScreenAdaptingConstant(29, 64) },
    { SScreenAdaptingConstant(8, 17), SScreenAdaptingConstant(30, 64) },
    { SScreenAdaptingConstant(8, 17), SScreenAdaptingConstant(30, 64) }
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
    /** Original inline helper: set the amount text when the text window exists. */
    void SetText(const XString& text)
    {
        if (m_text != NULL) {
            m_text->SetText(text);
        }
    }

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
    // The original calls the base constructor again here; it only builds a temporary.
    CBH_Button();
    m_itemImage = static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->CreateSurface(image, true);
    CItem* item = manager->getItemById(itemId);
    m_price = item->m_pricePoints;
    if (item->getPriceCredits() > 0) {
        m_price = item->getPriceCredits();
    }
    if (item->m_priceIAP > 0) {
        m_price = item->m_priceIAP;
    }
    if (!IsItemPurchased()) {
        m_priceText = DGHelper::FormatMoneyString(m_price);
    }

    if (m_type == Weapon) {
        m_activeImage = static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->CreateSurface(
            "IDB_WEAPON_SHOP_ICON_ACTIVE", true);
        m_background.Create("SUR_SHOP_BUTTON_BACKGROUND");
        SetSize(ImageWidth(m_background.GetSurface()), ImageHeight(m_background.GetSurface()) + CardImageOffset());
    } else if (m_type == GreenWeapon) {
        m_activeImage = static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->CreateSurface(
            "IDB_WEAPON_SHOP_ICON_ACTIVE_GREEN", true);
        m_background.Create("SUR_SHOP_BUTTON_BACKGROUND_GREEN");
        SetSize(ImageWidth(m_background.GetSurface()), ImageHeight(m_background.GetSurface()) + CardImageOffset());
    } else {
        const char* active = "IDB_SQUARE_SHOP_ICON_ACTIVE";
        if (m_type == IapCurrency) {
            CIAPCurrency* pack = static_cast<CIAPCurrency*>(
                static_cast<App*>(WindowApp::m_instance)->InitIAPCurrencyManager()->getItemById(m_itemId));
            if (XString(pack->m_type) == L"tapjoy") {
                active = "IDB_SQUARE_SHOP_ICON_ACTIVE_GREEN";
            }
        }
        m_activeImage = static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->CreateSurface(active, true);
        SetSize(ImageWidth(m_itemImage), ImageHeight(m_itemImage));
    }
    if (item->getPriceCredits() > 0) {
        m_currencyIcon.Create("SUR_HUD_ICON_CREDITS");
    } else {
        m_currencyIcon.Create("SUR_HUD_ICON_MONEY");
    }
    m_userData = itemId;
    m_titleFont = CFontMgr::GetInstance()->GetFont(15);
    m_priceFont = CFontMgr::GetInstance()->GetFont(11);

    // Byte size >= 4, i.e. at least one character.
    XString extraText = item->m_extraText;
    if (extraText.Length() > 0) {
        Window* frame = new Window(false);
        frame->m_debugFrame = true;
        frame->SetPercentHeight(100, 0, 0);
        AddToFront(frame);
        Window* spacer = new Window(false);
        AddToFront(spacer, 0, 0);
        spacer->SetPercentHeight(15, 0, 0);
        TextWindow* extra = new TextWindow(extraText, CFontMgr::GetInstance()->GetFont(17));
        extra->SetLayoutType(LAYOUT_SIMPLE);
        extra->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
        AddToFront(extra, 0, 1);
    }
    m_name = name;
    if (type == Ammo || type == AmmoPack) {
        int ammoId = m_itemId;
        if (type == AmmoPack) {
            CItem* pack = static_cast<App*>(WindowApp::m_instance)->m_ammoPackManager->getItemById(ammoId);
            XString ammoType = static_cast<CAmmoPack*>(pack)->m_ammoType;
            ammoId = static_cast<App*>(WindowApp::m_instance)->InitAmmoManager()->GetAmmoByType(ammoType)->m_id;
        }
        if (!static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->HasWeaponForAmmo(ammoId)) {
            m_name = ResString("IDS_SHOP_AMMO_NO_WEAPON");
            m_disabled = true;
        }
    } else if (type == IapCurrency) {
        bool tapjoy = XString(static_cast<CIAPCurrency*>(
            static_cast<App*>(WindowApp::m_instance)->InitIAPCurrencyManager()->getItemById(m_itemId))->m_type) ==
            L"tapjoy";
        CIAPCurrency* pack = static_cast<CIAPCurrency*>(
            static_cast<App*>(WindowApp::m_instance)->InitIAPCurrencyManager()->getItemById(m_itemId));
        if (tapjoy) {
            m_name = pack->m_name;
            m_titleFont = CFontMgr::GetInstance()->GetFont(17);
        } else if (pack->isAvailable()) {
            m_name = XString("IAP");
        } else if (static_cast<App*>(WindowApp::m_instance)->InitIAPCurrencyManager()->IsRetryWaiting()) {
            m_name = ResString("IDS_SHOP_IAP_RETRY");
        } else {
            m_name = ResString("IDS_SHOP_IAP_LOADING");
        }
    }

    // Offer badges (no ClearFlags in the original).
    if (!item->isAvailable()) {
        if (item->m_new) {
            CImageButton* marker = new CImageButton();
            marker->AddImage("IDB_SHOP_ICON_NEW", 0, 0);
            marker->SetLayoutType(LAYOUT_NONE);
            if (IsWeaponCard(type)) {
                marker->SetCorner(WeaponOfferPositions[0].x(), WeaponOfferPositions[0].y() + CardImageOffset());
            } else {
                marker->SetCorner(OfferPositions[0].x(), OfferPositions[0].y());
            }
            AddToFront(marker);
        }
        if (item->m_sale || item->m_field56) {
            CImageButton* marker = new CImageButton();
            m_saleMarker = marker;
            marker->AddImage("IDB_SHOP_ICON_SALE", 0, 0);
            m_saleMarker->SetLayoutType(LAYOUT_NONE);
            Window* saleMarker = m_saleMarker;
            if (IsWeaponCard(type)) {
                int x = WeaponOfferPositions[1].x();
                int y = WeaponOfferPositions[1].y() + CardImageOffset();
                saleMarker->SetCorner(x, y);
            } else {
                int x = OfferPositions[1].x();
                int y = OfferPositions[1].y();
                saleMarker->SetCorner(x, y);
            }
            AddToFront(m_saleMarker);
        }
        if (item->m_limited) {
            CImageButton* marker = new CImageButton();
            marker->AddImage("IDB_SHOP_ICON_LIMITED", 0, 0);
            marker->SetLayoutType(LAYOUT_NONE);
            if (IsWeaponCard(type)) {
                marker->SetCorner(WeaponOfferPositions[2].x(), WeaponOfferPositions[2].y() + CardImageOffset());
            } else {
                marker->SetCorner(OfferPositions[2].x(), OfferPositions[2].y());
            }
            AddToFront(marker);
        }
        if (item->m_featured) {
            CImageButton* marker = new CImageButton();
            marker->AddImage("IDB_SHOP_ICON_FEATURED", 0, 0);
            marker->SetLayoutType(LAYOUT_NONE);
            if (IsWeaponCard(type)) {
                marker->SetCorner(WeaponOfferPositions[3].x(), WeaponOfferPositions[3].y() + CardImageOffset());
            } else {
                marker->SetCorner(OfferPositions[3].x(), OfferPositions[3].y());
            }
            AddToFront(marker);
        }
    }

    bool ammoOrMedkit = type == Ammo || type == Medkit || type == AmmoPack;
    if (ammoOrMedkit && !m_disabled) {
        SetSize(m_width, m_height + m_titleFont->GetFontHeight() + m_priceFont->GetFontHeight() +
                             SScreenAdaptingConstant(37, 60, 60)());
    }
    if (m_type == Pill || m_type == Armor || type == Misc || ammoOrMedkit) {
        if (type != Misc || item->m_id != 2) {
            m_label = CreateLabel();
            UpdateLabel();
        }
    }
    m_debugFrame = true;
    switch (type) {
    case Ammo:
    case AmmoPack:
        if (!m_disabled) {
            if (type == Ammo) {
                int count = static_cast<CAmmo*>(
                    static_cast<App*>(WindowApp::m_instance)->InitAmmoManager()->getItemById(m_itemId))->m_countInPack;
                LabelWithIcon* label = new LabelWithIcon(
                    XString::AnsiString(XString(static_cast<CAmmo*>(item)->m_bulletIcon).c_str()), count);
                label->SetAlign(ALIGN_RIGHT | ALIGN_TOP);
                int imageHeight = ImageHeight(m_itemImage);
                int top = AmountTop();
                label->SetOutsetSpacing(top + imageHeight / 2, AmountRight(), 0, 0);
                AddToFront(label);
            } else {
                int count = 0;
                if (type == AmmoPack) {
                    count = static_cast<CAmmoPack*>(
                        static_cast<App*>(WindowApp::m_instance)->m_ammoPackManager->getItemById(m_itemId))->m_countInPack;
                }
                LabelWithIcon* label = new LabelWithIcon("IDB_HUD_HANDGUN_BULLET_ICON", count);
                label->SetAlign(ALIGN_RIGHT | ALIGN_TOP);
                int imageHeight = ImageHeight(m_itemImage);
                int top = AmountTop();
                label->SetOutsetSpacing(top + imageHeight / 2, AmountRight(), 0, 0);
                AddToFront(label);
            }
            break;
        }
        // disabled ammo: fall through to the default size
    default:
        if (type == GreenWeapon || type == Weapon) {
            if (type == GreenWeapon) {
                m_titleFont = CFontMgr::GetInstance()->GetFont(17);
            } else {
                WindowTransparent* holder = new WindowTransparent(false);
                CDH_Weapon* weapon = static_cast<CDH_Weapon*>(
                    static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->getItemById(m_itemId));
                SetSizesByContent(holder, 3);
                holder->SetOutsetSpacing(0, AmountRight(), 0, 0);
                AddToFront(holder);
                if (weapon->m_bonusXp > 0) {
                    CImageButton* bonus = new CImageButton();
                    bonus->AddImage("IDB_EXP_PLUS_SHOP_ICON", 0, 0);
                    bonus->SetLayoutType(LAYOUT_NONE);
                    bonus->SetCorner(XPBonusX(), XPBonusY() + CardImageOffset());
                    AddToFront(bonus);
                }
            }
            m_debugFrame = true;
            int width = m_width;
            int height = m_height + m_titleFont->GetFontHeight() + m_priceFont->GetFontHeight();
            SetSize(width, height);
            SetDesiredWidth(width, width, width);
            SetDesiredHeight(height, height, height);
        } else if (type == IapCurrency) {
            SetSize(m_width, m_height + m_priceFont->GetFontHeight());
            CIAPCurrency* pack = static_cast<CIAPCurrency*>(
                static_cast<App*>(WindowApp::m_instance)->InitIAPCurrencyManager()->getItemById(m_itemId));
            bool available = static_cast<App*>(WindowApp::m_instance)->InitIAPCurrencyManager()->getItemById(
                m_itemId)->isAvailable();
            if (!(XString(pack->m_type) == L"tapjoy") && available) {
                const char* icon = NULL;
                int amount = 0;
                if (pack->m_creditsGain != 0) {
                    icon = "IDB_HUD_ICON_CREDITS";
                    amount = pack->m_creditsGain;
                } else if (pack->m_cashGain != 0) {
                    icon = "IDB_HUD_ICON_MONEY";
                    amount = pack->m_cashGain;
                }
                LabelWithIcon* label = new LabelWithIcon(icon, amount);
                label->SetAlign(ALIGN_RIGHT | ALIGN_TOP);
                int imageHeight = ImageHeight(m_itemImage);
                int top = AmountTop();
                label->SetOutsetSpacing(top + imageHeight / 2, AmountRight(), 0, 0);
                AddToFront(label);
            }
        } else {
            SetSize(m_width, m_height + m_titleFont->GetFontHeight() + m_priceFont->GetFontHeight() +
                                 SScreenAdaptingConstant(37, 60, 60)());
        }
        break;
    case Medkit:
        break;
    case Energy: {
        LabelWithIcon* label = new LabelWithIcon(
            "IDB_HUD_ICON_ENERGY",
            static_cast<App*>(WindowApp::m_instance)->m_energyManager->getEnergyItem(m_itemId)->m_energy);
        label->SetAlign(ALIGN_RIGHT | ALIGN_TOP);
        int imageHeight = ImageHeight(m_itemImage);
        int top = AmountTop();
        label->SetOutsetSpacing(top + imageHeight / 2, AmountRight(), 0, 0);
        AddToFront(label);
        SetSize(m_width, m_height + m_titleFont->GetFontHeight() + m_priceFont->GetFontHeight() +
                             SScreenAdaptingConstant(37, 60, 60)());
        break;
    }
    case Cash: {
        LabelWithIcon* label = new LabelWithIcon(
            "IDB_HUD_ICON_MONEY",
            static_cast<App*>(WindowApp::m_instance)->m_currencyManager->getCurrencyById(m_itemId)->m_cashGain);
        label->SetAlign(ALIGN_RIGHT | ALIGN_TOP);
        int imageHeight = ImageHeight(m_itemImage);
        int top = AmountTop();
        label->SetOutsetSpacing(top + imageHeight / 2, AmountRight(), 0, 0);
        AddToFront(label);
        SetSize(m_width, m_height + m_titleFont->GetFontHeight() + m_priceFont->GetFontHeight() +
                             SScreenAdaptingConstant(37, 60, 60)());
        break;
    }
    }

    m_title = new TextWindow(m_name, m_titleFont);
    SScreenAdaptingConstant titleTop(ImageHeight(m_itemImage));
    m_title->SetLayoutType(LAYOUT_NONE);
    m_title->SetLayoutWidth(UnitValue(UNIT_CONTENT, 0, 0, ImageWidth(m_itemImage)));
    m_title->SetCorner(0, titleTop());
    m_title->SetAlign(ALIGN_HCENTER);
    AddToFront(m_title);
    if (static_cast<App*>(WindowApp::m_instance)->InitDailyDealManager()->GetCurrentDealItem() == item) {
        Window* timerArea = new Window(false);
        m_dealTimer = timerArea;
        timerArea->SetPercentHeight(100, 0, 0);
        m_dealTimer->SetPercentWidth(100, 0, 0);
        AddToFront(m_dealTimer);
        Window* spacer = new Window(false);
        m_dealTimer->AddToFront(spacer, 0, 0);
        spacer->SetPercentHeight(15, 0, 0);
        CChallengeTimerWindow* timer = new CChallengeTimerWindow(
            static_cast<App*>(WindowApp::m_instance)->InitDailyDealManager()->GetNextDealStartTime(),
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
    m_item = item;
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
    case Medkit:
        m_label->SetText(XString(CBH_Player::GetInstance()->m_medkits));
        break;
    case Pill:
        UpdateGoPills();
        break;
    case Ammo:
    case AmmoPack:
        m_label->SetAmount();
        break;
    case Armor:
        if (CBH_Player::GetInstance()->m_armorId == m_itemId) {
            m_label->SetText(XString(CBH_Player::GetInstance()->m_armorDamageProof / 1000));
        } else {
            m_label->SetText(XString(0));
        }
        break;
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
    if (m_background.GetSurface() != NULL) {
        DrawImage(graphics, m_background.GetSurface(), 0, y, 0, 0);
    }

    int scale = App::Scale();
    bool blend = false;
    float opacity;
    if (m_disabled) {
        opacity = 0.5f;
        blend = true;
    } else if (m_blink) {
        unsigned int uptime = CStdUtil_Cocoa::GetUpTimeMS();
        int phase = static_cast<int>(uptime % 600) - 300;
        opacity = fabsf(static_cast<float>(phase)) / 360.0f + 0.2f;
        blend = true;
    }
    if (blend) {
        int margin = scale * 7;
        int itemWidth = ImageWidth(m_itemImage);
        int itemHeight = ImageHeight(m_itemImage);
        FillRect(graphics, margin, margin + y, itemWidth - 2 * margin, itemHeight - 2 * margin,
                 Color_A8R8G8B8_t::Black.value);
        BeginBlend(graphics, opacity, 1.0f, 1.0f, 1.0f, true, false);
    }

    DrawImage(graphics, m_itemImage, 0, y, 0, 0);
    if ((m_pressed && !m_disabled) || m_alwaysActive) {
        DrawImage(graphics, m_activeImage, 0, y, 0, 0);
    }

    int imageHeight = ImageHeight(m_itemImage);
    if (m_type == IapCurrency) {
        if (blend) {
            EndBlend(graphics);
        }
        return;
    }

    int titleHeight = m_title->m_height;
    if (!IsItemPurchased()) {
        int priceY = imageHeight + titleHeight;
        ICRenderSurface* currency = m_currencyIcon.GetSurface();
        int center = m_width / 2;
        int currencyWidth = ImageWidth(m_currencyIcon.GetSurface());
        DrawImage(graphics, currency, center - currencyWidth, priceY, 0, 0);
        DrawFontText(graphics, m_priceFont, m_priceText.c_str(), m_width / 2, priceY, 0);
    }
    if (blend) {
        EndBlend(graphics);
    }
    Window::Paint(graphics);
}

void CShopItemButton::Update()
{
    if (m_disabled && m_type == IapCurrency) {
        // The original blinks the title only (unsigned up time).
        TextWindow* title = m_title;
        unsigned int uptime = CStdUtil_Cocoa::GetUpTimeMS();
        int phase = static_cast<int>(uptime % 1100) - 550;
        title->SetOpacity(fabsf(static_cast<float>(phase)) / 660.0f + 0.2f);
    }
    if (m_type == Ammo) {
        m_blink = static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsAmmoBlink(m_itemId) == 1;
    }
    if (m_dealTimer != NULL) {
        if (static_cast<App*>(WindowApp::m_instance)->InitDailyDealManager()->GetCurrentDealItem() == m_item) {
            return;
        }
        m_dealTimer->Close();
        m_dealTimer = NULL;
        // The original closes the sale badge without a null check.
        m_saleMarker->Close();
        m_saleMarker = NULL;
        m_item->m_sale = false;
    }
}

CShopItemButton::Label::Label(CItemManager* manager, int itemId, int type)
    : m_itemId(itemId), m_type(type), m_manager(manager), m_text(NULL)
{
    m_text = new TextWindow(L"", CFontMgr::GetInstance()->GetFont(8));
    m_text->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    m_text->SetOutsetSpacing(MARGIN_BARTEXT(), 0, 0, 0);
    AddToFront(m_text);
    SetAmount();
}

void CShopItemButton::Label::SetAmount()
{
    SetText(XString(m_manager->getAmount(m_itemId)));
}

CShopItemButton::~CShopItemButton()
{
    static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(m_activeImage);
    static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(m_itemImage);
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
    const CGoPill* pill = static_cast<App*>(WindowApp::m_instance)->m_goPillManager->GetPillById(m_itemId);
    if (pill == NULL) {
        return;
    }
    if (pill->m_pills == 0) {
        int seconds = CBH_Player::GetInstance()->GetReactixControlTimeSec();
        m_label->SetText(XString::Format(L"%02d:%02d", seconds / 60, seconds % 60));
    } else {
        m_label->SetText(XString(CBH_Player::GetInstance()->GetPillsCount()));
    }
}

void CShopItemButton::UpdateMisc()
{
    CItem* item = static_cast<App*>(WindowApp::m_instance)->InitMiscItemManager()->getItemById(m_itemId);
    if (item != NULL) {
        m_label->SetText(XString(item->getCurrentAmount()));
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
    // the original reads the surface manager for each call
    static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(m_activeImage);
    m_activeImage = static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->CreateSurface(image, true);
}
