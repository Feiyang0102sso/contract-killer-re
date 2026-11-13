// CScrollWindow.h - horizontal pages with swipe, arrows and page indicators.
#ifndef CSCROLLWINDOW_H
#define CSCROLLWINDOW_H

#include "PointerEventController.h"
#include "WindowBase.h"

class CScrollWindow : public Window {
public:
    explicit CScrollWindow(bool showArrows);
    virtual ~CScrollWindow();

    virtual void Update();
    virtual void OnKeyPressed(Event& event);
    virtual void OnPointerEvent(Event& event);
    virtual void OnUnhandledEvent(Event& event);
    virtual void OnCommand(Event& event);

    void Add(Window* content);
    void FlushPages();
    void ShowArrows();
    void HideArrows();
    void ShowPager(bool visible);
    void ScrollLeftRight();
    void ScrollRightLeft();
    void SetListener(Window* listener) { m_listener = listener; }

private:
    enum State { Idle = 0, MovingToNext = 2, MovingToPrevious = 3 };

    void Init(bool);
    void MoveContents(int x, int y);
    void SetState(int state);
    void UpdatePager();
    void PointerReleased();
    void SendEventToListener(unsigned int command);

    int m_previousPageOffset;              // 0xa4
    int m_scrollState;
    int m_pageIndex;
    int m_lastDeltaX;
    int m_dragDistance;
    int m_touchElapsed;
    float m_releaseVelocity;
    PointerEventController m_touch;        // 0xc0
    Vector<Window*> m_pages;                // 0xe8
    Window* m_contents;
    int m_reservedFC[2];
    int m_reserved104[2];
    int m_touchX;
    int m_touchY;
    Window* m_pager;
    int m_pagerPage;
    Window* m_listener;
    bool m_released;
    bool m_showArrows;
    Window* m_leftArrow;
    Window* m_rightArrow;
};

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char ScrollWindowSizeCheck[sizeof(CScrollWindow) == 0x12c ? 1 : -1];
#endif

#endif
