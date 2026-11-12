// The members of a CK mission squad.
#ifndef CSQUADTYPE_H
#define CSQUADTYPE_H

#include "Vector_ss.h"
#include "xString.h"

class TiXmlNode;

class CSquadType {
public:
    explicit CSquadType(TiXmlNode* node);
    ~CSquadType();
    void Parse(TiXmlNode* node);
    void ParseMinion(TiXmlNode* node);
    void CalcDPM();
    const XString& GetMiniomName() const;
    const XString& GetMiniomNameByIndex(int index) const;

    XString m_name;
    Vector<XString> m_minions;
    float m_damage;
    float m_field18;
};

#endif
