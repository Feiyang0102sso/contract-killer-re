// PROVISIONAL (created by b057x): owner src/bounty_hunter/Manager/CNationalityTypeManager.cpp not written yet
// CNationalityTypeManager.h - nationality types; only what src/general/App.cpp uses is declared.
// b078: the owned record layout follows HashTable<...>::DeleteContents in the original.
#ifndef CNATIONALITYTYPEMANAGER_H
#define CNATIONALITYTYPEMANAGER_H

#include <stddef.h>
#include "HashTable.h"
#include "Vector_ss.h"

/** Nationality name and three string lists; the individual list roles are not yet identified. */
// game1: the record moved to CNationalityType.h (owner src/bounty_hunter/Manager/CNationalityType.cpp):
// m_name 0x00, first names man 0x04, first names woman 0x14, surnames 0x24.
#include "CNationalityType.h"  // added by game1

class CNationalityTypeManager {
public:
    CNationalityTypeManager();
    virtual ~CNationalityTypeManager();
    /** Nationality named `name` (NULL when absent). */  // added by game1
    CNationalityType* GetType(const XString& name) const;  // added by game1

    // Former m_data04: layout not known yet (size from App.cpp).
    HashTable<XString, CNationalityType*> m_types;  // 0x04
};

#endif
