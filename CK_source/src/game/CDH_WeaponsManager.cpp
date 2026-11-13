// CDH_WeaponsManager.cpp - CK weapon catalog and saved equipment.
#include "CDH_WeaponsManager.h"

#include <stdlib.h>
#include "App.h"
#include "CAmmo.h"
#include "CAmmoManager.h"
#include "CDH_Weapon.h"
#include "CXmlHelper.h"
#include "ResMap.h"

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char CDH_WeaponsManager_ARMv7_Size[(sizeof(CDH_WeaponsManager) == 0x30) ? 1 : -1];
#endif

CDH_WeaponsManager::CDH_WeaponsManager()
    : m_maxClip(0)
{
    m_maxWander = 0.0f;
    m_maxWanderRounded = 0;
    m_maxRange = 0;
    m_maxDamage = 0;
    m_maxRPM = 0;

    App* app = static_cast<App*>(WindowApp::m_instance);
    char* text = app->InitResMap()->loadText(XString(L"res/common/configs/weapons.xml"));
    TiXmlDocument document;
    document.Parse(text, NULL, TIXML_ENCODING_UNKNOWN);
    if (text != NULL) {
        free(text);
    }
    TiXmlNode* node = document.FirstChild("weapon");
    CDH_Weapon::InitUniversalAmmo();
    for (; node != NULL; node = node->NextSibling("weapon")) {
        const int id = CXmlHelper::GetAttributeValue(node, "wid").ToInt();
        const XString path = CXmlHelper::GetAttributeValue(node, "config");
        CDH_Weapon* weapon = ParseWeapon(id, path);
        const float wander = weapon->wanderAmount();
        if (wander > m_maxWanderRounded) {
            m_maxWanderRounded = static_cast<int>(weapon->wanderAmount());
        }
        const float range = weapon->effectiveRange();
        if (range > m_maxRange) {
            m_maxRange = static_cast<int>(weapon->effectiveRange());
        }
        const int damage = weapon->damage();
        if (damage > m_maxDamage) {
            m_maxDamage = weapon->damage();
        }
        const int rpm = weapon->getRPM();
        if (rpm > m_maxRPM) {
            m_maxRPM = weapon->getRPM();
        }
        int clipSize = weapon->m_clipSize;
        if (clipSize > m_maxClip && clipSize != 1000000) {
            m_maxClip = clipSize;
        }
        m_weapons.addElement(weapon);
    }
    for (int index = 0; index < m_weapons.size(); ++index) {
        const float wander = m_weapons[index]->wanderAmountMax();
        if (wander > m_maxWander) {
            m_maxWander = wander;
        }
    }
}

CDH_Weapon* CDH_WeaponsManager::ParseWeapon(int id, const XString& path)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    char* text = app->InitResMap()->loadText(path);
    TiXmlDocument document;
    document.Parse(text, NULL, TIXML_ENCODING_UNKNOWN);
    if (text != NULL) {
        free(text);
    }
    TiXmlElement* element = document.FirstChild("weapon")->ToElement();
    CDH_Weapon* weapon = new CDH_Weapon(id);
    weapon->init(*element);
    return weapon;
}

CDH_WeaponsManager::~CDH_WeaponsManager()
{
    ReleaseResourcesImpl();
    for (int index = 0; index < m_weapons.size(); ++index) {
        m_weapons[index]->Save(true);
        CDH_Weapon* weapon = m_weapons[index];
        if (weapon != NULL) {
            delete weapon;
            m_weapons[index] = NULL;
        }
    }
}

void CDH_WeaponsManager::ReleaseResourcesImpl()
{
    for (int index = 0; index < m_weapons.size(); ++index) {
        m_weapons[index]->releaseResources();
    }
}

void CDH_WeaponsManager::loadResources()
{
    for (int index = 0; index < m_weapons.size(); ++index) {
        m_weapons[index]->loadResources();
    }
}

void CDH_WeaponsManager::freeResources()
{
    ReleaseResourcesImpl();
}

void CDH_WeaponsManager::resetData()
{
    for (int index = 0; index < m_weapons.size(); ++index) {
        m_weapons[index]->setAvailable(false);
    }
    GetWeaponById(0)->setAvailable(true);
    GetWeaponById(1)->setAvailable(true);
}

Vector<XString> CDH_WeaponsManager::GetUnlockedWeapons(int level) const
{
    Vector<XString> names;
    for (int index = 0; index < m_weapons.size(); ++index) {
        if (m_weapons[index]->m_unlockLevel == level) {
            names.addElement(m_weapons[index]->m_name);
        }
    }
    return names;
}

void CDH_WeaponsManager::load3dResources(int id, bool forMenu)
{
    // the original does not check for an unknown id
    GetWeaponById(id)->loadWeaponModel(forMenu);
}

void CDH_WeaponsManager::release3dResources(int id)
{
    // the original does not check for an unknown id
    GetWeaponById(id)->releaseWeaponModel();
}

int CDH_WeaponsManager::GetAvailableWeapons(Vector<CDH_Weapon*>& weapons)
{
    int index = 0;
    for (; index < m_weapons.size(); ++index) {
        CDH_Weapon* weapon = m_weapons[index];
        if (weapon->isAvailable()) {
            weapons.addElement(weapon);
        }
    }
    return index;
}

int CDH_WeaponsManager::GetUnavailableWeapons(Vector<CDH_Weapon*>& weapons)
{
    int index = 0;
    for (; index < m_weapons.size(); ++index) {
        CDH_Weapon* weapon = m_weapons[index];
        if (!weapon->isAvailable() && !weapon->isHidden()) {
            weapons.addElement(weapon);
        }
    }
    return index;
}

CItem* CDH_WeaponsManager::getItemByInternalId(int index)
{
    if (index < m_weapons.size()) {  // the original does not test index >= 0
        return m_weapons[index];
    }
    return NULL;
}

int CDH_WeaponsManager::GetUnlockedWeaponsIdOnLevel(int level, Vector<int>& ids)
{
    int count = 0;
    for (CItem* item = firstItem(); item != NULL; item = nextItem()) {
        if (item->m_unlockLevel == level) {
            int id = item->m_id;
            ++count;
            ids.addElement(id);
        }
    }
    return count;
}

bool CDH_WeaponsManager::HasWeaponForAmmo(int ammoId)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CAmmo* ammo = static_cast<CAmmo*>(app->InitAmmoManager()->getItemById(ammoId));
    if (ammo == NULL) {
        return false;
    }
    if (ammo->m_universal) {
        return true;
    }
    for (CItem* item = static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->firstItem();
         item != NULL;
         item = static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->nextItem()) {
        CDH_Weapon* weapon = static_cast<CDH_Weapon*>(item);
        if (weapon->isAvailable() == 1 && weapon->CanUseAmmo(ammo)) {
            return true;
        }
    }
    return false;
}
