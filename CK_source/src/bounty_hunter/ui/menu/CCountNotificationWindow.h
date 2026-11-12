// Originally declared while restoring CTabWindow.
// CCountNotificationWindow.h - small counter badge drawn over a tab (CTabWindow).
// Layout from CCountNotificationWindow::CCountNotificationWindow() (0x1353a4), size 0xe8.
#ifndef CCOUNTNOTIFICATIONWINDOW_H
#define CCOUNTNOTIFICATIONWINDOW_H

#include "WindowBase.h"

class CCountNotificationWindow : public Window {
public:
    CCountNotificationWindow();
    virtual ~CCountNotificationWindow();

    virtual void Paint(ICGraphics2d& graphics);

    void UpdateCount();
    void SetCount(int count);
    void SetPos(int x, int y);

    XString m_text;             // 0xa4
    ImageRes m_left;            // 0xa8
    ImageRes m_middle;          // 0xb0
    ImageRes m_right;           // 0xb8
    int m_leftWidth;            // 0xc0
    int m_middleWidth;          // 0xc4
    int m_rightWidth;           // 0xc8
    int m_barHeight;            // 0xcc
    CFont* m_font;              // 0xd0
    int m_posX;                 // 0xd4
    int m_posY;                 // 0xd8
    int m_textWidth;            // 0xdc
    int m_textHeight;           // 0xe0
    int m_count;                // 0xe4
};

typedef char CountNotificationSizeCheck[sizeof(CCountNotificationWindow) == 0xe8 ? 1 : -1];

#endif
