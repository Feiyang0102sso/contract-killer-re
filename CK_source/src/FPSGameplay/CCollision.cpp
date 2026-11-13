// CCollision.cpp - original owner u431, 0xf1468..0xf5e50.
// Geometry, caches and unusual boundary cases follow the shipped ARM implementation.
#include "CCollision.h"
#include "SwerveHelper.h"
#include <string.h>


typedef Node CollisionNode;
typedef Group CollisionGroup;
typedef World CollisionWorld;
typedef Mesh CollisionMesh;
typedef VertexBuffer CollisionVertexBuffer;
typedef VertexArray CollisionVertexArray;
typedef IndexBuffer CollisionIndexBuffer;
typedef Transform CollisionTransform;
typedef Appearance CollisionAppearance;

typedef char CollisionSizeCheck[sizeof(CCollision) == 0xc0 ? 1 : -1];

CCollision::OcTreeSector::OcTreeSector(OcTreeSector* parent, const AABB& bounds,
                                      CCollision* owner, int leafThreshold)
    : m_parent(parent), m_bounds(bounds)
{
    __builtin_memset(m_children, 0, sizeof(m_children));
    if (parent == NULL) {
        m_triangleCount = owner->m_triangleCount;
        m_triangleIndices = new int[m_triangleCount];
        for (int index = 0; index < m_triangleCount; index++) {
            m_triangleIndices[index] = index;
        }
    } else {
        m_triangleCount = 0;
        m_triangleIndices = new int[parent->m_triangleCount];
        int count = parent->m_triangleCount;
        for (int index = 0; index < count; index++) {
            int triangleIndex = parent->m_triangleIndices[index];
            const Triangle& triangle = owner->m_triangles[triangleIndex];
            if (Intersect(m_bounds, owner->m_vertices[triangle.m_vertices[0]],
                          owner->m_vertices[triangle.m_vertices[1]], owner->m_vertices[triangle.m_vertices[2]]) == true) {
                m_triangleIndices[m_triangleCount] = triangleIndex;
                m_triangleCount++;
                count = parent->m_triangleCount;
            }
        }
    }
    if (m_triangleCount < leafThreshold) {
        return;
    }
    // Three longest-axis bisections create eight children, in low-to-high traversal order.
    AABB childBounds[8];
    m_bounds.Split(childBounds[0], childBounds[4]);
    childBounds[0].Split(childBounds[0], childBounds[2]);
    childBounds[4].Split(childBounds[4], childBounds[6]);
    childBounds[0].Split(childBounds[0], childBounds[1]);
    childBounds[2].Split(childBounds[2], childBounds[3]);
    childBounds[4].Split(childBounds[4], childBounds[5]);
    childBounds[6].Split(childBounds[6], childBounds[7]);
    for (int index = 0; index < 8; index++) {
        m_children[index] = new OcTreeSector(this, childBounds[index], owner, leafThreshold);
    }
    m_triangleCount = 0;
    delete[] m_triangleIndices;
    m_triangleIndices = NULL;
}

CCollision::OcTreeSector::~OcTreeSector()
{
    delete[] m_triangleIndices;
    for (int index = 0; index < 8; index++) {
        delete m_children[index];
    }
}

CCollision::CCollision(CollisionNode& node, int leafThreshold)
    : m_transform(SwerveHelper::NewTransform()), m_inverseTransform(SwerveHelper::NewTransform()),
      m_usingCache(false)
{
    m_vertexCount = 0;
    m_triangleCount = 0;
    m_meshCount = 0;
    CollectMeshInfo(node);
    m_vertices = new vec3[m_vertexCount];
    m_triangles = new Triangle[m_triangleCount];
    m_normals = new vec3[m_triangleCount];
    m_edgeVectors1c = new vec3[m_triangleCount];
    m_edgeVectors20 = new vec3[m_triangleCount];
    m_planes = new vec4[m_triangleCount];
    m_meshNodes = new CollisionAppearance[m_meshCount];
    unsigned int vertexOffset = 0;
    unsigned int triangleOffset = 0;
    unsigned int meshOffset = 0;
    BuildMesh(node, vertexOffset, triangleOffset, meshOffset);
    m_triangleCount = triangleOffset;
    // The original reads vertex zero even for an empty mesh; no invented empty-mesh fallback.
    // Original copy-constructs both corners from vertex zero (no zeroing), then widens them.
    AABB bounds = { m_vertices[0], m_vertices[0] };
    bounds.m_min = bounds.m_min + vec3(-1.0f, -1.0f, -1.0f);
    bounds.m_max = bounds.m_max + vec3(1.0f, 1.0f, 1.0f);
    for (unsigned int index = 0; index < (unsigned int)m_vertexCount; index++) {
        bounds.Add(m_vertices[index]);
    }
    m_root = new OcTreeSector(NULL, bounds, this, leafThreshold);
}

