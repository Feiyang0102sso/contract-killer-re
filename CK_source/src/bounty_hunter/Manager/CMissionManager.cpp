// CMissionManager.cpp - original CK mission table indexed by the XML id.
#include "CMissionManager.h"

#include "App.h"
#include "ResMap.h"
#include "tinyxml.h"

CMissionManager::CMissionManager()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    char* xml = app->InitResMap()->loadText(XString(L"res/common/configs/missions.xml"));
    TiXmlDocument document;
    document.Parse(xml);
    delete[] xml;
    TiXmlNode* root = document.FirstChild("missions");
    if (root != NULL) {
        for (TiXmlNode* node = root->FirstChild("mission"); node != NULL; node = node->NextSibling("mission")) {
            m_missions.addElement(new CMission(node));
        }
    }
}

CMissionManager::~CMissionManager()
{
    SAFE_DELETE_VECTOR(m_missions);
}

CMission* CMissionManager::GetMissionById(int id) const
{
    for (int index = 0; index < m_missions.size(); ++index) {
        CMission* mission = m_missions[index];
        if (mission->m_id == id) {
            return mission;
        }
    }
    return NULL;
}

typedef char AssertMissionManagerSize[sizeof(CMissionManager) == 0x14 ? 1 : -1];
