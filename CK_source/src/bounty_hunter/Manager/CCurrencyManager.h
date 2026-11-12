// CCurrencyManager.h - cash packs of res/common/configs/currency.xml.
#ifndef CCURRENCYMANAGER_H
#define CCURRENCYMANAGER_H

#include "CCurrency.h"
#include "Vector_ss.h"

class CCurrencyManager : public CItemManager {
public:
    CCurrencyManager();
    ~CCurrencyManager();

    virtual int getItemsCount() { return m_items.size(); }
    virtual int getAmount(int index) { return 0; }
    virtual void loadResources();
    virtual void freeResources();
    virtual CItem* getItemByInternalId(int index);
    virtual void onTabStartup() {}

    virtual CCurrency* getCurrencyById(int id) { return (CCurrency*)getItemById(id); }

    Vector<CCurrency*> m_items;            // 0x08
};

#endif
