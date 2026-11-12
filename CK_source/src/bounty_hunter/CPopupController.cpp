#include "CPopupController.h"

#include "App.h"
#include "CAchievementManager.h"
#include "CAchievementsWindow.h"
#include "CFontMgr.h"
#include "CStdUtil_Cocoa.h"
#include "ServiceWindows.h"

const unsigned int CPopupController::POPUP_TIME_INTERVAL_MS[PositionCount] = {500, 1500, 500, 500, 7000};
const int CPopupController::POPUP_STACK_MAX_SIZE[PositionCount] = {8, 8, 8, 8, 25};
typedef char CKPopupControllerSize[sizeof(CPopupController) == 0x10c ? 1 : -1];

CPopupController::CPopupController() : WindowTransparent(false), m_enemiesLeft(NULL)
{
    for (int index = 0; index < PositionCount; ++index) {
        m_lastPopupMS[index] = 0;
        m_queues[index].ensureCapacity(POPUP_STACK_MAX_SIZE[index]);
    }
    ClearFlags(FLAG_FOCUSABLE);
}

CPopupController::~CPopupController()
{
    for (int position = 0; position < PositionCount; ++position) {
        for (int index = 0; index < m_queues[position].size(); ++index) {
            Window* popup = m_queues[position][index];
            popup->CloseChilds();
            popup->HandleUpdate(true);
            popup->HandleUpdate(true);
            delete popup;
        }
    }
}

void CPopupController::OnCommand(Event& event)
{
    switch (event.m_id) {
    case -0x628818b5:
        AddPopup(NULL, ResString("IDS_POPUP_BOSS_ESCAPING"), RightCenter, Announcement, 15);
        break;
    case -0x2b9c8b2f:
        AddPopup(NULL, ResString("IDS_POPUP_BOSS_ARRIVED"), RightCenter, Announcement, 15);
        break;
    case -0x1c8ca178:
        AddPopup(NULL, ResString("IDS_POPUP_HEADSHOT"), Center, Brief, 9);
        break;
    case 0x6aa69fd2:
        AddPopup(NULL, ResString("IDS_POPUP_LEVELUP"), Center, Brief, 9);
        break;
    case -0x53245bd8: {
        XString text = ResString("IDS_POPUP_XP_MULTIPLIER");
        AddPopup(NULL, XString::Format(text.c_str(), event.m_param1), RightCenter, Announcement, 15);
        break;
    }
    case -0x5220a649: {
        XString text;
        if (event.m_param1 < 1) {
            text = ResString("IDS_POPUP_CLEAR");
        } else if (event.m_param1 == 1) {
            text = XString::Format(ResString("IDS_POPUP_ENEMY_LEFT").c_str(), 1);
        } else {
            text = XString::Format(ResString("IDS_POPUP_ENEMIES_LEFT").c_str(), event.m_param1);
        }
        if (m_enemiesLeft == NULL) {
            m_enemiesLeft = AddPopup(NULL, text, RightCenter, Announcement, 15);
        } else {
            static_cast<LabelWithIcon*>(m_enemiesLeft->m_firstChild)->SetTextWithSameFont(text);
        }
        break;
    }
    case 0x48a1ff19:
        AddPopup("IDB_HUD_ICON_HEALTH", event.m_param1 / 1000, BottomLeft, Recovery, 15);
        break;
    case -0x2e9b0c33:
    case -0x7c44c75e:
        break;
    default:
        return;
    }
    event.Clear();
}

Window* CPopupController::AddPopup(const char* icon, XString text, EPosition position, EAnimationStyle style, int font)
{
    if (m_queues[position].size() >= POPUP_STACK_MAX_SIZE[position]) {
        return NULL;
    }
    AnimatedWindow* popup = new AnimatedWindow(static_cast<AnimatedWindow::EMode>(2));
    popup->AddToFront(new LabelWithIcon(icon, text, false, CFontMgr::GetInstance()->GetFont(font), 0));
    SetupStyle(popup, style);
    SetupPosition(popup, position);
    AddPopupInQueue(popup, position);
    return popup;
}

Window* CPopupController::AddPopup(const char* icon, int value, EPosition position, EAnimationStyle style, int font)
{
    return AddPopup(icon, XString::Format(L"+%d", value), position, style, font);
}

