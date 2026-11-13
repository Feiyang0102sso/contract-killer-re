// CDailyDealManager.cpp - read, rotate and save the current daily deal.
#include "CDailyDealManager.h"
#include "App.h"
#include "CShopItem.h"
#include "CSaveManager.h"
#include "CStdUtil_Cocoa.h"
#include "CXmlHelper.h"
#include "ResMap.h"

static const unsigned int kDealDurationSeconds = 86400;

CDailyDealManager::CDailyDealManager()
    : m_currentIndex(0), m_startTime(CStdUtil_Cocoa::GetTimeSeconds())
{
    ParseConfig();
    Load();
}

CDailyDealManager::~CDailyDealManager()
{
    SAFE_DELETE_VECTOR(m_deals);
}

void CDailyDealManager::ParseConfig()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    char* config = app->InitResMap()->loadText(XString(L"res/common/configs/daily_deal.xml"));
    TiXmlDocument document;
    document.Parse(config);
    delete[] config;
    TiXmlNode* list = document.FirstChild("daily_deals");
    for (TiXmlElement* element = list->FirstChildElement("deal"); element != NULL;
         element = element->NextSiblingElement("deal")) {
        SDailyDeal* deal = new SDailyDeal();
        deal->m_itemType = CXmlHelper::GetAttributeValue(element, "itemType");
        deal->m_itemId = CXmlHelper::GetAttributeValue(element, "itemId").ToInt();
        deal->m_priceCredits = CXmlHelper::GetAttributeValue(element, "newPrice").ToInt();
        deal->m_text = CXmlHelper::GetAttributeValue(element, "text");
        m_deals.addElement(deal);
    }
}

void CDailyDealManager::Load()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    TiXmlElement* root = app->m_saveManager->GetRootElement();
    TiXmlElement* state = root->FirstChildElement("daily_deal");
    if (state != NULL) {
        m_currentIndex = CXmlHelper::GetAttributeValue(state, "currentDealId").ToInt();
        m_startTime = CXmlHelper::GetAttributeValue(state, "lastDealStartTime").ToInt();
    }
}

SDailyDeal* CDailyDealManager::GetCurrentDailyDeal()
{
    CheckForDealFinishByTime();
    CItem* item = GetItemFromDeal(m_currentIndex);
    if (item == NULL) {
        return NULL;
    }
    int attempts = 0;
    while (item != NULL && (item->isAvailable() || item->m_sale) && attempts < m_deals.size()) {
        ++attempts;
        NextDeal();
        item = GetItemFromDeal(m_currentIndex);
    }
    if (attempts >= m_deals.size()) {
        return NULL;
    }
    Save();
    return m_deals[m_currentIndex];
}

void CDailyDealManager::CheckForDealFinishByTime()
{
    if (m_startTime != 0 && CStdUtil_Cocoa::GetTimeSeconds() - m_startTime > kDealDurationSeconds) {
        NextDeal();
    }
}

CItem* CDailyDealManager::GetItemFromDeal(int index)
{
    // The original has no lower bound check.
    if (m_deals.size() > index) {
        SDailyDeal* deal = m_deals[index];
        if (deal != NULL) {
            CItemManager* manager = App::GetItemManagerByName(deal->m_itemType);
            if (manager != NULL) {
                return manager->getItemById(deal->m_itemId);
            }
        }
    }
    return NULL;
}

void CDailyDealManager::NextDeal()
{
    ++m_currentIndex;
    if (m_currentIndex >= m_deals.size()) {
        m_currentIndex = 0;
    }
    m_startTime = CStdUtil_Cocoa::GetTimeSeconds();
    Save();
}

void CDailyDealManager::Save()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    TiXmlElement* root = app->m_saveManager->GetRootElement();
    TiXmlElement* state = CXmlHelper::ObtainElement(root, XString("daily_deal"));
    state->Clear();
    state->SetAttribute("currentDealId", m_currentIndex);
    state->SetAttribute("lastDealStartTime", m_startTime);
}

CItem* CDailyDealManager::GetCurrentDealItem()
{
    GetCurrentDailyDeal();
    CItem* item = GetItemFromDeal(m_currentIndex);
    if (item != NULL) {
        item->m_field56 = true;
    }
    return item;
}

void CDailyDealManager::ItemPurchased(CItem* item)
{
    if (GetCurrentDealItem() == item) {
        NextDeal();
    }
}

unsigned int CDailyDealManager::GetNextDealStartTime()
{
    return m_startTime + kDealDurationSeconds;
}
