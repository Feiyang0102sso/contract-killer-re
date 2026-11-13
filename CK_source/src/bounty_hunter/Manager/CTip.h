// CTip.h - XML loading-screen tip; layout from CTip/CTipManager (b079).
#ifndef CTIP_H
#define CTIP_H

#include "xString.h"

class TiXmlNode;

class CTip {
public:
    explicit CTip(TiXmlNode* node);
    void Parse(TiXmlNode* node);

    XString m_description;       // 0x00 localized "desc" attribute
    const char* m_picture;       // 0x04 resource id of the "picture" attribute
};

#endif
