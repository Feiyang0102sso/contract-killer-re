#include "CBH_GPSHud.h"

#include "App.h"
#include "BaseDialog.h"
#include "CAdColonyWindow.h"
#include "CBH_Player.h"
#include "CBH_ProgressBar.h"
#include "CFont.h"
#include "CFontMgr.h"
#include "CImageButton.h"
#include "CPHInterface.h"
#include "CRegularShop.h"
#include "SScreenAdaptingConstant.h"

namespace {
const int CashCommand = (int)0xccda6872;
const int CreditsCommand = 0x346c7326;
const int EnergyCommand = (int)0x94396966;
const int WatchAd = 0x71f7f612;
const int MoreGames = 0x71f7f615;
const SScreenAdaptingConstant LeftMargin(10, 20);
const SScreenAdaptingConstant TopMargin(10, 20);
const SScreenAdaptingConstant BarGap(-5, -10);
const SScreenAdaptingConstant CashWidth(90, 180);
const SScreenAdaptingConstant CreditsWidth(75, 150);
const SScreenAdaptingConstant EnergyWidth(100, 200);
const SScreenAdaptingConstant XPWidth(150, 300);
const SScreenAdaptingConstant CashButtonX(43, 87, 89);
const SScreenAdaptingConstant CreditsButtonX(28, 57, 59);
const SScreenAdaptingConstant AddButtonY(3, 6, 4);
const SScreenAdaptingConstant CashBonusX(5, 16);
const SScreenAdaptingConstant BonusY(0, 4);

}

__attribute__((noinline)) CBH_GPSHud::CBH_GPSHud(bool animateXP)
    : Window(false), m_energy(NULL), m_xp(NULL), m_credits(NULL), m_money(NULL), m_levelBadge(NULL),
      m_startX(0), m_startY(0), m_fieldbc(0), m_animateXP(animateXP), m_updateLevel(false), m_fieldc4(0),
      m_level(CBH_Player::GetInstance()->GetLevel()), m_experience(0)
{
}

CBH_GPSHud::~CBH_GPSHud()
{
}

void CBH_GPSHud::SetOrigin(int x, int y)
{
    m_startX = x;
    m_startY = y;
}

