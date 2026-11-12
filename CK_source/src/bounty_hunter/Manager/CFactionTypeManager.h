#ifndef CFACTIONTYPEMANAGER_H
#define CFACTIONTYPEMANAGER_H

#include "HashTable.h"
#include "Vector_ss.h"

class CFactionType;

class CFactionTypeManager {
public:
    CFactionTypeManager();
    virtual ~CFactionTypeManager();
    CFactionType* GetType(const XString& name) const;
    CFactionType* GetRandomFaction() const;
    CFactionType* GetFactionAtIndex(int index) const;
    void GetFactionTypeNames(Vector<XString*>& names);

    HashTable<XString, CFactionType*> m_factions;
};

#endif
