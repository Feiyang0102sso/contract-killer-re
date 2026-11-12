#include "CSquad.h"

#include "CUnit.h"

namespace {
const vec3 MemberOffsets[] = {
    vec3(1.5f, -1.5f, 0.0f), vec3(-1.5f, -1.5f, 0.0f),
    vec3(1.5f, 1.5f, 0.0f), vec3(-1.5f, 1.5f, 0.0f),
    vec3(0.0f, -1.5f, 0.0f), vec3(0.0f, 1.5f, 0.0f)
};
}

CSquad::CSquad() : m_members(8), m_position(0.0f, 0.0f, 0.0f), m_right(0.0f, 0.0f, 0.0f),
                   m_forward(0.0f, 0.0f, 0.0f), m_leader(NULL), m_hasLeader(false)
{
}

void CSquad::AddLeader(CUnit* unit)
{
    if (unit != NULL) {
        m_leader = unit;
        m_members.Add(HashTableKey<const CUnit*>(unit), vec3(0.0f, 0.0f, 0.0f));
        unit->SetSquad(this);
        m_hasLeader = true;
    }
}

void CSquad::AddMember(CUnit* unit)
{
    static int index = 0;
    if (unit != NULL) {
        m_members.Add(HashTableKey<const CUnit*>(unit), MemberOffsets[index]);
        index = (index + 1) % 6;
        unit->SetSquad(this);
    }
}

void CSquad::NoticeUnitParams(CUnit* unit)
{
    if (unit != NULL && m_leader == unit) {
        m_position = unit->m_body.m_position;
        vec3 direction = unit->m_body.m_target - m_position;
        float squared = direction.LengthSqr();
        float length = squared * MathLib::InvSqrt(squared);
        if (length >= 0.001f) {
            float inverse = MathLib::InvSqrt(squared);
            m_forward = vec3(direction.m_x * inverse, direction.m_y * inverse, 0.0f);
            m_right = vec3(m_forward.m_y, -m_forward.m_x, 0.0f);
        }
    }
}

void CSquad::LeaveSquad(CUnit* unit)
{
    if (unit != NULL && m_leader == unit) {
        m_leader = NULL;
        m_hasLeader = false;
    }
}

vec3 CSquad::GetLocalSquadPosition(CUnit* unit)
{
    if (unit != NULL) {
        const vec3* offset = m_members.Get(HashTableKey<const CUnit*>(unit));
        if (offset != NULL) {
            return m_position + m_right * offset->m_x + m_forward * offset->m_y;
        }
    }
    return m_position;
}

typedef char AssertSquadSize[sizeof(CSquad) == 0x38 ? 1 : -1];
