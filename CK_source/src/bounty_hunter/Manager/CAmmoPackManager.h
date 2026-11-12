#ifndef CAMMOPACKMANAGER_H
#define CAMMOPACKMANAGER_H

#include "CShopItem.h"
#include "Vector_ss.h"

class CAmmoPack;

class CAmmoPackManager : public CItemManager {
public:
    CAmmoPackManager();
    ~CAmmoPackManager();
    virtual int getItemsCount() { return m_packs.size(); }
    virtual int getAmount(int id);
    virtual void loadResources();
    virtual void freeResources();
    virtual CItem* getItemByInternalId(int index);
    Vector<CAmmoPack*> getAmmoPacksByType(const XString& type);

private:
    void Parse(const TiXmlElement& root);
    Vector<CAmmoPack*> m_packs;
};

#endif
