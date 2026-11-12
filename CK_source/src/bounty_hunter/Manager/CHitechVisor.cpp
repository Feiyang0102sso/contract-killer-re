#include "CHitechVisor.h"

#include "App.h"
#include "CSaveManager.h"
#include "CXmlHelper.h"

CHitechVisor::CHitechVisor(const TiXmlElement& config)
    : CItem(0), m_owned(false)
{
    Parse(config);
    Load();
}

void CHitechVisor::Load()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    TiXmlElement* root = app->m_saveManager->GetRootElement();
    const TiXmlElement* visor = root->FirstChildElement("visor");
    m_owned = visor != NULL && CXmlHelper::GetAttributeValueOptional(*visor, "isEquiped").ToBool();
}

void CHitechVisor::setAvailable(bool available)
{
    m_owned = available;
    Save();
}

void CHitechVisor::Save()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    TiXmlElement* root = app->m_saveManager->GetRootElement();
    CXmlHelper::ObtainElement(root, XString("visor"))->SetAttribute("isEquiped", m_owned);
}

XString CHitechVisor::getUIDString() const
{
    return XString::Format(L"misc_%d", m_id);
}

typedef char AssertHitechVisorSize[sizeof(CHitechVisor) == 0x5c ? 1 : -1];
