#ifndef CAMMOPACK_H
#define CAMMOPACK_H

#include "CShopItem.h"

class CAmmoPack : public CItem {
public:
    CAmmoPack();
    virtual bool isAvailable() const { return false; }
    virtual void setAvailable(bool available);
    virtual int getType() { return 9; }
    virtual XString GetStatsString() const;
    virtual void Parse(const TiXmlElement& element);
    virtual void ParseTexts(const TiXmlElement& element);
    virtual void ParseStats(const TiXmlElement& element);
    virtual XString getUIDString() const;

    void InitStatsStr();

    XString m_ammoType;
    XString m_stats;
    int m_countInPack;
};

#endif
