// CShopButtonWeapon.h - a weapon card with its current equipment status.
#ifndef CSHOPBUTTONWEAPON_H
#define CSHOPBUTTONWEAPON_H

#include "CShopItemButton.h"

class CShopButtonWeapon : public CShopItemButton {
public:
    enum EWeaponState { Available = 1, Locked = 2, Equipped = 3, Owned = 4 };

    CShopButtonWeapon(const XString& name, int unlockLevel, int price, int weaponId, const char* image);
    virtual ~CShopButtonWeapon() {}
    virtual void Update();
    virtual void Paint(ICGraphics2d& graphics);

private:
    EWeaponState GetCurrState();
    void UpdateCaption(EWeaponState state);
    bool IsOwned();
    bool IsEquiped();
    bool IsLocked();

    int m_weaponId;
    TextWindow* m_status;
    EWeaponState m_state;
    bool m_tutorialBlink;
};

typedef char ShopButtonWeaponSizeCheck[sizeof(CShopButtonWeapon) == 0x13c ? 1 : -1];

#endif
