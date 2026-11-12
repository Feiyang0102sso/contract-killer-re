#ifndef CCAREERWINDOW_H
#define CCAREERWINDOW_H

#include "CBaseScreen.h"
#include "Vector_ss.h"

class CCareerWindow : public CBaseScreen {
public:
    struct StatItemInfo {
        StatItemInfo(const char* name, int value, bool percent, bool gameCenter)
            : m_name(name), m_value(value), m_percent(percent), m_gameCenter(gameCenter) {}

        const char* m_name;
        int m_value;
        bool m_percent;
        bool m_gameCenter;
    };

    CCareerWindow();
    virtual void OnCommand(Event& event);
    virtual void Update();
    virtual void OnClosed();

private:
    void FillStats();

    Window* m_careerContent;
    int m_elapsedMS;
    Vector<StatItemInfo> m_stats;
};

class VertScrollerSimpleRed : public ScrollingContainer::VertScroller {
public:
    explicit VertScrollerSimpleRed(ScrollingContainer* container);
    virtual void Paint(ICGraphics2d& graphics);
};

class SimpleGreyProgressBar : public Window {
public:
    explicit SimpleGreyProgressBar(float percent);
    virtual void Paint(ICGraphics2d& graphics);

private:
    ImageRes m_background;
    ImageRes m_center;
    ImageRes m_left;
    ImageRes m_right;
    float m_percent;
};

class StatItem : public Window {
public:
    StatItem(const CCareerWindow::StatItemInfo& info, int index);
    virtual void OnStartupEasy();

private:
    int m_index;
    bool m_started;
};

#endif
