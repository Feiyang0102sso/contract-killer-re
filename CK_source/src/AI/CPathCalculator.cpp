// CPathCalculator.cpp - path queries forwarded to the AI map (all fail without a map).
#include "CPathCalculator.h"
#include "CGameAIMap.h"
#include "CPath.h"

CPathCalculator::CPathCalculator()
{
    m_map = NULL;
}

CPathCalculator::~CPathCalculator()
{
}

void CPathCalculator::Init(CGameAIMap* map)
{
    m_map = map;
}

bool CPathCalculator::FindPath(CPath* path, int from, int to)
{
    if (m_map == NULL) {
        return false;
    }
    return m_map->FindPath(path, from, to);
}

bool CPathCalculator::FindPathToNearestExitPoint(CPath* path, int from)
{
    if (m_map == NULL) {
        return false;
    }
    return m_map->FindPathToNearestExitPoint(path, from);
}

bool CPathCalculator::FindPathToNearestAttackPoint(CPath* path, int from, int target, bool ignoreOccupied)
{
    if (m_map == NULL) {
        return false;
    }
    if (m_map->FindPathToNearestAttackPoint(path, from, target, ignoreOccupied) != true) {
        return false;
    }
    CUnitBody* body = path->m_body;
    m_map->UnregisterUnitBody(body);
    m_map->RegisterUnitBodyInPoint(body, path->m_targetPoint);
    return true;
}
