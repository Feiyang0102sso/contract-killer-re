#ifndef CSQUADTYPEMANAGER_H
#define CSQUADTYPEMANAGER_H

#include "HashTable.h"
#include "xString.h"

class CSquadType;

class CSquadTypeManager {
public:
    CSquadTypeManager();
    virtual ~CSquadTypeManager();
    CSquadType* GetType(const XString& name) const;

private:
    HashTable<XString, CSquadType*> m_squads;
};

#endif
