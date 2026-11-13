#include "CMedicantManager.h"

#include "CMedicant.h"
#include "CXmlHelper.h"

CMedicantManager::CMedicantManager()
{
    TiXmlDocument config = CXmlHelper::Parse(L"res/common/configs/medicants.xml");
    for (const TiXmlElement* element = config.FirstChildElement("medicant"); element != NULL;
         element = element->NextSiblingElement("medicant")) {
        CMedicant* item = new CMedicant();
        item->init(*element);
        m_items.addElement(item);
    }
}

CMedicantManager::~CMedicantManager()
{
    SAFE_DELETE_VECTOR(m_items);
}

void CMedicantManager::loadResources()
{
    for (int index = 0; index < m_items.size(); ++index) {
        m_items[index]->loadResources();
    }
}

void CMedicantManager::freeResources()
{
    for (int index = 0; index < m_items.size(); ++index) {
        m_items[index]->releaseResources();
    }
}

CItem* CMedicantManager::getItemByInternalId(int index)
{
    if (index < m_items.size()) {  // the original does not test index >= 0
        return m_items[index];
    }
    return NULL;
}

int CMedicantManager::getAmount(int)
{
    return 0;
}

int CMedicantManager::getItemsCount()
{
    return m_items.size();
}

typedef char AssertCMedicantManagerSize[sizeof(CMedicantManager) == 0x18 ? 1 : -1];
