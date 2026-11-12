#ifndef CSOCIALGIFTSMANAGER_H
#define CSOCIALGIFTSMANAGER_H

#include "Vector_ss.h"
#include "xString.h"

class CItem;
class CItemManager;

struct SGift {
    CItem* GetItem() const;
    int m_itemId;
    bool m_hidden;
    CItemManager* m_itemManager;
};

class CSocialGiftManager {
public:
    CSocialGiftManager();
    ~CSocialGiftManager();
    int GetGiftsCount();
    SGift* GetGift(int index);
    void GiveGift(int index);

private:
    void ParseConfig();
    CItemManager* GetItemManager(XString& type);
    Vector<SGift*> m_gifts;
};

#endif
