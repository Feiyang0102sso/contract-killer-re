#include "CScriptType.h"

#include "CXmlHelper.h"

CScriptType::CScriptType(TiXmlNode* node)
    : m_cameraId(0), m_spawnPointId(-1), m_looped(false), m_minLevel(0), m_maxLevel(0)
{
    Parse(node);
}

CScriptType::~CScriptType()
{
    // The vector and the two strings are destroyed in reverse member order.
}

void CScriptType::Parse(TiXmlNode* node)
{
    m_name = CXmlHelper::GetAttributeValue(node, "name");
    ParseInfo(node->FirstChild("info"));
    m_spawnPointId = CXmlHelper::GetAttributeValue(node->FirstChild("swarmPoint"), "mapPointID").ToInt();
    ParsePoints(node->FirstChild("points"));
}

void CScriptType::ParseInfo(TiXmlNode* node)
{
    m_location = CXmlHelper::GetAttributeValue(node, "location");
    m_cameraId = CXmlHelper::GetAttributeValue(node, "cameraID").ToInt();
    m_minLevel = CXmlHelper::GetAttributeValue(node, "minLevel").ToInt();
    m_maxLevel = CXmlHelper::GetAttributeValue(node, "maxLevel").ToInt();
    m_looped = CXmlHelper::GetAttributeValue(node, "looped").ToInt() == 1;
}

void CScriptType::ParsePoints(TiXmlNode* node)
{
    if (node != NULL) {
        for (TiXmlNode* point = node->FirstChild("waypoint"); point != NULL; point = point->NextSibling("waypoint")) {
            SMapPoint data;
            data.m_mapPointId = CXmlHelper::GetAttributeValue(point, "mapPointID").ToInt();
            data.m_idleId = CXmlHelper::GetAttributeValue(point, "idleID").ToInt();
            m_waypoints.addElement(data);
        }
    }
}

int CScriptType::GetWayPointID(int index) const
{
    if (index >= 0 && index < m_waypoints.size()) {
        return m_waypoints[index].m_mapPointId;
    }
    return -1;
}

int CScriptType::GetEldeBehaviorID(int index) const
{
    if (index >= 0 && index < m_waypoints.size()) {
        return m_waypoints[index].m_idleId;
    }
    return -1;
}

typedef char AssertScriptTypeSize[sizeof(CScriptType) == 0x2c ? 1 : -1];
