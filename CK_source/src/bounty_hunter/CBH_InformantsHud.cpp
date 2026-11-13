#include "CBH_InformantsHud.h"

#include "App.h"
#include "BaseDialog.h"
#include "CBH_GPSMap.h"
#include "CCountNotificationWindow.h"
#include "CFontMgr.h"
#include "CNewsFeedManager.h"
#include "CTutorialManager.h"
#include "SScreenAdaptingConstant.h"

typedef char CKHudBarButtonSize[sizeof(HudBarButton) == 0x100 ? 1 : -1];
typedef char CKInformantsHudSize[sizeof(CBH_InformantsHud) == 0xcc ? 1 : -1];

static const SScreenAdaptingConstant BUTTON_NAME_Y_OFFSET(36, 75);

HudBarButton::HudBarButton(const char* image, const char* glow)
    : CImageButton()
{
    SetSize(0, 0);
    m_glow = NULL;
    SetHeightByContent(0, 0);
    SetWidthByContent(0, 0);
    if (glow != NULL) {
        m_glow = new ImageWindow();
        m_glow->SetImageById(glow);
        AddToFront(m_glow);
    }
    AddImage(image, 0, 0);
    // The original hides the glow without a null check (every caller passes one).
    m_glow->ClearFlags(FLAG_VISIBLE);
}

__attribute__((noinline))
void HudBarButton::SetText(XString text, int align, CFont* font, int topSpacing, int)
{
    CImageButton::SetText(text, align, font, topSpacing, 0);
    m_textWindow->SetFlags(FLAG_CLIP_SCREEN);
}

void HudBarButton::Update()
{
    // The original only toggles the glow (no base Update, no null check).
    if (m_pressed) {
        m_glow->SetFlags(FLAG_VISIBLE);
    } else {
        m_glow->ClearFlags(FLAG_VISIBLE);
    }
}

CBH_InformantsHud::CBH_InformantsHud()
    : Window(false), m_coopNotification(NULL), m_friendsNotification(NULL)
{
    // The original leaves the button pointers to Init and builds the bar right here.
    Init();
}

void CBH_InformantsHud::OnStartClosing()
{
}

void CBH_InformantsHud::OnCommand(Event& event)
{
}

void CBH_InformantsHud::OnGetFocus()
{
}

void CBH_InformantsHud::OnLostFocus()
{
}

void CBH_InformantsHud::Init()
{
    // The constructor builds the shortcuts before the map loads their unlocks.
    // The window tree calls Init again when it first attaches this HUD; the original has no guard
    // and builds the whole bar a second time (kept as in the original).
    Window* left = new Window(false);
    left->SetAlign(ALIGN_LEFT | ALIGN_BOTTOM);
    left->SetWidthByContent(0, 0);
    left->SetHeightByContent(0, 0);
    m_friendsNotification = new CCountNotificationWindow();
    m_friends = new HudBarButton("IDB_SIDEBAR_FRIENDS", "IDB_SIDEBAR_ICON_GLOW");
    m_friends->SetWidthByContent(0, 0);
    m_friends->SetHeightByContent(0, 0);
    m_friends->SetCommandParameters(Friends, 0, 0, 0);
    m_friends->AddToFront(m_friendsNotification);
    left->AddToFront(m_friends, 0, 0);
    m_friends->SetOpacity(0.3f);
    m_friends->ClearFlags(FLAG_ENABLED);
    m_target = new HudBarButton("IDB_SIDEBAR_TARGET", "IDB_SIDEBAR_ICON_GLOW");
    m_target->SetWidthByContent(0, 0);
    m_target->SetHeightByContent(0, 0);
    m_target->SetCommandParameters(Target, 0, 0, 0);
    left->AddToFront(m_target, 1, 0);
    m_target->SetOpacity(0.3f);
    m_target->ClearFlags(FLAG_ENABLED);
    m_coopNotification = new CCountNotificationWindow();
    m_coop = new HudBarButton("IDB_SIDEBAR_COOP", "IDB_SIDEBAR_ICON_GLOW");
    m_coop->SetWidthByContent(0, 0);
    m_coop->SetHeightByContent(0, 0);
    m_coop->SetCommandParameters(Coop, 0, 0, 0);
    m_coop->AddToFront(m_coopNotification);
    left->AddToFront(m_coop, 2, 0);
    m_coop->SetOpacity(0.3f);
    m_coop->ClearFlags(FLAG_ENABLED);
    m_weapon = new HudBarButton("IDB_SIDEBAR_WEAPON", "IDB_SIDEBAR_ICON_GLOW");
    m_weapon->SetWidthByContent(0, 0);
    m_weapon->SetHeightByContent(0, 0);
    m_weapon->SetCommandParameters(Weapon, 0, 0, 0);
    left->AddToFront(m_weapon, 3, 0);
    m_weapon->SetOpacity(0.3f);
    m_weapon->ClearFlags(FLAG_ENABLED);
    m_vehicle = new HudBarButton("IDB_SIDEBAR_VEHICLE", "IDB_SIDEBAR_ICON_GLOW");
    m_vehicle->SetWidthByContent(0, 0);
    m_vehicle->SetHeightByContent(0, 0);
    m_vehicle->SetCommandParameters(Vehicle, 0, 0, 0);
    left->AddToFront(m_vehicle, 4, 0);
    m_vehicle->SetOpacity(0.3f);
    m_vehicle->ClearFlags(FLAG_ENABLED);
    int edgeSpacing = m_weapon->m_height / 9;
    left->SetOutsetSpacing(0, 0, 0, edgeSpacing);
    AddToFront(left);

    Window* right = new Window(false);
    right->SetAlign(ALIGN_RIGHT | ALIGN_BOTTOM);
    right->SetWidthByContent(0, 0);
    right->SetHeightByContent(0, 0);
    m_home = new HudBarButton("IDB_SIDEBAR_SAFE_HOUSE", "IDB_SIDEBAR_ICON_GLOW");
    m_home->SetText(ResString("IDS_SIDEBAR_SAFE_HOUSE_LOWER"), ALIGN_HCENTER | ALIGN_VCENTER, NULL,
                    BUTTON_NAME_Y_OFFSET(), 0);
    m_home->SetWidthByContent(0, 0);
    m_home->SetHeightByContent(0, 0);
    m_home->SetCommandParameters(SafeHouse, 0, 0, 0);
    right->AddToFront(m_home, 0, 0);
    TextWindow* homeTitle = new TextWindow();
    homeTitle->SetText(ResString("IDS_SIDEBAR_SAFE_HOUSE_UPPER"));
    homeTitle->SetFont(CFontMgr::GetInstance()->GetFont(15));
    homeTitle->SetWidthByContent(0, 0);
    homeTitle->SetOutsetSpacing(-3, 0);
    homeTitle->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
    m_home->AddToFront(homeTitle, 0, 1);
    m_bank = new HudBarButton("IDB_SIDEBAR_BANK", "IDB_SIDEBAR_ICON_GLOW");
    m_bank->SetText(ResString("IDS_SIDEBAR_BANK"), ALIGN_HCENTER | ALIGN_VCENTER, NULL, BUTTON_NAME_Y_OFFSET(), 0);
    m_bank->SetWidthByContent(0, 0);
    m_bank->SetHeightByContent(0, 0);
    m_bank->SetCommandParameters(Bank, 0, 0, 0);
    right->AddToFront(m_bank, 1, 0);
    m_shop = new HudBarButton("IDB_SIDEBAR_SHOP", "IDB_SIDEBAR_STORE_GLOW");
    m_shop->SetText(ResString("IDS_SIDEBAR_SHOP"), ALIGN_HCENTER | ALIGN_VCENTER, NULL, BUTTON_NAME_Y_OFFSET(), 0);
    m_shop->SetWidthByContent(0, 0);
    m_shop->SetHeightByContent(0, 0);
    m_shop->SetAlign(ALIGN_RIGHT);
    m_shop->SetCommandParameters(Shop, 0, 0, 0);
    right->AddToFront(m_shop, 2, 0);
    right->SetOutsetSpacing(0, edgeSpacing, 0, 0);
    AddToFront(right);
}

