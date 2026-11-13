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
    : Window(false)
{
    m_energy = NULL;
    m_xp = NULL;
    m_credits = NULL;
    m_money = NULL;
    m_levelBadge = NULL;
    m_fieldbc = 0;
    m_startX = 0;
    m_startY = 0;
    m_animateXP = animateXP;
    m_updateLevel = false;
    m_fieldc4 = 0;
    m_level = CBH_Player::GetInstance()->GetLevel();
    m_startY = 0;
    m_startX = 0;
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
        CBH_ProgressBar* money = new CBH_ProgressBar(0, CashWidth(), -1);
        m_money = money;
        money->SetCorner(left, TopMargin());
        m_money->m_displayFlags = CBH_ProgressBar::DISPLAY_CURRENT | CBH_ProgressBar::HIDE_FILL;
        m_money->SetTextAlign(TEXT_ALIGN_RIGHT);
        money = m_money;
        money->SetCurrValue(CBH_Player::GetInstance()->m_cash);
        m_money->SetDescImage("IDB_HUD_ICON_MONEY");
        m_money->SetCommandParameters(CashCommand, 0, 0, 0);
        AddToFront(m_money);
        COverrideManager* overrides = static_cast<App*>(WindowApp::m_instance)->m_overrideManager;
        int cashMultiplier;
        {
            CBH_Player* player = CBH_Player::GetInstance();
            XString category(L"missions_mult");
            XString key(L"soft");
            cashMultiplier = player->GetOverride(*overrides, category, key);
        }
        if (cashMultiplier > 0) {
            XString text = XString::Format(L"x%d%", cashMultiplier);
            TextWindow* bonus = new TextWindow(text, CFontMgr::GetInstance()->GetFont(17));
            bonus->SetAlign(ALIGN_LEFT | ALIGN_TOP);
            int height = m_money->m_height;
            int bonusY = BonusY();
            int bonusX = static_cast<int>(x + CashBonusX());
            bonus->SetOutsetSpacing(height / 3 + height + bonusY, bonusX);
            AddToFront(bonus, 0, 0);
        }
        CImageButton* cashButton = new CImageButton();
        cashButton->AddImage("IDB_MAP_HUD_ADD_BUTTONG", 0, 0);
        cashButton->SetWidthByContent(0, 0);
        cashButton->SetLayoutType(LAYOUT_NONE);
        cashButton->SetHeightByContent(0, 0);
        cashButton->SetCommandParameters(CashCommand, 0, 0, 0);
        int cashButtonOffset = CashButtonX();
        int cashButtonY = TopMargin();
        cashButtonY += AddButtonY();
        cashButton->SetCorner(static_cast<int>(x + cashButtonOffset), cashButtonY);
        AddToFront(cashButton);

        int moneyWidth = m_money->m_width;
        int moneySpan = moneyWidth + BarGap();
        CBH_ProgressBar* credits = new CBH_ProgressBar(0, CreditsWidth(), -1);
        m_credits = credits;
        x += moneySpan;
        credits->SetCorner(static_cast<int>(x), TopMargin());
        m_credits->m_displayFlags = CBH_ProgressBar::DISPLAY_CURRENT | CBH_ProgressBar::HIDE_FILL;
        m_credits->SetTextAlign(TEXT_ALIGN_RIGHT);
        credits = m_credits;
        credits->SetCurrValue(CBH_Player::GetInstance()->m_credits);
        m_credits->SetDescImage("IDB_HUD_ICON_CREDITS");
        m_credits->SetCommandParameters(CreditsCommand, 0, 0, 0);
        AddToFront(m_credits);
        CImageButton* creditsButton = new CImageButton();
        creditsButton->AddImage("IDB_MAP_HUD_ADD_BUTTONG", 0, 0);
        creditsButton->SetWidthByContent(0, 0);
        creditsButton->SetLayoutType(LAYOUT_NONE);
        creditsButton->SetHeightByContent(0, 0);
        creditsButton->SetCommandParameters(CreditsCommand, 0, 0, 0);
        int creditsButtonX = static_cast<int>(x + CreditsButtonX());
        int creditsButtonY = TopMargin();
        creditsButtonY += AddButtonY();
        creditsButton->SetCorner(creditsButtonX, creditsButtonY);
        AddToFront(creditsButton);

        int creditsWidth = m_credits->m_width;
        x += creditsWidth + BarGap();
        if (CBH_Player::GetInstance()->IsEnegyEnabled()) {
            int energyLeft = static_cast<int>(x);
            CEnergyProgressBar* energy = new CEnergyProgressBar(0, EnergyWidth());
            m_energy = energy;
            energy->SetCorner(energyLeft, TopMargin());
            m_energy->SetDescImage("IDB_HUD_ICON_ENERGY");
            energy = m_energy;
            energy->SetMaxValue(CBH_Player::GetInstance()->GetMaxEnergy());
            m_energy->SetCommandParameters(EnergyCommand, 0, 0, 0);
            AddToFront(m_energy);
            int energyWidth = m_energy->m_width;
            x += energyWidth + BarGap();
        }
    }

    int xpLeft = static_cast<int>(x);
    CAnimatedProgressBar* xp = new CAnimatedProgressBar(3, XPWidth(), 4);
    m_xp = xp;
    xp->m_displayFlags = CBH_ProgressBar::DISPLAY_CURRENT | CBH_ProgressBar::DISPLAY_MAX | CBH_ProgressBar::MANUAL_PERCENT;
    m_xp->SetCurrValue(0);
    m_xp->SetMaxValue(100);
    m_xp->SetCorner(xpLeft, TopMargin());
    if (m_animateXP) {
        CAnimatedProgressBar* bar = m_xp;
        int top = TopMargin();
        bar->SetCorner(xpLeft, top - SScreenAdaptingConstant(3, 6)());
    }
    m_xp->SetDescImage("IDB_HUD_ICON_EXP");
    UpdateXPBar(-1);
    AddToFront(m_xp);

    if (!m_animateXP) {
        COverrideManager* overrides = static_cast<App*>(WindowApp::m_instance)->m_overrideManager;
        int xpMultiplier;
        {
            CBH_Player* player = CBH_Player::GetInstance();
            XString category(L"missions_mult");
            XString key(L"xp");
            xpMultiplier = player->GetOverride(*overrides, category, key);
        }
        if (xpMultiplier > 0) {
            XString text = XString::Format(L"x%d%", xpMultiplier);
            TextWindow* bonus = new TextWindow(text, CFontMgr::GetInstance()->GetFont(17));
            int xpHeight = m_xp->m_height;
            int moneyHeight = m_money->m_height;
            int bonusY = BonusY();
            int xpWidth = m_xp->m_width;
            CFont* font = CFontMgr::GetInstance()->GetFont(22);
            int textWidth = font->MeasureTextWidth(text.c_str(), -1, -1, NULL);
            int bonusX = static_cast<int>(x + xpWidth / 2 - textWidth / 2);
            bonus->SetOutsetSpacing(bonusY + moneyHeight / 3 + xpHeight, bonusX);
            bonus->SetAlign(ALIGN_LEFT | ALIGN_TOP);
            AddToFront(bonus);
        }
    }

    int xpWidth = m_xp->m_width;
    m_levelBadge = new CImageButton();
    m_levelBadge->AddImage("IDB_MAP_HUD_LEVEL_BG", 0, 0);
    int badgeY = m_xp->m_y + m_xp->m_height / 2 - m_levelBadge->m_height / 2;
    m_levelBadge->SetLayoutType(LAYOUT_NONE);
    int badgeX = static_cast<int>(x + xpWidth - m_levelBadge->m_width / 2);
    m_levelBadge->SetCorner(badgeX, badgeY);
    {
        CImageButton* badge = m_levelBadge;
        badge->SetText(XString(CBH_Player::GetInstance()->GetLevel()), ALIGN_HCENTER | ALIGN_VCENTER,
                       CFontMgr::GetInstance()->GetFont(9), App::Scale() * -3, 0);
    }
    AddToFront(m_levelBadge);
}

