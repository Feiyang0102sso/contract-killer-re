#include "CAchievementsWindow.h"

#include "App.h"
#include "CAchievementManager.h"
#include "CCareerWindow.h"
#include "CFontMgr.h"
#include "CImageButton.h"
#include "ServiceWindows.h"
#include "SScreenAdaptingConstant.h"

namespace {
const SScreenAdaptingConstant ScrollbarInset(10, 20);
const SScreenAdaptingConstant RowSpacing(5, 10);
const SScreenAdaptingConstant ScreenBottom(10, 20);
}

typedef char AchievementsWindowSize[sizeof(CAchievementsWindow) == 0xdc ? 1 : -1];

CAchievementsWindow::CAchievementsWindow()
    : CBaseScreen(false, true, true, false, true, false)
{
    SetTitleText(ResString("IDS_TITLE_ACHIEVEMENTS"));
    SetInsetSpacing(m_titleHeight, 0, ScreenBottom(), 0);
    CFontMgr::GetInstance()->GetFont(10);
    ImageWindow* background = new ImageWindow();
    background->SetImageById("IDB_AWARDS_BG");
    AddToFront(background);

    m_achievementList = new Window(false);
    m_achievementList->SetLayoutType(LAYOUT_SIMPLE);
    m_achievementList->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    m_achievementList->SetPercentHeight(100, 0, 0);
    m_achievementList->SetWidthByContent(0, 0);

    App* app = static_cast<App*>(WindowApp::m_instance);
    ScrollingContainer* scroll = new ScrollingContainer();
    scroll->SetCellPos(0, 0, 1, 1);
    scroll->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    scroll->SetDesiredWidth((int)((float)app->m_screenWidth * 0.95f));

    Window* rows = new WindowTransparent(false);
    rows->SetLayoutType(LAYOUT_SIMPLE);
    rows->SetHeightByContent(0, 0);
    rows->SetWidthByContent(0, 0);
    rows->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    int insetTop = App::Scale() * 10;
    int insetLeft = App::Scale() * 5;
    rows->SetInsetSpacing(insetTop, 0, 0, insetLeft);
    CAchievementManager* manager = app->InitAchievementManager();
    int visibleRow = 0;
    for (int index = 0; index < manager->m_names.size(); ++index) {
        CAchievement** achievement = manager->m_achievements.Get(manager->m_names[index]);
        if (achievement != NULL && (!(*achievement)->m_hidden || (*achievement)->m_progress != 0)) {
            AchievementInfoWindow* row = new AchievementInfoWindow(*achievement);
            row->SetOutsetSpacing(0, 0, RowSpacing(), 0);
            rows->AddToFront(row, 0, visibleRow);
            ++visibleRow;
        }
    }

    Window* scrollContent = new WindowTransparent(false);
    scrollContent->SetPercentWidth(100, 0, 0);
    scrollContent->SetHeightByContent(0, 0);
    scrollContent->SetLayoutType(LAYOUT_NONE);
    scrollContent->AddToFront(rows);
    scroll->AddToFront(scrollContent);
    m_achievementList->AddToFront(scroll);

    VertScrollerSimpleRed* bar = new VertScrollerSimpleRed(scroll);
    bar->SetOutsetSpacing(0, 0, 0, ScrollbarInset());
    bar->SetCellPos(1, 0, 1, 1);
    m_achievementList->AddToFront(bar);
    Window* cell = createTD(m_achievementList, 3);
    AddToFront(cell, 0, 0);
    cell->SetPercentHeight(92, 0, 0);
}

