// CAmmoManager.cpp - parse the CK ammo table and own its entries.
#include "CAmmoManager.h"

#include <stdlib.h>
#include "App.h"
#include "CAmmo.h"
#include "CBH_Player.h"
#include "CDH_WeaponsManager.h"
#include "ResMap.h"
#include "tinyxml.h"

typedef char CAmmoManager_ARMv7_Size[(sizeof(CAmmoManager) == 0x24) ? 1 : -1];

CAmmoManager::CAmmoManager()
    : m_ammo(8)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    char* text = app->InitResMap()->loadText(XString(L"res/common/configs/ammo.xml"));
    if (text == NULL) {
        return;
    }
    TiXmlDocument document;
    document.Parse(text, NULL, TIXML_ENCODING_UNKNOWN);
    free(text);
    for (TiXmlElement* element = document.FirstChildElement("ammo"); element != NULL;
         element = element->NextSiblingElement("ammo")) {
        CAmmo* ammo = new CAmmo;
        ammo->init(*element);
        if (m_ammo.Get(ammo->m_ammoType) == NULL) {
            m_types.addElement(ammo->m_ammoType);
        } else {
            delete *m_ammo.Get(ammo->m_ammoType);
        }
        m_ammo.Set(ammo->m_ammoType, ammo);
    }
}

CAmmoManager::~CAmmoManager()
{
    m_ammo.DeleteContents();
}

void CAmmoManager::loadResources()
{
    for (int index = 0; index < m_ammo.m_count; ++index) {
        (*m_ammo.GetAt(index, NULL))->loadResources();
    }
}

void CAmmoManager::freeResources()
{
    for (int index = 0; index < m_ammo.m_count; ++index) {
        (*m_ammo.GetAt(index, NULL))->releaseResources();
    }
}

CAmmo* CAmmoManager::GetAmmoByType(const XString& type)
{
    CAmmo** entry = m_ammo.Get(type);
    if (entry == NULL) {
        return NULL;
    }
    return *entry;
}

int CAmmoManager::getAmount(int id)
{
    return CBH_Player::GetInstance()->GetAvailableAmmo(static_cast<CAmmo*>(getItemById(id)), true);
}

CItem* CAmmoManager::getItemByInternalId(int index)
{
    if (index >= 0 && index < m_types.size() && index < m_ammo.m_count) {
        return GetAmmoByType(m_types[index]);
    }
    return NULL;
}

bool CAmmoManager::IsAllAmmoTypesOpen()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    for (CItem* item = firstItem(); item != NULL; item = nextItem()) {
        if (!app->InitWeaponManager()->HasWeaponForAmmo(item->m_id)) {
            return false;
        }
    }
    return true;
}
