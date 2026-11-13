#include "CMiscItemManager.h"

#include "CCyberScope.h"
#include "CHitechVisor.h"
#include "CRevive.h"
#include "CSilencer.h"
#include "CUltrasoundEmitter.h"
#include "CXmlHelper.h"

CItem* CMiscItemManager::getItemByInternalId(int index)
{
    if (index < m_items.size()) {  // the original does not test index >= 0
        return m_items[index];
    }
    return NULL;
}

int CMiscItemManager::getItemsCount()
{
    return m_items.size();
}

CMiscItemManager::CMiscItemManager()
{
    TiXmlDocument config = CXmlHelper::Parse(L"res/common/configs/misc_items.xml");
    for (const TiXmlElement* element = config.FirstChildElement("item"); element != NULL;
         element = element->NextSiblingElement("item")) {
        CItem* item = CreateItem(CXmlHelper::GetAttributeValue(*element, "type"), *element);
        if (item != NULL) {
            m_items.addElement(item);
        }
    }
}

CItem* CMiscItemManager::CreateItem(const XString& type, const TiXmlElement& element)
{
    if (type == XString("silencer")) {
        return new CSilencer(element);
    }
    if (type == XString("revive")) {
        return new CRevive(element);
    }
    if (type == XString("hitech_visor")) {
        return new CHitechVisor(element);
    }
    if (type == XString("cyber_scope")) {
        return new CCyberScope(element);
    }
    if (type == XString("ultrasound_emitter")) {
        return new CUltrasoundEmitter(element);
    }
    return NULL;
}

CMiscItemManager::~CMiscItemManager()
{
    SAFE_DELETE_VECTOR(m_items);
}

void CMiscItemManager::loadResources()
{
    for (int index = 0; index < m_items.size(); ++index) {
        m_items[index]->loadResources();
    }
}

void CMiscItemManager::freeResources()
{
    for (int index = 0; index < m_items.size(); ++index) {
        m_items[index]->releaseResources();
    }
}

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AssertMiscItemManagerSize[sizeof(CMiscItemManager) == 0x18 ? 1 : -1];
#endif
