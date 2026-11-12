#ifndef CACHIEVEMENTSWINDOW_H
#define CACHIEVEMENTSWINDOW_H

#include "BaseDialog.h"
#include "CBaseScreen.h"

class CAchievement;

class CAchievementsWindow : public CBaseScreen {
public:
    CAchievementsWindow();
    virtual ~CAchievementsWindow() {}
};

class AchievementInfoWindow : public ImageWindow {
public:
    explicit AchievementInfoWindow(const CAchievement* achievement);
    virtual ~AchievementInfoWindow() {}
};

class AchievementReceivedWindow : public ImageWindow {
public:
    AchievementReceivedWindow(int progress, int steps, const XString& name, const XString& description,
                              const XString& completedDescription, const XString& image);
    virtual ~AchievementReceivedWindow() {}
};

#endif
