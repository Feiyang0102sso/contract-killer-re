#include "CGameAIMap.h"
#include "Point.h"

#include <math.h>
#include <stdio.h>
#include "App.h"
#include "CBH_Player.h"
#include "CPath.h"
#include "CSwerveGame.h"
#include "CRandGen.h"
#include "CUnitBody.h"
#include "DGHelper.h"
#include "ICStdUtil.h"

namespace {
typedef ::Group Group;
typedef ::Mesh Mesh;
typedef ::Transform Transform;
typedef int (*GetIndexedObject)(IQueryInterface*, int, IQueryInterface**);
typedef int (*GetCount)(IQueryInterface*, int*);
typedef int (*GetIndexValues)(IQueryInterface*, int, int*);

const vec3 EmptyPosition;

vec3 PlanarOffset(const vec3& position, const vec3& origin)
{
    return vec3(position.m_x - origin.m_x, position.m_y - origin.m_y, 0.0f);
}

vec3 NormalizePlanar(const vec3& offset)
{
    return offset * MathLib::InvSqrt(offset.LengthSqr());
}

float CrossXY(const vec3& a, const vec3& b)
{
    return a.m_x * b.m_y - a.m_y * b.m_x;
}

bool ChooseOther(const Vector<int>& points, int previous, int* result)
{
    if (points.size() == 0) {
        return false;
    }
    int start = CRandGen::GetInstance()->GetRandRange(0, points.size() - 1);
    for (int index = 0; index < points.size(); ++index) {
        int candidate = points[(start + index) % points.size()];
        if (candidate != previous) {
            *result = candidate;
            return true;
        }
    }
    return false;
}
}

unsigned long long CGameAIMap::m_byPassSessionID;

void CGameAIMap::Init(World& world)
{
    LoadNavigationMesh(world);
    LoadSpecialPoint(world);
    BindPointToCell();
    FindSpecialPoints();
}

void CGameAIMap::LoadNavigationMesh(World& world)
{
    CreateNavigationMeshVertices(world);
    CreateNavigationMeshCells(world);
    CreateNavigationMeshGates();
    CreateNavigationMeshCohesion();
}

void CGameAIMap::CreateNavigationMeshVertices(World& world)
{
    if (world == NULL) {
        return;
    }
    Group root(world.find(DGHelper::getSwerveID("navigation_meshTransform")));
    if (root == NULL) {
        return;
    }
    Mesh mesh(world.find(DGHelper::getSwerveID("navigation_mesh")));
    if (mesh == NULL) {
        return;
    }
    Transform toWorld(CSwerve::GetInstance()->m_swerve);
    CSwerveGame* game = static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame;
    mesh.getTransformTo(game->m_world, toWorld);
    VertexBuffer vertexBuffer = mesh.getVertexBuffer();
    if (vertexBuffer != NULL) {
        float scaleBias[4];
        VertexArray positions = vertexBuffer.getPositions(scaleBias);
        if (positions != NULL) {
            int count = positions.getVertexCount();
            m_vertices.ensureCapacity(count);
            short* values = new short[count * 3];
            positions.get(0, count, count * 3, values);
            for (int index = 0; index < count; ++index) {
                float point[4];
                point[0] = values[index * 3] * scaleBias[0] + scaleBias[1];
                point[1] = values[index * 3 + 1] * scaleBias[0] + scaleBias[2];
                point[2] = values[index * 3 + 2] * scaleBias[0] + scaleBias[3];
                point[3] = 1.0f;
                toWorld.transform(4, point);
                SPointAI vertex;
                vertex.m_position.m_x = point[0];
                vertex.m_position.m_y = point[1];
                vertex.m_position.m_z = point[2];
                vertex.m_boundary = false;
                m_vertices.addElement(vertex);
            }
            delete[] values;
        }
    }
}

