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

namespace {
const SScreenAdaptingConstant LabelY(36, 75);

void EnableShortcut(HudBarButton* button, const char* text)
{
    if (button != NULL && (button->m_flags & Window::FLAG_ENABLED) == 0) {
        button->SetFlags(Window::FLAG_ENABLED);
        button->SetOpacity(1.0f);
        button->SetText(Window::ResString(text), Window::ALIGN_HCENTER | Window::ALIGN_TOP,
                        CFontMgr::GetInstance()->GetFont(15), LabelY(), 0);
    }
}

HudBarButton* CreateShortcut(Window* row, int column, const char* image, const char* glow, int command)
{
    HudBarButton* button = new HudBarButton(image, glow);
    button->m_command = command;
    button->ClearFlags(Window::FLAG_ENABLED);
    button->SetOpacity(0.3f);
    row->AddToFront(button, column, 0);
    return button;
}
}

HudBarButton::HudBarButton(const char* image, const char* glow)
    : CImageButton(), m_glow(NULL)
{
    if (glow != NULL) {
        m_glow = new ImageWindow();
        m_glow->SetImageById(glow);
        m_glow->ClearFlags(FLAG_VISIBLE);
        AddToFront(m_glow);
    }
    AddImage(image, 0, 0);
}

void HudBarButton::SetText(XString text, int align, CFont* font, int topSpacing, int percentHeight)
{
    CImageButton::SetText(text, align, font, topSpacing, percentHeight);
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
    const char* glow = "IDB_SIDEBAR_ICON_GLOW";
    Window* left = new Window(false);
    left->SetAlign(ALIGN_LEFT | ALIGN_BOTTOM);
    left->SetWidthByContent(0, 0);
    left->SetHeightByContent(0, 0);
    m_friends = CreateShortcut(left, 0, "IDB_SIDEBAR_FRIENDS", glow, Friends);
    m_friendsNotification = new CCountNotificationWindow();
    m_friendsNotification->ClearFlags(FLAG_ENABLED | FLAG_FOCUSABLE);
    m_friends->AddToFront(m_friendsNotification);
    m_target = CreateShortcut(left, 1, "IDB_SIDEBAR_TARGET", glow, Target);
    m_coop = CreateShortcut(left, 2, "IDB_SIDEBAR_COOP", glow, Coop);
    m_coopNotification = new CCountNotificationWindow();
    m_coopNotification->ClearFlags(FLAG_ENABLED | FLAG_FOCUSABLE);
    m_coop->AddToFront(m_coopNotification);
    m_weapon = CreateShortcut(left, 3, "IDB_SIDEBAR_WEAPON", glow, Weapon);
    m_vehicle = CreateShortcut(left, 4, "IDB_SIDEBAR_VEHICLE", glow, Vehicle);
    AddToFront(left);

    Window* right = new Window(false);
    right->SetAlign(ALIGN_RIGHT | ALIGN_BOTTOM);
    right->SetWidthByContent(0, 0);
    right->SetHeightByContent(0, 0);
    m_home = CreateShortcut(right, 0, "IDB_SIDEBAR_SAFE_HOUSE", glow, SafeHouse);
    m_bank = CreateShortcut(right, 1, "IDB_SIDEBAR_BANK", glow, Bank);
    m_shop = CreateShortcut(right, 2, "IDB_SIDEBAR_SHOP", "IDB_SIDEBAR_STORE_GLOW", Shop);
    EnableShortcut(m_bank, "IDS_SIDEBAR_BANK");
    EnableShortcut(m_shop, "IDS_SIDEBAR_SHOP");
    AddToFront(right);
}

void CBH_InformantsHud::ShowFriendsButton()
{
    EnableShortcut(m_friends, "IDS_SIDEBAR_FRIENDS");
}

void CBH_InformantsHud::ShowTargetButton()
{
    EnableShortcut(m_target, "IDS_SIDEBAR_TARGET");
}

void CBH_InformantsHud::ShowCoopButton()
{
    EnableShortcut(m_coop, "IDS_SIDEBAR_COOP");
}

void CBH_InformantsHud::ShowWeaponGatedButton()
{
    EnableShortcut(m_weapon, "IDS_SIDEBAR_WEAPON");
}

void CBH_InformantsHud::ShowVehicleButton()
{
    EnableShortcut(m_vehicle, "IDS_SIDEBAR_VEHICLE");
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
