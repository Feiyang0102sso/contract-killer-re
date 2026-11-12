#ifndef CSQUAD_H
#define CSQUAD_H

#include "HashTable.h"
#include "MathLib.h"

class CUnit;

class CSquad {
public:
    CSquad();
    void AddLeader(CUnit* unit);
    void AddMember(CUnit* unit);
    void NoticeUnitParams(CUnit* unit);
    void LeaveSquad(CUnit* unit);
    vec3 GetLocalSquadPosition(CUnit* unit);

    HashTable<HashTableKey<const CUnit*>, vec3> m_members;
    vec3 m_position;                    // 0x0c
    vec3 m_right;
    vec3 m_forward;
    CUnit* m_leader;
    bool m_hasLeader;
};

#endif
