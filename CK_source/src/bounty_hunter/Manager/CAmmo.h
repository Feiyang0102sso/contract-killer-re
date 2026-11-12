// CAmmo.h - the original shop ammunition configuration.
#ifndef CAMMO_H
#define CAMMO_H

#include "CShopItem.h"

class CAmmo : public CItem {
public:
    CAmmo();

    virtual void init(const TiXmlElement& element);
    virtual bool isAvailable() const { return false; }
    virtual void setAvailable(bool available);
    virtual int getType() { return 2; }
    virtual XString GetStatsString() const;
    virtual void Parse(const TiXmlElement& element);
    virtual void ParseStats(const TiXmlElement& element);
    virtual void ParseGraphics(const TiXmlElement& element);
    virtual XString getUIDString() const;

    bool m_owned;           // 0x5b
    int m_extraDamage;      // 0x5c
    int m_countInPack;      // 0x60
    bool m_universal;       // 0x64
    XString m_ammoType;     // 0x68
    XString m_equipButton;  // 0x6c
    XString m_bulletIcon;   // 0x70
};

#endif
