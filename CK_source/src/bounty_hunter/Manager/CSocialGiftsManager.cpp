#include "CSocialGiftsManager.h"

#include "App.h"
#include "CBH_Player.h"
#include "CEnergy.h"
#include "CShopItem.h"
#include "CXmlHelper.h"
#include "ResMap.h"
#include "tinyxml.h"

CSocialGiftManager::CSocialGiftManager()
{
    ParseConfig();
}

CSocialGiftManager::~CSocialGiftManager()
{
    SAFE_DELETE_VECTOR(m_gifts);
}

void CSocialGiftManager::ParseConfig()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    char* text = app->InitResMap()->loadText(XString(L"res/common/configs/gserve_gifts.xml"));
    if (text == NULL) {
        return;
    }
    TiXmlDocument document;
    document.Parse(text, NULL, TIXML_ENCODING_UNKNOWN);
    free(text);
    const TiXmlElement* root = document.FirstChildElement("gifts");
    if (root == NULL) {
        return;
    }
    // The server sends the one-based XML position, including gifts hidden from the sender.
    for (const TiXmlElement* entry = root->FirstChildElement("gift"); entry != NULL;
         entry = entry->NextSiblingElement("gift")) {
        XString type = CXmlHelper::GetAttributeValue(*entry, "type");
        SGift* gift = new SGift;
        gift->m_itemManager = GetItemManager(type);
        gift->m_itemId = CXmlHelper::GetAttributeValue(*entry, "item_id").ToInt();
        gift->m_hidden = CXmlHelper::GetAttributeValue(*entry, "hidden").ToBool();
        m_gifts.addElement(gift);
    }
}

CItemManager* CSocialGiftManager::GetItemManager(XString& type)
{
    if (type == XString("ammo_pack")) {
        return App::GetItemManagerByType(9);
    }
    if (type == XString("misc")) {
        return App::GetItemManagerByType(12);
    }
    return App::GetItemManagerByName(type);
}

int CSocialGiftManager::GetGiftsCount()
{
    return m_gifts.size();
}

SGift* CSocialGiftManager::GetGift(int index)
{
    if (index >= 0 && index < m_gifts.size()) {
        return m_gifts[index];
    }
    return NULL;
}

void CSocialGiftManager::GiveGift(int index)
{
    // the original does not check the gift index result
    CItem* item = GetGift(index)->GetItem();
    if (item == NULL) {
        return;
    }
    if (item->getType() == 4) {
        int energy = static_cast<CEnergy*>(item)->m_energy;
        CBH_Player::GetInstance()->AddEnergy(energy, true);
    } else {
        item->setAvailable(true);
    }
}

typedef char AssertGiftSize[sizeof(SGift) == 0xc ? 1 : -1];
typedef char AssertGiftManagerSize[sizeof(CSocialGiftManager) == 0x10 ? 1 : -1];