CCollision::~CCollision()
{
    delete[] m_vertices;
    delete[] m_triangles;
    delete[] m_normals;
    delete[] m_edgeVectors1c;
    delete[] m_edgeVectors20;
    delete[] m_planes;
    delete[] m_meshNodes;
    delete m_root;
}

void CCollision::CollectMeshInfo(CollisionNode& node)
{
    CollisionMesh mesh(node);
    if (mesh.m_interface != NULL) {
        m_vertexCount += mesh.getVertexBuffer().getVertexCount();
        m_meshCount += mesh.getSubmeshCount();
        for (int index = 0; index < mesh.getSubmeshCount(); index++) {
            m_triangleCount += mesh.getTriangleIndexBuffer(index).getIndexCount() / 3;
        }
    }
    CollisionGroup group(node);
    if (group.m_interface != NULL) {
        int count = group.getChildCount();
        for (int index = 0; index < count; index++) {
            CollisionNode child = group.getChild(index);
            CollectMeshInfo(child);
        }
    }
}

void CCollision::BuildMesh(CollisionNode& node, unsigned int& vertexOffset,
                           unsigned int& triangleOffset, unsigned int& meshOffset)
{
    CollisionNode reference = node;
    while (CollisionWorld(reference).m_interface == NULL && reference.getParent().m_interface != NULL) {
        reference = reference.getParent();
    }
    CollisionMesh mesh(node);
    if (mesh.m_interface != NULL) {
        CollisionTransform transform = SwerveHelper::NewTransform();
        Mat4x4 matrix(1.0f);
        mesh.getTransformTo(reference, transform);
        SwerveHelper::SetMatrix(matrix, transform);
        CollisionVertexBuffer vertexBuffer = mesh.getVertexBuffer();
        float scaleBias[4];
        CollisionVertexArray positions = vertexBuffer.getPositions(scaleBias);
        int vertexCount = positions.getVertexCount();
        short* components = new short[vertexCount * 3];
        positions.get(0, vertexCount, vertexCount * 3, components);
        for (int index = 0; index < vertexCount; index++) {
            vec3& vertex = m_vertices[vertexOffset + index];
            vertex.m_x = (float)components[index * 3] * scaleBias[0] + scaleBias[1];
            vertex.m_y = (float)components[index * 3 + 1] * scaleBias[0] + scaleBias[2];
            vertex.m_z = (float)components[index * 3 + 2] * scaleBias[0] + scaleBias[3];
            vec3& transformedVertex = m_vertices[vertexOffset + index];
            transformedVertex = matrix.TransformFast(transformedVertex);
        }
        delete[] components;
        for (int submesh = 0; submesh < mesh.getSubmeshCount(); submesh++) {
            m_meshNodes[meshOffset + submesh] = mesh.getAppearance(submesh);
            CollisionIndexBuffer buffer = mesh.getTriangleIndexBuffer(submesh);
            unsigned int indexCount = buffer.getIndexCount();
            unsigned int* indices = new unsigned int[indexCount];
            buffer.getIndices(buffer.getIndexCount(), indices);
            int triangleCount = buffer.getIndexCount() / 3;
            for (int index = 0; index < triangleCount; index++) {
                Triangle& triangle = m_triangles[triangleOffset];
                triangle.m_meshIndex = meshOffset + submesh;
                int first = vertexOffset + indices[index * 3];
                triangle.m_vertices[0] = first;
                int second = vertexOffset + indices[index * 3 + 1];
                triangle.m_vertices[1] = second;
                int third = vertexOffset + indices[index * 3 + 2];
                triangle.m_vertices[2] = third;
                if (first == second || first == third || second == third) {
                    continue;
                }
                m_edgeVectors1c[triangleOffset] = m_vertices[second] - m_vertices[first];
                m_edgeVectors20[triangleOffset] = m_vertices[triangle.m_vertices[2]] - m_vertices[triangle.m_vertices[0]];
                const vec3& firstEdge = m_edgeVectors1c[triangleOffset];
                const vec3& secondEdge = m_edgeVectors20[triangleOffset];
                vec3 normal = firstEdge.Cross(secondEdge);
                vec3& storedNormal = m_normals[triangleOffset];
                storedNormal = normal * MathLib::InvSqrt(normal.LengthSqr());
                m_planes[triangleOffset].m_x = m_edgeVectors1c[triangleOffset].Dot(m_edgeVectors1c[triangleOffset]);
                m_planes[triangleOffset].m_y = m_edgeVectors1c[triangleOffset].Dot(m_edgeVectors20[triangleOffset]);
                m_planes[triangleOffset].m_z = m_edgeVectors20[triangleOffset].Dot(m_edgeVectors20[triangleOffset]);
                vec4& products = m_planes[triangleOffset];
                products.m_w = products.m_y * products.m_y - products.m_x * products.m_z;
                const vec3& finalNormal = m_normals[triangleOffset];
                if (finalNormal.m_x != 0.0f || finalNormal.m_y != 0.0f || finalNormal.m_z != 0.0f) {
                    triangleOffset++;
                }
            }
            delete[] indices;
        }
        vertexOffset += vertexCount;
        meshOffset += mesh.getSubmeshCount();
    }
    CollisionGroup group(node);
    if (group.m_interface != NULL) {
        int count = group.getChildCount();
        for (int index = 0; index < count; index++) {
            CollisionNode child = group.getChild(index);
            BuildMesh(child, vertexOffset, triangleOffset, meshOffset);
        }
    }
}

