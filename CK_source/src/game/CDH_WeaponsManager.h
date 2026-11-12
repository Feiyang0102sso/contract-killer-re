// CDH_WeaponsManager.h - the weapons of the shop, read from res/common/configs/weapons.xml.
// (b055x: replaces the PROVISIONAL of b057x / b048; their names stay as inline aliases.)
#ifndef CDH_WEAPONSMANAGER_H
#define CDH_WEAPONSMANAGER_H

#include <stddef.h>
#include "xString.h"
#include "Vector_ss.h"
#include "CShopItem.h"

class CShopItem;
class CDH_Weapon;
class TiXmlNode;

/** Manager of the CDH_Weapon items. No virtual destructor (App deletes it directly). */
class CDH_WeaponsManager : public CItemManager {
public:
    CDH_WeaponsManager();
    ~CDH_WeaponsManager();

    virtual int getItemsCount()                                     // 0x00
    {
        return m_weapons.size();
    }
    virtual void loadResources();                                  // 0x18
    virtual void freeResources();                                  // 0x1c
    virtual CItem* getItemByInternalId(int index);                  // 0x20

    /** Reload weapons.xml when it changed. */
    void ReloadConfig();
    /** Save and delete every weapon. */
    void CleanupAll();
    /** releaseResources() of every weapon. */
    void releaseResource();
    /** Release the 3D model of weapon `id`. */
    void release3dResources(int id);
    /** Ids of the weapons (not hidden) unlocked at `level`; returns their count. */
    int GetUnlockedWeaponsIdOnLevel(int level, Vector<int>& ids);
    bool HasWeaponForAmmo(int ammoId);
    CDH_Weapon* ParseWeapon(int id, const XString& path);
    void ReleaseResourcesImpl();
    void resetData();
    Vector<XString> GetUnlockedWeapons(int level) const;
    void load3dResources(int id, bool forMenu);
    int GetAvailableWeapons(Vector<CDH_Weapon*>& weapons);
    int GetUnavailableWeapons(Vector<CDH_Weapon*>& weapons);

    /** Old name used by src/general/App.cpp (b057x): CItemManager::getItemByStringUId. */
    CShopItem* GetItemByStringUId(const XString& id)
    {
        return (CShopItem*)getItemByStringUId(id);
    }
    /** Old name used by src/dh3d (b048): CItemManager::getItemById (slot 0x0c). */
    CDH_Weapon* GetWeaponById(int weaponId)
    {
        return (CDH_Weapon*)getItemById(weaponId);
    }
    /** Old name used by src/general/App.cpp (b057x): loadResources (slot 0x18). */
    void Init()
    {
        loadResources();
    }
    /** The owned weapons (used by src/dh3d, b048). */
    Vector<CDH_Weapon*>& GetWeapons()
    {
        return m_weapons;
    }

    Vector<CDH_Weapon*> m_weapons;  // CK 0x08 (CKZ 0x10)
    float m_maxWander;              // 0x18
    int m_maxWanderRounded;
    int m_maxRange;
    int m_maxDamage;
    int m_maxRPM;
    int m_maxClip;
};

#endif
