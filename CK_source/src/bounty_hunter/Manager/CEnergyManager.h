#ifndef CENERGYMANAGER_H
#define CENERGYMANAGER_H

#include "CShopItem.h"
#include "Vector_ss.h"

class CEnergy;

class CEnergyManager : public CItemManager {
public:
    CEnergyManager();
    ~CEnergyManager();
    virtual int getItemsCount();
    virtual CItem* getItemByInternalId(int index);
    virtual void loadResources();
    virtual void freeResources();
    CEnergy* getEnergyItem(int id);

private:
    Vector<CEnergy*> m_items;
};

#endif
