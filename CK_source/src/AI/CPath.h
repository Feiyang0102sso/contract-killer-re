// PROVISIONAL (created by game3): owner src/AI/CPath.cpp not written yet
// CPath.h - a computed walking path of one unit body (points, smoothing, movement).
#ifndef CPATH_H
#define CPATH_H

#include <stddef.h>
#include "MathLib.h"

class CUnitBody;
class CAuxRender;

class CPath {
public:
    CPath();

    void SetBody(CUnitBody* body);
    void Tick(int deltaTime);
    void HandleMovement(int deltaTime);
    void MakeStep();
    bool DidBodyReachNextPoint();
    void Render(CAuxRender* render);
    void Clear();
    bool AddCell(int cellId);
    int GetCellID(int index) const;
    bool AddWayPoint(const vec3& position, int cellId);
    bool AddGateWayPoint(const vec3& position, int cellId, const vec3& left, const vec3& right);

    struct WayPoint {
        vec3 m_position;
        vec3 m_left;
        vec3 m_right;
        int m_cellId;
        bool m_smoothed;
    };
    struct SSmoothPoint {
        WayPoint* m_next;
        WayPoint* m_point;
        WayPoint* m_previous;
        float m_angle;

        void CalcAngleCoef();
    };

    bool SmoothPoint(SSmoothPoint* entry);
    void CreateSmoothInfo();
    void Smooth();

    WayPoint m_points[256];                 // 0x0000
    int m_pointCount;                       // 0x2c00
    SSmoothPoint m_smoothPoints[256];       // 0x2c04
    int m_smoothCount;                      // 0x3c04
    float m_speed;                          // 0x3c08
    int m_field3c0c;                        // 0x3c0c
    bool m_field3c10;                       // 0x3c10
    bool m_moving;                          // 0x3c11 HandleMovement runs only while set
    CUnitBody* m_body;                      // 0x3c14
    int m_cellIds[256];                     // 0x3c18
    int m_cellCount;                        // 0x4018
    int m_targetPoint;                      // 0x401c AI map point the path ends in, -1 when none
};

#endif