void CBH_InformantsHud::ShowFriendsButton()
{
    // The original has no null or already-enabled check.
    m_friends->SetOpacity(1.0f);
    m_friends->SetFlags(FLAG_ENABLED);
    m_friends->SetText(ResString("IDS_SIDEBAR_FRIENDS"), ALIGN_HCENTER | ALIGN_VCENTER, NULL, BUTTON_NAME_Y_OFFSET(), 0);
}

void CBH_InformantsHud::ShowTargetButton()
{
    // The original has no null or already-enabled check.
    m_target->SetFlags(FLAG_ENABLED);
    m_target->SetOpacity(1.0f);
    m_target->SetText(ResString("IDS_SIDEBAR_TARGET"), ALIGN_HCENTER | ALIGN_VCENTER, NULL, BUTTON_NAME_Y_OFFSET(), 0);
}

void CBH_InformantsHud::ShowCoopButton()
{
    // The original has no null or already-enabled check.
    m_coop->SetFlags(FLAG_ENABLED);
    m_coop->SetOpacity(1.0f);
    m_coop->SetText(ResString("IDS_SIDEBAR_COOP"), ALIGN_HCENTER | ALIGN_VCENTER, NULL, BUTTON_NAME_Y_OFFSET(), 0);
}

void CBH_InformantsHud::ShowWeaponGatedButton()
{
    // The original has no null or already-enabled check.
    m_weapon->SetFlags(FLAG_ENABLED);
    m_weapon->SetOpacity(1.0f);
    m_weapon->SetText(ResString("IDS_SIDEBAR_WEAPON"), ALIGN_HCENTER | ALIGN_VCENTER, NULL, BUTTON_NAME_Y_OFFSET(), 0);
}

void CBH_InformantsHud::ShowVehicleButton()
{
    // The original has no null or already-enabled check.
    m_vehicle->SetFlags(FLAG_ENABLED);
    m_vehicle->SetOpacity(1.0f);
    m_vehicle->SetText(ResString("IDS_SIDEBAR_VEHICLE"), ALIGN_HCENTER | ALIGN_VCENTER, NULL, BUTTON_NAME_Y_OFFSET(), 0);
}

void CBH_InformantsHud::Update()
{
    // CK only refreshes the two notification badges here.
    if (m_coopNotification != NULL && m_friendsNotification != NULL) {
        App* app = static_cast<App*>(WindowApp::m_instance);
        CNewsFeedManager* news = app->m_newsFeedManager;
        if (news == NULL) {
            return;
        }
        m_friendsNotification->SetCount(news->GetUnreadNonEmptyMessageCount());
        m_coopNotification->SetCount(news->GetNewsCount(NEWS_CHALLENGE_COMPLETED));
    }
}
