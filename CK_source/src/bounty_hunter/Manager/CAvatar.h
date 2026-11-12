// CAvatar.h - one player avatar of res/common/configs/avatars.xml.
// Layout (0x14) from CAvatar::CAvatar, ParseNode, ParseGraphics, ParseStats and CAvatarManager.
#ifndef CAVATAR_H
#define CAVATAR_H

class TiXmlNode;
class TiXmlElement;

class CAvatar {
public:
    CAvatar(TiXmlNode* node);
    ~CAvatar();

    /** Read the id attribute and the <stats> / <graphics> children. */
    void ParseNode(TiXmlNode* node);
    void ParseGraphics(TiXmlElement* element);
    void ParseStats(TiXmlElement* element);

    int m_unknown00;                // 0x00 never written by the original unit
    const char* m_imageResId;       // 0x04 resource of the "image" attribute
    bool m_isNew;                   // 0x08 cleared by the constructor (meaning unknown)
    int m_unlockLvl;                // 0x0c
    int m_id;                       // 0x10
};

#endif