AchievementInfoWindow::AchievementInfoWindow(const CAchievement* achievement)
    : ImageWindow(FILL_NONE, ImageRes("SUR_ACHIEVEMENT_INFO_BG"))
{
    int progress = achievement->m_progress;
    int steps = achievement->m_steps;
    bool completed = progress == steps;
    if (progress < steps) {
        float opacity = 0.25f;
        if (progress > 0) {
            opacity = 0.5f + (float)progress / (float)steps * 0.5f;
        }
        SetOpacity(opacity);
    }

    Window* content = new WindowTransparent(false);
    int top = SScreenAdaptingConstant(2, 10)();
    int right = App::Scale() * 10;
    int bottom = SScreenAdaptingConstant(20, 10)();
    int left = App::Scale() * 10;
    content->SetInsetSpacing(top, right, bottom, left);
    XString title = achievement->m_screenName;
    const XString* details = &achievement->m_description;
    int textColumn = 0;
    if (completed) {
        CImageButton* award = new CImageButton();
        int awardTop = SScreenAdaptingConstant(7, 0)();
        int awardBottom = SScreenAdaptingConstant(10, 0)();
        award->SetOutsetSpacing(awardTop, 0, awardBottom, 0);
        award->AddImage(XString::AnsiString(achievement->m_imageId.c_str()), 0, 0);
        award->SetCellPos(0, 0, 1, 2);
        award->SetWidthByContent(0, 0);
        content->AddToFront(award);
        textColumn = 1;
        details = &achievement->m_completedDescription;
    } else if (progress > 0 && steps > 0) {
        int percent = (int)((float)progress * 100.0f / (float)steps);
        title = XString(percent) + XString("% ") + title;
    }

    TextWindow* heading = new TextWindow(title.Upper(), CFontMgr::GetInstance()->GetFont(9));
    heading->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    content->AddToFront(heading, textColumn, 0);
    TextWindow* text = new TextWindow(*details, CFontMgr::GetInstance()->GetFont(15));
    text->SetOutsetSpacing(SScreenAdaptingConstant(-2, 0)(), 0, 0, 0);
    text->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    text->SetCellPos(textColumn, 1, 1, 1);
    text->SetPercentWidth(100, 0, 0);
    content->AddToFront(text);
    content->SetPercentWidth(100, 0, 0);
    content->SetPercentHeight(100, 0, 0);
    AddToFront(content);
}

AchievementReceivedWindow::AchievementReceivedWindow(int progress, int steps, const XString& name,
                                                     const XString& description, const XString& completedDescription,
                                                     const XString& image)
    : ImageWindow(FILL_NONE, ImageRes("SUR_ACHIEVEMENT_BG"))
{
    int top = SScreenAdaptingConstant(3, 16)();
    int left = SScreenAdaptingConstant(14, 28)();
    int bottom = SScreenAdaptingConstant(80, 140)();
    int right = SScreenAdaptingConstant(14, 28)();
    Window* content = new WindowTransparent(false);
    XString title = name;
    const XString* details = &description;
    if (progress == steps) {
        CImageButton* award = new CImageButton();
        award->AddImage(XString::AnsiString(image.c_str()), 0, 0);
        int awardTop = SScreenAdaptingConstant(12, 24)();
        int awardLeft = SScreenAdaptingConstant(14, 28)();
        award->SetOutsetSpacing(awardTop, 0, 0, awardLeft);
        award->SetLayoutType(LAYOUT_SIMPLE);
        award->SetAlign(ALIGN_LEFT | ALIGN_TOP);
        award->SetWidthByContent(0, 0);
        AddToFront(award);
        left += SScreenAdaptingConstant(48, 96)();
        details = &completedDescription;
    } else {
        int percent = 0;
        if (steps > 0) {
            percent = (int)((float)progress * 100.0f / (float)steps);
        }
        title = XString(percent) + XString("%  ") + title;
    }
    content->SetInsetSpacing(top, right, bottom, left);

    TextWindow* heading = new TextWindow(title, CFontMgr::GetInstance()->GetFont(9));
    heading->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    content->AddToFront(heading, 0, 0);
    TextWindow* text = new TextWindow(*details, CFontMgr::GetInstance()->GetFont(15));
    text->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    text->SetCellPos(0, 1, 1, 1);
    text->SetPercentWidth(100, 0, 0);
    content->AddToFront(text);
    AddToFront(content);
}
