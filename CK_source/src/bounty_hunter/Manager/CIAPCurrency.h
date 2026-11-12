// CIAPCurrency.h - one currency pack listed by CK's in-app purchase configuration.
#ifndef CIAPCURRENCY_H
#define CIAPCURRENCY_H

#include "CShopItem.h"

class CIAPCurrency : public CItem {
public:
    CIAPCurrency();

    virtual void init(const TiXmlElement& element);
    virtual void Parse(const TiXmlElement& element);
    virtual void ParseTexts(const TiXmlElement& element);
    virtual void ParseStats(const TiXmlElement& element);
    virtual XString getUIDString() const;
    virtual void setAvailable(bool available);
    virtual bool isAvailable() const { return m_available; }
    virtual int getType() { return 5; }

    void Buy();

    bool m_available;                     // 0x5b
    int m_creditsGain;                     // 0x5c
    int m_cashGain;                        // 0x60
    XString m_appleId;                     // 0x64
    XString m_type;                        // 0x68
};

#endif
