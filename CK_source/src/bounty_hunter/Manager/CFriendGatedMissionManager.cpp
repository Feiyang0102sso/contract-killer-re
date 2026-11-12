// CFriendGatedMissionManager.cpp - loads friends_gated_missions.xml.
#include "CFriendGatedMissionManager.h"
#include "CFriendGatedMission.h"
#include "App.h"
#include "ResMap.h"
#include "tinyxml.h"

CFriendGatedMissionManager::CFriendGatedMissionManager()
{
    char* text = ((App*)WindowApp::m_instance)->InitResMap()->loadText(XString(L"res/common/configs/friends_gated_missions.xml"));
    TiXmlDocument document;
    document.Parse(text);
    if (text != NULL) {
        delete[] text;
    }
    for (TiXmlNode* node = document.FirstChild("mission"); node != NULL; node = node->NextSibling("mission")) {
        m_missions.addElement(new CFriendGatedMission(node));
    }
}

CFriendGatedMissionManager::~CFriendGatedMissionManager()
{
    SAFE_DELETE_VECTOR(m_missions);
}

CFriendGatedMission* CFriendGatedMissionManager::GetMissionById(int id) const
{
    if (id >= 0 && id < m_missions.size()) {
        return m_missions[id];
    }
    return NULL;
}

int CFriendGatedMissionManager::MissionCount() const
{
    return m_missions.size();
}
