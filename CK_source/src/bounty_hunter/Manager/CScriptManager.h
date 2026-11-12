// PROVISIONAL (created by b057x): owner src/bounty_hunter/Manager/CScriptManager.cpp not written yet
// CScriptManager.h - originally described as Lua scripts in the partial declaration.
// Restored by b079: the two preceding comments describe the earlier partial declaration.
#ifndef CSCRIPTMANAGER_H
#define CSCRIPTMANAGER_H

#include <stddef.h>
#include "xString.h"  // added by b078b
#include "Vector_ss.h"  // added by b078b
#include "HashTable.h"

class CScriptType;

class CScriptManager {
public:
    /** Eight-byte parameter, verified by vector copy/destruction at 0x1364b4/0x12d718. */  // added by b078b
    struct SScriptParam {
        XString m_name;
        XString m_value;
    };
    static void ParseScriptParams(Vector<SScriptParam>& params, XString text);  // added by b078b
    CScriptManager();
    virtual ~CScriptManager();
    CScriptType* GetType(const XString& name) const;

    // Former m_data04[0x14 - 0x4]: layout not known yet (size from App.cpp).
    HashTable<XString, CScriptType*> m_scripts; // +4; owns the waypoint scripts from scripts.xml.
};

#endif
