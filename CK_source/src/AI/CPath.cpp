#include "CPath.h"

#include "CAuxRender.h"
#include "CGameAIMap.h"
#include "CUnitBody.h"
#include "QuickSort.h"

#include <math.h>
#include <string.h>

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
    : m_pointCount(0), m_smoothCount(0), m_speed(0.0f), m_field3c0c(0),
      m_field3c10(true), m_moving(true), m_body(NULL), m_cellCount(0), m_targetPoint(-1)
{
    memset(m_smoothPoints, 0, sizeof(m_smoothPoints));
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
    if (m_body == NULL || m_pointCount < 1 || m_speed <= 0.0f) {
        return;
    }
    if (m_field3c10) {
        m_field3c0c = m_pointCount - 1;
        m_field3c10 = false;
    }
    MakeStep();
    if (m_field3c0c == 0) {
        m_moving = false;
        m_body->m_pointId = m_targetPoint;
        m_body->SetExactlyToPoint();
        m_body->OnPathComplete();
        return;
    }
    if (DidBodyReachNextPoint()) {
        int cell = m_points[--m_field3c0c].m_cellId;
        if (cell != -1) {
            m_body->m_cellId = cell;
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
    float distanceSq = offset.LengthSqr();
    if (distanceSq == 0.0f) {
        return true;
    }
    float planarDistance = sqrtf(offset.m_x * offset.m_x + offset.m_y * offset.m_y);
    return distanceSq < 0.5f * planarDistance;
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
    WayPoint& point = m_points[m_pointCount++];
    point.m_position = position;
    point.m_left = vec3();
    point.m_right = vec3();
    point.m_cellId = cellId;
    point.m_smoothed = false;
    return true;
}

bool CPath::AddGateWayPoint(const vec3& position, int cellId, const vec3& left, const vec3& right)
{
    if (m_pointCount >= 256) {
        return false;
    }
    WayPoint& point = m_points[m_pointCount++];
    point.m_position = position;
    point.m_left = left;
    point.m_right = right;
    point.m_cellId = cellId;
    point.m_smoothed = false;
    return true;
}

bool CPath::SmoothPoint(SSmoothPoint* entry)
{
    if (entry == NULL) {
        return false;
    }
    WayPoint* point = entry->m_point;
    vec3 oldPosition = point->m_position;
    vec3 through = entry->m_next->m_position - entry->m_previous->m_position;
    vec3 normal(-through.m_y, through.m_x, 0.0f);
    vec3 gate = point->m_left - point->m_right;
    float denominator = normal.Dot(gate);
    if (denominator == 0.0f) {
        return false;
    }
    float fraction = normal.Dot(entry->m_next->m_position - point->m_left) / denominator;
    vec3 intersection = point->m_left + gate * fraction;
    if (fraction > -1.0f && fraction < 0.0f) {
        point->m_position = intersection;
    } else if ((intersection - point->m_right).LengthSqr() <= (intersection - point->m_left).LengthSqr()) {
        point->m_position = point->m_right;
    } else {
        point->m_position = point->m_left;
    }
    point->m_smoothed = true;
    return !CGameAIMap::IsPositionTheSame(oldPosition, point->m_position);
}

void CPath::CreateSmoothInfo()
{
    m_smoothCount = 0;
    if (m_pointCount >= 256) {
        return;
    }
    for (int index = 1; index < m_pointCount - 1; ++index) {
        SSmoothPoint& entry = m_smoothPoints[m_smoothCount++];
        entry.m_next = &m_points[index - 1];
        entry.m_point = &m_points[index];
        entry.m_previous = &m_points[index + 1];
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
