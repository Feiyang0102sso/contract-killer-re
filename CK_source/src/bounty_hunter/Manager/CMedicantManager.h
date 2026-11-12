#ifndef CMEDICANTMANAGER_H
#define CMEDICANTMANAGER_H

#include "CShopItem.h"
#include "Vector_ss.h"

class CMedicant;

class CMedicantManager : public CItemManager {
public:
    CMedicantManager();
    ~CMedicantManager();
    virtual int getItemsCount();
    virtual int getAmount(int id);
    virtual CItem* getItemByInternalId(int index);
    virtual void loadResources();
    virtual void freeResources();

private:
    Vector<CMedicant*> m_items;
};

#endif
