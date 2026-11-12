// CFactionsWindow.cpp - show faction bars and resolve titles by rating range.
#include "CFactionsWindow.h"

#include "App.h"
#include "CBH_Player.h"
#include "CBH_ProgressBar.h"
#include "CFactionType.h"
#include "CFactionTypeManager.h"
#include "CFontMgr.h"
#include "CRatingBar.h"
#include "CXmlHelper.h"
#include "ResMap.h"

#include <stdlib.h>

namespace {
Window* CreateFactionCell(const XString& caption, CBH_ProgressBar* bar)
{
    Window* cell = new Window(false);
    cell->SetLayoutType(Window::LAYOUT_TABLE);
    cell->SetPercentHeight(40, 0, 0);
    TextWindow* name = new TextWindow(caption, CFontMgr::GetInstance()->GetFont(9));
    name->SetCellPos(0, 0, 1, 1);
    name->SetPercentHeight(40, 0, 0);
    cell->AddToFront(name);
    bar->m_displayFlags = CBH_ProgressBar::DISPLAY_CURRENT;
    bar->SetPercentHeight(60, 0, 0);
    bar->SetCellPos(0, 1, 1, 1);
    cell->AddToFront(bar);
    return cell;
}
}

CFactionsWindow::CFactionsWindow()
    : CBaseScreen(false, false, true, false, true, false)
{
    LoadTitleStrings();
    m_background.Create("SUR_TAB_BACKGROUND");
    SetTitleText(ResString("IDS_TITLE_FACTIONS"));
    m_content->SetLayoutType(LAYOUT_SIMPLE);

    App* app = static_cast<App*>(WindowApp::m_instance);
    CFactionTypeManager* factions = app->InitFactionTypeManager();
    Vector<XString*> names;
    factions->GetFactionTypeNames(names);
    Window* standings = new Window(false);
    standings->SetPercentHeight(70, 0, 0);
    CBH_Player* player = CBH_Player::GetInstance();
    int reputationLimit = player->GetReputationLimit();
    int factionRows = 0;
    if (names.size() >= 2) {
        factionRows = names.size() / 2;
    }
    for (int column = 0; column < 2; ++column) {
        for (int row = 0; row < factionRows; ++row) {
            const XString& key = *names[2 * row + column];
            const CFactionType* faction = factions->GetType(key);
            int color = 2;
            if (faction->m_legal) {
                color = 3;
            }
            CBH_ProgressBar* bar = new CBH_ProgressBar(color, m_width / 2, -1);
            bar->SetDescImage(faction->m_smallIcon);
            bar->SetMaxValue(reputationLimit * 2);
            bar->SetCurrValue(reputationLimit + player->GetReputation(key));
            Window* cell = CreateFactionCell(key.Upper(), bar);
            cell->SetCellPos(column, row, 1, 1);
            standings->AddToFront(cell);
        }
    }

    int ratingRow = factionRows + 2;
    for (int row = factionRows; row < ratingRow; ++row) {
        for (int column = 0; column < 2; ++column) {
            CBH_ProgressBar* bar = new CBH_ProgressBar(7, m_width / 2, -1);
            bar->SetDescImage("IDB_IMAGE_SUSPENSE_ICON");
            Window* cell = CreateFactionCell(XString(L"???"), bar);
            cell->SetCellPos(column, row, 1, 1);
            standings->AddToFront(cell);
        }
    }

    WindowTransparent* meterRow = new WindowTransparent(false);
    meterRow->SetPercentHeight(30, 0, 0);
    meterRow->SetPercentWidth(100, 0, 0);
    meterRow->SetCellPos(0, ratingRow, 2, 1);
    CRatingBar* meter = new CRatingBar();
    int rating = CalculateOverallRating();
    meter->SetLimitValues(-reputationLimit, reputationLimit);
    meter->SetValue(rating);
    meter->SetPercentWidth(70, 0, 0);
    meter->m_debugFrame = true;
    meterRow->AddToFront(meter);

    TextWindow* title = new TextWindow(GetTitleForRating(rating), CFontMgr::GetInstance()->GetFont(10));
    title->SetCellPos(0, ratingRow + 1, 2, 1);
    title->SetOutsetSpacing(0, 50, 0, 0);
    title->SetPercentHeight(40, 0, 0);
    standings->AddToFront(title);
    standings->SetAlign(ALIGN_BOTTOM);
    standings->AddToFront(meterRow);
    m_content->AddToFront(standings);
}

CFactionsWindow::~CFactionsWindow()
{
    SAFE_DELETE_VECTOR(m_titles);
}

void CFactionsWindow::LoadTitleStrings()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    char* text = app->InitResMap()->loadText(XString(L"res/common/configs/rogue_meter.xml"));
    m_titleConfig.Parse(text, NULL, TIXML_ENCODING_UNKNOWN);
    if (text != NULL) {
        free(text);
    }
    for (const TiXmlElement* element = m_titleConfig.FirstChildElement("title");
         element != NULL; element = element->NextSiblingElement("title")) {
        TRougueTitle* title = new TRougueTitle();
        title->m_max = CXmlHelper::GetAttributeValue(element, "max").ToInt();
        title->m_min = CXmlHelper::GetAttributeValue(element, "min").ToInt();
        title->m_title = ResString(CXmlHelper::GetCharAttributeValue(element, "title"));
        m_titles.addElement(title);
    }
}

int CFactionsWindow::CalculateOverallRating()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    Vector<XString*> names;
    app->InitFactionTypeManager()->GetFactionTypeNames(names);
    int rating = 0;
    for (int index = 0; index < names.size(); ++index) {
        const XString& name = *names[index];
        int reputation = CBH_Player::GetInstance()->GetReputation(name);
        const CFactionType* faction = app->InitFactionTypeManager()->GetType(name);
        if (faction->m_legal) {
            rating += reputation;
        } else {
            rating -= reputation;
        }
    }
    return rating;
}

XString CFactionsWindow::GetTitleForRating(int rating)
{
    for (int index = 0; index < m_titles.size(); ++index) {
        const TRougueTitle* title = m_titles[index];
        if (title->m_min <= rating && rating < title->m_max) {
            return title->m_title;
        }
    }
    return XString("2");
}

void CFactionsWindow::Paint(ICGraphics2d& graphics)
{
    DrawTileImageRect(graphics, m_background.GetSurface(), 0, 0, m_width, m_height);
    CBaseScreen::Paint(graphics);
}

void CFactionsWindow::OnClosed()
{
    WindowApp::HandleTunnelCommand(0x703ba476, 0, 0, 0);
}