bool CCollision::IntersectAny(const Line& line, vec3& point, vec3& normal, Triangle& triangle)
{
    if (m_triangleCount == 0) {
        return false;
    }
    return IntersectAnyOcTree(m_root, line, point, normal, triangle);
}

bool CCollision::IntersectAnyOcTree(OcTreeSector* sector, const Line& line, vec3& point,
                                   vec3& normal, Triangle& triangle)
{
    if (!Intersect(line, sector->m_bounds, point)) {
        return false;
    }
    if (sector->m_triangleCount == 0) {
        if (sector->m_children[0] != NULL) {
            for (int index = 0; index < 8; ++index) {
                if (IntersectAnyOcTree(sector->m_children[index], line, point, normal, triangle)) {
                    return true;
                }
            }
        }
        return false;
    }
    vec3 direction = line.m_to - line.m_from;
    for (int index = 0; index < sector->m_triangleCount; ++index) {
        int triangleIndex = sector->m_triangleIndices[index];
        const vec3& triangleNormal = m_normals[triangleIndex];
        float denominator = triangleNormal.Dot(direction);
        if (denominator > 0.0f || MathLib::Fabs(denominator) < 0.00000001f) {
            continue;
        }
        const Triangle& candidate = m_triangles[triangleIndex];
        const vec3& first = m_vertices[candidate.m_vertices[0]];
        float fraction = -triangleNormal.Dot(line.m_from - first) / denominator;
        if (fraction < 0.0f) {
            continue;
        }
        point = line.m_from + direction * fraction;
        vec3 relative = point - first;
        float firstDot = relative.Dot(m_edgeVectors1c[triangleIndex]);
        float secondDot = relative.Dot(m_edgeVectors20[triangleIndex]);
        const vec4& products = m_planes[triangleIndex];
        float firstWeight = (products.m_y * secondDot - products.m_z * firstDot) / products.m_w;
        if ((double)firstWeight < 0.0 || firstWeight > 1.0f) {
            continue;
        }
        float secondWeight = (products.m_y * firstDot - products.m_x * secondDot) / products.m_w;
        if ((double)secondWeight < 0.0 || firstWeight + secondWeight > 1.0f) {
            continue;
        }
        normal = triangleNormal;
        triangle = candidate;
        return true;
    }
    return false;
}

