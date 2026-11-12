// CAvatar.cpp - one player avatar.
#include "CAvatar.h"
#include "CXmlHelper.h"
#include "App.h"
#include "ResMap.h"

CAvatar::CAvatar(TiXmlNode* node)
{
    m_isNew = false;
    ParseNode(node);
}

void CAvatar::ParseNode(TiXmlNode* node)
{
    m_id = CXmlHelper::GetAttributeValue(node, "id").ToInt();
    ParseStats(node->FirstChild("stats")->ToElement());
    ParseGraphics(node->FirstChild("graphics")->ToElement());
}

CAvatar::~CAvatar()
{
}

void CAvatar::ParseGraphics(TiXmlElement* element)
{
    XString image = CXmlHelper::GetAttributeValue(element, "image");
    m_imageResId = ((App*)WindowApp::m_instance)->InitResMap()->getResourceID(image);
}

void CAvatar::ParseStats(TiXmlElement* element)
{
    m_unlockLvl = CXmlHelper::GetAttributeValue(element, "unlockLvl").ToInt();
}
