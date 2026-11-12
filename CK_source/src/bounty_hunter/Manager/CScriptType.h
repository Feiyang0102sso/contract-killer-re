// A CK mission's spawn point and ordered waypoints.
#ifndef CSCRIPTTYPE_H
#define CSCRIPTTYPE_H

#include "Vector_ss.h"
#include "xString.h"

class TiXmlNode;

class CScriptType {
public:
    struct SMapPoint {
        int m_mapPointId;
        int m_idleId;
    };

    explicit CScriptType(TiXmlNode* node);
    ~CScriptType();
    void Parse(TiXmlNode* node);
    void ParseInfo(TiXmlNode* node);
    void ParsePoints(TiXmlNode* node);
    int GetWayPointID(int index) const;
    int GetEldeBehaviorID(int index) const;

    XString m_name;
    XString m_location;
    Vector<SMapPoint> m_waypoints;
    int m_cameraId;
    int m_spawnPointId;
    bool m_looped;
    int m_minLevel;
    int m_maxLevel;
};

#endif