void CGameAIMap::CreateNavigationMeshCells(World& world)
{
    if (world == NULL) {
        return;
    }
    Mesh mesh(world.find(DGHelper::getSwerveID("navigation_mesh")));
    if (mesh == NULL) {
        return;
    }
    mesh.setRenderingEnable(false);
    mesh.setPickingEnable(false);
    Vector<int> canonical;
    int vertexCount = m_vertices.size();
    canonical.ensureCapacity(vertexCount);
    for (int index = 0; index < vertexCount; ++index) {
        canonical.addElement(index);
    }
    for (int index = 0; index < vertexCount; ++index) {
        for (int other = 0; other < vertexCount; ++other) {
            if (index != other && canonical[index] == index &&
                IsPointTheSame(&m_vertices[index], &m_vertices[other])) {
                canonical[other] = index;
            }
        }
    }

    int meshParts;
    CKMethod<GetCount>(mesh.m_interface, 0xc0)(mesh.m_interface, &meshParts);
    int totalIndices = 0;
    for (int part = 0; part < meshParts; ++part) {
        IQueryInterface* indices;
        CKMethod<GetIndexedObject>(mesh.m_interface, 0xc8)(mesh.m_interface, part, &indices);
        if (indices != NULL) {
            int count;
            CKMethod<GetCount>(indices, 0x4c)(indices, &count);
            totalIndices += count;
            indices->Release();
        }
    }
    m_openCells.reserve(totalIndices / 3);
    m_cells.ensureCapacity(totalIndices / 3);
    for (int part = 0; part < meshParts; ++part) {
        IQueryInterface* indices;
        CKMethod<GetIndexedObject>(mesh.m_interface, 0xc8)(mesh.m_interface, part, &indices);
        if (indices == NULL) {
            continue;
        }
        int count;
        CKMethod<GetCount>(indices, 0x4c)(indices, &count);
        if (count % 3 == 0) {
            int* values = new int[count];
            CKMethod<GetIndexValues>(indices, 0x50)(indices, count, values);
            for (int index = 0; index < count; index += 3) {
                int a = values[index];
                int b = values[index + 1];
                int c = values[index + 2];
                if (a < 0 || b < 0 || c < 0 || a >= m_vertices.size() || b >= m_vertices.size() ||
                    c >= m_vertices.size() || a == b || a == c || b == c) {
                    continue;
                }
                a = canonical[a];
                b = canonical[b];
                c = canonical[c];
                SCellAI cell;
                cell.m_field00 = 0;
                cell.m_field04 = 0;
                cell.m_id = m_cells.size();
                cell.m_meshPart = part;
                cell.m_vertexIds[0] = a;
                cell.m_vertexIds[1] = b;
                cell.m_vertexIds[2] = c;
                cell.m_vertices[0] = &m_vertices[a];
                cell.m_vertices[1] = &m_vertices[b];
                cell.m_vertices[2] = &m_vertices[c];
                cell.m_center = vec3(0.0f, 0.0f, 0.0f);
                for (int side = 0; side < 3; ++side) {
                    cell.m_center.m_x += cell.m_vertices[side]->m_position.m_x;
                    cell.m_center.m_y += cell.m_vertices[side]->m_position.m_y;
                    cell.m_center.m_z += cell.m_vertices[side]->m_position.m_z;
                }
                cell.m_center.m_x /= 3.0f;
                cell.m_center.m_y /= 3.0f;
                cell.m_center.m_z /= 3.0f;
                m_cells.addElement(cell);
            }
            delete[] values;
        }
        indices->Release();
    }
}

bool CGameAIMap::IsPositionTheSame(const vec3& a, const vec3& b)
{
    // The original wraps both positions in temporary points.
    SPointAI first;
    first.m_position = a;
    SPointAI second;
    second.m_position = b;
    return IsPointTheSame(&first, &second);
}

bool CGameAIMap::IsPointTheSame(const SPointAI* a, const SPointAI* b)
{
    return a != NULL && b != NULL && MathLib::Sqrt((a->m_position - b->m_position).LengthSqr()) < 0.001f;
}

CGameAIMap::SGateAI* CGameAIMap::FindGate(const SPointAI* a, const SPointAI* b)
{
    if (a != NULL && b != NULL) {
        for (int index = 0; index < m_gates.size(); ++index) {
            SGateAI* gate = &m_gates[index];
            const SPointAI* start = gate->m_start;
            const SPointAI* end = gate->m_end;
            if ((start == a && end == b) || (end == a && start == b)) {
                return gate;
            }
        }
    }
    return NULL;
}

CGameAIMap::SCellAI* CGameAIMap::FindNeighbour(const SCellAI* cell, const SGateAI* gate)
{
    if (cell != NULL && gate != NULL) {
        for (int index = 0; index < m_cells.size(); ++index) {
            SCellAI& candidate = m_cells[index];
            if (&candidate == cell) {
                continue;
            }
            // The neighbour shares the gate object itself.
            for (int side = 0; side < 3; ++side) {
                if (candidate.m_gates[side] == gate) {
                    return &candidate;
                }
            }
        }
    }
    return NULL;
}

void CGameAIMap::CreateNavigationMeshGates()
{
    int cellCount = m_cells.size();
    Vector<CPoint> edges;
    edges.ensureCapacity(cellCount * 3);
    for (int index = 0; index < cellCount; ++index) {
        const SCellAI& cell = m_cells[index];
        for (int otherIndex = 0; otherIndex < cellCount; ++otherIndex) {
            if (index == otherIndex) {
                continue;
            }
            const SCellAI& other = m_cells[otherIndex];
            bool shared[3];
            int sharedCount = 0;
            for (int side = 0; side < 3; ++side) {
                int vertexId = cell.m_vertexIds[side];
                shared[side] = vertexId == other.m_vertexIds[0] || vertexId == other.m_vertexIds[1] ||
                               vertexId == other.m_vertexIds[2];
                if (shared[side]) {
                    ++sharedCount;
                }
            }
            if (sharedCount != 2) {
                continue;
            }
            CPoint edge(-1, -1);
            for (int side = 0; side < 3; ++side) {
                if (shared[side]) {
                    if (edge.m_x == -1) {
                        edge.m_x = cell.m_vertexIds[side];
                    } else {
                        edge.m_y = cell.m_vertexIds[side];
                    }
                }
            }
            if (edge.m_x == edge.m_y) {
                continue;
            }
            int edgeIndex = 0;
            for (; edgeIndex < edges.size(); ++edgeIndex) {
                const CPoint& existing = edges[edgeIndex];
                if ((existing.m_x == edge.m_x && existing.m_y == edge.m_y) ||
                    (existing.m_x == edge.m_y && existing.m_y == edge.m_x)) {
                    break;
                }
            }
            if (edgeIndex == edges.size()) {
                edges.addElement(edge);
            }
        }
    }

    int edgeCount = edges.size();
    if (edgeCount > 0) {
        m_gates.ensureCapacity(edgeCount);
        for (int index = 0; index < edgeCount; ++index) {
            SPointAI* start = m_vertices.m_items + edges[index].m_x;
            SPointAI* end = m_vertices.m_items + edges[index].m_y;
            if (start != NULL && end != NULL) {
                SGateAI gate;
                gate.m_id = index;
                gate.m_start = start;
                gate.m_end = end;
                m_gates.addElement(gate);
            }
        }
    }
}

