// CGPSZombiesPromo.h - map marker for the Zombies promotion.
#ifndef CGPSZOMBIESPROMO_H
#define CGPSZOMBIESPROMO_H

#include "CGPSItem.h"

class CGPSZombiesPromo;

class CGPSZombiesPromoListener {
public:
    virtual void OnZombiesItemSelected(CGPSZombiesPromo* promo) = 0;
};

class CGPSZombiesPromo : public CGPSItem {
public:
    CGPSZombiesPromo();
    virtual ~CGPSZombiesPromo() {}
    virtual void OnClick();
    virtual void Update(long elapsed);
    virtual void Save(TiXmlElement* parent);
    virtual void Load(TiXmlElement* parent);
    virtual void Paint(ICGraphics2d& graphics, int mapX, int mapY);

    CGPSZombiesPromoListener* m_listener;   // 0x48
};

typedef char GPSZombiesPromoSizeCheck[sizeof(CGPSZombiesPromo) == 0x4c ? 1 : -1];

#endif
