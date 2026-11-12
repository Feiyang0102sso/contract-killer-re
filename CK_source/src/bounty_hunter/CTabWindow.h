// CTabWindow.h - window with a row of tabs; each tab shows one child window.
// CK layout from CTabWindow::CTabWindow(bool) (0x1059e0) and ~CTabWindow (0x106980), size 0x128.
#ifndef CTABWINDOW_H
#define CTABWINDOW_H

#include "WindowBase.h"

class CBH_ProgressBar;
class CCountNotificationWindow;

class CTabWindow : public Window {
public:
    /** One tab: caption, page window and caption rectangle (0x20 bytes). */
    struct TTab {
        /** Caption rectangle, relative to the tab window's parent. */
        struct Bounds {
            Bounds()
            {
                m_width = 0;
                m_height = 0;
                m_x = 0;
                m_y = 0;
            }

            short m_x;              // 0x0c
            short m_y;              // 0x0e
            short m_width;          // 0x10
            short m_height;         // 0x12
        };

        TTab()
            : m_window(NULL), m_notification(NULL)
        {
        }

        ~TTab();

        XString m_name;                             // 0x00
        Window* m_window;                           // 0x04 page, owned by the window tree
        CCountNotificationWindow* m_notification;   // 0x08 counter badge (NULL: none)
        Bounds m_bounds;                            // 0x0c
        ImageRes m_image;                           // 0x14 optional caption image
        bool m_locked;                              // 0x1c
    };

    explicit CTabWindow(bool showInfo);
    virtual ~CTabWindow();
    virtual void OnGetFocus();
    virtual void OnPointerEvent(Event& event);
    virtual void Paint(ICGraphics2d& graphics);

    void CreateInfoTab();
    void AddTab(XString name, Window* window, int padding);
    void AddTab(XString name, Window* window, const char* image, bool locked, int padding);
    int GetTabCount() const;
    int GetCaptionHeight();
    int GetTabWidth();
    void SetCurrTab(int index, bool fromPointer);
    void PaintInactiveTab(ICGraphics2d& graphics, TTab* tab);
    void PaintActiveTab(ICGraphics2d& graphics, TTab* tab);
    void DrawInfoTab(ICGraphics2d& graphics);
    void SetTabLock(int index, bool locked);
    void UpdateInfoTab();
    void SetTabsWidthByWindowWidth();
    int GetMaxTabCaptionWidth();
    void CreateCountNotification(int index);
    void SetCountToNotification(int index, int count);

    Vector<TTab*> m_tabs;               // 0xa4
    TTab* m_infoTab;                    // 0xb4 player info area left of the tabs
    bool m_showInfo;                    // 0xb8
    Window* m_infoWindow;               // 0xbc
    CBH_ProgressBar* m_moneyBar;        // 0xc0
    CBH_ProgressBar* m_creditsBar;      // 0xc4
    ImageRes m_background;              // 0xc8
    ImageRes m_leftOff;                 // 0xd0
    ImageRes m_middleOff;               // 0xd8
    ImageRes m_rightOff;                // 0xe0
    ImageRes m_leftOn;                  // 0xe8
    ImageRes m_middleOn;                // 0xf0
    ImageRes m_rightOn;                 // 0xf8
    ImageRes m_middleOnHigh;            // 0x100
    ImageRes m_rightOnHigh;             // 0x108
    ImageRes m_leftOnHigh;              // 0x110
    ImageRes m_border;                  // 0x118
    int m_currentTab;                   // 0x120 -1: none
    CFont* m_font;                      // 0x124
};

#endif