void CGameAIMap::CreateNavigationMeshCohesion()
{
    int cellCount = m_cells.size();
    int boundaryCount = 0;
    for (int index = 0; index < cellCount; ++index) {
        SCellAI& cell = m_cells[index];
        for (int side = 0; side < 3; ++side) {
            SPointAI* a = cell.m_vertices[side];
            SPointAI* b = cell.m_vertices[(side + 1) % 3];
            cell.m_gates[side] = FindGate(a, b);
            if (cell.m_gates[side] == NULL) {
                ++boundaryCount;
            }
        }
    }
    m_boundaries.ensureCapacity(boundaryCount);
    for (int index = 0; index < cellCount; ++index) {
        SCellAI& cell = m_cells[index];
        for (int side = 0; side < 3; ++side) {
            SGateAI* boundaryGate = NULL;
            if (cell.m_gates[side] == NULL) {
                SGateAI boundary;
                boundary.m_start = cell.m_vertices[side];
                boundary.m_end = cell.m_vertices[(side + 1) % 3];
                m_boundaries.addElement(boundary);
                boundaryGate = &m_boundaries[m_boundaries.size() - 1];
            }
            cell.m_boundaries[side] = boundaryGate;
        }
    }
    for (int index = 0; index < boundaryCount; ++index) {
        m_boundaries[index].m_start->m_boundary = true;
        m_boundaries[index].m_end->m_boundary = true;
    }
    for (int index = 0; index < cellCount; ++index) {
        SCellAI& cell = m_cells[index];
        for (int side = 0; side < 3; ++side) {
            SGateAI* gate = cell.m_gates[side];
            if (gate != NULL) {
                SCellAI* neighbour = FindNeighbour(&cell, gate);
                cell.m_neighbours[side] = neighbour;
                if (neighbour != NULL) {
                    cell.m_gates[side]->m_cost = MathLib::Sqrt((cell.m_center - neighbour->m_center).LengthSqr());
                }
            } else {
                cell.m_neighbours[side] = NULL;
            }
        }
    }
}

void CGameAIMap::LoadSpecialPoint(World& world)
{
    if (world == NULL) {
        return;
    }
    Group root(world.find(DGHelper::getSwerveID("AI_RootTransform")));
    if (root == NULL) {
        return;
    }
    int count = root.getChildCount();
    m_specialPoints.ensureCapacity(count);
    for (int index = 0; index < count; ++index) {
        Node node = root.getChild(index);
        if (node == NULL) {
            break;
        }
        // the original appends an empty point first and fills the stored copy
        SSpecialPointAI emptyPoint;
        m_specialPoints.addElement(emptyPoint);
        SSpecialPointAI& point = m_specialPoints[m_specialPoints.size() - 1];
        float position[3];
        node.getTranslation(position);
        point.m_position.m_x = position[0];
        point.m_position.m_y = position[1];
        point.m_position.m_z = position[2];
        point.m_id = index;
        DefineCameraCharacteristics(&point, node);
    }
}

// Original 0xcde7c: the parameters are read through a 1000-byte name buffer; "ID" is read and ignored.
void CGameAIMap::DefineCameraCharacteristics(SSpecialPointAI* point, Node& node)
{
    char name[1000];
    int type;
    int id;
    int cameraCount;
    SCameraAttitude attitude;
    int visible;
    int safe;
    int startLevel;
    ICStdUtil::SPrintF(name, "PointType");
    static_cast<App*>(WindowApp::m_instance)->m_helper->getUserParameter(node, name, &type);
    int pointType = type;
    if (pointType < 0) {
        pointType = 0;
    }
    point->m_type = pointType;
    ICStdUtil::SPrintF(name, "ID");
    static_cast<App*>(WindowApp::m_instance)->m_helper->getUserParameter(node, name, &id);
    ICStdUtil::SPrintF(name, "SceneCameraCount");
    static_cast<App*>(WindowApp::m_instance)->m_helper->getUserParameter(node, name, &cameraCount);
    if (cameraCount > 0) {
        point->m_cameraAttitudes.ensureCapacity(cameraCount);
        for (int camera = 0; camera < cameraCount; ++camera) {
            ICStdUtil::SPrintF(name, "cameraVisibility_%d", camera);
            static_cast<App*>(WindowApp::m_instance)->m_helper->getUserParameter(node, name, &visible);
            attitude.m_visible = visible == 1;
            ICStdUtil::SPrintF(name, "cameraSafity_%d", camera);
            static_cast<App*>(WindowApp::m_instance)->m_helper->getUserParameter(node, name, &safe);
            attitude.m_safe = safe == 1;
            ICStdUtil::SPrintF(name, "cameraSafityType_%d", camera);
            static_cast<App*>(WindowApp::m_instance)->m_helper->getUserParameter(node, name, &attitude.m_safetyType);
            ICStdUtil::SPrintF(name, "cameraStartLevel_%d", camera);
            static_cast<App*>(WindowApp::m_instance)->m_helper->getUserParameter(node, name, &startLevel);
            if (startLevel < 1) {
                startLevel = 1;
            }
            attitude.m_startLevel = startLevel;
            point->m_cameraAttitudes.addElement(attitude);
        }
    }
}

