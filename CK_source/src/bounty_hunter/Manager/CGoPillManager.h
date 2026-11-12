#ifndef CGOPILLMANAGER_H
#define CGOPILLMANAGER_H

#include "CShopItem.h"
#include "Vector_ss.h"

class CGoPill;

class CGoPillManager : public CItemManager {
public:
    CGoPillManager();
    ~CGoPillManager();
    virtual int getItemsCount();
    virtual CItem* getItemByInternalId(int index);
    virtual void loadResources();
    virtual void freeResources();
    CGoPill* GetPillById(int id);

private:
    Vector<CGoPill*> m_items;
};

#endif
