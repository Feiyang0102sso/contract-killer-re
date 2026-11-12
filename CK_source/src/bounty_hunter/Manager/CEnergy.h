#ifndef CENERGY_H
#define CENERGY_H

#include "CShopItem.h"

class CEnergy : public CItem {
public:
    CEnergy();
    virtual void setAvailable(bool available);
    virtual int getType() { return 4; }
    virtual void ParseStats(const TiXmlElement& stats);
    virtual XString getUIDString() const;
    virtual bool isAvailable() const { return false; }

    int m_energy; // 0x5c
};

#endif