bool CCollision::Intersect(const Line& line, const AABB& box, vec3& point)
{
    // Cohen-Sutherland style region code of a point against the box.
    struct Outcode {
        static int Calculate(const AABB& box, const vec3& point)
        {
            int code = 0;
            if (point.m_x > box.m_max.m_x) { code |= 1; }
            else if (point.m_x < box.m_min.m_x) { code |= 2; }
            if (point.m_y > box.m_max.m_y) { code |= 4; }
            else if (point.m_y < box.m_min.m_y) { code |= 8; }
            if (point.m_z > box.m_max.m_z) { code |= 0x20; }
            else if (point.m_z < box.m_min.m_z) { code |= 0x10; }
            return code;
        }
    };

    int fromCode = Outcode::Calculate(box, line.m_from);
    if (fromCode == 0) {
        point = line.m_from;
        return true;
    }
    int toCode = Outcode::Calculate(box, line.m_to);
    if (toCode == 0) {
        point = line.m_to;
        return true;
    }
    if ((fromCode & toCode) != 0) {
        return false;
    }
    if ((fromCode & 3) != 0) {
        if ((fromCode & 1) != 0) {
            point.m_x = box.m_max.m_x;
        } else {
            point.m_x = box.m_min.m_x;
        }
        float offset = point.m_x - line.m_from.m_x;
        float length = line.m_to.m_x - line.m_from.m_x;
        point.m_y = line.m_from.m_y + offset * (line.m_to.m_y - line.m_from.m_y) / length;
        point.m_z = line.m_from.m_z + offset * (line.m_to.m_z - line.m_from.m_z) / length;
        if (point.m_y <= box.m_max.m_y && point.m_y >= box.m_min.m_y &&
            point.m_z <= box.m_max.m_z && point.m_z >= box.m_min.m_z) {
            return true;
        }
    }
    if ((fromCode & 0xc) != 0) {
        if ((fromCode & 4) != 0) {
            point.m_y = box.m_max.m_y;
        } else {
            point.m_y = box.m_min.m_y;
        }
        float offset = point.m_y - line.m_from.m_y;
        float length = line.m_to.m_y - line.m_from.m_y;
        point.m_x = line.m_from.m_x + offset * (line.m_to.m_x - line.m_from.m_x) / length;
        point.m_z = line.m_from.m_z + offset * (line.m_to.m_z - line.m_from.m_z) / length;
        if (point.m_x <= box.m_max.m_x && point.m_x >= box.m_min.m_x &&
            point.m_z <= box.m_max.m_z && point.m_z >= box.m_min.m_z) {
            return true;
        }
    }
    if ((fromCode & 0x30) != 0) {
        if ((fromCode & 0x20) != 0) {
            point.m_z = box.m_max.m_z;
        } else {
            point.m_z = box.m_min.m_z;
        }
        float offset = point.m_z - line.m_from.m_z;
        float length = line.m_to.m_z - line.m_from.m_z;
        point.m_x = line.m_from.m_x + offset * (line.m_to.m_x - line.m_from.m_x) / length;
        point.m_y = line.m_from.m_y + offset * (line.m_to.m_y - line.m_from.m_y) / length;
        if (point.m_x <= box.m_max.m_x && point.m_x >= box.m_min.m_x &&
            point.m_y <= box.m_max.m_y && point.m_y >= box.m_min.m_y) {
            return true;
        }
    }
    return false;
}

