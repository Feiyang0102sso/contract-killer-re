// CEquipWeaponWindow.h - preview an owned weapon in either slot.
#ifndef CEQUIPWEAPONWINDOW_H
#define CEQUIPWEAPONWINDOW_H

#include "CBloodWindow.h"

class CEquipWeaponWindow : public CBloodWindow {
public:
    explicit CEquipWeaponWindow(int weaponId);
    virtual ~CEquipWeaponWindow() {}
    virtual void Paint(ICGraphics2d& graphics);
    virtual void OnCommand(Event& event);
    virtual void OnClosed();

private:
    Window* CreateTextWindow(const char* resource);
    Window* CreateSlotButton(int slot, int command);

    int m_originalFirst;
    int m_originalSecond;
    int m_weaponId;
};

typedef char EquipWeaponWindowSizeCheck[sizeof(CEquipWeaponWindow) == 0x12c ? 1 : -1];

#endif
