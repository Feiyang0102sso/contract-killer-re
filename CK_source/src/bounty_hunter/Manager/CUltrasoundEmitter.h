#ifndef CULTRASOUNDEMITTER_H
#define CULTRASOUNDEMITTER_H

#include "CShopItem.h"

class CUltrasoundEmitter : public CItem {
public:
    explicit CUltrasoundEmitter(const TiXmlElement& config);
    virtual void setAvailable(bool available);
    virtual int getType() { return 12; }
    virtual void ParseStats(const TiXmlElement& stats);
    virtual XString getUIDString() const;
    virtual bool isAvailable() const { return false; }
    virtual int getCurrentAmount();

    int m_count; // 0x5c
};

#endif
