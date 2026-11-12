// CDailyDealManager.h - daily deal rotation and item pricing.
#ifndef CDAILYDEALMANAGER_H
#define CDAILYDEALMANAGER_H

#include "Vector_ss.h"
#include "xString.h"

class CItem;

struct SDailyDeal {
    int m_priceCredits;
    int m_itemId;
    XString m_itemType;
    XString m_text;
};

class CDailyDealManager {
public:
    CDailyDealManager();
    ~CDailyDealManager();

    void ParseConfig();
    void Load();
    void Save();
    SDailyDeal* GetCurrentDailyDeal();
    void CheckForDealFinishByTime();
    CItem* GetItemFromDeal(int index);
    void NextDeal();
    CItem* GetCurrentDealItem();
    void ItemPurchased(CItem* item);
    unsigned int GetNextDealStartTime();

private:
    int m_currentIndex;
    unsigned int m_startTime;
    Vector<SDailyDeal*> m_deals;
};

#endif