// The original ignores the origin set by SetOrigin: the bars start at LeftMargin / TopMargin and
// the left edge is accumulated as a float.
void CBH_GPSHud::Init()
{
    float x = static_cast<float>(LeftMargin());
    if (!m_animateXP) {
        int left = static_cast<int>(x);
        m_money = new CBH_ProgressBar(0, CashWidth(), -1);
        m_money->SetCorner(left, TopMargin());
        m_money->m_displayFlags = CBH_ProgressBar::DISPLAY_CURRENT | CBH_ProgressBar::HIDE_FILL;
        m_money->SetTextAlign(TEXT_ALIGN_RIGHT);
        m_money->SetCurrValue(CBH_Player::GetInstance()->m_cash);
        m_money->SetDescImage("IDB_HUD_ICON_MONEY");
        m_money->SetCommandParameters(CashCommand, 0, 0, 0);
        AddToFront(m_money);
        COverrideManager* overrides = static_cast<App*>(WindowApp::m_instance)->m_overrideManager;
        int cashMultiplier = CBH_Player::GetInstance()->GetOverride(*overrides, XString(L"missions_mult"),
                                                                    XString(L"soft"));
        if (cashMultiplier > 0) {
            XString text = XString::Format(L"x%d%%", cashMultiplier);
            TextWindow* bonus = new TextWindow(text, CFontMgr::GetInstance()->GetFont(17));
            bonus->SetAlign(ALIGN_LEFT | ALIGN_TOP);
            bonus->SetOutsetSpacing(m_money->m_height / 3 + m_money->m_height + BonusY(),
                                    static_cast<int>(x + CashBonusX()));
            AddToFront(bonus, 0, 0);
        }
        CImageButton* cashButton = new CImageButton();
        cashButton->AddImage("IDB_MAP_HUD_ADD_BUTTONG", 0, 0);
        cashButton->SetWidthByContent(0, 0);
        cashButton->SetLayoutType(LAYOUT_NONE);
        cashButton->SetHeightByContent(0, 0);
        cashButton->SetCommandParameters(CashCommand, 0, 0, 0);
        cashButton->SetCorner(static_cast<int>(x + CashButtonX()), TopMargin() + AddButtonY());
        AddToFront(cashButton);

        x += m_money->m_width + BarGap();
        m_credits = new CBH_ProgressBar(0, CreditsWidth(), -1);
        m_credits->SetCorner(static_cast<int>(x), TopMargin());
        m_credits->m_displayFlags = CBH_ProgressBar::DISPLAY_CURRENT | CBH_ProgressBar::HIDE_FILL;
        m_credits->SetTextAlign(TEXT_ALIGN_RIGHT);
        m_credits->SetCurrValue(CBH_Player::GetInstance()->m_credits);
        m_credits->SetDescImage("IDB_HUD_ICON_CREDITS");
        m_credits->SetCommandParameters(CreditsCommand, 0, 0, 0);
        AddToFront(m_credits);
        CImageButton* creditsButton = new CImageButton();
        creditsButton->AddImage("IDB_MAP_HUD_ADD_BUTTONG", 0, 0);
        creditsButton->SetWidthByContent(0, 0);
        creditsButton->SetLayoutType(LAYOUT_NONE);
        creditsButton->SetHeightByContent(0, 0);
        creditsButton->SetCommandParameters(CreditsCommand, 0, 0, 0);
        creditsButton->SetCorner(static_cast<int>(x + CreditsButtonX()), TopMargin() + AddButtonY());
        AddToFront(creditsButton);

        x += m_credits->m_width + BarGap();
        if (CBH_Player::GetInstance()->IsEnegyEnabled()) {
            int energyLeft = static_cast<int>(x);
            m_energy = new CEnergyProgressBar(0, EnergyWidth());
            m_energy->SetCorner(energyLeft, TopMargin());
            m_energy->SetDescImage("IDB_HUD_ICON_ENERGY");
            m_energy->SetMaxValue(CBH_Player::GetInstance()->GetMaxEnergy());
            m_energy->SetCommandParameters(EnergyCommand, 0, 0, 0);
            AddToFront(m_energy);
            x += m_energy->m_width + BarGap();
        }
    }

    int xpLeft = static_cast<int>(x);
    m_xp = new CAnimatedProgressBar(3, XPWidth(), 4);
    m_xp->m_displayFlags = CBH_ProgressBar::DISPLAY_CURRENT | CBH_ProgressBar::DISPLAY_MAX | CBH_ProgressBar::MANUAL_PERCENT;
    m_xp->SetCurrValue(0);
    m_xp->SetMaxValue(100);
    m_xp->SetCorner(xpLeft, TopMargin());
    if (m_animateXP) {
        m_xp->SetCorner(xpLeft, TopMargin() - SScreenAdaptingConstant(3, 6)());
    }
    m_xp->SetDescImage("IDB_HUD_ICON_EXP");
    UpdateXPBar(-1);
    AddToFront(m_xp);

    if (!m_animateXP) {
        COverrideManager* overrides = static_cast<App*>(WindowApp::m_instance)->m_overrideManager;
        int xpMultiplier = CBH_Player::GetInstance()->GetOverride(*overrides, XString(L"missions_mult"), XString(L"xp"));
        if (xpMultiplier > 0) {
            XString text = XString::Format(L"x%d%%", xpMultiplier);
            TextWindow* bonus = new TextWindow(text, CFontMgr::GetInstance()->GetFont(17));
            bonus->SetOutsetSpacing(m_xp->m_height + m_money->m_height / 3 + BonusY(),
                                    static_cast<int>(x + m_xp->m_width / 2
                                                     - CFontMgr::GetInstance()->GetFont(22)->MeasureTextWidth(
                                                           text.c_str(), -1, -1, NULL) / 2));
            bonus->SetAlign(ALIGN_LEFT | ALIGN_TOP);
            AddToFront(bonus);
        }
    }

    float badgeRight = x + m_xp->m_width;
    m_levelBadge = new CImageButton();
    m_levelBadge->AddImage("IDB_MAP_HUD_LEVEL_BG", 0, 0);
    int badgeY = m_xp->m_y + m_xp->m_height / 2 - m_levelBadge->m_height / 2;
    m_levelBadge->SetLayoutType(LAYOUT_NONE);
    m_levelBadge->SetCorner(static_cast<int>(badgeRight - m_levelBadge->m_width / 2), badgeY);
    m_levelBadge->SetText(XString(CBH_Player::GetInstance()->GetLevel()), ALIGN_HCENTER | ALIGN_VCENTER,
                          CFontMgr::GetInstance()->GetFont(9), App::Scale() * -3, 0);
    AddToFront(m_levelBadge);
}