void CPopupController::AddAchievementPopup(const CAchievement* achievement)
{
    AnimatedWindow* popup = new AnimatedWindow(static_cast<AnimatedWindow::EMode>(2));
    popup->AddToFront(new AchievementReceivedWindow(achievement->m_progress, achievement->m_steps,
        achievement->m_screenName, achievement->m_description, achievement->m_completedDescription,
        achievement->m_imageId));
    SetupStyle(popup, Award);
    SetupPosition(popup, Achievement);
    AddPopupInQueue(popup, Achievement);
}

void CPopupController::AddTrickShotPopup(const XString& name, const XString& points)
{
    AddPopup(NULL, XString::Format(L"%ls %ls", name.c_str(), points.c_str()), Center, Brief, 9);
}

void CPopupController::SetupStyle(AnimatedWindow* window, EAnimationStyle style)
{
    int scale = App::Scale();
    EasyLib::EMethod linear = static_cast<EasyLib::EMethod>(0);
    switch (style) {
    case Announcement:
        window->SetCloseEasy(0, -50 * scale, 1.5f, 1.0f, 0.0f, 1.5f, linear);
        break;
    case Recovery:
        window->SetStartEasy(0, 20 * scale, 0.5f, 0.0f, 1.0f, 0.0f, linear);
        window->SetCloseEasy(0, -40 * scale, 0.5f, 1.0f, 0.0f, 0.5f, linear);
        break;
    case Brief:
        window->SetStartEasy(0, 0, 0.55f, 0.0f, 0.7f, 0.0f, linear);
        window->SetCloseEasy(0, -70 * scale, 0.3f, 0.7f, 0.0f, 0.05f, linear);
        break;
    case Award:
        window->SetStartEasy(0, 70 * scale, 0.5f, 0.0f, 1.0f, 0.0f, static_cast<EasyLib::EMethod>(26));
        window->SetCloseEasy(0, 0, 2.0f, 1.0f, 0.0f, 4.0f, linear);
        break;
    }
}

void CPopupController::SetupPosition(AnimatedWindow* window, EPosition position)
{
    int scale = App::Scale();
    window->SetLayoutType(LAYOUT_SIMPLE);
    window->ClearFlags(FLAG_FOCUSABLE | FLAG_ENABLED);
    switch (position) {
    case TopRight:
        window->SetAlign(ALIGN_RIGHT | ALIGN_TOP);
        window->SetOutsetSpacing(45 * scale, 20 * scale, 0, 0);
        break;
    case RightCenter:
        window->SetAlign(ALIGN_RIGHT | ALIGN_VCENTER);
        window->SetOutsetSpacing(20 * scale, 20 * scale, 0, 0);
        break;
    case BottomLeft:
        window->SetAlign(ALIGN_LEFT | ALIGN_BOTTOM);
        window->SetOutsetSpacing(0, 0, 50 * scale, 60 * scale);
        break;
    case Center:
        window->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
        window->SetOutsetSpacing(0, 0, 20 * scale, 0);
        break;
    case Achievement:
        window->SetAlign(ALIGN_HCENTER | ALIGN_BOTTOM);
        window->SetOutsetSpacing(0, 0, -35 * scale, 0);
        break;
    default:
        break;
    }
}

void CPopupController::AddPopupInQueue(Window* window, EPosition position)
{
    Vector<Window*>& queue = m_queues[position];
    if (queue.size() >= POPUP_STACK_MAX_SIZE[position]) {
        window->CloseChilds();
        window->HandleUpdate(true);
        window->HandleUpdate(true);
        delete window;
    } else {
        queue.insertElementAt(window, 0);
    }
}

void CPopupController::Update()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    unsigned int now = CStdUtil_Cocoa::GetUpTimeMS();
    for (int position = 0; position < PositionCount; ++position) {
        Vector<Window*>& queue = m_queues[position];
        if (queue.size() > 0 && now - m_lastPopupMS[position] > POPUP_TIME_INTERVAL_MS[position]) {
            int index = queue.size() - 1;
            Window* popup = queue[index];
            queue.removeElementAt(index);
            if (popup == m_enemiesLeft) {
                m_enemiesLeft = NULL;
            }
            app->m_topLayer->AddToFront(popup);
            m_lastPopupMS[position] = now;
        }
    }
}
