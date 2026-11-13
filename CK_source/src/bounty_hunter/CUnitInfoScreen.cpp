#include "CUnitInfoScreen.h"

#include "App.h"
#include "BaseDialog.h"
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
#include "SScreenAdaptingConstant.h"
#include "ServiceWindows.h"

typedef char CKUnitInfoWindowSize[sizeof(CUnitInfoScreen) == 0x160 ? 1 : -1];

namespace {
TextWindow* AddInfoLine(Window* content, const XString& text, CFont* font, int row)
{
    TextWindow* line = new TextWindow(text, font);
    line->SetAlign(Window::ALIGN_HCENTER | Window::ALIGN_VCENTER);
    content->AddToFront(line, 0, row);
    return line;
}
}

CUnitInfoScreen::CUnitInfoScreen(CSwerveGame* game, CUnit* unit, float distance,
                                 float identificationRange, bool tagged, int startingDamage, bool revealTarget)
    : Window(false), m_hpBar(NULL), m_distance(distance), m_identificationRange(identificationRange),
      m_startingDamage(startingDamage), m_maxHp(0), m_hp(0), m_game(game), m_openedAt(0), m_state(0),
      m_tagged(tagged), m_civilian(false), m_revealTarget(revealTarget), m_duration(0), m_guid(-1)
{
    ClearAndSetFlags(FLAG_FOCUSABLE | FLAG_ENABLED, 0);
    if (unit == NULL) {
        return;
    }
    m_guid = unit->m_guid;
    SetLayoutType(LAYOUT_NONE);
    const SScreenAdaptingConstant topOffset(75, 150);
    SetCorner(0, -topOffset());
    if (m_tagged) {
        m_title = CUtility::GetString("IDS_GAME_PLAY_TAGGED").GetWCharPtr();
    } else {
        m_title = CUtility::GetString("IDS_GAME_PLAY_KILLED").GetWCharPtr();
    }
    m_name = unit->m_name;
    switch (unit->m_type) {
    case UnitCivilian:
        m_type = CUtility::GetString("IDS_GAME_PLAY_CIVILIAN").GetWCharPtr();
        m_name = XString("");
        m_civilian = true;
        break;
    case UnitWarrior:
        m_type = CUtility::GetString("IDS_GAME_PLAY_MINION").GetWCharPtr();
        break;
    case UnitBoss:
        m_type = CUtility::GetString("IDS_GAME_PLAY_BOSS").GetWCharPtr();
        break;
    }
    m_maxHp = unit->m_maxHp;
    m_hp = unit->m_hp + m_startingDamage;
}

void CUnitInfoScreen::Init()
{
    CreateContent();
    StartEasy(m_fade, FLAG_SHOW_AFTER_EASY);
    m_state = 1;
}

void CUnitInfoScreen::CreateContent()
{
    CUnit* unit = m_game->GetUnitByGUID(m_guid);
    if (unit == NULL) {
        return;
    }
    Window* content = new Window(false);
    content->SetPercentWidth(40, 0, 0);
    content->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    content->SetHeightByContent(0, 0);
    m_fade.Init(content, EasyLib::LINEAR);
    m_fade.SetFullTime(0.5f);
    m_fade.SetOpacity(0.0f, 0.8f);

    CFontMgr* fonts = CFontMgr::GetInstance();
    CFont* heading = fonts->GetFont(16);
    CFont* font = fonts->GetFont(17);
    CFont* civilian = fonts->GetFont(18);
    if (m_tagged && m_distance > m_identificationRange) {
        AddInfoLine(content, ResString("IDS_GAME_PLAY_UNKNOWN"), font, 0);
        m_distanceText = ResString("IDS_GAME_PLAY_DISTANCE") + L" " + ResString("IDS_GAME_PLAY_DISTANCE_TOO_FAR");
        AddInfoLine(content, m_distanceText, font, 1);
        AddToFront(content);
        return;
    }

    int row = 0;
    if (!m_tagged) {
        CFont* titleFont = font;
        if (unit->m_tranquillized) {
            m_title = ResString("IDS_GAME_PLAY_TRANQUILIZED");
            titleFont = civilian;
        }
        AddInfoLine(content, m_title, titleFont, row++);
    }
    TextWindow* name = AddInfoLine(content, m_name, font, row++);
    if (m_revealTarget) {
        const SScreenAdaptingConstant nameHeight(25, 40);
        name->SetDesiredHeight(nameHeight());
    }
    CFont* typeFont = heading;
    if (m_civilian) {
        typeFont = civilian;
    }
    AddInfoLine(content, m_type, typeFont, row++);

    if (!m_tagged) {
        XString reward = XString::Format(L"+%d", unit->m_field1f0);
        LabelWithIcon* xp = new LabelWithIcon("IDB_HUD_ICON_EXP", reward, true, font, 0);
        xp->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
        Window* xpRow = new Window(false);
        xpRow->SetHeightByContent(0, 0);
        xpRow->SetWidthByContent(0, 0);
        xpRow->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
        xpRow->AddToFront(xp);
        content->AddToFront(xpRow, 0, row);
        content->SetWidthByContent(0, 0);
        AddToFront(content);
        return;
    }

    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_Weapon* weapon = app->m_helper->GetGameData()->GetActiveWeapon();
    if (weapon != NULL) {
        XString distance = ResString("IDS_GAME_PLAY_DISTANCE");
        float distanceInFeet = m_distance * 3.28f;
        if (distanceInFeet <= weapon->effectiveRange()) {
            XString measure = ResString("IDS_GAME_PLAY_MEASURE");
            m_distanceText = XString::Format(L"%ls: %d %ls", distance.c_str(),
                                             static_cast<int>(distanceInFeet), measure.c_str());
        } else {
            XString outOfRange = ResString("IDS_GAME_PLAY_DISTANCE_OUT_OF_RANGE");
            m_distanceText = XString::Format(L"%ls : %ls", distance.c_str(), outOfRange.c_str());
        }
        content->AddToFront(new TextWindow(m_distanceText, font), 0, row++);

        int width = 100;
        if (App::IsVGA() || App::IsXGA() || App::IsWVGA()) {
            width = 200;
        }
        m_hpBar = new CAnimatedProgressBar(width, "IDB_HUD_PROGRESSBAR_NPC_DARK_RED",
                                           "IDB_HUD_PROGRESSBAR_NPC_LEFT", "IDB_HUD_PROGRESSBAR_NPC_RIGHT",
                                           "IDB_HUD_PROGRESSBAR_NPC", "IDB_HUD_PROGRESSBAR_NPC_PINK");
        m_hpBar->SetMaxValue(m_maxHp);
        m_hpBar->SetCurrValue(m_hp);
        m_hpBar->m_drawText = false;
        m_hpBar->ClearAndSetFlags(FLAG_FOCUSABLE | FLAG_ENABLED, 0);
        content->AddToFront(m_hpBar, 0, row);
    }
    content->SetWidthByContent(0, 0);
    AddToFront(content);
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
        if (m_game->m_unitInfo == this) {
            m_game->OnUnitInfoScreenClosed();
        }
        m_state = 3;
        Close();
        return;
    }
    CUnit* unit = m_game->GetUnitByGUID(m_guid);
    if (m_tagged && unit != NULL && m_hpBar != NULL) {
        if (m_hp != unit->m_hp) {
            m_hp = unit->m_hp;
            m_hpBar->SetCurrValueWithAnimation(m_hp);
            m_hpBar->SetMaxValue(unit->m_maxHp);
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
