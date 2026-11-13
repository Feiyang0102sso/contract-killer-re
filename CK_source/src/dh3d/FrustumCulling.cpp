// FrustumCulling.cpp - view frustum of a camera and bounding sphere tests against it.
#include "FrustumCulling.h"
#include "CSwerve.h"
#include "App.h"
#include "DGHelper.h"
#include "CSwerveGame.h"

/** Start values of the bounding box search. */
static const float BOX_START = 10000.0f;

FrustumCulling::FrustumCulling(Camera camera)
{
    SetCamera(camera);
}

bool FrustumCulling::CheckVisibility(const CullSphere& sphere) const
{
    Transform transform(CSwerve::GetInstance()->m_swerve);
    CSwerveGame* game = ((App*)WindowApp::m_instance)->m_helper->m_swerveGame;
    {
        // Swerve writes the connection result, which the caller does not use.
        bool connected;
        Transform targetTransform(transform);
        IQueryInterface* world = game->m_world.m_interface;
        typedef int (*GetTransformTo)(IQueryInterface*, INodeInterface*, ITransformInterface*, bool*);
        CKMethod<GetTransformTo>(world, 0xa4)(world, m_camera.node(), targetTransform.transformInterface(), &connected);
    }
    float point[4];
    point[0] = sphere.m_center.x + sphere.m_offset.x;
    point[1] = sphere.m_center.y + sphere.m_offset.y;
    point[2] = sphere.m_center.z + sphere.m_offset.z;
    point[3] = 1.0f;
    transform.transform(4, point);
    const vec3 cameraPoint(point[0], point[1], point[2]);
    for (int index = 0; index < PLANE_COUNT; index++) {
        const Plane& plane = m_planes[index];
        float distance = plane.m_normal.m_x * cameraPoint.m_x + plane.m_normal.m_y * cameraPoint.m_y +
                         plane.m_normal.m_z * cameraPoint.m_z - plane.m_distance;
        if (distance < -sphere.GetRadius()) {
            return false;
        }
    }
    return true;
}

float FrustumCulling::CullSphere::GetRadius() const
{
    float scale[3];
    m_group.getScale(scale);
    float largest = scale[0];
    // Original BGT falls through for unordered scale comparisons as well.
    if (!(scale[0] > scale[1])) {
        largest = scale[2];
        if (scale[1] > scale[2]) {
            largest = scale[1];
        }
    }
    return m_radius * largest;
}

void FrustumCulling::UpdatePlanes()
{
    float params[4];
    m_camera.getProjection(params);
    float halfFov = params[0] * 0.5f;
    float sinHalf = MathLib::Sin(halfFov);
    float cosHalf = MathLib::Cos(halfFov);
    float slope = cosHalf / (params[1] * sinHalf + cosHalf);
    float sideX = MathLib::InvSqrt(slope) * slope;
    float sideZ = 1.0f - sideX * sideX;
    float sideZLength = MathLib::InvSqrt(sideZ) * sideZ;
    for (int index = 0; index < PLANE_COUNT; index++) {
        Plane& plane = m_planes[index];
        switch (index) {
        case 0:     // near
            plane.m_normal.m_x = 0;
            plane.m_normal.m_y = 0;
            plane.m_normal.m_z = -1.0f;
            plane.m_distance = params[2];
            break;
        case 2:     // left
            plane.m_normal.m_x = sideX;
            plane.m_normal.m_y = 0;
            plane.m_normal.m_z = -sideZLength;
            plane.m_distance = 0;
            break;
        case 3:     // right
            plane.m_normal.m_x = -sideX;
            plane.m_normal.m_y = 0;
            plane.m_normal.m_z = -sideZLength;
            plane.m_distance = 0;
            break;
        case 1:     // far
            plane.m_normal.m_x = 0;
            plane.m_normal.m_y = 0;
            plane.m_normal.m_z = 1.0f;
            plane.m_distance = -params[3];
            break;
        case 4:     // bottom
            plane.m_normal.m_x = 0;
            plane.m_normal.m_y = -cosHalf;
            plane.m_normal.m_z = -sinHalf;
            plane.m_distance = 0;
            break;
        case 5:     // top
            plane.m_normal.m_x = 0;
            plane.m_normal.m_y = cosHalf;
            plane.m_normal.m_z = -sinHalf;
            plane.m_distance = 0;
            break;
        }
    }
}

