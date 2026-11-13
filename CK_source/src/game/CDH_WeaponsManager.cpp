// CDH_WeaponsManager.cpp - CK weapon catalog and saved equipment.
#include "CDH_WeaponsManager.h"

#include <stdlib.h>
#include "App.h"
#include "CAmmo.h"
#include "CAmmoManager.h"
#include "CDH_Weapon.h"
#include "CXmlHelper.h"
#include "ResMap.h"

typedef char CDH_WeaponsManager_ARMv7_Size[(sizeof(CDH_WeaponsManager) == 0x30) ? 1 : -1];

CDH_WeaponsManager::CDH_WeaponsManager()
    : m_maxWander(0.0f)
    , m_maxWanderRounded(0)
    , m_maxRange(0)
    , m_maxDamage(0)
    , m_maxRPM(0)
    , m_maxClip(0)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    char* text = app->InitResMap()->loadText(XString(L"res/common/configs/weapons.xml"));
    TiXmlDocument document;
    if (text != NULL) {
        document.Parse(text, NULL, TIXML_ENCODING_UNKNOWN);
        free(text);
    }

    CDH_Weapon::InitUniversalAmmo();
    for (TiXmlNode* node = document.FirstChild("weapon"); node != NULL; node = node->NextSibling("weapon")) {
        const int id = CXmlHelper::GetAttributeValue(node, "wid").ToInt();
        const XString path = CXmlHelper::GetAttributeValue(node, "config");
        CDH_Weapon* weapon = ParseWeapon(id, path);
        if (weapon == NULL) {
            continue;
        }
        const float wander = weapon->wanderAmount();
        if (m_maxWanderRounded < wander) {
            m_maxWanderRounded = static_cast<int>(wander);
        }
        const float range = weapon->effectiveRange();
        if (m_maxRange < range) {
            m_maxRange = static_cast<int>(range);
        }
        const int damage = weapon->damage();
        if (m_maxDamage < damage) {
            m_maxDamage = damage;
        }
        const int rpm = weapon->getRPM();
        if (m_maxRPM < rpm) {
            m_maxRPM = rpm;
        }
        if (m_maxClip < weapon->m_clipSize && weapon->m_clipSize != 1000000) {
            m_maxClip = weapon->m_clipSize;
        }
        m_weapons.addElement(weapon);
    }
    for (int index = 0; index < m_weapons.size(); ++index) {
        const float wander = m_weapons[index]->wanderAmountMax();
        if (m_maxWander < wander) {
            m_maxWander = wander;
        }
    }
}

CDH_Weapon* CDH_WeaponsManager::ParseWeapon(int id, const XString& path)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    char* text = app->InitResMap()->loadText(path);
    if (text == NULL) {
        return NULL;
    }
    TiXmlDocument document;
    document.Parse(text, NULL, TIXML_ENCODING_UNKNOWN);
    free(text);
    const TiXmlElement* element = document.FirstChildElement("weapon");
    if (element == NULL) {
        return NULL;
    }
    CDH_Weapon* weapon = new CDH_Weapon(id);
    weapon->init(*element);
    return weapon;
}

CDH_WeaponsManager::~CDH_WeaponsManager()
{
    ReleaseResourcesImpl();
    CleanupAll();
}

void CDH_WeaponsManager::CleanupAll()
{
    for (int index = 0; index < m_weapons.size(); ++index) {
        CDH_Weapon* weapon = m_weapons[index];
        weapon->Save(true);
        delete weapon;
    }
    m_weapons.setSize(0);
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
    CDH_Weapon* first = GetWeaponById(0);
    if (first != NULL) {
        first->setAvailable(true);
    }
    CDH_Weapon* second = GetWeaponById(1);
    if (second != NULL) {
        second->setAvailable(true);
    }
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
            ++count;
            ids.addElement(item->m_id);
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
    for (CItem* item = firstItem(); item != NULL; item = nextItem()) {
        CDH_Weapon* weapon = static_cast<CDH_Weapon*>(item);
        if (weapon->isAvailable() && weapon->CanUseAmmo(ammo)) {
            return true;
        }
    }
    return false;
}
