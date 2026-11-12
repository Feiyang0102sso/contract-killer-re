#ifndef CPOPUPCONTROLLER_H
#define CPOPUPCONTROLLER_H

#include "Vector_ss.h"
#include "WindowBase.h"

class AnimatedWindow;
class CAchievement;

class CPopupController : public WindowTransparent {
public:
    enum EPosition { TopRight, RightCenter, BottomLeft, Center, Achievement, PositionCount };
    enum EAnimationStyle { Announcement, Recovery, Brief, Award };

    CPopupController();
    virtual ~CPopupController();
    virtual void OnCommand(Event& event);
    virtual void Update();
    Window* AddPopup(const char* icon, XString text, EPosition position, EAnimationStyle style, int font);
    Window* AddPopup(const char* icon, int value, EPosition position, EAnimationStyle style, int font);
    void AddAchievementPopup(const CAchievement* achievement);
    void AddTrickShotPopup(const XString& name, const XString& points);
    void SetupStyle(AnimatedWindow* window, EAnimationStyle style);
    void SetupPosition(AnimatedWindow* window, EPosition position);
    void AddPopupInQueue(Window* window, EPosition position);

    static const unsigned int POPUP_TIME_INTERVAL_MS[PositionCount];
    static const int POPUP_STACK_MAX_SIZE[PositionCount];

    Vector<Window*> m_queues[PositionCount];    // 0xa4
    unsigned int m_lastPopupMS[PositionCount];  // 0xf4
    Window* m_enemiesLeft;                     // 0x108
};

#endif
