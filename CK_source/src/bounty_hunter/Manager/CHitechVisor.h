#ifndef CHITECHVISOR_H
#define CHITECHVISOR_H

#include "CShopItem.h"

class CHitechVisor : public CItem {
public:
    explicit CHitechVisor(const TiXmlElement& config);
    virtual bool isAvailable() const { return m_owned; }
    virtual void setAvailable(bool available);
    virtual int getType() { return 12; }
    virtual XString getUIDString() const;
    void Load();
    void Save();

private:
    bool m_owned; // 0x5b
};

#endif
