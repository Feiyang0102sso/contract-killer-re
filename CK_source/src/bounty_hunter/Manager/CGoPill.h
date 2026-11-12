#ifndef CGOPILL_H
#define CGOPILL_H

#include "CShopItem.h"

class CGoPill : public CItem {
public:
    CGoPill();
    virtual void setAvailable(bool available);
    virtual int getType() ;
    virtual void ParseStats(const TiXmlElement& stats);
    virtual XString getUIDString() const;
    virtual void ParseGraphics(const TiXmlElement& graphics);
    virtual XString GetStatsString() const;

    int m_pills; // 0x5c
    int m_seconds; // 0x60
};

#endif
