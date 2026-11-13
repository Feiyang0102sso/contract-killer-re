// CCurrencyManager.cpp - read the cash pack catalog.
#include "CCurrencyManager.h"
#include "App.h"
#include "ResMap.h"
#include "tinyxml.h"

CCurrencyManager::CCurrencyManager()
{
    char* config = static_cast<App*>(WindowApp::m_instance)->InitResMap()->loadText(XString(L"res/common/configs/currency.xml"));
    TiXmlDocument document;
    document.Parse(config);
    if (config != NULL) {
        delete[] config;
    }
    const TiXmlNode* node = document.FirstChild("currency");
    const TiXmlElement* element = node->ToElement();
    do {
        CCurrency* currency = new CCurrency();
        currency->init(*element);
        m_items.addElement(currency);
        element = element->NextSiblingElement("currency");
    } while (element != NULL);
}

void CCurrencyManager::loadResources()
{
    for (int index = 0; index < m_items.size(); index++) {
        m_items[index]->loadResources();
    }
}

void CCurrencyManager::freeResources()
{
}

CCurrencyManager::~CCurrencyManager()
{
    for (int index = 0; index < m_items.size(); index++) {
        if (m_items[index] != NULL) {
            delete m_items[index];
            m_items[index] = NULL;
        }
    }
}

CItem* CCurrencyManager::getItemByInternalId(int index)
{
    CItem* item = NULL;
    if (index < m_items.size()) {
        item = m_items[index];
    }
    return item;
}
