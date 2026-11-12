// CPathCalculator.h - path queries of the units, forwarded to the AI map.
#ifndef CPATHCALCULATOR_H
#define CPATHCALCULATOR_H

#include <stddef.h>

class CGameAIMap;
class CPath;

class CPathCalculator {
public:
    CPathCalculator();
    virtual ~CPathCalculator();

    void Init(CGameAIMap* map);
    bool FindPath(CPath* path, int from, int to);
    bool FindPathToNearestExitPoint(CPath* path, int from);
    /** Path to the nearest free attack point; the body is then registered in the end point. */
    bool FindPathToNearestAttackPoint(CPath* path, int from, int target, bool ignoreOccupied);

private:
    CGameAIMap* m_map;  // 0x04
};

#endif
