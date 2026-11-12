// CAmmoManager.h - load ammunition and preserve its XML order in the shop.
#ifndef CAMMOMANAGER_H
#define CAMMOMANAGER_H

#include "CShopItem.h"
#include "HashTable.h"
#include "Vector_ss.h"

class CAmmo;

class CAmmoManager : public CItemManager {
public:
    CAmmoManager();
    ~CAmmoManager();

    virtual int getItemsCount() { return m_ammo.m_count; }
    virtual int getAmount(int id);
    virtual CItem* getItemByInternalId(int index);
    virtual void loadResources();
    virtual void freeResources();

    CAmmo* GetAmmoByType(const XString& type);
    bool IsAllAmmoTypesOpen();

private:
    HashTable<XString, CAmmo*> m_ammo; // 0x08
    Vector<XString> m_types;          // 0x14
};

#endif
