#ifndef CGPSNETWORK_H
#define CGPSNETWORK_H

#include "CGPSItem.h"
#include "xString.h"

class CFont;
class CGPSNetwork;

class CGPSNetworkListener {
public:
    virtual void OnGPSNetworkSelected(CGPSNetwork* network) = 0;
};

class CGPSNetwork : public CGPSItem {
public:
    CGPSNetwork();
    virtual ~CGPSNetwork();
    virtual void SetBlinking(bool enabled);
    virtual void Update(long elapsed);
    virtual void Save(TiXmlElement* parent);
    virtual void Load(TiXmlElement* parent);
    virtual void OnClick();
    virtual void Paint(ICGraphics2d& graphics, int mapX, int mapY);
    void SetListener(CGPSNetworkListener* listener);

private:
    CGPSNetworkListener* m_listener; // 0x48
    CFont* m_font;
    int m_labelWidth;
    XString m_label;
};

#endif
