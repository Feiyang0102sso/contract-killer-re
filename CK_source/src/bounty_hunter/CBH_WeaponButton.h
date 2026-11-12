#ifndef CBH_WEAPONBUTTON_H
#define CBH_WEAPONBUTTON_H

#include "CBH_Button.h"

class CDH_Weapon;
class ImageWindow;
class TextWindow;

class CBH_WeaponButton : public CBH_Button {
public:
    CBH_WeaponButton(int type, int slot, bool showAmmo, bool blink, int weaponId);
    virtual ~CBH_WeaponButton();
    virtual void ReleaseResources();
    virtual void Update();
    virtual void OnPointerEvent(Event& event);
    virtual void Paint(ICGraphics2d& graphics);
    virtual void SetRect(int x, int y, int width, int height, bool force);
    CDH_Weapon* Weapon();
    void ShowName();
    void SetBlinking(bool enabled);

private:
    ICRenderSurface* m_background;        // 0xd4
    ICRenderSurface* m_locked;
    int m_reservedDC;
    bool m_blinking;                     // 0xe0
    bool m_showAmmo;
    int m_type;
    int m_scale;
    int m_weaponId;                      // 0xec
    int m_slot;
    int m_ammoCount;
    TextWindow* m_name;
    TextWindow* m_ammo;
    ImageWindow* m_lockImage;
    ImageWindow* m_backgroundImage;       // 0x104
    ImageWindow* m_weaponImage;
    CDH_Weapon* m_shownWeapon;
    unsigned char m_reserved110[0x14];
};

#endif
