#ifndef CMEDICANT_H
#define CMEDICANT_H

#include "CShopItem.h"

class CMedicant : public CItem {
public:
    CMedicant();
    virtual void setAvailable(bool available);
    virtual int getType() { return 3; }
    virtual void ParseStats(const TiXmlElement& stats);
    virtual XString getUIDString() const;
    virtual XString GetStatsString() const;
    virtual bool isAvailable() const { return false; }

    int m_medkits; // 0x5c
};

#endif
