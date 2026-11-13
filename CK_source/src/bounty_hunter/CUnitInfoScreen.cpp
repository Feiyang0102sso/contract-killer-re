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
#include "ICStdUtil.h"
#include "ICSystemFont.h"
#include "SScreenAdaptingConstant.h"
#include "ServiceWindows.h"

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char CKUnitInfoWindowSize[sizeof(CUnitInfoScreen) == 0x160 ? 1 : -1];
#endif

namespace {
inline __attribute__((always_inline)) TextWindow* AddInfoLine(Window* content, const XString& text, CFont* font, int row)
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
      m_tagged(tagged), m_civilian(false), m_duration(0), m_guid(-1)
{
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
    m_distanceText = XString("");
    switch (unit->m_type) {
    case UnitCivilian:
        m_type = CUtility::GetString("IDS_GAME_PLAY_CIVILIAN").GetWCharPtr();
        m_distanceText = XString("");
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
    m_revealTarget = revealTarget;
}

void CUnitInfoScreen::Init()
{
    CreateContent();
    StartEasy(m_fade, FLAG_SHOW_AFTER_EASY);
    m_state = 1;
}

void CUnitInfoScreen::CreateContent()
{
    if (CSwerveGame::m_pInstance == NULL) {
        return;
    }
    CUnit* unit = CSwerveGame::m_pInstance->GetUnitByGUID(m_guid);
    if (unit == NULL) {
        return;
    }

    Window* content = new Window(false);
    content->SetPercentWidth(40, 0, 0);
    content->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    m_fade.Init(content, EasyLib::LINEAR);
    m_fade.SetFullTime(0.5f);
    m_fade.SetOpacity(0.0f, 0.8f);

    CFont* heading = CFontMgr::GetInstance()->GetFont(16);
    CFont* font = CFontMgr::GetInstance()->GetFont(17);
    CFont* civilian = CFontMgr::GetInstance()->GetFont(18);

    int row = 0;
    if (m_tagged) {
        if (m_distance > m_identificationRange) {
            XString unknown(CUtility::GetString("IDS_GAME_PLAY_UNKNOWN").GetWCharPtr());
            AddInfoLine(content, unknown, font, 0);
            XString distance(CUtility::GetString("IDS_GAME_PLAY_DISTANCE").GetWCharPtr());
            XString tooFar(CUtility::GetString("IDS_GAME_PLAY_DISTANCE_TOO_FAR").GetWCharPtr());
            wchar_t text[1000];
            ICStdUtil::SWPrintF(text, L"%s %s", distance.c_str(), tooFar.c_str());
            XString rangeText(text);
            AddInfoLine(content, rangeText, font, 1);
            content->SetHeightByContent(0, 0);
            AddToFront(content);
            return;
        }
    } else {
        CFont* titleFont = font;
        if (unit != NULL && unit->m_tranquillized) {
            m_title = ResString("IDS_GAME_PLAY_TRANQUILIZED");
            titleFont = civilian;
        }
        TextWindow* title = new TextWindow(m_title.c_str(), titleFont);
        title->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
        content->AddToFront(title, 0, 0);
        row = 1;
    }

    CFont* nameFont = font;
    if (m_revealTarget) {
        ICSystemFont* systemFont = ICSystemFont::CreateInstance();
        const SScreenAdaptingConstant nameSize(15, 30);
        systemFont->Initialize(nameSize(), NULL, 1);
        nameFont = systemFont;
    }
    TextWindow* name = AddInfoLine(content, m_name, nameFont, row);
    if (m_revealTarget) {
        const SScreenAdaptingConstant nameHeight(25, 40);
        name->SetDesiredHeight(nameHeight());
    }
    CFont* typeFont = heading;
    if (m_civilian) {
        typeFont = civilian;
    }
    AddInfoLine(content, m_type, typeFont, row + 1);
    row += 2;

    if (!m_tagged) {
        wchar_t text[10];
        ICStdUtil::SWPrintF(text, L"+%d", unit->m_field1f0);
        XString reward(text);
        LabelWithIcon* xp = new LabelWithIcon("IDB_HUD_ICON_EXP", reward, true, font, 0);
        Window* xpRow = new Window(false);
        xp->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
        xpRow->AddToFront(xp);
        xpRow->SetHeightByContent(0, 0);
        xpRow->SetWidthByContent(0, 0);
        xpRow->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
        content->AddToFront(xpRow, 0, row);
    } else {
        XString distance(CUtility::GetString("IDS_GAME_PLAY_DISTANCE").GetWCharPtr());
        XString measure(CUtility::GetString("IDS_GAME_PLAY_MEASURE").GetWCharPtr());
        XString outOfRange(CUtility::GetString("IDS_GAME_PLAY_DISTANCE_OUT_OF_RANGE").GetWCharPtr());
        float distanceInFeet = m_distance * 3.28f;
        CDH_Weapon* weapon = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetActiveWeapon();
        if (weapon == NULL) {
            return;
        }

        wchar_t text[1000];
        float currentDistanceInFeet = m_distance * 3.28f;
        if (!(currentDistanceInFeet > weapon->effectiveRange())) {
            ICStdUtil::SWPrintF(text, L"%s: %d %s", distance.c_str(),
                               static_cast<int>(distanceInFeet), measure.c_str());
        } else {
            ICStdUtil::SWPrintF(text, L"%s : %s", distance.c_str(), outOfRange.c_str());
        }
        XString rangeText(text);
        content->AddToFront(new TextWindow(rangeText, font), 0, row);

        int width = 100;
        if (App::IsVGA() || App::IsXGA() || App::IsWVGA() == 1) {
            width = 200;
        }
        m_hpBar = new CAnimatedProgressBar(width, "IDB_HUD_PROGRESSBAR_NPC_DARK_RED",
                                           "IDB_HUD_PROGRESSBAR_NPC_LEFT", "IDB_HUD_PROGRESSBAR_NPC_RIGHT",
                                           "IDB_HUD_PROGRESSBAR_NPC", "IDB_HUD_PROGRESSBAR_NPC_PINK");
        m_hpBar->SetMaxValue(m_maxHp);
        m_hpBar->SetCurrValue(m_hp);
        m_hpBar->SetMaxValue(m_maxHp);
        m_hpBar->m_drawText = false;
        content->AddToFront(m_hpBar, 0, row + 1);
    }
    content->SetWidthByContent(0, 0);
    content->SetHeightByContent(0, 0);
    AddToFront(content);
}

void CUnitInfoScreen::Update()
{
    if (m_state != 2 || CSwerveGame::m_pInstance == NULL) {
        return;
    }
    CUnit* unit = CSwerveGame::m_pInstance->GetUnitByGUID(m_guid);
    if (unit == NULL) {
        return;
    }

    unsigned int now = CStdUtil_Cocoa::GetUpTimeMS();
    unsigned int duration = m_duration;
    unsigned int openedAt = m_openedAt;
    if (duration == 0) {
        duration = 3000;
        if (m_tagged) {
            duration = 1500;
        }
    }
    if (now - openedAt > duration) {
        ReverseEasy(FLAG_CLOSE_AFTER_EASY);
        m_state = 3;
    }
    if (m_tagged && m_hp != unit->m_hp) {
        m_hp = unit->m_hp;
        m_hpBar->SetCurrValueWithAnimation(m_hp);
        m_hpBar->SetMaxValue(unit->m_maxHp);
    }
    if (m_tagged && m_hpBar->m_deltaPercent == 0.0f && (unit->m_dead || unit->m_tranquillized)) {
        static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->ShowUnitKillScreen(unit);
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