void CBH_GPSHud::UpdateXPBar(int delta)
{
    if (m_xp == NULL) {
        return;
    }
    CBH_Player* player = CBH_Player::GetInstance();
    int xp = player->m_experience;
    int next = player->GetXpForNextLvl();
    int current = player->GetXpForCurrentLvl();
    if (next <= 0 || next <= current) {
        m_xp->m_displayFlags = CBH_ProgressBar::DISPLAY_CURRENT;
        m_xp->SetCurrValue(xp);
        m_xp->SetSpecialPercents(100);
    } else {
        m_xp->SetCurrValue(xp);
        m_xp->SetMaxValue(next);
        int percent = 100;
        if (xp - delta >= current) {
            percent = (xp - current) * 100 / (next - current);
        }
        m_xp->SetSpecialPercents(percent);
    }
    if (m_animateXP) {
        m_updateLevel = true;
    }
}

void CBH_GPSHud::SetLevelOnBar(int level)
{
    if (m_levelBadge != NULL) {
        m_levelBadge->UpdateText(XString(level));
    }
}

void CBH_GPSHud::Update()
{
    CBH_Player* player = CBH_Player::GetInstance();
    if (m_money != NULL) {
        m_money->SetCurrValue(player->m_cash);
    }
    if (m_credits != NULL) {
        m_credits->SetCurrValue(player->m_credits);
    }
    if (m_energy != NULL) {
        m_energy->SetValues(player->m_energy, player->GetMaxEnergy(), player->m_energyRemainingSeconds);
    }
    if (m_levelBadge != NULL && m_level != player->GetLevel()) {
        m_level = player->GetLevel();
        SetLevelOnBar(m_level);
    }
    if (!m_animateXP) {
        UpdateXPBar(-1);
    } else if (m_xp != NULL) {
        if (m_xp->m_deltaPercent == 0.0f && m_updateLevel) {
            m_updateLevel = false;
            SetLevelOnBar(player->GetLevel());
            if (m_xp->m_percent >= 100.0f && !player->IsPlayerLevelMax() &&
                player->m_experience < player->GetXpForNextLvl()) {
                m_xp->SetSpecialPercents(0);
                UpdateXPBar(-1);
            }
        }
        if (m_experience != player->m_experience) {
            UpdateXPBar(player->m_experience - m_experience);
            m_experience = player->m_experience;
        }
    }
}

void CBH_GPSHud::OnCommand(Event& event)
{
    // The original does not clear the event.
    switch (event.m_id) {
    case EnergyCommand: {
        WindowApp::HandleTunnelCommand(0x718a0676, 0, 0, 0);
        CRegularShop* shop = CRegularShop::GetInstance(true);
        shop->SetTab(2);
        static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddModal(shop);
        break;
    }
    case CashCommand:
    case CreditsCommand: {
        WindowApp::HandleTunnelCommand(0x718a0676, 0, 0, 0);
        CRegularShop* shop = CRegularShop::GetInstance(true);
        shop->SetTab(0);
        static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddModal(shop);
        break;
    }
    case WatchAd:
        AddModal(new CAdColonyWindow(false));
        break;
    case MoreGames:
        CPHInterface::startPublisherContentRequest("more_games", false, true);
        break;
    }
}

void CBH_GPSHud::OnGetFocus()
{
    if (!m_animateXP) {
        WindowApp::HandleTunnelCommand(0x40067282, 0, 0, 0);
    }
}

void CBH_GPSHud::OnLostFocus()
{
}

void CBH_GPSHud::OnStartClosing()
{
    m_credits = NULL;
    m_money = NULL;
}

typedef char AssertGPSHudSize[sizeof(CBH_GPSHud) == 0xd0 ? 1 : -1];
