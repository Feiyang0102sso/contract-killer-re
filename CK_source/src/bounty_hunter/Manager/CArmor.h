#ifndef CARMOR_H
#define CARMOR_H

#include "CShopItem.h"

class CArmor : public CItem {
public:
    CArmor();
    virtual void setAvailable(bool available);
    virtual int getType() ;
    virtual void ParseStats(const TiXmlElement& stats);
    virtual XString getUIDString() const;
    virtual void ParseGraphics(const TiXmlElement& graphics);
    virtual XString GetStatsString() const;

    int m_damageProof; // 0x5c
};

#endif
