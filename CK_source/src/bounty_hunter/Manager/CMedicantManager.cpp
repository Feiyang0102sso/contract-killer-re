#include "CMedicantManager.h"

#include "App.h"
#include "CMedicant.h"
#include "ResMap.h"
#include "tinyxml.h"

CMedicantManager::CMedicantManager()
{
    ResMap* resources = static_cast<App*>(WindowApp::m_instance)->InitResMap();
    char* text = resources->loadText(XString(L"res/common/configs/medicants.xml"));
    TiXmlDocument config;
    config.Parse(text, NULL, TIXML_ENCODING_UNKNOWN);
    if (text != NULL) {
        free(text);
    }
    const TiXmlElement* element = static_cast<const TiXmlDocument&>(config).FirstChild("medicant")->ToElement();
    do {
        CMedicant* item = new CMedicant();
        item->init(*element);
        m_items.addElement(item);
        element = element->NextSiblingElement("medicant");
    } while (element != NULL);
}

CMedicantManager::~CMedicantManager()
{
    for (int index = 0; index < m_items.size(); ++index) {
        if (m_items.m_items[index] != NULL) {
            delete m_items.m_items[index];
            m_items.m_items[index] = NULL;
        }
    }
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

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AssertCMedicantManagerSize[sizeof(CMedicantManager) == 0x18 ? 1 : -1];
#endif
