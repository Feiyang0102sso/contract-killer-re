#include "CEnergyManager.h"

#include "App.h"
#include "CEnergy.h"
#include "ResMap.h"
#include "tinyxml.h"

CEnergyManager::CEnergyManager()
{
    ResMap* resources = static_cast<App*>(WindowApp::m_instance)->InitResMap();
    char* text = resources->loadText(XString(L"res/common/configs/energy.xml"));
    TiXmlDocument config;
    config.Parse(text, NULL, TIXML_ENCODING_UNKNOWN);
    if (text != NULL) {
        free(text);
    }
    const TiXmlElement* element = static_cast<const TiXmlDocument&>(config).FirstChild("energy")->ToElement();
    do {
        CEnergy* item = new CEnergy();
        item->init(*element);
        m_items.addElement(item);
        element = element->NextSiblingElement("energy");
    } while (element != NULL);
}

CEnergyManager::~CEnergyManager()
{
    for (int index = 0; index < m_items.size(); ++index) {
        if (m_items.m_items[index] != NULL) {
            delete m_items.m_items[index];
            m_items.m_items[index] = NULL;
        }
    }
}

void CEnergyManager::loadResources()
{
    for (int index = 0; index < m_items.size(); ++index) {
        m_items[index]->loadResources();
    }
}

void CEnergyManager::freeResources()
{
    for (int index = 0; index < m_items.size(); ++index) {
        m_items[index]->releaseResources();
    }
}

CItem* CEnergyManager::getItemByInternalId(int index)
{
    if (index < m_items.size()) {  // the original does not test index >= 0
        return m_items[index];
    }
    return NULL;
}

CEnergy* CEnergyManager::getEnergyItem(int id)
{
    // the original indexes the item list directly (same body as getItemByInternalId)
    return static_cast<CEnergy*>(CEnergyManager::getItemByInternalId(id));
}

int CEnergyManager::getItemsCount()
{
    return m_items.size();
}

typedef char AssertCEnergyManagerSize[sizeof(CEnergyManager) == 0x18 ? 1 : -1];
