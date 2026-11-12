// CScrollWindow.cpp - keep every page at its offset from the selected page.
#include "CScrollWindow.h"

#include "App.h"
#include "BaseDialog.h"
#include "CImageButton.h"
#include "SScreenAdaptingConstant.h"

#include <math.h>

namespace {
const int ArrowFlags = Window::FLAG_VISIBLE | Window::FLAG_ENABLED;
const int PreviousCommand = 0x441b9a28;
const int NextCommand = (int)0xb4c8a72a;
const unsigned int ReachedLastPage = 0x236889e6;
const unsigned int WentToNextPage = 0x087412a4;
const unsigned int WentToPreviousPage = 0xfea612a5;
const float SwipeVelocity = 0.3f;
}

void CScrollWindow::Add(Window* content)
{
    for (int index = 0; index < m_pages.size(); ++index) {
        if (m_pages[index]->IsChildExisting(content)) {
            return;
        }
    }
    int offset = 0;
    if (m_pages.size() != 0) {
        offset = m_pages[0]->m_x;
    }
    Window* page = new Window(false);
    page->SetLayoutType(LAYOUT_NONE);
    int x = WindowApp::m_instance->m_screenWidth * m_pages.size() + offset;
    page->SetRect(x, 0, m_width, content->m_height, true);
    page->AddToFront(content);
    m_pages.addElement(page);
    m_contents->AddToFront(page);

    bool wide = App::IsWVGA();
    m_pager->SetLayoutType(LAYOUT_SIMPLE);
    if (wide) {
        m_pager->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
        m_pager->SetOutsetSpacing(SScreenAdaptingConstant(7, 15, 15, 7)(), 0, 0, 0);
    } else {
        m_pager->SetAlign(ALIGN_HCENTER | ALIGN_BOTTOM);
        m_pager->SetOutsetSpacing(0, 0, SScreenAdaptingConstant(7, 15, -1, -1)(), 0);
    }
    if (m_showArrows) {
        ShowArrows();
    }
}

void CScrollWindow::ShowArrows()
{
    if (m_leftArrow != NULL && m_pageIndex > 0) {
        m_leftArrow->SetFlags(ArrowFlags);
    }
    if (m_rightArrow != NULL && m_pageIndex < m_pages.size() - 1) {
        m_rightArrow->SetFlags(ArrowFlags);
    }
}

void CScrollWindow::Update()
{
    if (m_pages.size() == 0) {
        return;
    }
    int elapsed = WindowApp::m_instance->m_updateFrameTime;
    m_touchElapsed += elapsed;
    int step = 30 * App::Scale();
    m_touch.TouchUpdate(elapsed);
    PointerEventController::Position position = { 0, 0 };
    PointerEventController::Event event(position, 0, 0, PointerEventController::EVENT_NONE);
    while (m_touch.GetEvent(event)) {
        if (event.m_type == PointerEventController::EVENT_UP) {
            return;
        }
        if (event.m_type == PointerEventController::EVENT_MOVE) {
            m_lastDeltaX = event.m_deltaX;
            MoveContents(m_lastDeltaX, 0);
        }
    }

    Window* page = m_pages[m_pageIndex];
    int offset = page->m_x;
    if (m_scrollState == MovingToPrevious) {
        if (offset + step >= 0) {
            MoveContents(-offset, 0);
            SetState(Idle);
        } else {
            MoveContents(step, 0);
        }
    } else if (m_scrollState == MovingToNext) {
        if (offset - step <= 0) {
            MoveContents(-offset, 0);
            SetState(Idle);
        } else {
            MoveContents(-step, 0);
        }
    } else if (m_released) {
        if (offset < 0) {
            int correction = -offset;
            if (step < correction) {
                correction = step;
            }
            MoveContents(correction, 0);
        }
        offset = page->m_x;
        if (offset > 0) {
            int correction = offset;
            if (step < correction) {
                correction = step;
            }
            MoveContents(-correction, 0);
        }
    }
    UpdatePager();
}

void CScrollWindow::MoveContents(int x, int)
{
    for (int index = 0; index < m_pages.size(); ++index) {
        Window* page = m_pages[index];
        page->SetCorner(page->m_x + x, page->m_y);
    }
}

