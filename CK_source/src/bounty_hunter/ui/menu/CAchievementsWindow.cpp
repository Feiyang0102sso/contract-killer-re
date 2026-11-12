#include "CAchievementsWindow.h"

#include "App.h"
#include "CAchievementManager.h"
#include "CFontMgr.h"

namespace {
void SetAchievementContent(ImageWindow* panel, int progress, int steps, const XString& name,
                           const XString& description, const XString& completedDescription, const XString& image)
{
    const int scale = App::Scale();
    const int margin = 5 * scale;
    ImageWindow* icon = new ImageWindow();
    icon->SetImageById(XString::AnsiString(image.c_str()));
    icon->SetLayoutType(Window::LAYOUT_NONE);
    icon->SetCorner(margin, margin);
    panel->AddToFront(icon);
    int x = icon->m_width + 2 * margin;
    int width = panel->m_width - x - margin;
    if (width <= 0) {
        width = 180 * scale;
    }
    int percent = 0;
    if (steps > 0) {
        percent = progress * 100 / steps;
    }
    XString title = XString::Format(L"%ls  %d%%", name.c_str(), percent);
    TextWindow* heading = new TextWindow(title, CFontMgr::GetInstance()->GetFont(16));
    heading->SetLayoutType(Window::LAYOUT_NONE);
    heading->SetRect(x, margin, width, 16 * scale, false);
    panel->AddToFront(heading);
    XString details = description;
    if (progress >= steps) {
        details = completedDescription;
    }
    TextWindow* text = new TextWindow(details, CFontMgr::GetInstance()->GetFont(15));
    text->SetLayoutType(Window::LAYOUT_NONE);
    text->SetRect(x, 21 * scale, width, panel->m_height - 23 * scale, false);
    panel->AddToFront(text);
}
}

CAchievementsWindow::CAchievementsWindow()
    : CBaseScreen(false, true, true, false, true, false)
{
    SetTitleText(ResString("IDS_TITLE_ACHIEVEMENTS"));
    ImageWindow* background = new ImageWindow();
    background->SetImageById("IDB_AWARDS_BG");
    background->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    m_content->AddToFront(background);

    CAchievementManager* manager = static_cast<App*>(WindowApp::m_instance)->InitAchievementManager();
    ScrollingContainer* scroll = new ScrollingContainer();
    scroll->SetLayoutType(LAYOUT_NONE);
    int inset = 12 * App::Scale();
    int scrollWidth = m_content->m_width - 2 * inset - 10 * App::Scale();
    scroll->SetRect(inset, inset, scrollWidth, m_content->m_height - 2 * inset, false);
    int y = 0;
    for (int index = 0; index < manager->m_names.size(); ++index) {
        CAchievement** achievement = manager->m_achievements.Get(manager->m_names[index]);
        if (achievement != NULL && (!(*achievement)->m_hidden || (*achievement)->m_progress > 0)) {
            AchievementInfoWindow* row = new AchievementInfoWindow(*achievement);
            row->SetLayoutType(LAYOUT_NONE);
            row->SetCorner((scrollWidth - row->m_width) / 2, y);
            y += row->m_height + 5 * App::Scale();
            scroll->AddToFront(row);
        }
    }
    m_content->AddToFront(scroll);
    ScrollingContainer::VertScroller* bar = new ScrollingContainer::VertScroller(scroll);
    bar->SetLayoutType(LAYOUT_NONE);
    bar->SetRect(inset + scrollWidth, inset, 10 * App::Scale(), scroll->m_height, false);
    m_content->AddToFront(bar);
}

AchievementInfoWindow::AchievementInfoWindow(const CAchievement* achievement)
{
    SetImageById("SUR_ACHIEVEMENT_INFO_BG");
    SetAchievementContent(this, achievement->m_progress, achievement->m_steps, achievement->m_screenName,
                          achievement->m_description, achievement->m_completedDescription, achievement->m_imageId);
}

AchievementReceivedWindow::AchievementReceivedWindow(int progress, int steps, const XString& name,
                                                     const XString& description, const XString& completedDescription,
                                                     const XString& image)
{
    SetImageById("SUR_ACHIEVEMENT_BG");
    SetAchievementContent(this, progress, steps, name, description, completedDescription, image);
}
