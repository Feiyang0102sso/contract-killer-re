// CIAPCurrencyManager.h - available currency packs and StoreKit product queries.
#ifndef CIAPCURRENCYMANAGER_H
#define CIAPCURRENCYMANAGER_H

#include "CIAPCurrency.h"
#include "Vector_ss.h"

class CIAPCurrencyManager : public CItemManager {
public:
    CIAPCurrencyManager();
    ~CIAPCurrencyManager();

    virtual int getItemsCount();
    virtual void loadResources();
    virtual void freeResources();
    virtual CItem* getItemByInternalId(int index);

    CIAPCurrency* getIAPCurrencyByAppleId(XString appleId) const;
    int GetAvailableItemsCount();
    void SetAllUnavailable();
    void UpdateIAPInfo(bool resetRetries);
    bool IsRetryWaiting();

private:
    Vector<CIAPCurrency*> m_items;
    int m_retries;
};

#endif