void CScrollWindow::SetState(int state)
{
    if (m_showArrows) {
        if (state == MovingToNext || state == MovingToPrevious) {
            HideArrows();
        }
        if (state == Idle && (m_scrollState == MovingToNext || m_scrollState == MovingToPrevious)) {
            ShowArrows();
        }
    }
    m_scrollState = state;
}

void CScrollWindow::UpdatePager()
{
    m_pager->CloseChilds();
    for (int index = 0; index < m_pages.size(); ++index) {
        const char* asset = "SUR_BOUNTY_SCROLL_WINDOW_SCROLL_OFF";
        if (index == m_pageIndex) {
            asset = "SUR_BOUNTY_SCROLL_WINDOW_SCROLL_ON";
        }
        ImageWindow* indicator = new ImageWindow(ImageWindow::FILL_NONE, ImageRes(asset));
        indicator->SetCellPos(index, 0, 1, 1);
        if (index < m_pages.size() - 1) {
            indicator->SetOutsetSpacing(0, 5, 0, 0);
        }
        indicator->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
        m_pager->AddToFront(indicator);
    }
    m_pager->SetWidthByContent(0, 0);
    m_pager->SetCorner(m_width / 2 - m_pager->m_width / 2, m_pager->m_y);
    m_pagerPage = m_pageIndex;
}

void CScrollWindow::OnKeyPressed(Event&)
{
}

void CScrollWindow::OnPointerEvent(Event& event)
{
    int x = event.m_param1;
    int y = event.m_param2;
    if (event.m_type == Event::POINTER_PRESSED) {
        m_touch.TouchDown(x, y);
        m_touchX = x;
        m_touchY = y;
        m_released = false;
        m_touchElapsed = 0;
        return;
    }
    if (m_released) {
        return;
    }
    if (event.m_type == Event::POINTER_RELEASED) {
        m_touch.TouchUp();
        m_released = true;
        m_dragDistance = static_cast<int>(fabsf(x - m_touchX) + fabsf(y - m_touchY));
        if (m_dragDistance <= 20 * App::Scale()) {
            return;
        }
    } else if (event.m_type == Event::POINTER_DRAGGED) {
        m_touch.TouchMove(x, y);
        if (GlobalToLocalX(x) > 4) {
            return;
        }
        m_dragDistance = static_cast<int>(fabsf(x - m_touchX) + fabsf(y - m_touchY));
        m_released = true;
    } else {
        return;
    }
    PointerReleased();
}

void CScrollWindow::PointerReleased()
{
    if (m_pagerPage > m_pages.size()) {
        return;
    }
    float distance = static_cast<float>(m_dragDistance);
    float velocity = fabsf(distance / static_cast<float>(m_touchElapsed));
    if (velocity >= 0.0f) {
        if (velocity > 2.0f) {
            velocity = 2.0f;
        }
    } else {
        velocity = 0.0f;
    }
    m_releaseVelocity = velocity;
    int halfScreen = WindowApp::m_instance->m_screenWidth / 2;
    if (fabsf(distance) < halfScreen && m_releaseVelocity < SwipeVelocity) {
        return;
    }
    if (m_lastDeltaX < 0) {
        ScrollLeftRight();
    } else if (m_lastDeltaX > 0) {
        ScrollRightLeft();
    }
}

void CScrollWindow::OnUnhandledEvent(Event& event)
{
    if (event.m_type == Event::POINTER_PRESSED || event.m_type == Event::POINTER_RELEASED) {
        event.Clear();
    }
}

CScrollWindow::CScrollWindow(bool showArrows)
    : Window(false), m_previousPageOffset(0), m_scrollState(Idle), m_pageIndex(0),
      m_lastDeltaX(0), m_dragDistance(0), m_touchElapsed(0), m_contents(NULL),
      m_touchX(0), m_touchY(0), m_pager(NULL), m_pagerPage(-1), m_listener(NULL),
      m_released(true), m_showArrows(showArrows), m_leftArrow(NULL), m_rightArrow(NULL)
{
    m_reserved104[0] = 0;
    m_reserved104[1] = 0;
    Init(false);
}