vec3 CCollision::GetPenetration(Ellipsoid& ellipsoid)
{
    if (m_triangleCount == 0) {
        return vec3(0.0f, 0.0f, 0.0f);
    }
    Mat4x4 transform(1.0f);
    transform.Ident(1.0f);
    transform.SetTranslation(vec3(ellipsoid.m_position.m_x, ellipsoid.m_position.m_y, ellipsoid.m_position.m_z));
    if (ellipsoid.m_angles.m_x != 0.0f) {
        transform.PostRotate(ellipsoid.m_angles.m_x, 1.0f, 0.0f, 0.0f);
    }
    if (ellipsoid.m_angles.m_y != 0.0f) {
        transform.PostRotate(ellipsoid.m_angles.m_y, 0.0f, 1.0f, 0.0f);
    }
    if (ellipsoid.m_angles.m_z != 0.0f) {
        transform.PostRotate(ellipsoid.m_angles.m_z, 0.0f, 0.0f, 1.0f);
    }
    transform.PostScale(ellipsoid.m_radii.m_x, ellipsoid.m_radii.m_y, ellipsoid.m_radii.m_z);
    Vectors directions;
    transform.ToVectors(directions);
    vec3 translation(transform.m_rows[0].m_w, transform.m_rows[1].m_w, transform.m_rows[2].m_w);
    AABB bounds;
    bounds.m_min = translation;
    bounds.m_max = translation;
    if (ellipsoid.m_angles.m_x != 0.0f || ellipsoid.m_angles.m_y != 0.0f || ellipsoid.m_angles.m_z != 0.0f) {
        // Keep the original eight-corner order and left-associated addition/subtraction.
        bounds.Add(translation + directions.m_right + directions.m_up + directions.m_forward);
        bounds.Add(translation + directions.m_right + directions.m_up - directions.m_forward);
        bounds.Add(translation + directions.m_right - directions.m_up + directions.m_forward);
        bounds.Add(translation + directions.m_right - directions.m_up - directions.m_forward);
        bounds.Add(translation - directions.m_right + directions.m_up + directions.m_forward);
        bounds.Add(translation - directions.m_right + directions.m_up - directions.m_forward);
        bounds.Add(translation - directions.m_right - directions.m_up + directions.m_forward);
        bounds.Add(translation - directions.m_right - directions.m_up - directions.m_forward);
    } else {
        bounds.Add(translation + directions.m_right + directions.m_up + directions.m_forward);
        bounds.Add(translation - directions.m_right - directions.m_up - directions.m_forward);
    }
    Mat4x4 inverse = transform;
    inverse.Invert();
    m_currentEllipsoid = ellipsoid;
    if (m_penetrations.m_data != NULL) {
        m_penetrations.Clear();
    }
    vec3 cacheMovement = m_cachedEllipsoid.m_position - ellipsoid.m_position;
    if (cacheMovement.LengthSqr() > 0.01f && m_cachedTriangles.m_data != NULL) {
        m_cachedTriangles.Clear();
    }
    if (m_cachedTriangles.Size() != 0) {
        CalculatePenetrationCache(transform, inverse);
    } else {
        m_cachedEllipsoid = ellipsoid;
        CalculatePenetrationOcTree(m_root, bounds, transform, inverse);
    }
    vec3 result;
    for (int index = 0; index < m_penetrations.Size(); index++) {
        vec3 direction = ellipsoid.m_position - m_penetrations[index].m_point;
        direction = direction * MathLib::InvSqrt(direction.LengthSqr());
        vec3 displacement = direction * (m_penetrations[index].m_radius - m_penetrations[index].m_distance);
        if (index == 0) {
            result = displacement;
        } else {
            vec3 oldDirection = result * MathLib::InvSqrt(result.LengthSqr());
            float projection = oldDirection.Dot(displacement);
            vec3 newDirection = displacement * MathLib::InvSqrt(displacement.LengthSqr());
            result = result + (displacement - newDirection * projection);
        }
    }
    return result;
}

void CCollision::CalculatePenetrationCache(const Mat4x4& transform, const Mat4x4& inverse)
{
    // Original: count read before the flag store, unsigned guard (cmp #0; beq).
    unsigned int count = m_cachedTriangles.Size();
    m_usingCache = true;
    for (unsigned int index = 0; index < count; index++) {
        CalculatePenetrationTriangle(transform, inverse, m_cachedTriangles[index]);
    }
    m_usingCache = false;
}

void CCollision::CalculatePenetrationOcTree(OcTreeSector* sector, const AABB& bounds,
                                           const Mat4x4& transform, const Mat4x4& inverse)
{
    if (bounds.m_max.m_x < sector->m_bounds.m_min.m_x || sector->m_bounds.m_max.m_x < bounds.m_min.m_x ||
        bounds.m_max.m_y < sector->m_bounds.m_min.m_y || sector->m_bounds.m_max.m_y < bounds.m_min.m_y ||
        bounds.m_max.m_z < sector->m_bounds.m_min.m_z || sector->m_bounds.m_max.m_z < bounds.m_min.m_z) {
        return;
    }
    int count = sector->m_triangleCount;
    if (count != 0) {
        for (int index = 0; index < count; index++) {
            CalculatePenetrationTriangle(transform, inverse, sector->m_triangleIndices[index]);
        }
    } else if (sector->m_children[0] != NULL) {
        for (int index = 0; index < 8; index++) {
            CalculatePenetrationOcTree(sector->m_children[index], bounds, transform, inverse);
        }
    }
}

/** Radius along the contact direction, using the same two RotateFast calls as the owner. */
static inline float CollisionContactRadius(const Mat4x4& transform, const Mat4x4& inverse,
                                           const vec3& offset)
{
    vec3 direction = inverse.RotateFast(offset);
    direction = direction * MathLib::InvSqrt(direction.LengthSqr());
    direction = transform.RotateFast(direction);
    float squaredLength = direction.LengthSqr();
    return MathLib::InvSqrt(squaredLength) * squaredLength;
}

