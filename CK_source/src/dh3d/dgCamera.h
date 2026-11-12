// dgCamera.h - CK ARMv7 camera, projection and aim motion.
#ifndef DGCAMERA_H
#define DGCAMERA_H

#include "FrustumCulling.h"
#include "App.h"

class DGCamera {
public:
    typedef ::Camera Camera;
    typedef ::Transform Transform;

    DGCamera(Camera camera, float horizontalLimit, float verticalLimit);
    ~DGCamera();
    void Init(Camera camera, float horizontalLimit, float verticalLimit);
    void ReInit();
    float GetFOV() const;
    void SetFOV(float fov);
    void restoreView(bool immediately);
    void updateTransform(bool immediately);
    bool rotateLeft(float angle, bool unconstrained);
    bool rotateRight(float angle, bool unconstrained);
    bool rotateUp(float angle, bool unconstrained);
    bool rotateDown(float angle, bool unconstrained);
    void centerView(int elapsedMs);
    void returnToSavedView(int elapsedMs);
    void RestoreOriginalFOV();
    void SetWandering(float horizontalAmplitude, float verticalAmplitude, int horizontalPeriod, int verticalPeriod);
    void Shot(float recoil, int durationMs);
    void Update(int elapsedMs);
    void UpdateReturnToCenter(int elapsedMs);
    void updateWander(int elapsedMs);
    void HandleZoomEffect(int elapsedMs);
    void CheckConstraints();
    void SetTargetPoint(float x, float y);
    void SetViewPortWidthAndHeight(float width, float height);
    bool IsShotEffectInProgress() const;
    void SetWanderngByActiveWeapon();
    void SetAngleH(float angle, bool unconstrained);
    void SetAngleV(float angle, bool unconstrained);
    vec3 GetViewDirection();

    Camera m_camera;                     // 0x00
    Transform m_baseTransform;           // 0x04
    float m_defaultFov;                   // 0x08
    float m_field0c;                      // 0x0c horizontal aim angle
    swvVector3D m_horizontalAxis;
    float m_field1c;                      // 0x1c vertical aim angle
    swvVector3D m_verticalAxis;
    float m_horizontalLimit;
    float m_verticalLimit;
    float m_previousAngleH;
    float m_previousAngleV;
    float m_savedAngleH;
    float m_savedAngleV;
    bool m_zoomEffect;
    float m_savedFov;
    float m_effectFov;
    float m_headShotFov;
    float m_recoilAngle;                  // 0x54
    int m_recoilDuration;
    int m_recoilElapsed;
    float m_fov;                         // 0x60
    float m_viewportWidth;
    float m_viewportHeight;
    swvVector3D m_shotDisplacement;
    swvVector3D m_shotStartPosition;
    bool m_shotEffect;
    int m_shotDuration;
    int m_shotElapsed;
    float m_wanderLimitH;
    float m_wanderLimitV;
    float m_wanderAmplitudeH;
    float m_wanderAmplitudeV;
    int m_wanderPeriodH;
    int m_wanderPeriodV;
    int m_wanderElapsedH;
    int m_wanderElapsedV;
    float m_wanderAngleH;
    float m_wanderAngleV;
    FrustumCulling m_frustum;            // 0xb8
    SViewport m_viewport;                // 0x11c
    swvVector3D m_initialProjectionPoint;
    swvVector3D m_projectionPoint;
    swvVector3D m_bulletStartPosition;
    swvVector3D m_bulletEndPosition;
    bool m_wanderDisabled;               // 0x154
    int m_returnTimeLeft;
    float m_returnSpeedV;
    float m_returnSpeedH;
    bool m_returningToCenter;
    bool m_returningToSavedView;
};

#endif
