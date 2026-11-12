// CNGSFriendInviteeList.h - set of friend ids of one social network, in insertion order.
// The ids live in a CLinkList (node data = heap CStrWChar*) and are indexed by
// CStringToKey(id) in a CHash (value = the list node).
#ifndef CNGSFRIENDINVITEELIST_H
#define CNGSFRIENDINVITEELIST_H

#include "CLinkList.h"
#include "CHash.h"
#include "CStrWChar.h"

class CFriendInviteeList {
public:
    CFriendInviteeList();

    /** Forget every id (the strings and nodes are not freed). */
    void Clear();
    /** Append `id` unless it is already there. */
    void Add(CStrWChar id);
    unsigned char Exists(const CStrWChar& id);
    int Size();
    /** Unlink `id` from the list and the index. */
    void RemoveElement(const CStrWChar& id);

    CLinkList m_list;     // +0x00 node data: CStrWChar*
    CHash* m_index; // +0x10 CStringToKey(id) -> node
};

#endif
