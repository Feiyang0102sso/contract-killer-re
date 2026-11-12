#ifndef CARMORMANAGER_H
#define CARMORMANAGER_H

#include "CShopItem.h"
#include "Vector_ss.h"

class CArmor;

class CArmorManager : public CItemManager {
public:
    CArmorManager();
    ~CArmorManager();
    virtual int getItemsCount();
    virtual CItem* getItemByInternalId(int index);
    virtual void loadResources();
    virtual void freeResources();
    CArmor* GetArmorById(int id);

private:
    Vector<CArmor*> m_items;
};

#endif