vec3 CGameAIMap::PutPointToCell(const SCellAI& cell, const vec3& position) const
{
    const vec3& a = cell.m_vertices[0]->m_position;
    const vec3& b = cell.m_vertices[1]->m_position;
    const vec3& c = cell.m_vertices[2]->m_position;
    Plane plane(a, b, c, true);
    const vec3& normal = plane.m_normal;
    vec3 point = position;
    if (normal.m_z != 0.0f) {
        point.m_z = (normal.m_x * (a.m_x - position.m_x) +
                     normal.m_y * (a.m_y - position.m_y)) / normal.m_z + a.m_z;
    }
    return point;
}

void CGameAIMap::BindPointToCell()
{
    for (int pointIndex = 0; pointIndex < m_specialPoints.size(); ++pointIndex) {
        SSpecialPointAI& point = m_specialPoints[pointIndex];
        float closestDistance = 1.0e18f;
        int closestCell = -1;
        for (int cellIndex = 0; cellIndex < m_cells.size(); ++cellIndex) {
            SCellAI& cell = m_cells[cellIndex];
            vec3 first = cell.m_vertices[0]->m_position;
            vec3 second = cell.m_vertices[1]->m_position;
            vec3 third = cell.m_vertices[2]->m_position;
            Plane plane(first, second, third, true);
            first.m_z = 0.0f;
            second.m_z = 0.0f;
            third.m_z = 0.0f;
            Plane flatPlane(first, second, third, true);
            vec3 projection = flatPlane.Projection(point.m_position);
            if (!Plane::IsProjectionBelongsToPlane(first, second, third, projection)) {
                continue;
            }
            float distance = fabs((double)plane.Dist(point.m_position));
            if (distance < closestDistance) {
                closestDistance = distance;
                closestCell = cellIndex;
            }
        }
        if (closestCell >= 0) {
            point.m_position = PutPointToCell(m_cells[closestCell], point.m_position);
            point.m_cell = &m_cells[closestCell];
        }
    }

    for (int cellIndex = 0; cellIndex < m_cells.size(); ++cellIndex) {
        SCellAI* cell = &m_cells[cellIndex];
        int pointCount = 0;
        for (int pointIndex = 0; pointIndex < m_specialPoints.size(); ++pointIndex) {
            if (m_specialPoints[pointIndex].m_cell == cell) {
                ++pointCount;
            }
        }
        cell->m_specialPoints.ensureCapacity(pointCount);
        for (int pointIndex = 0; pointIndex < m_specialPoints.size(); ++pointIndex) {
            SSpecialPointAI* point = &m_specialPoints[pointIndex];
            if (point->m_cell == cell) {
                m_cells[cellIndex].m_specialPoints.addElement(point);
            }
        }
    }
}

void CGameAIMap::FindSpecialPoints()
{
    int pathCount = 0;
    int swarmCount = 0;
    int exitCount = 0;
    int cameraCount = 0;
    int pointCount = m_specialPoints.size();
    for (int index = 0; index < pointCount; ++index) {
        const SSpecialPointAI& point = m_specialPoints[index];
        switch (point.m_type) {
        case 0:
            ++pathCount;
            break;
        case 1:
            ++swarmCount;
            break;
        case 2:
            ++exitCount;
            break;
        }
        if (cameraCount < point.m_cameraAttitudes.size()) {
            cameraCount = point.m_cameraAttitudes.size();
        }
    }
    m_attackPoints.ensureCapacity(cameraCount);
    m_pathPoints.ensureCapacity(pathCount);
    m_swarmPoints.ensureCapacity(swarmCount);
    m_exitPoints.ensureCapacity(exitCount);

    for (int index = 0; index < pointCount; ++index) {
        switch (m_specialPoints[index].m_type) {
        case 0:
            m_pathPoints.addElement(index);
            break;
        case 1:
            m_swarmPoints.addElement(index);
            break;
        case 2:
            m_exitPoints.addElement(index);
            break;
        }
    }

    int playerLevel = CBH_Player::GetInstance()->GetLevel();
    for (int camera = 0; camera < cameraCount; ++camera) {
        Vector<int> cameraPoints;
        m_attackPoints.addElement(cameraPoints);
    }
    for (int index = 0; index < pointCount; ++index) {
        SSpecialPointAI& point = m_specialPoints[index];
        if (point.m_type == 0) {
            for (int camera = 0; camera < point.m_cameraAttitudes.size(); ++camera) {
                SCameraAttitude& attitude = point.m_cameraAttitudes[camera];
                if (playerLevel < attitude.m_startLevel) {
                    attitude.m_visible = false;
                }
                if (attitude.m_visible) {
                    m_attackPoints[camera].addElement(index);
                }
            }
        }
    }
}

int CGameAIMap::GetPointIDImpl(const Vector<int>& ids) const
{
    if (m_specialPoints.size() > 0) {
        int count = ids.size();
        if (count > 0) {
            return ids[CRandGen::GetInstance()->GetRandRange(0, count - 1)];
        }
    }
    return -1;
}

int CGameAIMap::GetAnyPathPointID() const
{
    return GetPointIDImpl(m_pathPoints);
}

int CGameAIMap::GetAnySwarmPointID() const
{
    return GetPointIDImpl(m_swarmPoints);
}

int CGameAIMap::GetAnyAttackPointID(int camera) const
{
    // The original does not range check the camera index.
    return GetPointIDImpl(m_attackPoints[camera]);
}

const vec3& CGameAIMap::GetMapPointPostition(int point) const
{
    // The original does not range check the point index.
    return m_specialPoints[point].m_position;
}

int CGameAIMap::GetCellIdByPointId(int point) const
{
    int cellId = -1;
    if (point >= 0 && point < m_specialPoints.size()) {
        // The original does not check the cell pointer.
        cellId = m_specialPoints[point].m_cell->m_id;
    }
    return cellId;
}

