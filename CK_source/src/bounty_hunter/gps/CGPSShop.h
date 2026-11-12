#ifndef CGPSSHOP_H
#define CGPSSHOP_H

#include "CGPSItem.h"
#include "xString.h"

class CFont;
class CGPSShop;

class CGPSShopListener {
public:
    virtual void OnShopItemSelected(CGPSShop* shop) = 0;
};

class CGPSShop : public CGPSItem {
public:
    CGPSShop();
    virtual ~CGPSShop();
    virtual void Update(long elapsed);
    virtual void Save(TiXmlElement* parent);
    virtual void Load(TiXmlElement* parent);
    virtual void OnClick();
    virtual void Paint(ICGraphics2d& graphics, int mapX, int mapY);
    void SetListener(CGPSShopListener* listener);
    virtual void SetBlinking(bool blinking);

private:
    CGPSShopListener* m_listener;
    CFont* m_font;
    int m_textWidth;
    XString m_label;
};

#endif
