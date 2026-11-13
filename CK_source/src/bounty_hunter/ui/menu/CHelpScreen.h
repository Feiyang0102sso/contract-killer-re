// CHelpScreen.h - the in-game manual and public build information.
#ifndef CHELPSCREEN_H
#define CHELPSCREEN_H

#include "CBaseScreen.h"

class CTabWindow;

/** Delete each non-null pointer of `items` and clear it. */
template <class T>
void SAFE_DELETE_ARRAY_OBJS(T* items, int count)
{
    for (int index = 0; index < count; ++index) {
        if (items[index] != NULL) {
            delete items[index];
            items[index] = NULL;
        }
    }
}

/** Hidden gesture pad: dragging through the four quarters in order unlocks the about text. */
class CUnlockWindow : public Window {
public:
    /** One quarter of the window. */
    class CFingerZone {
    public:
        struct SRect {
            SRect(short x, short y, short width, short height)
            {
                m_x = x;
                m_y = y;
                m_width = width;
                m_height = height;
            }
            short m_x;
            short m_y;
            short m_width;
            short m_height;
        };

        CFingerZone(short x, short y, short width, short height)
        {
            m_passed = false;
            m_rect = new SRect(x, y, width, height);
        }
        ~CFingerZone()
        {
            if (m_rect != NULL) {
                delete m_rect;
                m_rect = NULL;
            }
        }
        bool ContainsPoint(int x, int y);

        SRect* m_rect;      // 0x00
        bool m_passed;      // 0x04
    };

    CUnlockWindow();
    virtual ~CUnlockWindow();
    virtual void OnPointerPressed(Event& event);
    virtual void OnPointerDragged(Event& event);
    virtual void OnPointerReleased(Event& event);
    void InitFingerZones();

    enum { ZoneCount = 4 };
    CFingerZone* m_zones[ZoneCount];    // 0xa4
    bool m_pressed;                     // 0xb4
    bool m_unlocked;                    // 0xb5
    int m_nextZone;                     // 0xb8, total 0xbc
};

class CHelpScreen : public CBaseScreen {
public:
    CHelpScreen();
    virtual ~CHelpScreen();
    virtual void Paint(ICGraphics2d& graphics);
    virtual void Update();

private:
    void AddTab(XString title, XString text, TextWindow** output);

    XString m_about;
    ImageRes m_background;
    TextWindow* m_aboutText;    // 0xe4
    CTabWindow* m_tabs;
    CUnlockWindow* m_unlock;    // 0xec
    bool m_aboutPending;        // 0xf0
    unsigned char m_reservedF1[3];
};

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char HelpScreenSizeCheck[sizeof(CHelpScreen) == 0xf4 ? 1 : -1];
#endif

#endif
