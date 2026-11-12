#ifndef CMISCITEMMANAGER_H
#define CMISCITEMMANAGER_H

#include "CShopItem.h"
#include "Vector_ss.h"

class CMiscItemManager : public CItemManager {
public:
    CMiscItemManager();
    ~CMiscItemManager();
    virtual int getItemsCount();
    virtual CItem* getItemByInternalId(int index);
    virtual void loadResources();
    virtual void freeResources();
    CItem* CreateItem(const XString& type, const TiXmlElement& element);

private:
    Vector<CItem*> m_items;
};

#endif