void FrustumCulling::GetBoundingSphere(Group group, swvVector3D& center,
                                       float& radius)
{
    radius = -1.0f;
    center.x = 0;
    center.y = 0;
    center.z = 0;
    if (group.m_interface == NULL) {
        return;
    }
    // Prefer the root's own meshes. If it has none, inspect only its first child group.
    bool hasMesh = false;
    for (int index = 0; index < group.getChildCount(); index++) {
        Mesh child = group.getChild(index);
        if (child.m_interface != NULL) {
            hasMesh = true;
            break;
        }
    }
    if (!hasMesh) {
        Group firstChild = group.getChild(0);
        if (firstChild.m_interface != NULL) {
            group = firstChild;
        }
    }
    float maxX = -BOX_START;
    float minX = BOX_START;
    float maxY = -BOX_START;
    float minY = BOX_START;
    float maxZ = -BOX_START;
    float minZ = BOX_START;
    for (int index = 0; index < group.getChildCount(); index++) {
        Mesh mesh = group.getChild(index);
        if (mesh.m_interface != NULL) {
            VertexBuffer vertexBuffer = mesh.getVertexBuffer();
            int vertexCount = vertexBuffer.getVertexCount();
            float scaleBias[4];
            VertexArray positions = vertexBuffer.getPositions(scaleBias);
            Transform transform(CSwerve::GetInstance()->m_swerve);
            float* vertices = new float[vertexCount * 4];
            transform.transform((IVertexArrayInterface*)positions.m_interface, vertexCount * 4, vertices, true);
            {
                bool connected;
                Group targetGroup(group);
                Transform targetTransform(transform);
                typedef int (*GetTransformTo)(IQueryInterface*, INodeInterface*, ITransformInterface*, bool*);
                CKMethod<GetTransformTo>(mesh.m_interface, 0xa4)(mesh.m_interface, targetGroup.node(),
                                                               targetTransform.transformInterface(), &connected);
            }
            float* vertex = vertices;
            for (int vertexIndex = 0; vertexIndex < vertexCount; vertexIndex++) {
                vertex[0] = vertex[0] * scaleBias[0] + scaleBias[1];
                vertex[1] = vertex[1] * scaleBias[0] + scaleBias[2];
                vertex[2] = vertex[2] * scaleBias[0] + scaleBias[3];
                vertex[3] = 1.0f;
                transform.transform(4, vertex);
                if (vertex[2] > maxZ) {
                    maxZ = vertex[2];
                }
                if (vertex[2] < minZ) {
                    minZ = vertex[2];
                }
                if (vertex[1] > maxY) {
                    maxY = vertex[1];
                }
                if (vertex[1] < minY) {
                    minY = vertex[1];
                }
                if (vertex[0] > maxX) {
                    maxX = vertex[0];
                }
                if (vertex[0] < minX) {
                    minX = vertex[0];
                }
                vertex += 4;
            }
            center.x = (maxX + minX) * 0.5f;
            center.y = (maxY + minY) * 0.5f;
            center.z = (maxZ + minZ) * 0.5f;
            vertex = vertices;
            for (int vertexIndex = 0; vertexIndex < vertexCount; vertexIndex++) {
                float deltaX = vertex[0] - center.x;
                float deltaY = vertex[1] - center.y;
                float deltaZ = vertex[2] - center.z;
                float distance = DGHelper::sqrt(deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ);
                if (radius < distance) {
                    radius = distance;
                }
                vertex += 4;
            }
            if (vertices != NULL) {
                delete[] vertices;
            }
        } else {
            Sprite3D sprite = group.getChild(index);
            if (sprite.m_interface != NULL) {
                float scale[3];
                sprite.getScale(scale);
                radius = DGHelper::sqrt(scale[0] * scale[0] + scale[1] * scale[1]) * 0.5f;
            }
        }
    }
}

FrustumCulling::CullSphere::CullSphere(Group group, float radiusScale)
    : m_group(group)
{
    m_field04.m_interface = NULL; // The original ctor zeroes +4; the real wrapper must also release it in the destructor.
    m_center.x = 0;
    m_center.y = 0;
    m_center.z = 0;
    m_offset.x = 0;
    m_offset.y = 0;
    m_offset.z = 0;
    m_radius = 0;
    GetBoundingSphere(group, m_center, m_radius);
    m_radius = m_radius * radiusScale;
}

