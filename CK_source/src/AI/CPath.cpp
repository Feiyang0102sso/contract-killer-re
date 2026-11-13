#include "CPath.h"

#include "CAuxRender.h"
#include "CGameAIMap.h"
#include "CUnitBody.h"
#include "QuickSort.h"

void CPath::Render(CAuxRender* render)
{
    if (render != NULL) {
        for (int index = 0; index < m_pointCount - 1; ++index) {
            render->DrawLine(m_points[index].m_position, m_points[index + 1].m_position,
                0.3f, -0xff0100);
        }
    }
}

void CPath::SSmoothPoint::CalcAngleCoef()
{
    if (m_next != NULL && m_point != NULL && m_previous != NULL) {
        vec3 towardNext = m_next->m_position - m_point->m_position;
        vec3 towardPrevious = m_previous->m_position - m_point->m_position;
        towardNext = towardNext * MathLib::InvSqrt(towardNext.LengthSqr());
        towardPrevious = towardPrevious * MathLib::InvSqrt(towardPrevious.LengthSqr());
        m_angle = towardNext.Dot(towardPrevious);
    }
}

CPath::CPath()
{
    m_pointCount = 0;
    for (int index = 0; index < 256; ++index) {
        m_smoothPoints[index].m_next = NULL;
        m_smoothPoints[index].m_point = NULL;
        m_smoothPoints[index].m_previous = NULL;
        m_smoothPoints[index].m_angle = 0.0f;
    }
    m_smoothCount = 0;
    m_speed = 0.0f;
    m_field3c0c = 0;
    m_field3c10 = true;
    m_moving = true;
    m_body = NULL;
    m_cellCount = 0;
    m_targetPoint = -1;
}

void CPath::SetBody(CUnitBody* body)
{
    if (body != NULL) {
        m_body = body;
    }
}

void CPath::Tick(int elapsed)
{
    if (m_moving) {
        HandleMovement(elapsed);
    }
}

void CPath::HandleMovement(int)
{
    if (m_body != NULL && m_pointCount > 0 && m_speed > 0.0f) {
        int waypoint;
        if (m_field3c10) {
            waypoint = m_pointCount - 1;
            m_field3c0c = waypoint;
            m_field3c10 = false;
        } else {
            waypoint = m_field3c0c;
        }
        MakeStep();
        if (waypoint == 0) {
            m_moving = false;
            if (m_body != NULL) {
                m_body->m_pointId = m_targetPoint;
                m_body->SetExactlyToPoint();
                m_body->OnPathComplete();
            }
        } else if (DidBodyReachNextPoint()) {
            int previousWaypoint = m_field3c0c--;
            if (previousWaypoint > 0) {
                int cell = m_points[previousWaypoint - 1].m_cellId;
                if (cell != -1) {
                    m_body->m_cellId = cell;
                }
            }
        }
    }
}

void CPath::MakeStep()
{
    int next = m_field3c0c - 1;
    if (next >= 0) {
        m_body->SetMovementParam(m_points[next].m_position, m_speed);
    }
}

bool CPath::DidBodyReachNextPoint()
{
    vec3 offset = m_points[m_field3c0c - 1].m_position - m_body->m_position;
    float planarDistanceSq = offset.m_x * offset.m_x + offset.m_y * offset.m_y;
    float distanceSq = planarDistanceSq + offset.m_z * offset.m_z;
    return MathLib::InvSqrt(distanceSq) * distanceSq < 0.5f;
}

void CPath::Clear()
{
    m_pointCount = 0;
    m_cellCount = 0;
    m_targetPoint = -1;
}

bool CPath::AddCell(int cellId)
{
    if (m_cellCount >= 256) {
        return false;
    }
    m_cellIds[m_cellCount] = cellId;
    m_cellCount++;
    return true;
}

int CPath::GetCellID(int index) const
{
    if (index >= 0 && index < m_cellCount) {
        return m_cellIds[index];
    }
    return -1;
}

bool CPath::AddWayPoint(const vec3& position, int cellId)
{
    if (m_pointCount >= 256) {
        return false;
    }
    m_points[m_pointCount].m_position = position;
    WayPoint& point = m_points[m_pointCount];
    point.m_left = vec3();
    point.m_right = vec3();
    point.m_cellId = cellId;
    point.m_smoothed = false;
    m_pointCount++;
    return true;
}

