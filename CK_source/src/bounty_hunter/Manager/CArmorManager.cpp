#include "CArmorManager.h"

#include "App.h"
#include "CArmor.h"
#include "ResMap.h"
#include "tinyxml.h"

CArmorManager::CArmorManager()
{
    ResMap* resources = static_cast<App*>(WindowApp::m_instance)->InitResMap();
    char* text = resources->loadText(XString(L"res/common/configs/armors.xml"));
    TiXmlDocument config;
    config.Parse(text, NULL, TIXML_ENCODING_UNKNOWN);
    if (text != NULL) {
        free(text);
    }
    const TiXmlElement* element = static_cast<const TiXmlDocument&>(config).FirstChild("armor")->ToElement();
    do {
        CArmor* item = new CArmor();
        item->init(*element);
        m_items.addElement(item);
        element = element->NextSiblingElement("armor");
    } while (element != NULL);
}

CArmorManager::~CArmorManager()
{
    for (int index = 0; index < m_items.size(); ++index) {
        if (m_items.m_items[index] != NULL) {
            delete m_items.m_items[index];
            m_items.m_items[index] = NULL;
        }
    }
}

void CArmorManager::loadResources()
{
    for (int index = 0; index < m_items.size(); ++index) {
        m_items[index]->loadResources();
    }
}

void CArmorManager::freeResources()
{
    for (int index = 0; index < m_items.size(); ++index) {
        m_items[index]->releaseResources();
    }
}

CItem* CArmorManager::getItemByInternalId(int index)
{
    if (index < m_items.size()) {  // the original does not test index >= 0
        return m_items[index];
    }
    return NULL;
}

CArmor* CArmorManager::GetArmorById(int id)
{
    // the original indexes the item list directly (same body as getItemByInternalId)
    return static_cast<CArmor*>(CArmorManager::getItemByInternalId(id));
}

int CArmorManager::getItemsCount()
{
    return m_items.size();
}

typedef char AssertCArmorManagerSize[sizeof(CArmorManager) == 0x18 ? 1 : -1];
