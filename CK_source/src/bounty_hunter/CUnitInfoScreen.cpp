#include "CUnitInfoScreen.h"

#include "App.h"
#include "CBH_ProgressBar.h"
#include "CDH_BasicGameData.h"
#include "CDH_Weapon.h"
#include "CFont.h"
#include "CFontMgr.h"
#include "CSwerveGame.h"
#include "CStdUtil_Cocoa.h"
#include "CUnit.h"
#include "CUtility.h"
#include "DGHelper.h"

typedef char CKUnitInfoWindowSize[sizeof(CUnitInfoScreen) == 0x160 ? 1 : -1];

CUnitInfoScreen::CUnitInfoScreen(CSwerveGame* game, CUnit* unit, float distance,
                                 float identificationRange, bool tagged, int startingDamage, bool revealTarget)
    : Window(false), m_hpBar(NULL), m_revealTarget(revealTarget), m_distance(distance),
      m_identificationRange(identificationRange), m_maxHp(unit->m_maxHp), m_hp(unit->m_hp + startingDamage),
      m_game(game), m_openedAt(0), m_state(0), m_tagged(tagged), m_civilian(unit->m_type == UnitCivilian),
      m_outOfRange(false), m_duration(0), m_guid(unit->m_guid)
{
    ClearAndSetFlags(FLAG_FOCUSABLE | FLAG_ENABLED, 0);
    m_title = CUtility::GetString("IDS_GAME_PLAY_TAGGED").GetWCharPtr();
    if (!m_tagged) {
        const char* status = "IDS_GAME_PLAY_KILLED";
        if (unit->m_tranquillized) {
            status = "IDS_GAME_PLAY_TRANQUILIZED";
        }
        m_title = CUtility::GetString(status).GetWCharPtr();
    }
    if (m_tagged && !m_revealTarget && m_distance >= m_identificationRange) {
        m_type = CUtility::GetString("IDS_GAME_PLAY_UNKNOWN").GetWCharPtr();
    } else if (m_civilian) {
        m_type = CUtility::GetString("IDS_GAME_PLAY_CIVILIAN").GetWCharPtr();
    } else {
        m_name = unit->m_name;
        const char* role = "IDS_GAME_PLAY_MINION";
        if (unit->m_type == UnitBoss) {
            role = "IDS_GAME_PLAY_BOSS";
        }
        m_type = CUtility::GetString(role).GetWCharPtr();
    }
}

void CUnitInfoScreen::Init()
{
    CreateContent();
    FlushBeforeCloseTimer();
    m_state = 2;
}

void CUnitInfoScreen::CreateContent()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    int panelWidth = m_game->m_width * 40 / 100;
    int inset = 6 * App::Scale();
    CFont* font = CFontMgr::GetInstance()->GetFont(16);
    int lineHeight = font->GetFontHeight() + App::Scale();
    int panelHeight = 4 * lineHeight + 2 * inset;
    SetRect((m_game->m_width - panelWidth) / 2, 2 * App::Scale(), panelWidth, panelHeight, true);

    XString distance(CUtility::GetString("IDS_GAME_PLAY_DISTANCE").GetWCharPtr());
    XString measure(CUtility::GetString("IDS_GAME_PLAY_MEASURE").GetWCharPtr());
    m_distanceText = XString::Format(L"%ls %d %ls", distance.c_str(),
                                    static_cast<int>(m_distance * 3.28f), measure.c_str());
    CDH_Weapon* weapon = app->m_helper->GetGameData()->GetActiveWeapon();
    m_outOfRange = weapon != NULL && m_distance > weapon->effectiveRange();
    if (m_tagged && m_outOfRange) {
        m_distanceText = CUtility::GetString("IDS_GAME_PLAY_DISTANCE_OUT_OF_RANGE").GetWCharPtr();
    }
    if (m_tagged && !m_civilian && (m_revealTarget || m_distance < m_identificationRange)) {
        m_hpBar = new CAnimatedProgressBar(panelWidth - 2 * inset, "IDB_HUD_PROGRESSBAR_NPC_PINK",
                                           "IDB_HUD_PROGRESSBAR_NPC_LEFT", "IDB_HUD_PROGRESSBAR_NPC_RIGHT",
                                           "IDB_HUD_PROGRESSBAR_NPC", "IDB_HUD_PROGRESSBAR_NPC_DARK_RED");
        m_hpBar->m_drawText = false;
        m_hpBar->SetMaxValue(m_maxHp);
        m_hpBar->SetCurrValue(m_hp);
        m_hpBar->ClearAndSetFlags(FLAG_FOCUSABLE | FLAG_ENABLED, 0);
        m_hpBar->SetCorner(inset, inset + 3 * lineHeight);
        AddToFront(m_hpBar);
    }
}

void CUnitInfoScreen::Paint(ICGraphics2d& graphics)
{
    CFont* heading = CFontMgr::GetInstance()->GetFont(16);
    CFont* font = CFontMgr::GetInstance()->GetFont(17);
    int inset = 6 * App::Scale();
    int lineHeight = heading->GetFontHeight() + App::Scale();
    int center = m_width / 2;
    FillRect(graphics, 0, 0, m_width, m_height, 0xb0101210);
    DrawFontText(graphics, heading, m_title.c_str(), center, inset, ALIGN_HCENTER | ALIGN_TOP);
    XString name = m_type;
    if (m_name.Length() != 0) {
        name = m_name;
    }
    DrawFontText(graphics, font, name.c_str(), center, inset + lineHeight, ALIGN_HCENTER | ALIGN_TOP);
    DrawFontText(graphics, font, m_distanceText.c_str(), center, inset + 2 * lineHeight, ALIGN_HCENTER | ALIGN_TOP);
}

void CUnitInfoScreen::Update()
{
    Window::Update();
    if (m_state != 2) {
        return;
    }
    unsigned int duration = m_duration;
    if (duration == 0) {
        duration = 3000;
        if (m_tagged) {
            duration = 1500;
        }
    }
    if (CStdUtil_Cocoa::GetUpTimeMS() - m_openedAt > duration) {
        m_state = 3;
        Close();
        return;
    }
    CUnit* unit = m_game->GetUnitByGUID(m_guid);
    if (m_tagged && unit != NULL && m_hpBar != NULL) {
        if (m_hp != unit->m_hp) {
            m_hp = unit->m_hp;
            m_hpBar->SetCurrValueWithAnimation(m_hp);
        }
        if (m_hpBar->m_deltaPercent == 0.0f && (unit->m_dead || unit->m_tranquillized)) {
            m_game->ShowUnitKillScreen(unit);
        }
    }
}

void CUnitInfoScreen::FlushBeforeCloseTimer()
{
    m_openedAt = CStdUtil_Cocoa::GetUpTimeMS();
    m_duration = 0;
}

void CUnitInfoScreen::OnClosed()
{
    if (m_game->m_unitInfo == this) {
        m_game->OnUnitInfoScreenClosed();
    }
}

void CUnitInfoScreen::OnEasyFinished()
{
    switch (m_state) {
    case 1:
        m_openedAt = CStdUtil_Cocoa::GetUpTimeMS();
        m_state = 2;
        break;
    case 3:
        m_game->OnUnitInfoScreenClosed();
        m_state = 0;
        break;
    }
}