int CGameAIMap::GetHideBehaviour(int point, int camera) const
{
    // The original does not range check either index.
    return m_specialPoints[point].m_cameraAttitudes[camera].m_safetyType;
}

// Original 0xcc6b8: random picks, at least one per special point, and then on until one differs from
// `previous` (an original bug: this never ends when no other point can be picked).
bool CGameAIMap::FindAnotherPoint(int* point, int previous) const
{
    int count = m_specialPoints.size();
    if (count < 2) {
        return false;
    }
    if (previous < 0 || previous >= count) {
        return false;
    }
    bool found = false;
    int attempt = 1;
    bool more = true;
    while (more || !found) {
        int id = GetPointIDImpl(m_pathPoints);
        if (id != previous) {
            *point = id;
            found = true;
        }
        more = attempt < count;
        attempt++;
    }
    return true;
}

// Original 0xcc72c: as FindAnotherPoint over the camera's attack points (no camera range check).
bool CGameAIMap::FindAnotherAttackPoint(int camera, int* point, int previous) const
{
    int count = m_specialPoints.size();
    if (count < 2) {
        return false;
    }
    if (previous < 0 || previous >= count) {
        return false;
    }
    bool found = false;
    int attempt = 1;
    bool more = true;
    while (more || !found) {
        int id = GetPointIDImpl(m_attackPoints[camera]);
        if (id != previous) {
            *point = id;
            found = true;
        }
        more = attempt < count;
        attempt++;
    }
    return true;
}

bool CGameAIMap::RegisterUnitBodyInPoint(const CUnitBody* body, int point)
{
    // The original does not range check the point index.
    SSpecialPointAI& special = m_specialPoints[point];
    if (special.m_type != 0 || special.m_occupant != NULL) {
        return false;
    }
    special.m_occupant = body;
    return true;
}

void CGameAIMap::UnregisterUnitBody(const CUnitBody* body)
{
    if (body != NULL) {
        for (int index = 0; index < m_pathPoints.size(); ++index) {
            SSpecialPointAI& point = m_specialPoints[m_pathPoints[index]];
            if (point.m_occupant == body) {
                point.m_occupant = NULL;
                break;
            }
        }
    }
}

bool CGameAIMap::DoesGateCollideWithBody(SGateAI* gate, CUnitBody* body, vec3* pushOut) const
{
    if (gate == NULL || body == NULL) {
        return false;
    }
    const vec3& start = gate->m_start->m_position;
    const vec3& end = gate->m_end->m_position;
    vec3 direction = NormalizePlanar(PlanarOffset(start, end));
    vec3 fromStart = NormalizePlanar(PlanarOffset(body->m_position, start));
    vec3 normal(-direction.m_y, direction.m_x, 0.0f);
    if (direction.Dot(fromStart) < 0.0f) {
        normal.m_x = direction.m_y;
        normal.m_y = -direction.m_x;
        normal.m_z = -0.0f;
    }
    fromStart = PlanarOffset(body->m_position, start);
    float distance = normal.m_x * fromStart.m_x + normal.m_y * fromStart.m_y;
    float radius = body->GetRadius();
    if (distance > radius) {
        return false;
    }

    vec3 endpoints[2];
    endpoints[0] = PlanarOffset(start, body->m_position);
    endpoints[1] = PlanarOffset(end, body->m_position);
    float lengthSquared = endpoints[0].LengthSqr();
    float lengths[2];
    lengths[0] = MathLib::InvSqrt(lengthSquared) * lengthSquared;
    lengthSquared = endpoints[1].LengthSqr();
    lengths[1] = MathLib::InvSqrt(lengthSquared) * lengthSquared;
    float sideProduct = CrossXY(normal, endpoints[0]) * CrossXY(normal, endpoints[1]);
    float penetration;
    if (sideProduct < 0.0f) {
        penetration = radius - distance;
    } else {
        if (!(lengths[0] < radius) && !(lengths[1] < radius)) {
            return false;
        }
        float endpointPenetration[2];
        endpointPenetration[0] = 0.0f;
        endpointPenetration[1] = 0.0f;
        for (int index = 0; index < 2; ++index) {
            if (lengths[index] < radius) {
                const vec3& endpoint = endpoints[index];
                float fraction = fabs(static_cast<double>(direction.Dot(endpoint))) / radius;
                float fractionSquared = fraction * fraction;
                float perpendicularSquared = 1.0f - fractionSquared;
                float scale = MathLib::InvSqrt(perpendicularSquared) * perpendicularSquared;
                float normalDistance = fabs(static_cast<double>(normal.Dot(endpoint)));
                endpointPenetration[index] = radius / scale - normalDistance;
            }
        }
        penetration = endpointPenetration[1];
        if (endpointPenetration[0] > penetration) {
            penetration = endpointPenetration[0];
        }
    }
    pushOut->m_x = -(normal.m_x * penetration);
    pushOut->m_y = -(normal.m_y * penetration);
    pushOut->m_z = -(normal.m_z * penetration);
    return true;
}