void CScrollWindow::Init(bool)
{
    SetLayoutType(LAYOUT_NONE);
    m_contents = new Window(false);
    AddToFront(m_contents);
    m_pager = new Window(false);
    m_pager->SetLayoutType(LAYOUT_NONE);
    m_pager->SetWidthByContent(0, 0);
    m_pager->SetHeightByContent(0, 0);
    AddToFront(m_pager);
    ClearFlags(FLAG_POINTER_EXCLUSIVE);
    ClearFlags(FLAG_CHILDREN_FIRST);
    m_releaseVelocity = 0.0f;
    if (!m_showArrows) {
        return;
    }

    CImageButton* left = new CImageButton();
    m_leftArrow = left;
    left->AddImage("IDB_MENU_ARROW_LEFT", 0, 0);
    left->SetAlign(ALIGN_LEFT | ALIGN_VCENTER);
    left->m_command = PreviousCommand;
    left->m_commandParameter1 = 0;
    left->m_commandParameter2 = 0;
    left->m_commandParameter3 = 0;
    left->ClearFlags(ArrowFlags);
    AddToFront(left);

    CImageButton* right = new CImageButton();
    m_rightArrow = right;
    right->AddImage("IDB_MENU_ARROW_RIGHT", 0, 0);
    right->SetLayoutType(LAYOUT_SIMPLE);
    right->SetAlign(ALIGN_RIGHT | ALIGN_VCENTER);
    right->m_command = NextCommand;
    right->m_commandParameter1 = 0;
    right->m_commandParameter2 = 0;
    right->m_commandParameter3 = 0;
    right->ClearFlags(ArrowFlags);
    AddToFront(right);
}

void CScrollWindow::ScrollLeftRight()
{
    if (m_pageIndex == m_pages.size() - 1) {
        SendEventToListener(ReachedLastPage);
    }
    if (m_pageIndex < m_pages.size() - 1 && m_scrollState != MovingToNext) {
        m_previousPageOffset = m_pages[m_pageIndex]->m_x - m_x;
        ++m_pageIndex;
        m_pages[m_pageIndex]->SetFlags(ArrowFlags);
        SetState(MovingToNext);
        SendEventToListener(WentToNextPage);
    }
}

void CScrollWindow::ScrollRightLeft()
{
    if (m_pageIndex > 0 && m_scrollState != MovingToPrevious) {
        --m_pageIndex;
        m_pages[m_pageIndex]->SetFlags(ArrowFlags);
        SetState(MovingToPrevious);
        SendEventToListener(WentToPreviousPage);
    }
}

void CScrollWindow::SendEventToListener(unsigned int command)
{
    if (m_listener != NULL) {
        Event event(Event::NONE, command, 0, 0, 0);
        m_listener->OnCommand(event);
    }
}

void CScrollWindow::FlushPages()
{
    for (int index = 0; index < m_pages.size(); ++index) {
        Window* page = m_pages[index];
        page->Close();
        page->HandleUpdate(true);
        page->HandleUpdate(true);
    }
    m_pages.removeAllElements();
    m_pagerPage = -1;
    m_pageIndex = 0;
    HideArrows();
}

void CScrollWindow::HideArrows()
{
    if (m_leftArrow != NULL) {
        m_leftArrow->ClearFlags(ArrowFlags);
    }
    if (m_rightArrow != NULL) {
        m_rightArrow->ClearFlags(ArrowFlags);
    }
}

CScrollWindow::~CScrollWindow()
{
    m_pages.removeAllElements();
}

void CScrollWindow::OnCommand(Event& event)
{
    if (event.m_id == NextCommand) {
        ScrollLeftRight();
    } else if (event.m_id == PreviousCommand) {
        ScrollRightLeft();
    }
}

void CScrollWindow::ShowPager(bool visible)
{
    if (m_pager != NULL) {
        if (visible) {
            m_pager->SetFlags(ArrowFlags);
        } else {
            m_pager->ClearFlags(ArrowFlags);
        }
    }
}
