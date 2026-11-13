#include "CSquadTypeManager.h"

#include "App.h"
#include "CSquadType.h"
#include "ResMap.h"
#include "tinyxml.h"

CSquadTypeManager::CSquadTypeManager() : m_squads(8)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    char* text = app->InitResMap()->loadText(XString(L"res/common/configs/squads.xml"));
    TiXmlDocument document;
    document.Parse(text);
    delete[] text;
    for (TiXmlNode* node = document.FirstChild("squad"); node != NULL; node = node->NextSibling("squad")) {
        CSquadType* squad = new CSquadType(node);
        // The original leaves a replaced squad allocated.
        m_squads.Get(squad->m_name);
        m_squads.Set(squad->m_name, squad);
    }
}

CSquadTypeManager::~CSquadTypeManager()
{
    m_squads.DeleteContents();
}

CSquadType* CSquadTypeManager::GetType(const XString& name) const
{
    CSquadType* const* squad = m_squads.Get(name);
    if (squad != NULL) {
        return *squad;
    }
    return NULL;
}

typedef char AssertSquadTypeManagerSize[sizeof(CSquadTypeManager) == 0x10 ? 1 : -1];
