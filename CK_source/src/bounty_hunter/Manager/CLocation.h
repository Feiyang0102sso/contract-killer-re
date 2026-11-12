// CLocation.h - location resources, camera constraints and named scene sections.
// game1: replaced the CKZ layout by the CK 1.5.2 one (0x14) from CLocation::CLocation and the
// four Parse* methods; CKZ copy of the old header kept out of the tree (ambient, sectors, parts are CKZ-only).
#ifndef CLOCATION_H
#define CLOCATION_H
#include "xString.h"
class TiXmlNode;
class TiXmlElement;

class CLocation {
public:
    CLocation(TiXmlNode* node);
    void Parse(TiXmlNode* node);
    /** <info desc spawn_civilians>. */
    void ParseInfo(TiXmlNode* node);
    /** <model m3g>. */
    void ParseModel(TiXmlNode* node);
    /** <graphics camera_images_keyset>. */
    void ParseGraphics(TiXmlElement* element);

    XString m_name;                  // 0x00
    XString m_desc;                  // 0x04 localized description
    XString m_m3g;                   // 0x08 scene model
    XString m_cameraImagesKeyset;    // 0x0c
    bool m_spawnCivilians;           // 0x10
};
#endif
