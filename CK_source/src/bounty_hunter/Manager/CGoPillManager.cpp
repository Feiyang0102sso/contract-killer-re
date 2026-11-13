#include "CGoPillManager.h"

#include "App.h"
#include "CGoPill.h"
#include "ResMap.h"
#include "tinyxml.h"

CGoPillManager::CGoPillManager()
{
    ResMap* resources = static_cast<App*>(WindowApp::m_instance)->InitResMap();
    char* text = resources->loadText(XString(L"res/common/configs/go_pills.xml"));
    TiXmlDocument config;
    config.Parse(text, NULL, TIXML_ENCODING_UNKNOWN);
    if (text != NULL) {
        free(text);
    }
    const TiXmlElement* element = static_cast<const TiXmlDocument&>(config).FirstChild("goPill")->ToElement();
    do {
        CGoPill* item = new CGoPill();
        item->init(*element);
        m_items.addElement(item);
        element = element->NextSiblingElement("goPill");
    } while (element != NULL);
}

CGoPillManager::~CGoPillManager()
{
    for (int index = 0; index < m_items.size(); ++index) {
        if (m_items.m_items[index] != NULL) {
            delete m_items.m_items[index];
            m_items.m_items[index] = NULL;
        }
    }
}

void CGoPillManager::loadResources()
{
    for (int index = 0; index < m_items.size(); ++index) {
        m_items[index]->loadResources();
    }
}

void CGoPillManager::freeResources()
{
    for (int index = 0; index < m_items.size(); ++index) {
        m_items[index]->releaseResources();
    }
}

CItem* CGoPillManager::getItemByInternalId(int index)
{
    if (index < m_items.size()) {  // the original does not test index >= 0
        return m_items[index];
    }
    return NULL;
}

CGoPill* CGoPillManager::GetPillById(int id)
{
    // the original indexes the item list directly (same body as getItemByInternalId)
    return static_cast<CGoPill*>(CGoPillManager::getItemByInternalId(id));
}

int CGoPillManager::getItemsCount()
{
    return m_items.size();
}

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AssertCGoPillManagerSize[sizeof(CGoPillManager) == 0x18 ? 1 : -1];
#endif
