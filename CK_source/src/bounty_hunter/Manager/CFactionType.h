#ifndef CFACTIONTYPE_H
#define CFACTIONTYPE_H

#include "Vector_ss.h"
#include "xString.h"

class TiXmlNode;

class CFactionType {
public:
    struct SReputationChanges {
        SReputationChanges();
        int m_values[5];
    };

    explicit CFactionType(TiXmlNode* node);
    void Parse(TiXmlNode* node);
    void ParseInfo(TiXmlNode* node);
    void ParseGraphics(TiXmlNode* node);
    void ParseBoss(TiXmlNode* node);
    const XString& GetBoss() const;

    XString m_name;
    XString m_screenName;
    bool m_legal;
    XString m_description;
    Vector<XString> m_bosses;
    Vector<XString> m_friends;
    Vector<SReputationChanges> m_reputationChanges;
    Vector<XString> m_enemies;
    mutable Vector<XString*> m_eligibleBosses;
    const char* m_smallIcon;
};

#endif
