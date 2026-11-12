// CLocation.cpp - one location of locations.xml.
#include "CLocation.h"
#include "CXmlHelper.h"

CLocation::CLocation(TiXmlNode* node)
{
    m_spawnCivilians = true;
    Parse(node);
}

void CLocation::Parse(TiXmlNode* node)
{
    m_name = CXmlHelper::GetAttributeValue(node, "name");
    ParseInfo(node->FirstChild("info"));
    ParseModel(node->FirstChild("model"));
    ParseGraphics(node->FirstChild("graphics")->ToElement());
}

void CLocation::ParseInfo(TiXmlNode* node)
{
    if (node != NULL) {
        m_desc = CXmlHelper::GetResString(node, "desc");
        m_spawnCivilians = CXmlHelper::GetAttributeValue(node, "spawn_civilians").ToBool();
    }
}

void CLocation::ParseModel(TiXmlNode* node)
{
    if (node != NULL) {
        m_m3g = CXmlHelper::GetAttributeValue(node, "m3g");
    }
}

void CLocation::ParseGraphics(TiXmlElement* element)
{
    if (element != NULL) {
        m_cameraImagesKeyset = CXmlHelper::GetAttributeValue(element, "camera_images_keyset");
    }
}
