#ifndef CBH_WEAPONBUTTON_H
#define CBH_WEAPONBUTTON_H

#include "CBH_Button.h"

class CAmmo;
class CDH_Weapon;
class CImageButton;
class ImageWindow;
class LabelWithIcon;

class CBH_WeaponButton : public CBH_Button {
public:
    CBH_WeaponButton(int type, int slot, bool showAmmo, bool blockPointer, int weaponId);
    virtual ~CBH_WeaponButton();
    virtual void ReleaseResources();
    virtual void Update();
    virtual void OnPointerEvent(Event& event);
    virtual void Paint(ICGraphics2d& graphics);
    virtual void SetRect(int x, int y, int width, int height, bool force);
    CDH_Weapon* Weapon();
    void ShowName();

private:
    friend class CBH_NetworkWindow;

    ICRenderSurface* m_background;        // 0xd4
    ICRenderSurface* m_locked;
    int m_reservedDC;
    bool m_blockPointer;                 // 0xe0
    CImageButton* m_bonusXp;
    XString m_label;
    int m_weaponId;                      // 0xec
    int m_slot;
    int m_ammoCount;
    const CAmmo* m_shownAmmo;             // 0xf8
    LabelWithIcon* m_ammo;                // 0xfc
    CDH_Weapon* m_weaponOverride;
    ImageWindow* m_backgroundImage;      // 0x104
    ImageWindow* m_weaponImage;
    bool m_reserved10C;
    bool m_showStats;
    int m_reserved110;
    LabelWithIcon* m_wander;
    LabelWithIcon* m_range;
    LabelWithIcon* m_damage;
    LabelWithIcon* m_rpm;
};

#endif
