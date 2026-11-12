#ifndef CCYBERSCOPE_H
#define CCYBERSCOPE_H

#include "CShopItem.h"

class CCyberScope : public CItem {
public:
    explicit CCyberScope(const TiXmlElement& config);
    virtual void setAvailable(bool available);
    virtual int getType() { return 12; }
    virtual void ParseStats(const TiXmlElement& stats);
    virtual XString getUIDString() const;
    virtual int getCurrentAmount();

    int m_count; // 0x5c
};

#endif
