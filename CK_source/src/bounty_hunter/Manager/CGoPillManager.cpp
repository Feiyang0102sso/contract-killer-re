#include "CGoPillManager.h"

#include "CGoPill.h"
#include "CXmlHelper.h"

CGoPillManager::CGoPillManager()
{
    TiXmlDocument config = CXmlHelper::Parse(L"res/common/configs/go_pills.xml");
    for (const TiXmlElement* element = config.FirstChildElement("goPill"); element != NULL;
         element = element->NextSiblingElement("goPill")) {
        CGoPill* item = new CGoPill();
        item->init(*element);
        m_items.addElement(item);
    }
}

CGoPillManager::~CGoPillManager()
{
    SAFE_DELETE_VECTOR(m_items);
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

typedef char AssertCGoPillManagerSize[sizeof(CGoPillManager) == 0x18 ? 1 : -1];
