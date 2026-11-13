// CTip.cpp - localizes the description carried by an XML tip node.
#include "CTip.h"
#include "CXmlHelper.h"
#include "WindowBase.h"
#include "App.h"
#include "ResMap.h"

CTip::CTip(TiXmlNode* node)
{
    m_picture = NULL;
    Parse(node);
}

void CTip::Parse(TiXmlNode* node)
{
    if (node != NULL) {
        m_description = CXmlHelper::GetResString(node, "desc");
        m_picture = static_cast<App*>(WindowApp::m_instance)->InitResMap()->getResourceID(
            CXmlHelper::GetAttributeValue(node, "picture"));
    }
}
