// CEquipItemButton.h - item icon, quantity and equipment status.
#ifndef CEQUIPITEMBUTTON_H
#define CEQUIPITEMBUTTON_H

#include "CBH_Button.h"

class CAmmo;
class CDH_Weapon;
class ImageWindow;
class LabelWithIcon;
class TextWindow;

class CEquipItemButton : public CBH_Button {
public:
    CEquipItemButton(const char* image, const XString& text, CFont* font, const char* countIcon, int count);
    virtual ~CEquipItemButton() {}
    virtual void OnPointerReleased(Event& event);
    virtual void SetClickable(bool clickable);
    virtual void SetAvailable(bool available);
    virtual void SetBottomText(const XString& text, CFont* font);
    virtual void SetItemCount(int count);

    int GetIconHeight() const;

protected:
    CEquipItemButton();
    Window* CreateUI(const char* image, const XString& text, CFont* font, const char* countIcon, int count);

private:
    bool m_clickable;
    bool m_available;
    ImageWindow* m_image;
    TextWindow* m_bottomText;
    LabelWithIcon* m_count;
};

class CEquipAmmoButton : public CEquipItemButton {
public:
    CEquipAmmoButton(const CDH_Weapon* weapon, const CAmmo* ammo);
    virtual ~CEquipAmmoButton() {}
    virtual void OnPointerReleased(Event& event);

    void UpdateStatus();

private:
    const CDH_Weapon* m_weapon;
    const CAmmo* m_ammo;
};

typedef char EquipItemButtonSizeCheck[sizeof(CEquipItemButton) == 0xe4 ? 1 : -1];
typedef char EquipAmmoButtonSizeCheck[sizeof(CEquipAmmoButton) == 0xec ? 1 : -1];

#endif