bool CGameAIMap::DoesCellCollideWithBody(SCellAI* cell, CUnitBody* body) const
{
    if (cell == NULL || body == NULL) {
        return false;
    }
    float positionX = body->m_position.m_x;
    float positionY = body->m_position.m_y;
    for (int edge = 0; edge < 3; ++edge) {
        const SGateAI* gate = cell->m_gates[edge];
        if (gate == NULL) {
            gate = cell->m_boundaries[edge];
        }
        const vec3& start = gate->m_start->m_position;
        const vec3& end = gate->m_end->m_position;
        const vec3& opposite = cell->m_vertices[(edge + 2) % 3]->m_position;
        vec3 direction = NormalizePlanar(PlanarOffset(start, end));
        vec3 normal(-direction.m_y, direction.m_x, 0.0f);
        if (CrossXY(direction, PlanarOffset(start, opposite)) < 0.0f) {
            normal = vec3(direction.m_y, -direction.m_x, 0.0f);
        }
        float distance = normal.m_x * (positionX - start.m_x) + normal.m_y * (positionY - start.m_y);
        if (distance > body->GetRadius()) {
            return false;
        }
    }

    for (int index = 0; index < 3; ++index) {
        const vec3& vertex = cell->m_vertices[index]->m_position;
        const vec3& next = cell->m_vertices[(index + 1) % 3]->m_position;
        const vec3& previous = cell->m_vertices[(index + 2) % 3]->m_position;
        vec3 bisector = NormalizePlanar(PlanarOffset(vertex, next)) + NormalizePlanar(PlanarOffset(vertex, previous));
        bisector = NormalizePlanar(bisector);
        float distance = bisector.m_x * (positionX - vertex.m_x) + bisector.m_y * (positionY - vertex.m_y);
        if (distance > body->GetRadius()) {
            return false;
        }
    }
    return true;
}

void CGameAIMap::RegisterStaticCollisionImpl(SCellAI* cell, CUnitBody* body) const
{
    if (cell == NULL) {
        return;
    }
    unsigned long long lastSession = static_cast<unsigned int>(cell->m_field00);
    lastSession |= static_cast<unsigned long long>(static_cast<unsigned int>(cell->m_field04)) << 32;
    unsigned long long session = m_byPassSessionID;
    if (lastSession == session) {
        return;
    }
    cell->m_field00 = static_cast<unsigned int>(session);
    cell->m_field04 = static_cast<unsigned int>(session >> 32);
    if (!DoesCellCollideWithBody(cell, body)) {
        return;
    }
    vec3 pushOut;
    pushOut.m_z = 0.0f;
    pushOut.m_y = 0.0f;
    pushOut.m_x = 0.0f;
    for (int side = 0; side < 3; ++side) {
        if (cell->m_boundaries[side] != NULL && DoesGateCollideWithBody(cell->m_boundaries[side], body, &pushOut)) {
            SGateAI* boundary = cell->m_boundaries[side];
            body->AddStaticObstacle(boundary->m_start->m_position, boundary->m_end->m_position);
        }
    }
    for (int side = 0; side < 3; ++side) {
        RegisterStaticCollisionImpl(cell->m_neighbours[side], body);
    }
}

void CGameAIMap::RegisterStaticCollision(CUnitBody* body) const
{
    int cellId = body->m_cellId;
    if (cellId >= 0 && cellId < m_cells.size()) {
        SCellAI* cell = m_cells.m_items + cellId;
        if (cell != NULL) {
            ++m_byPassSessionID;
            RegisterStaticCollisionImpl(cell, body);
        }
    }
}

void CGameAIMap::AStarInit()
{
    int count = m_cells.size();
    for (int index = 0; index < count; ++index) {
        SCellAI& cell = m_cells[index];
        cell.m_distance = 0.0f;
        cell.m_priority = 0.0f;
        cell.m_parentCost = 0.0f;
        cell.m_parent = NULL;
        cell.m_searchState = 0;
    }
    m_openCells.m_size = 0;
}

__attribute__((noinline)) void CGameAIMap::DijkstraInit()
{
    // Dijkstra marks unvisited cells with a negative cost (see GetNewWorkCell).
    int count = m_cells.size();
    for (int index = 0; index < count; ++index) {
        SCellAI& cell = m_cells[index];
        cell.m_parentCost = -10.0f;
        cell.m_parent = NULL;
        cell.m_searchState = 0;
    }
    m_openCells.m_size = 0;
}

__attribute__((noinline)) CGameAIMap::SCellAI* CGameAIMap::GetNewWorkCell()
{
    SCellAI* cell = NULL;
    int count = m_openCells.m_size;
    if (count > 0) {
        int bestIndex = 0;
        cell = m_openCells.m_data[0];
        if (count > 1) {
            float bestCost = cell->m_parentCost;
            for (int index = 1; index != count; ++index) {
                SCellAI* candidate = m_openCells.m_data[index];
                if (candidate->m_parentCost >= 0.0f && candidate->m_parentCost < bestCost) {
                    // Original bug: the cost is taken from the previous best cell.
                    bestCost = cell->m_parentCost;
                    bestIndex = index;
                    cell = candidate;
                }
            }
        }
        m_openCells.remove_at(bestIndex);
    }
    return cell;
}

CGameAIMap::SCellAI* CGameAIMap::OpenCellListPop()
{
    SCellAI* bestCell = NULL;
    int count = m_openCells.m_size;
    if (count > 0) {
        bestCell = m_openCells.m_data[0];
        int bestIndex = 0;
        if (count > 1) {
            float bestCost = bestCell->m_distance + bestCell->m_priority;
            for (int index = 1; index != count; ++index) {
                SCellAI* candidate = m_openCells.m_data[index];
                float cost = candidate->m_distance + candidate->m_priority;
                if (cost < bestCost) {
                    bestCost = cost;
                    bestCell = candidate;
                    bestIndex = index;
                }
            }
        }
        m_openCells.remove_at(bestIndex);
        bestCell->m_searchState = 2;
    }
    return bestCell;
}

