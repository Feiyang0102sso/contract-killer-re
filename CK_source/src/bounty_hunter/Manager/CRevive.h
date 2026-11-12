#ifndef CREVIVE_H
#define CREVIVE_H

#include "CShopItem.h"

class CRevive : public CItem {
public:
    explicit CRevive(const TiXmlElement& config);
    virtual void setAvailable(bool available);
    virtual int getType() { return 12; }
    virtual void ParseStats(const TiXmlElement& stats);
    virtual XString getUIDString() const;
    virtual XString GetStatsString() const;
    virtual int getCurrentAmount();

    int m_count; // 0x5c
};

#endif
