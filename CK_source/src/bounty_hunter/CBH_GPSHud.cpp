#include "CBH_GPSHud.h"

#include "App.h"
#include "CAdColonyWindow.h"
#include "CBH_Player.h"
#include "CBH_ProgressBar.h"
#include "CFontMgr.h"
#include "CImageButton.h"
#include "CRegularShop.h"
#include "SScreenAdaptingConstant.h"

namespace {
const int CashCommand = (int)0xccda6872;
const int CreditsCommand = 0x346c7326;
const int EnergyCommand = (int)0x94396966;
const int WatchAd = 0x71f7f612;
const SScreenAdaptingConstant Margin(10, 20);
const SScreenAdaptingConstant BarGap(-5, -10);
const SScreenAdaptingConstant CashWidth(90, 180);
const SScreenAdaptingConstant CreditsWidth(75, 150);
const SScreenAdaptingConstant EnergyWidth(100, 200);
const SScreenAdaptingConstant XPWidth(150, 300);
}

CBH_GPSHud::CBH_GPSHud(bool animateXP)
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

void CBH_GPSHud::Init()
{
    int x = m_startX + Margin();
    int y = m_startY + Margin();
    CBH_Player* player = CBH_Player::GetInstance();
    if (!m_animateXP) {
        m_money = new CBH_ProgressBar(0, CashWidth(), -1);
        m_money->m_displayFlags = CBH_ProgressBar::DISPLAY_CURRENT | CBH_ProgressBar::HIDE_FILL;
        m_money->SetDescImage("IDB_HUD_ICON_MONEY");
        m_money->SetTextAlign(TEXT_ALIGN_RIGHT);
        m_money->m_command = CashCommand;
        m_money->SetCorner(x, y);
        AddToFront(m_money);
        x += m_money->m_width + BarGap();

        m_credits = new CBH_ProgressBar(0, CreditsWidth(), -1);
        m_credits->m_displayFlags = CBH_ProgressBar::DISPLAY_CURRENT | CBH_ProgressBar::HIDE_FILL;
        m_credits->SetDescImage("IDB_HUD_ICON_CREDITS");
        m_credits->SetTextAlign(TEXT_ALIGN_RIGHT);
        m_credits->m_command = CreditsCommand;
        m_credits->SetCorner(x, y);
        AddToFront(m_credits);
        x += m_credits->m_width + BarGap();

        if (player->IsEnegyEnabled()) {
            m_energy = new CEnergyProgressBar(4, EnergyWidth());
            m_energy->SetDescImage("IDB_HUD_ICON_ENERGY");
            m_energy->m_command = EnergyCommand;
            m_energy->SetCorner(x, y);
            AddToFront(m_energy);
            x += m_energy->m_width + BarGap();
        }
    }

    m_xp = new CAnimatedProgressBar(3, XPWidth(), 4);
    m_xp->m_displayFlags = CBH_ProgressBar::DISPLAY_CURRENT | CBH_ProgressBar::DISPLAY_MAX | CBH_ProgressBar::MANUAL_PERCENT;
    m_xp->SetMaxValue(100);
    m_xp->SetDescImage("IDB_HUD_ICON_EXP");
    m_xp->SetCorner(x, y);
    AddToFront(m_xp);
    UpdateXPBar(-1);

    m_levelBadge = new CImageButton();
    m_levelBadge->AddImage("IDB_MAP_HUD_LEVEL_BG", 0, 0);
    m_levelBadge->SetText(XString(m_level), ALIGN_HCENTER | ALIGN_VCENTER, CFontMgr::GetInstance()->GetFont(9),
                          -3 * App::Scale(), 0);
    m_levelBadge->SetCorner(x + m_xp->m_width - m_levelBadge->m_width / 2,
                           y + (m_xp->m_height - m_levelBadge->m_height) / 2);
    AddToFront(m_levelBadge);
}

void CBH_GPSHud::UpdateXPBar(int delta)
{
    if (m_xp == NULL) {
        return;
    }
    CBH_Player* player = CBH_Player::GetInstance();
    int xp = player->m_experience;
    int current = player->GetXpForCurrentLvl();
    int next = player->GetXpForNextLvl();
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
        if (m_animateXP) {
            m_xp->SetSpecialPercents(percent);
        } else {
            m_xp->CBH_ProgressBar::SetSpecialPercents(percent);
        }
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
        m_energy->SetValues(player->m_energy, player->GetMaxEnergy(), player->GetEnergyCooldownTimeLeft());
    }
    if (m_level != player->GetLevel() && (!m_animateXP || m_xp->m_deltaPercent == 0.0f)) {
        m_level = player->GetLevel();
        SetLevelOnBar(m_level);
        m_updateLevel = false;
    }
    if (!m_animateXP) {
        UpdateXPBar(-1);
    } else if (m_experience != player->m_experience) {
        UpdateXPBar(player->m_experience - m_experience);
        m_experience = player->m_experience;
    }
}

void CBH_GPSHud::OnCommand(Event& event)
{
    if (event.m_id == CashCommand || event.m_id == CreditsCommand) {
        CRegularShop::Show(CShopWindow::TAB_BANK);
    } else if (event.m_id == EnergyCommand) {
        CRegularShop::Show(CShopWindow::TAB_OTHER);
    } else if (event.m_id == WatchAd) {
        AddModal(new CAdColonyWindow(false));
    } else {
        return;
    }
    event.Clear();
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
    m_money = NULL;
    m_credits = NULL;
}

typedef char AssertGPSHudSize[sizeof(CBH_GPSHud) == 0xd0 ? 1 : -1];