bool CGameAIMap::DoesCellHaveAppropriatePoint(SSpecialPointAI** found, CPath* path, SCellAI* cell,
                                              PointCondition condition, int camera)
{
    for (int index = 0; index < cell->m_specialPoints.size(); ++index) {
        if ((this->*condition)(path, cell->m_specialPoints[index], camera) == true) {
            *found = cell->m_specialPoints[index];
            return true;
        }
    }
    *found = NULL;
    return false;
}

void CGameAIMap::OpenCellListPushBack(SCellAI* cell, SCellAI* target, SCellAI* parent, float distance)
{
    const float lengthSquared = (target->m_center - cell->m_center).LengthSqr();
    cell->m_parentCost = distance;
    cell->m_parent = parent;
    cell->m_searchState = 1;
    cell->m_priority = MathLib::InvSqrt(lengthSquared) * lengthSquared;
    cell->m_distance = cell->m_parentCost;
    if (parent != NULL) {
        cell->m_distance = cell->m_distance + parent->m_distance;
    }
    if (m_openCells.m_size < m_openCells.m_capacity) {
        m_openCells.m_data[m_openCells.m_size] = cell;
        ++m_openCells.m_size;
    }
}

bool CGameAIMap::FindPathAStarImpl(CPath* path, SCellAI* start, SSpecialPointAI* end)
{
    if (start == NULL || end == NULL || end->m_cell == NULL) {
        return false;
    }
    SCellAI* target = end->m_cell;
    if (start == target) {
        path->m_targetPoint = end->m_id;
        return true;
    }
    path->Clear();
    AStarInit();
    OpenCellListPushBack(start, target, NULL, 0.0f);
    bool found = false;
    while (m_openCells.m_size != 0 && !found) {
        SCellAI* cell = OpenCellListPop();
        for (int side = 0; side < 3; ++side) {
            SCellAI* next = cell->m_neighbours[side];
            if (next == NULL) {
                continue;
            }
            const SGateAI* gate = cell->m_gates[side];
            if (gate == NULL) {
                return false;
            }
            float distance = gate->m_cost;
            if (next->m_searchState == 0) {
                OpenCellListPushBack(next, target, cell, distance);
                if (next == target) {
                    found = true;
                }
            } else if (next->m_searchState == 1 && cell->m_distance + distance < next->m_distance) {
                // The original replaces the incoming edge cost in the existing total.
                float total = next->m_distance - next->m_parentCost + distance;
                next->m_parentCost = distance;
                next->m_distance = total;
                next->m_parent = cell;
            }
        }
    }
    if (!found) {
        return false;
    }
    path->m_targetPoint = end->m_id;
    for (SCellAI* cell = end->m_cell; cell != NULL;) {
        int cellId = cell->m_id;
        cell = cell->m_parent;
        path->AddCell(cellId);
    }
    return true;
}

// Original 0xcc9ac: breadth over the open list; the first neighbour with an appropriate point ends the
// search. The cell list is filled from the point's cell up the parent chain (AddCell overflow ignored).
// Both success branches assign the target ID twice.
bool CGameAIMap::FindPathDijkstraImpl(CPath* path, SCellAI* start, PointCondition condition, int camera)
{
    if (start == NULL) {
        return false;
    }
    DijkstraInit();
    start->m_parentCost = 0.0f;
    path->Clear();
    SSpecialPointAI* point;
    if (DoesCellHaveAppropriatePoint(&point, path, start, condition, camera)) {
        if (point == NULL) {
            return false;
        }
        path->m_targetPoint = point->m_id;
        path->m_targetPoint = point->m_id;
        for (SCellAI* cell = point->m_cell; cell != NULL;) {
            int cellId = cell->m_id;
            cell = cell->m_parent;
            path->AddCell(cellId);
        }
        return true;
    }
    SCellAI* cell = start;
    while (cell != NULL) {
        for (int side = 0; side < 3; ++side) {
            SCellAI* next = cell->m_neighbours[side];
            if (next == NULL || next->m_searchState == 2) {
                continue;
            }
            if (DoesCellHaveAppropriatePoint(&point, path, next, condition, camera)) {
                next->m_parent = cell;
                if (point == NULL) {
                    return false;
                }
                path->m_targetPoint = point->m_id;
                path->m_targetPoint = point->m_id;
                for (SCellAI* found = point->m_cell; found != NULL;) {
                    int cellId = found->m_id;
                    found = found->m_parent;
                    path->AddCell(cellId);
                }
                return true;
            }
            if (next->m_searchState != 1) {
                if (m_openCells.m_size < m_openCells.m_capacity) {
                    m_openCells.m_data[m_openCells.m_size] = next;
                    m_openCells.m_size++;
                }
                next->m_searchState = 1;
            }
            const SGateAI* gate = cell->m_gates[side];
            if (gate == NULL) {
                return false;
            }
            float cost = gate->m_cost;
            float parentCost = cell->m_parentCost;
            if (parentCost >= 0.0f) {
                cost += parentCost;
            }
            if (cost < next->m_parentCost || next->m_parentCost < 0.0f) {
                next->m_parentCost = cost;
                next->m_parent = cell;
            }
        }
        cell->m_searchState = 2;
        cell = GetNewWorkCell();
    }
    return false;
}

void CGameAIMap::ConstructPrecisePath(CPath* path)
{
    int target = path->m_targetPoint;
    if (target >= 0 && target < m_specialPoints.size()) {
        path->AddWayPoint(m_specialPoints[target].m_position, -1);
        DefineGatesForPath(path);
        SmoothPath(path);
    }
}