void CBH_GPSHud::UpdateXPBar(int delta)
{
    if (m_xp == NULL) {
        return;
    }
    int xp = CBH_Player::GetInstance()->m_experience;
    int next = CBH_Player::GetInstance()->GetXpForNextLvl();
    int current = CBH_Player::GetInstance()->GetXpForCurrentLvl();
    if (next > 0 && next > current) {
        m_xp->SetCurrValue(xp);
        m_xp->SetMaxValue(next);
        CAnimatedProgressBar* bar = m_xp;
        if (xp - delta < current) {
            bar->SetSpecialPercents(100);
        } else {
            bar->SetSpecialPercents((xp - current) * 100 / (next - current));
        }
    } else {
        m_xp->m_displayFlags = CBH_ProgressBar::DISPLAY_CURRENT;
        m_xp->SetCurrValue(xp);
        m_xp->SetSpecialPercents(100);
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
    if (m_money != NULL) {
        CBH_ProgressBar* money = m_money;
        money->SetCurrValue(CBH_Player::GetInstance()->m_cash);
    }
    if (m_credits != NULL) {
        CBH_ProgressBar* credits = m_credits;
        credits->SetCurrValue(CBH_Player::GetInstance()->m_credits);
    }
    if (m_energy != NULL) {
        CEnergyProgressBar* energy = m_energy;
        int current = CBH_Player::GetInstance()->m_energy;
        int maximum = CBH_Player::GetInstance()->GetMaxEnergy();
        int seconds = CBH_Player::GetInstance()->m_energyRemainingSeconds;
        energy->SetValues(current, maximum, seconds);
    }
    if (m_levelBadge != NULL && m_level != CBH_Player::GetInstance()->GetLevel()) {
        m_level = CBH_Player::GetInstance()->GetLevel();
        CImageButton* badge = m_levelBadge;
        badge->UpdateText(XString(m_level));
    }
    if (!m_animateXP) {
        UpdateXPBar(-1);
        return;
    }

    if (m_xp->m_deltaPercent == 0.0f && m_updateLevel) {
        m_updateLevel = false;
        SetLevelOnBar(CBH_Player::GetInstance()->GetLevel());
        if (static_cast<int>(m_xp->m_percent) >= 100 && !CBH_Player::GetInstance()->IsPlayerLevelMax()) {
            int xp = CBH_Player::GetInstance()->m_experience;
            if (xp < CBH_Player::GetInstance()->GetXpForNextLvl()) {
                m_xp->SetSpecialPercents(0);
                UpdateXPBar(-1);
            }
        }
    }
    if (m_experience != CBH_Player::GetInstance()->m_experience) {
        UpdateXPBar(CBH_Player::GetInstance()->m_experience - m_experience);
        m_experience = CBH_Player::GetInstance()->m_experience;
    }
}

void CBH_GPSHud::OnCommand(Event& event)
{
    // The original does not clear the event.
    CRegularShop* shop;
    switch (event.m_id) {
    case EnergyCommand:
        WindowApp::HandleTunnelCommand(0x718a0676, 0, 0, 0);
        shop = CRegularShop::GetInstance(true);
        shop->SetTab(2);
        break;
    case CashCommand:
    case CreditsCommand:
        WindowApp::HandleTunnelCommand(0x718a0676, 0, 0, 0);
        shop = CRegularShop::GetInstance(true);
        shop->SetTab(0);
        break;
    case WatchAd:
        AddModal(new CAdColonyWindow(false));
        return;
    case MoreGames:
        CPHInterface::startPublisherContentRequest("more_games", false, true);
        return;
    default:
        return;
    }
    static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddModal(shop);
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

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AssertGPSHudSize[sizeof(CBH_GPSHud) == 0xd0 ? 1 : -1];
#endif
