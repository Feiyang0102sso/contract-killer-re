// CIAPCurrencyManager.cpp - read the game currency catalog and request the matching store products.
#include "CIAPCurrencyManager.h"
#define AD_BANNER_SIZES_NO_MARGINS
#include "AdBannerSizes.h"
#undef AD_BANNER_SIZES_NO_MARGINS
#include "App.h"
#include "ICInAppPurchase.h"
#include "ResMap.h"
#include "tinyxml.h"

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char CIAPCurrencyManagerSizeCheck[sizeof(CIAPCurrencyManager) == 0x1c ? 1 : -1];
#endif

CIAPCurrencyManager::CIAPCurrencyManager()
    : m_retries(0)
{
    m_iterator = 0;
    App* app = static_cast<App*>(WindowApp::m_instance);
    char* config = app->InitResMap()->loadText(XString(L"res/common/configs/iapcurrency.xml"));
    TiXmlDocument document;
    if (config != NULL) {
        document.Parse(config);
        delete[] config;
    }
    for (const TiXmlElement* currency = document.FirstChildElement("currency"); currency != NULL;
         currency = currency->NextSiblingElement("currency")) {
        CIAPCurrency* item = new CIAPCurrency;
        item->init(*currency);
        m_items.addElement(item);
    }
}

CIAPCurrencyManager::~CIAPCurrencyManager()
{
    for (int index = 0; index < m_items.size(); ++index) {
        delete m_items[index];
        m_items[index] = NULL;
    }
}

CIAPCurrency* CIAPCurrencyManager::getIAPCurrencyByAppleId(XString appleId) const
{
    for (int index = 0; index < m_items.size(); ++index) {
        XString current = m_items[index]->m_appleId;
        if (current == appleId) {
            return m_items[index];
        }
    }
    return NULL;
}

int CIAPCurrencyManager::getItemsCount()
{
    if (GetAvailableItemsCount() == 0 && m_items.size() > 1) {
        return 2;
    }
    return m_items.size();
}

int CIAPCurrencyManager::GetAvailableItemsCount()
{
    int count = 0;
    for (int index = 0; index < m_items.size(); ++index) {
        XString type = m_items[index]->m_type;
        if (!(type == L"tapjoy")) {
            count += m_items[index]->isAvailable();
        }
    }
    return count;
}

void CIAPCurrencyManager::SetAllUnavailable()
{
    for (int index = 0; index < m_items.size(); ++index) {
        XString type = m_items[index]->m_type;
        if (!(type == L"tapjoy")) {
            m_items[index]->setAvailable(false);
        }
    }
}

void CIAPCurrencyManager::loadResources()
{
    for (int index = 0; index < m_items.size(); ++index) {
        m_items[index]->loadResources();
    }
}

void CIAPCurrencyManager::UpdateIAPInfo(bool resetRetries)
{
    if (resetRetries) {
        m_retries = 0;
    } else if (m_retries > 2) {
        return;
    }
    SetAllUnavailable();
    ICInAppPurchase* purchase = ICInAppPurchase::GetInstance();
    TCVector<CStrWChar> ids;
    for (int index = 0; index < m_items.size(); ++index) {
        CIAPCurrency* item = m_items[index];
        if (item->m_type == L"iap" && item->m_appleId.Length() > 0) {
            ids.Add(CStrWChar(item->m_appleId.c_str()));
        }
    }
    if (ids.Size() > 0) {
        purchase->RequestProductData(ids, 10000);
        ++m_retries;
    }
}

void CIAPCurrencyManager::freeResources()
{
    for (int index = 0; index < m_items.size(); ++index) {
        m_items[index]->releaseResources();
    }
}

CItem* CIAPCurrencyManager::getItemByInternalId(int index)
{
    if (index < m_items.size()) {  // the original does not test index >= 0
        return m_items[index];
    }
    return NULL;
}

bool CIAPCurrencyManager::IsRetryWaiting()
{
    return m_retries > 2;
}