bool CPath::AddGateWayPoint(const vec3& position, int cellId, const vec3& left, const vec3& right)
{
    if (m_pointCount >= 256) {
        return false;
    }
    m_points[m_pointCount].m_position = position;
    WayPoint& point = m_points[m_pointCount];
    point.m_cellId = cellId;
    point.m_left = left;
    point.m_right = right;
    point.m_smoothed = false;
    m_pointCount++;
    return true;
}

bool CPath::SmoothPoint(SSmoothPoint* entry)
{
    if (entry == NULL) {
        return false;
    }
    WayPoint* point = entry->m_point;
    vec3 oldPosition = point->m_position;
    const vec3& next = entry->m_next->m_position;
    vec3 through = next - entry->m_previous->m_position;
    vec3 normal(vec3(0.0f, 0.0f, 1.0f).Cross(through));
    vec3 gate = point->m_left - point->m_right;
    float denominator = normal.Dot(gate);
    if (denominator == 0.0f) {
        return false;
    }
    float fraction = normal.Dot(next - point->m_left) / denominator;
    vec3 intersection = point->m_left + gate * fraction;
    // Use the two oriented areas in the horizontal plane to test the gate bounds.
    float leftArea = (point->m_left - next).Cross(intersection - next).m_z;
    float rightArea = (point->m_right - next).Cross(intersection - next).m_z;
    if (leftArea * rightArea < 0.0f) {
        point->m_position = intersection;
    } else {
        const float leftDistance = (point->m_left - intersection).LengthSqr();
        const float rightDistance = (point->m_right - intersection).LengthSqr();
        if (leftDistance < rightDistance) {
            point->m_position = point->m_left;
        } else {
            point->m_position = point->m_right;
        }
    }
    entry->m_point->m_smoothed = true;
    return !CGameAIMap::IsPositionTheSame(oldPosition, entry->m_point->m_position);
}

void CPath::CreateSmoothInfo()
{
    if (m_pointCount < 256) {
        int end = m_pointCount - 1;
        m_smoothCount = 0;
        for (int index = 1; index < end && m_smoothCount < 256; ++index) {
            m_smoothPoints[m_smoothCount].m_next = &m_points[index - 1];
            m_smoothPoints[m_smoothCount].m_point = &m_points[index];
            m_smoothPoints[m_smoothCount].m_previous = &m_points[index + 1];
            m_smoothCount++;
        }
    }
}

static int CompareTwoPathPoint(const void* first, const void* second)
{
    const CPath::SSmoothPoint* left = static_cast<const CPath::SSmoothPoint*>(first);
    const CPath::SSmoothPoint* right = static_cast<const CPath::SSmoothPoint*>(second);
    return left != NULL && right != NULL && left->m_angle < right->m_angle;
}

void CPath::Smooth()
{
    if (m_smoothCount <= 0 || m_smoothCount >= 256) {
        return;
    }
    for (int index = 0; index < m_smoothCount; ++index) {
        m_smoothPoints[index].CalcAngleCoef();
    }
    if (m_smoothCount == 1) {
        SmoothPoint(m_smoothPoints);
        return;
    }
    WayPoint* lastChanged = NULL;
    for (int pass = 0; pass < m_smoothCount * 3; ++pass) {
        QuickSort::Run(m_smoothPoints, m_smoothCount, sizeof(SSmoothPoint), CompareTwoPathPoint);
        for (int index = 0; index < m_smoothCount; ++index) {
            SSmoothPoint& entry = m_smoothPoints[index];
            if (entry.m_point == lastChanged) {
                ++index;
            } else if (SmoothPoint(&entry)) {
                lastChanged = entry.m_point;
                for (int update = 0; update < m_smoothCount; ++update) {
                    if (m_smoothPoints[update].m_point->m_smoothed) {
                        m_smoothPoints[update].CalcAngleCoef();
                    }
                }
                break;
            }
        }
    }
}

typedef char AssertPathSize[sizeof(CPath) == 0x4020 ? 1 : -1];
