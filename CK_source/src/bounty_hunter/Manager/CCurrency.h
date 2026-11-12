// CCurrency.h - one cash pack of currency.xml (bought with credits, adds cash).
#ifndef CCURRENCY_H
#define CCURRENCY_H

#include "CShopItem.h"

class CCurrency : public CItem {
public:
    CCurrency();

    virtual bool isAvailable() const { return false; }
    virtual void setAvailable(bool available);
    virtual int getType() { return 7; }
    virtual void ParseTexts(const TiXmlElement& element);
    virtual void ParseStats(const TiXmlElement& element);
    virtual XString getUIDString() const;

    int m_cashGain;                        // 0x5c
};

#endif
