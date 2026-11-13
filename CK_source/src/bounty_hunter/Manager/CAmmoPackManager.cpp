#include "CAmmoPackManager.h"

#include "App.h"
#include "CAmmoManager.h"
#include "CAmmoPack.h"
#include "CBH_Player.h"
#include "ResMap.h"
#include "tinyxml.h"

CAmmoPackManager::CAmmoPackManager()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    char* text = app->InitResMap()->loadText(XString(L"res/common/configs/ammo_packs.xml"));
    if (text == NULL) {
        return;
    }
    TiXmlDocument document;
    document.Parse(text, NULL, TIXML_ENCODING_UNKNOWN);
    free(text);
    const TiXmlElement* root = document.FirstChildElement("ammo_packs");
    if (root != NULL) {
        Parse(*root);
    }
}

CAmmoPackManager::~CAmmoPackManager()
{
    SAFE_DELETE_VECTOR(m_packs);
}

void CAmmoPackManager::Parse(const TiXmlElement& root)
{
    for (const TiXmlElement* element = root.FirstChildElement("ammo_pack"); element != NULL;
         element = element->NextSiblingElement("ammo_pack")) {
        CAmmoPack* pack = new CAmmoPack();
        pack->init(*element);
        m_packs.addElement(pack);
    }
}

int CAmmoPackManager::getAmount(int id)
{
    CAmmoPack* pack = static_cast<CAmmoPack*>(getItemById(id));
    if (pack == NULL) {
        return 0;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    CAmmo* ammo = app->InitAmmoManager()->GetAmmoByType(pack->m_ammoType);
    return CBH_Player::GetInstance()->GetAvailableAmmo(ammo, true);
}

void CAmmoPackManager::loadResources()
{
    for (int index = 0; index < m_packs.size(); ++index) {
        m_packs[index]->loadResources();
    }
}

void CAmmoPackManager::freeResources()
{
    for (int index = 0; index < m_packs.size(); ++index) {
        m_packs[index]->releaseResources();
    }
}

CItem* CAmmoPackManager::getItemByInternalId(int index)
{
    if (index < m_packs.size()) {  // the original does not test index >= 0
        return m_packs[index];
    }
    return NULL;
}

Vector<CAmmoPack*> CAmmoPackManager::getAmmoPacksByType(const XString& type)
{
    Vector<CAmmoPack*> packs;
    for (int index = 0; index < m_packs.size(); ++index) {
        if (m_packs[index]->m_ammoType == type) {
            packs.addElement(m_packs[index]);
        }
    }
    return packs;
}

typedef char AssertAmmoPackManagerSize[sizeof(CAmmoPackManager) == 0x18 ? 1 : -1];