void CCollision::CalculatePenetrationTriangle(const Mat4x4& transform, const Mat4x4& inverse, int index)
{
    for (int item = 0; item < m_penetrations.Size(); item++) {
        if (m_penetrations[item].m_triangle == index) {
            return;
        }
    }
    if (!m_usingCache) {
        unsigned int triangleIndex = index;
        m_cachedTriangles.Add(triangleIndex);
    }
    if (m_movementFilter.m_x != 0.0f || m_movementFilter.m_y != 0.0f || m_movementFilter.m_z != 0.0f) {
        if (MathLib::Fabs(m_movementFilter.Dot(m_normals[index])) < 0.707f) {
            return;
        }
    }
    const Triangle& triangle = m_triangles[index];
    vec3 first = inverse.TransformFast(m_vertices[triangle.m_vertices[0]]);
    vec3 second = inverse.TransformFast(m_vertices[triangle.m_vertices[1]]);
    vec3 third = inverse.TransformFast(m_vertices[triangle.m_vertices[2]]);
    Sphere sphere;
    sphere.m_radius = 1.0f;
    vec3 point;
    float ignoredDistance;
    if (Intersect(sphere, first, second, third, point, ignoredDistance) != true) {
        return;
    }
    point = transform.TransformFast(point);
    const vec3& normal = m_normals[index];
    for (int item = 0; item < m_penetrations.Size(); item++) {
        Penetration& penetration = m_penetrations[item];
        vec3 cross = m_normals[penetration.m_triangle].Cross(normal);
        float squaredLength = cross.LengthSqr();
        float crossLength = MathLib::InvSqrt(squaredLength) * squaredLength;
        if (crossLength != 0.0f) {
            continue;
        }
        vec3 direction = m_currentEllipsoid.m_position - point;
        direction = direction * MathLib::InvSqrt(direction.LengthSqr());
        cross = direction.Cross(normal);
        if (cross.LengthSqr() == 0.0f) {
            penetration.m_triangle = index;
            direction = point - m_currentEllipsoid.m_position;
            squaredLength = direction.LengthSqr();
            penetration.m_distance = MathLib::InvSqrt(squaredLength) * squaredLength;
            penetration.m_point = point;
        } else {
            penetration.m_triangle = index;
            const vec3& planePoint = m_vertices[triangle.m_vertices[0]];
            float planeOffset = -normal.m_x * planePoint.m_x - normal.m_y * planePoint.m_y - normal.m_z * planePoint.m_z;
            float centerProjection = normal.m_x * m_currentEllipsoid.m_position.m_x +
                                     normal.m_y * m_currentEllipsoid.m_position.m_y + normal.m_z * m_currentEllipsoid.m_position.m_z;
            penetration.m_distance = MathLib::Fabs(centerProjection + planeOffset);
            penetration.m_point = m_currentEllipsoid.m_position - normal * penetration.m_distance;
        }
        penetration.m_radius = CollisionContactRadius(transform, inverse, penetration.m_point - m_currentEllipsoid.m_position);
        return;
    }
    vec3 direction = point - m_currentEllipsoid.m_position;
    float radius = CollisionContactRadius(transform, inverse, direction);
    Penetration penetration;
    penetration.m_triangle = index;
    float squaredLength = direction.LengthSqr();
    penetration.m_distance = MathLib::InvSqrt(squaredLength) * squaredLength;
    penetration.m_radius = radius;
    penetration.m_point = point;
    m_penetrations.Add(penetration);
}

void CCollision::MoveTo(Ellipsoid& ellipsoid, const vec3& target, bool adjustHeight, int unused)
{
    m_movementFilter = vec3(0.0f, 0.0f, 0.0f);
    vec3 movement = target - ellipsoid.m_position;
    float squaredLength = movement.LengthSqr();
    float remaining = MathLib::Sqrt(squaredLength);
    vec3 direction = movement * MathLib::InvSqrt(squaredLength);
    if (direction.m_x == 0.0f && direction.m_y == 0.0f) {
        m_movementFilter = vec3(0.0f, 0.0f, 1.0f);
    }
    if (remaining > 0.0f) {
        float stepLength = remaining * 1.1f;
        vec3 position = ellipsoid.m_position;
        do {
            float length = stepLength;
            if (length > remaining) {
                length = remaining;
            }
            vec3 step = direction * length;
            ellipsoid.m_position = position + step;
            vec3 correction = GetPenetration(ellipsoid);
            if (adjustHeight == true) {
                // Lift by the vertical correction only, then reapply the step.
                ellipsoid.m_position = position + vec3(0.0f, 0.0f, correction.m_z * 1.001f) + step;
                correction = GetPenetration(ellipsoid);
            }
            remaining = remaining - length;
            position = ellipsoid.m_position + correction;
            ellipsoid.m_position = position;
        } while (remaining > 0.0f);
    }
}
