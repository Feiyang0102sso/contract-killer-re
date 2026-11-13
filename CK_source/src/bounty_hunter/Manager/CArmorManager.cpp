#include "CArmorManager.h"

#include "CArmor.h"
#include "CXmlHelper.h"

CArmorManager::CArmorManager()
{
    TiXmlDocument config = CXmlHelper::Parse(L"res/common/configs/armors.xml");
    for (const TiXmlElement* element = config.FirstChildElement("armor"); element != NULL;
         element = element->NextSiblingElement("armor")) {
        CArmor* item = new CArmor();
        item->init(*element);
        m_items.addElement(item);
    }
}

CArmorManager::~CArmorManager()
{
    SAFE_DELETE_VECTOR(m_items);
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
