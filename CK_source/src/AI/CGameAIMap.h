// Navigation mesh and special points loaded from a CK location model.
#ifndef CGAMEAIMAP_H
#define CGAMEAIMAP_H

#include "CFixedVector.h"
#include "MathLib.h"
#include "Vector_ss.h"
#include "tws.h"

class CPath;
class CUnitBody;

class CGameAIMap {
public:
    typedef ::World World;
    typedef ::Node Node;

    struct SPointAI {
        vec3 m_position;
        bool m_boundary;
    };

    struct SGateAI {
        int m_id;
        SPointAI* m_start;
        SPointAI* m_end;
        float m_cost;
    };

    struct SCellAI;

    struct SCameraAttitude {
        int m_safetyType;
        int m_startLevel;
        bool m_visible;
        bool m_safe;
    };

    struct SSpecialPointAI {
        int m_id;
        Vector<SCameraAttitude> m_cameraAttitudes;
        vec3 m_position;
        int m_type;
        SCellAI* m_cell;
        const CUnitBody* m_occupant;
    };

    struct SCellAI {
        int m_field00;
        int m_field04;
        int m_id;
        int m_meshPart;
        int m_vertexIds[3];
        SPointAI* m_vertices[3];
        vec3 m_center;
        SGateAI* m_gates[3];
        SGateAI* m_boundaries[3];
        SCellAI* m_neighbours[3];
        Vector<SSpecialPointAI*> m_specialPoints;
        float m_parentCost;
        SCellAI* m_parent;
        int m_searchState;
        float m_distance;
        float m_priority;
    };

    void Init(World& world);
    void LoadNavigationMesh(World& world);
    void CreateNavigationMeshVertices(World& world);
    void CreateNavigationMeshCells(World& world);
    void CreateNavigationMeshGates();
    void CreateNavigationMeshCohesion();
    void LoadSpecialPoint(World& world);
    void BindPointToCell();
    void FindSpecialPoints();
    void DefineCameraCharacteristics(SSpecialPointAI* point, Node& node);

    int GetAnyPathPointID() const;
    int GetAnySwarmPointID() const;
    int GetAnyAttackPointID(int camera) const;
    int GetPointIDImpl(const Vector<int>& ids) const;
    const vec3& GetMapPointPostition(int point) const;
    int GetCellIdByPointId(int point) const;
    int GetHideBehaviour(int point, int camera) const;
    bool FindAnotherPoint(int* point, int previous) const;
    bool FindAnotherAttackPoint(int camera, int* point, int previous) const;
    bool RegisterUnitBodyInPoint(const CUnitBody* body, int point);
    void UnregisterUnitBody(const CUnitBody* body);
    bool DoesGateCollideWithBody(SGateAI* gate, CUnitBody* body, vec3* nearest) const;
    bool DoesCellCollideWithBody(SCellAI* cell, CUnitBody* body) const;
    void RegisterStaticCollisionImpl(SCellAI* cell, CUnitBody* body) const;
    void RegisterStaticCollision(CUnitBody* body) const;

    bool FindPath(CPath* path, int from, int to);
    bool FindPathToNearestExitPoint(CPath* path, int from);
    bool FindPathToNearestAttackPoint(CPath* path, int from, int camera, bool ignoreOccupied);
    bool FindPathAStarImpl(CPath* path, SCellAI* start, SSpecialPointAI* end);
    void DefineGatesForPath(CPath* path);
    void ConstructPrecisePath(CPath* path);
    void SmoothPath(CPath* path);
    bool IsPointExit(const CPath* path, const SSpecialPointAI* point, int camera);
    bool IsPointSafetyForAttackCamera(const CPath* path, const SSpecialPointAI* point, int camera);
    bool IsPointSuitableForAttackCamera(const CPath* path, const SSpecialPointAI* point, int camera);
    typedef bool (CGameAIMap::*PointCondition)(const CPath*, const SSpecialPointAI*, int);
    bool FindPathDijkstraImpl(CPath* path, SCellAI* start, PointCondition condition, int camera);
    bool DoesCellHaveAppropriatePoint(SSpecialPointAI** found, CPath* path, SCellAI* cell, PointCondition condition,
                                      int camera);

    vec3 PutPointToCell(const SCellAI& cell, const vec3& position) const;
    static bool IsPositionTheSame(const vec3& a, const vec3& b);
    static bool IsPointTheSame(const SPointAI* a, const SPointAI* b);
    SGateAI* FindGate(const SPointAI* a, const SPointAI* b);
    SCellAI* FindNeighbour(const SCellAI* cell, const SGateAI* gate);
    void AStarInit();
    void DijkstraInit();
    SCellAI* GetNewWorkCell();
    SCellAI* OpenCellListPop();
    void OpenCellListPushBack(SCellAI* cell, SCellAI* target, SCellAI* parent, float distance);

    Vector<SPointAI> m_vertices;                         // 0x00
    Vector<SGateAI> m_gates;                             // 0x10
    Vector<SGateAI> m_boundaries;                        // 0x20
    Vector<SCellAI> m_cells;                             // 0x30
    Vector<SSpecialPointAI> m_specialPoints;             // 0x40
    CFixedVector<SCellAI*> m_openCells;                  // 0x50
    Vector<int> m_pathPoints;                           // 0x5c
    Vector<int> m_swarmPoints;                          // 0x6c
    Vector<int> m_exitPoints;                           // 0x7c
    Vector<Vector<int> > m_attackPoints;                // 0x8c
    static unsigned long long m_byPassSessionID;
};

#endif
