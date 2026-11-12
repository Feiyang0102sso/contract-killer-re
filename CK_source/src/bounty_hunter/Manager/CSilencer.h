#ifndef CSILENCER_H
#define CSILENCER_H

#include "CShopItem.h"

class CSilencer : public CItem {
public:
    explicit CSilencer(const TiXmlElement& config);
    virtual void setAvailable(bool available);
    virtual int getType() { return 12; }
    virtual void ParseStats(const TiXmlElement& stats);
    virtual XString getUIDString() const;
    virtual XString GetStatsString() const;
    virtual int getCurrentAmount();

    int m_shots; // 0x5c
};

#endif
