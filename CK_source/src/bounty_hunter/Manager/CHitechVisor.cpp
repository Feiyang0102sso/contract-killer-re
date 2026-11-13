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
    TiXmlElement* visor = root->FirstChildElement("visor");
    if (visor != NULL) {
        // Original bug: the string pointer is tested, so any saved visor node means owned.
        m_owned = CXmlHelper::GetAttributeValueOrUseDefault(visor, "isEquiped", XString("0")).c_str() != NULL;
    }
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
