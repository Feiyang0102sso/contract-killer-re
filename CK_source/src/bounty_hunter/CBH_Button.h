// CBH_Button.h - base button of the game UI: fires its command when a press is released
// near the press position. Owner: src/bounty_hunter/CBH_Button.cpp (game2).
// CK layout from CBH_Button::CBH_Button() 0xe68b8 and vtable 0x33ca00 (no IsLocalPos/OnFire
// slots and no toggle mode, unlike CKZ); CImageButton fields start at 0xd4.
// (CKZ b074 draft: button ABI from 0x1151b0 and vtable 0x4dd050; Window owns its children.)
#ifndef CBH_BUTTON_H
#define CBH_BUTTON_H
#include "WindowBase.h"

class CBH_Button : public Window {
public:
    CBH_Button();
    virtual ~CBH_Button() {}
    virtual void OnPointerEvent(Event& event);
    /** Set the release command and its parameters (one object pointer for all four stores). */
    void SetCommandParameters(int command, int parameter1, int parameter2, int parameter3)
    {
        m_command = command;
        m_commandParameter1 = parameter1;
        m_commandParameter2 = parameter2;
        m_commandParameter3 = parameter3;
    }

    bool m_expandDragArea;        // a4 drag/release hit area is twice the window size
    int m_unresolvedA8[2];        // a8..ac, not initialized by the original ctor
    int m_fixedWidth;             // b0 written by CBountyResultScreen::AddButton before SetSize
    int m_unresolvedB4;           // b4
    int m_command;                // b8 tunnel command fired on release (0: none)
    int m_commandParameter1;      // bc
    int m_commandParameter2;      // c0
    int m_commandParameter3;      // c4
    bool m_pressed;               // c8
    int m_pressGlobalX;           // cc
    int m_pressGlobalY;           // d0 (original intentionally uses LocalToGlobalX)
};
#endif