void CGameAIMap::DefineGatesForPath(CPath* path)
{
    // The original gate walk adjusts only the endpoints that touch the mesh boundary.
    int lastCell = path->m_cellCount - 1;
    for (int index = 0; index < lastCell; ++index) {
        int targetCell = path->GetCellID(index);
        int startCell = path->GetCellID(index + 1);
        if (targetCell < 0 || targetCell >= m_cells.size() || startCell < 0 || startCell >= m_cells.size()) {
            continue;
        }
        SCellAI* towardTarget = m_cells.m_items + targetCell;
        SCellAI* towardStart = m_cells.m_items + startCell;
        if (towardTarget == NULL || towardStart == NULL) {
            continue;
        }
        float radius = path->m_body->GetRadius();
        for (int side = 0; side < 3; ++side) {
            if (towardTarget->m_neighbours[side] != towardStart) {
                continue;
            }
            SGateAI& gate = *towardTarget->m_gates[side];
            vec3 across = gate.m_start->m_position - gate.m_end->m_position;
            float lengthSquared = across.LengthSqr();
            float length = MathLib::InvSqrt(lengthSquared) * lengthSquared;
            if (!(length < radius + radius)) {
                vec3 reverse = gate.m_end->m_position - gate.m_start->m_position;
                vec3 directions[2] = {
                    reverse * MathLib::InvSqrt(reverse.LengthSqr()),
                    across * MathLib::InvSqrt(lengthSquared)
                };
                vec3 endpoints[2] = {gate.m_start->m_position, gate.m_end->m_position};
                for (int endpoint = 0; endpoint < 2; ++endpoint) {
                    SPointAI* point = gate.m_start;
                    if (endpoint == 1) {
                        point = gate.m_end;
                    }
                    if (point->m_boundary) {
                        endpoints[endpoint] = endpoints[endpoint] + directions[endpoint] * radius;
                    }
                }
                path->AddGateWayPoint((endpoints[0] + endpoints[1]) * 0.5f,
                                      towardTarget->m_id, endpoints[0], endpoints[1]);
            } else {
                vec3 center = (gate.m_start->m_position + gate.m_end->m_position) * 0.5f;
                path->AddGateWayPoint(center, towardTarget->m_id, center, center);
            }
            break;
        }
    }
    if (path->m_body != NULL) {
        path->AddWayPoint(path->m_body->m_position, path->m_body->m_cellId);
    }
}

void CGameAIMap::SmoothPath(CPath* path)
{
    path->CreateSmoothInfo();
    if (path->m_smoothCount > 0) {
        path->Smooth();
    }
}

bool CGameAIMap::IsPointExit(const CPath*, const SSpecialPointAI* point, int)
{
    return point->m_type == 2;
}

bool CGameAIMap::IsPointSuitableForAttackCamera(const CPath* path, const SSpecialPointAI* point, int camera)
{
    // The original checks only the upper bound of the camera index.
    if (camera < point->m_cameraAttitudes.size()) {
        bool visible = point->m_cameraAttitudes[camera].m_visible;
        if (path != NULL && path->m_body != point->m_occupant && point->m_occupant != NULL) {
            return false;
        }
        return visible;
    }
    return false;
}

bool CGameAIMap::IsPointSafetyForAttackCamera(const CPath* path, const SSpecialPointAI* point, int camera)
{
    bool safe = false;
    if (camera < point->m_cameraAttitudes.size()) {
        safe = point->m_cameraAttitudes[camera].m_visible && point->m_cameraAttitudes[camera].m_safe;
        if (path != NULL && path->m_body != point->m_occupant && point->m_occupant != NULL) {
            safe = false;
        }
    }
    return safe;
}

bool CGameAIMap::FindPath(CPath* path, int from, int to)
{
    bool found = false;
    // The original checks only the start cell and builds the precise path even when the search fails.
    if (from >= 0 && from < m_cells.size()) {
        found = FindPathAStarImpl(path, &m_cells[from], &m_specialPoints[to]);
        ConstructPrecisePath(path);
    }
    return found;
}

bool CGameAIMap::FindPathToNearestExitPoint(CPath* path, int from)
{
    bool found = false;
    if (from >= 0 && from < m_cells.size()) {
        // The precise path is built even when the search fails (original behaviour).
        found = FindPathDijkstraImpl(path, &m_cells[from], &CGameAIMap::IsPointExit, 0);
        ConstructPrecisePath(path);
    }
    return found;
}

bool CGameAIMap::FindPathToNearestAttackPoint(CPath* path, int from, int camera, bool safeOnly)
{
    if (from < 0 || from >= m_cells.size()) {
        return false;
    }
    PointCondition condition = &CGameAIMap::IsPointSuitableForAttackCamera;
    if (safeOnly) {
        condition = &CGameAIMap::IsPointSafetyForAttackCamera;
    }
    if (!FindPathDijkstraImpl(path, &m_cells[from], condition, camera)) {
        return false;
    }
    ConstructPrecisePath(path);
    return true;
}

typedef char AssertGameAIMapSize[sizeof(CGameAIMap) == 0x9c ? 1 : -1];
typedef char AssertGameAIMapCellSize[sizeof(CGameAIMap::SCellAI) == 0x7c ? 1 : -1];
typedef char AssertGameAIMapPointSize[sizeof(CGameAIMap::SSpecialPointAI) == 0x2c ? 1 : -1];
