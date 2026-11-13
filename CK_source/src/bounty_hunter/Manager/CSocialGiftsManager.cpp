#include "CSocialGiftsManager.h"

#include "App.h"
#include "CAmmoManager.h"
#include "CAmmoPackManager.h"
#include "CArmorManager.h"
#include "CBH_Player.h"
#include "CDH_WeaponsManager.h"
#include "CEnergy.h"
#include "CEnergyManager.h"
#include "CGoPillManager.h"
#include "CMedicantManager.h"
#include "CMiscItemManager.h"
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
    ResMap* resources = static_cast<App*>(WindowApp::m_instance)->InitResMap();
    char* text = resources->loadText(XString(L"res/common/configs/gserve_gifts.xml"));
    TiXmlDocument document;
    document.Parse(text, NULL, TIXML_ENCODING_UNKNOWN);
    if (text != NULL) {
        free(text);
    }
    const TiXmlNode* root = document.FirstChild("gifts");
    // The server sends the one-based XML position, including gifts hidden from the sender.
    const TiXmlNode* entry = root->FirstChild("gift");
    while (entry != NULL) {
        XString type = CXmlHelper::GetAttributeValue(entry, "type");
        CItemManager* manager = GetItemManager(type);
        if (manager != NULL) {
            SGift* gift = new SGift();
            gift->m_itemManager = manager;
            gift->m_itemId = CXmlHelper::GetAttributeValue(entry, "item_id").ToInt();
            m_gifts.addElement(gift);
            gift->m_hidden = CXmlHelper::GetAttributeValue(entry, "hidden").ToBool();
        }
        entry = entry->NextSibling("gift");
    }
}

CItemManager* CSocialGiftManager::GetItemManager(XString& type)
{
    if (type == XString("ammo")) {
        return static_cast<App*>(WindowApp::m_instance)->InitAmmoManager();
    }
    if (type == XString("medkit")) {
        return static_cast<App*>(WindowApp::m_instance)->m_medicantManager;
    }
    if (type == XString("weapon")) {
        return static_cast<App*>(WindowApp::m_instance)->InitWeaponManager();
    }
    if (type == XString("goPill")) {
        return static_cast<App*>(WindowApp::m_instance)->m_goPillManager;
    }
    if (type == XString("energy")) {
        return static_cast<App*>(WindowApp::m_instance)->m_energyManager;
    }
    if (type == XString("ammo_pack")) {
        return static_cast<App*>(WindowApp::m_instance)->m_ammoPackManager;
    }
    if (type == XString("armor")) {
        return static_cast<App*>(WindowApp::m_instance)->m_armorManager;
    }
    if (type == XString("misc")) {
        return static_cast<App*>(WindowApp::m_instance)->InitMiscItemManager();
    }
    return NULL;
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

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AssertGiftSize[sizeof(SGift) == 0xc ? 1 : -1];
#endif
#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AssertGiftManagerSize[sizeof(CSocialGiftManager) == 0x10 ? 1 : -1];
#endif
