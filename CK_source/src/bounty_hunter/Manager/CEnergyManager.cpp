#include "CEnergyManager.h"

#include "CEnergy.h"
#include "CXmlHelper.h"

CEnergyManager::CEnergyManager()
{
    TiXmlDocument config = CXmlHelper::Parse(L"res/common/configs/energy.xml");
    for (const TiXmlElement* element = config.FirstChildElement("energy"); element != NULL;
         element = element->NextSiblingElement("energy")) {
        CEnergy* item = new CEnergy();
        item->init(*element);
        m_items.addElement(item);
    }
}

CEnergyManager::~CEnergyManager()
{
    SAFE_DELETE_VECTOR(m_items);
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
