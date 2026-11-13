// Camera motion, projection, recoil and zoom for the CK mission world.
#include "dgCamera.h"

#include "CDH_BasicGameData.h"
#include "CDH_Weapon.h"
#include "CRandGen.h"
#include "CSwerveGame.h"
#include "DGHelper.h"
#include "Fixed.h"

#include <math.h>

namespace {
typedef ::Group Group;
typedef int (*SetVector)(IQueryInterface*, const float*);
typedef int (*SetChild)(IQueryInterface*, IQueryInterface*);
typedef int (*SetRendering)(IQueryInterface*, bool);

float AimAngleFromSlope(float slope)
{
    // The original fixed-point series is accurate near the scope center.
    if (fabsf(slope) <= 1.0f) {
        return Fixed::toFloat(Fixed::rad2grad(Fixed::atan(Fixed::fromFloat(slope))));
    }
    return atanf(slope) * 57.2957795f;
}
}

DGCamera::DGCamera(Camera camera, float horizontalLimit, float verticalLimit)
    : m_camera(camera), m_baseTransform(CSwerve::GetInstance()->m_swerve),
      m_defaultFov(-1.0f), m_horizontalLimit(horizontalLimit), m_verticalLimit(verticalLimit + 15.0f),
      m_zoomEffect(false), m_savedFov(-1.0f), m_effectFov(-1.0f), m_recoilAngle(0),
      m_recoilDuration(0), m_recoilElapsed(-1), m_fov(-1.0f),
      m_viewportWidth(1.0f), m_viewportHeight(1.0f), m_shotEffect(false), m_shotDuration(0), m_shotElapsed(0),
      m_wanderLimitH(0), m_wanderLimitV(0), m_wanderAmplitudeH(0), m_wanderAmplitudeV(0),
      m_wanderPeriodH(0), m_wanderPeriodV(0), m_wanderElapsedH(0), m_wanderElapsedV(0),
      m_wanderAngleH(0), m_wanderAngleV(0), m_frustum(camera), m_wanderDisabled(false)
{
    Init(camera, horizontalLimit, verticalLimit);
    ReInit();
}

DGCamera::~DGCamera()
{
    m_camera.setTransform(m_baseTransform);
    restoreView(false);
}

void DGCamera::Init(Camera camera, float horizontalLimit, float verticalLimit)
{
    m_camera = camera;
    m_horizontalLimit = horizontalLimit;
    m_verticalLimit = verticalLimit + 15.0f;
    App* app = static_cast<App*>(WindowApp::m_instance);
    Background background = app->m_helper->m_swerveGame->m_world.getBackground();
    if (background.m_interface != NULL) {
        m_viewport.m_x = background.getCropX();
        m_viewport.m_y = background.getCropY();
        m_viewport.m_width = background.getCropWidth();
        m_viewport.m_height = background.getCropHeight();
    }
    Transform projection = app->m_helper->m_transform;
    m_camera.getProjection(projection);
    projection.invert();
    m_initialProjectionPoint.x = -1.0f;
    m_initialProjectionPoint.y = -1.0f;
    m_initialProjectionPoint.z = 0;
    DGHelper::TransformPoint(projection, m_initialProjectionPoint);
    m_projectionPoint = m_initialProjectionPoint;
    m_frustum.UpdatePlanes();
    if (background.m_interface != NULL) {
        background.setColorClearEnable(false);
    }
    m_headShotFov = app->m_helper->m_config.GetFloatValueByToken("HEAD_SHOT_EFFECT_FOV", true);
}

void DGCamera::ReInit()
{
    m_camera.getCompositeTransform(m_baseTransform);
    m_defaultFov = GetFOV();
    SetFOV(m_defaultFov);
    m_field0c = 0;
    m_previousAngleH = 0;
    m_previousAngleV = 0;
    m_horizontalAxis.x = 0;
    m_horizontalAxis.y = 1;
    m_horizontalAxis.z = 0;
    m_field1c = 0;
    m_verticalAxis.x = 1;
    m_verticalAxis.y = 0;
    m_verticalAxis.z = 0;
    m_returnTimeLeft = 0;
    m_returnSpeedV = 0;
    m_returnSpeedH = 0;
    m_returningToCenter = false;
    m_returningToSavedView = false;
    m_savedAngleH = 0;
    m_savedAngleV = 0;
}

float DGCamera::GetFOV() const
{
    // NaN takes the projection path, as in the original (bgt).
    if (!(m_fov > 0)) {
        float params[4];
        m_camera.getProjection(params);
        return params[0];
    }
    return m_fov;
}

void DGCamera::SetFOV(float fov)
{
    if (m_fov == fov) {
        return;
    }
    m_fov = fov;
    float params[4];
    m_camera.getProjection(params);
    m_camera.setPerspective(fov, params[1], params[2], params[3]);
    m_frustum.UpdatePlanes();
    App* app = static_cast<App*>(WindowApp::m_instance);
    Transform projection = app->m_helper->m_transform;
    m_camera.getProjection(projection);
    projection.invert();
    swvVector3D v = {-1.0f, -1.0f, 0.0f};  // the original constant is SetFOV::v
    DGHelper::TransformPoint(projection, v);
    m_projectionPoint.x = -1.0f;
    m_projectionPoint.y = -1.0f;
    m_projectionPoint.z = 0;
    DGHelper::TransformPoint(projection, m_projectionPoint);
}

void DGCamera::restoreView(bool immediately)
{
    if (!immediately) {
        centerView(0);
        SetWandering(0, 0, 0, 0);
        rotateLeft(0, false);
        rotateRight(0, false);
        rotateUp(0, false);
        rotateDown(0, false);
    }
    SetFOV(m_defaultFov);
}

void DGCamera::updateTransform(bool)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    Transform transform = app->m_helper->m_transform;
    m_camera.setTransform(m_baseTransform);
    app->m_helper->m_swerveGame->m_world.getTransformTo(m_camera, transform);
    static const swvVector3D axis_end = {0, 0, 1};
    swvVector3D end = axis_end;
    swvVector3D start = {0, 0, 0};
    DGHelper::TransformPoint(transform, end);
    DGHelper::TransformPoint(transform, start);
    m_horizontalAxis.x = end.x - start.x;
    m_horizontalAxis.y = end.y - start.y;
    m_horizontalAxis.z = end.z - start.z;

    transform.setIdentity();
    transform.postRotate(&m_field0c);
    transform.postRotate(&m_field1c);
    transform.postMultiply(m_baseTransform);
    if (m_recoilElapsed >= 0 && m_recoilDuration > 0) {
        float elapsed = m_recoilElapsed;
        float duration = m_recoilDuration;
        float recoil;
        if (elapsed < duration * 0.1) {
            recoil = m_recoilAngle * elapsed / duration;
        } else {
            duration *= 0.9f;
            recoil = m_recoilAngle * (1.0f - (elapsed - duration) / duration);
        }
        float rotation[] = {recoil, 1.0f, 0.0f, 0.0f};
        transform.postRotate(rotation);
    } else if (!m_wanderDisabled && (m_wanderLimitH > 0 || m_wanderLimitV > 0)) {
        float horizontal[] = {m_wanderAngleH, 0.0f, 1.0f, 0.0f};
        float vertical[] = {m_wanderAngleV, 1.0f, 0.0f, 0.0f};
        transform.postRotate(horizontal);
        transform.postRotate(vertical);
    }
    m_camera.setTransform(transform);
    m_previousAngleH = m_field0c;
    m_previousAngleV = m_field1c;
}

bool DGCamera::rotateLeft(float angle, bool unconstrained)
{
    bool moved = false;
    if (!m_returningToCenter) {
        moved = true;
        m_field0c += angle;
        if (!unconstrained && m_field0c > m_horizontalLimit) {
            m_field0c = m_horizontalLimit;
            moved = false;
        }
        updateTransform(false);
    }
    return moved;
}

bool DGCamera::rotateRight(float angle, bool unconstrained)
{
    bool moved = false;
    if (!m_returningToCenter) {
        moved = true;
        m_field0c -= angle;
        if (!unconstrained && m_field0c < -m_horizontalLimit) {
            m_field0c = -m_horizontalLimit;
            moved = false;
        }
        updateTransform(false);
    }
    return moved;
}

bool DGCamera::rotateUp(float angle, bool unconstrained)
{
    bool moved = false;
    if (!m_returningToCenter) {
        moved = true;
        m_field1c += angle;
        if (!unconstrained && m_field1c > m_verticalLimit) {
            m_field1c = m_verticalLimit;
            moved = false;
        }
        updateTransform(false);
    }
    return moved;
}

bool DGCamera::rotateDown(float angle, bool unconstrained)
{
    bool moved = false;
    if (!m_returningToCenter) {
        moved = true;
        m_field1c -= angle;
        if (!unconstrained && m_field1c < -m_verticalLimit) {
            m_field1c = -m_verticalLimit;
            moved = false;
        }
        updateTransform(false);
    }
    return moved;
}

void DGCamera::centerView(int elapsedMs)
{
    m_savedAngleH = m_field0c;
    m_savedAngleV = m_field1c;
    if (elapsedMs < 1) {
        m_field1c = 0;
        m_field0c = 0;
        updateTransform(false);
        return;
    }
    m_returnTimeLeft = elapsedMs;
    m_returningToCenter = true;
    m_returnSpeedV = m_field1c / elapsedMs;
    m_returnSpeedH = m_field0c / elapsedMs;
}

void DGCamera::returnToSavedView(int elapsedMs)
{
    m_returnTimeLeft = elapsedMs;
    m_returningToSavedView = true;
}

void DGCamera::RestoreOriginalFOV()
{
    SetFOV(m_defaultFov);
}

void DGCamera::SetWandering(float horizontalAmplitude, float verticalAmplitude, int horizontalPeriod, int verticalPeriod)
{
    m_wanderLimitH = horizontalAmplitude;
    m_wanderLimitV = verticalAmplitude;
    m_wanderPeriodH = horizontalPeriod;
    m_wanderPeriodV = verticalPeriod;
    m_wanderElapsedH = horizontalPeriod;
    m_wanderElapsedV = verticalPeriod;
    updateWander(1);
    m_wanderElapsedH = CRandGen::GetInstance()->GetRandRange(0, horizontalPeriod);
    m_wanderElapsedV = CRandGen::GetInstance()->GetRandRange(0, verticalPeriod);
    updateWander(0);
}

void DGCamera::Shot(float recoil, int durationMs)
{
    m_recoilAngle = recoil;
    m_recoilDuration = durationMs;
    m_recoilElapsed = 0;
}

void DGCamera::Update(int elapsedMs)
{
    UpdateReturnToCenter(elapsedMs);
    if (!m_shotEffect) {
        if (m_recoilElapsed < 0) {
            updateWander(elapsedMs);
            updateTransform(false);
        } else {
            m_recoilElapsed += elapsedMs;
            updateTransform(false);
            if (m_recoilElapsed > m_recoilDuration) {
                m_recoilElapsed = -1;
            }
        }
    } else {
        App* app = static_cast<App*>(WindowApp::m_instance);
        CSwerveGame::World& world = app->m_helper->m_swerveGame->m_world;
        Group bullet(world.find(DGHelper::getSwerveID("bulletTransform")));
        if (m_shotElapsed >= m_shotDuration) {
            m_shotEffect = false;
            Group cylinder(world.find(DGHelper::getSwerveID("Cylinder03Transform")));
            if (bullet.m_interface != NULL) {
                CKMethod<SetRendering>(bullet.m_interface, 0x90)(bullet.m_interface, false);
            }
            if (cylinder.m_interface != NULL) {
                CKMethod<SetRendering>(cylinder.m_interface, 0x90)(cylinder.m_interface, true);
            }
        } else {
            m_shotElapsed += elapsedMs;
            if (m_shotElapsed > m_shotDuration) {
                m_shotElapsed = m_shotDuration;
            }
            float phase = static_cast<float>(m_shotElapsed) / m_shotDuration;
            float camera[] = {
                m_shotStartPosition.x + (m_shotDisplacement.x - 3.0f) * phase,
                m_shotStartPosition.y + (m_shotDisplacement.y + 3.0f) * phase,
                m_shotStartPosition.z + (m_shotDisplacement.z + 3.0f) * phase + 3.0f
            };
            float bulletPhase = phase + 0.1f;
            if (bulletPhase > 1.0f) {
                bulletPhase = 1.0f;
            }
            float position[] = {
                m_bulletStartPosition.x + (m_bulletEndPosition.x - m_bulletStartPosition.x) * bulletPhase,
                m_bulletStartPosition.y + (m_bulletEndPosition.y - m_bulletStartPosition.y) * bulletPhase,
                m_bulletStartPosition.z + (m_bulletEndPosition.z - m_bulletStartPosition.z) * bulletPhase
            };
            CKMethod<SetVector>(m_camera.m_interface, 0x50)(m_camera.m_interface, camera);
            if (bullet.m_interface != NULL) {
                CKMethod<SetVector>(bullet.m_interface, 0x50)(bullet.m_interface, position);
                Group target(CSwerve::GetInstance()->m_swerve);
                CKMethod<SetVector>(target.m_interface, 0x50)(target.m_interface, &m_bulletEndPosition.x);
                CKMethod<SetChild>(world.m_interface, 0xc0)(world.m_interface, target.m_interface);
                static const swvVector3D scale = {-1.0f, 1.0f, -1.0f};
                CKMethod<SetVector>(m_camera.m_interface, 0x58)(m_camera.m_interface, &scale.x);
                typedef int (*Align)(IQueryInterface*, IQueryInterface*, int, IQueryInterface*, int);
                CKMethod<Align>(m_camera.m_interface, 0xa8)(m_camera.m_interface, target.m_interface,
                                                                  0x91, world.m_interface, 0x94);
                CKMethod<SetChild>(m_camera.m_interface, 0xb4)(m_camera.m_interface, bullet.m_interface);
                CKMethod<SetChild>(world.m_interface, 0xc4)(world.m_interface, target.m_interface);
            }
        }
    }
    HandleZoomEffect(elapsedMs);
    m_frustum.UpdatePlanes();
}

void DGCamera::UpdateReturnToCenter(int elapsedMs)
{
    if (m_returningToCenter) {
        m_returnTimeLeft -= elapsedMs;
        m_field1c -= m_returnSpeedV * elapsedMs;
        m_field0c -= m_returnSpeedH * elapsedMs;
        if (m_returnTimeLeft <= 0) {
            m_field1c = 0;
            m_field0c = 0;
            m_returningToCenter = false;
        }
        updateTransform(false);
    }
    if (m_returningToSavedView) {
        m_returnTimeLeft -= elapsedMs;
        m_field1c += m_returnSpeedV * elapsedMs;
        m_field0c += m_returnSpeedH * elapsedMs;
        if (m_returnTimeLeft <= 0) {
            m_field1c = m_savedAngleV;
            m_field0c = m_savedAngleH;
            m_returningToSavedView = false;
        }
        updateTransform(false);
    }
}

void DGCamera::updateWander(int elapsedMs)
{
    if (m_wanderLimitH <= 0 && m_wanderLimitV <= 0) {
        return;
    }
    m_wanderElapsedH += elapsedMs;
    if (m_wanderElapsedH > m_wanderPeriodH) {
        m_wanderElapsedH = 0;
        m_wanderAmplitudeH = (CRandGen::GetInstance()->GetRand(10) + 1) * m_wanderLimitH / 10.0f;
    }
    if (m_wanderPeriodH > 0) {
        float phase = static_cast<float>(m_wanderElapsedH) / m_wanderPeriodH;
        m_wanderAngleH = Fixed::toFloat(Fixed::sin(Fixed::fromFloat(phase * 360.0f))) * m_wanderAmplitudeH;
    } else {
        m_wanderAngleH = 0;
    }
    m_wanderElapsedV += elapsedMs;
    if (m_wanderElapsedV > m_wanderPeriodV) {
        m_wanderElapsedV = 0;
        m_wanderAmplitudeV = (CRandGen::GetInstance()->GetRand(10) + 1) * m_wanderLimitV / 10.0f;
    }
    if (m_wanderPeriodV > 0) {
        float phase = static_cast<float>(m_wanderElapsedV) / m_wanderPeriodV;
        m_wanderAngleV = Fixed::toFloat(Fixed::sin(Fixed::fromFloat(phase * 360.0f))) * m_wanderAmplitudeV;
    } else {
        m_wanderAngleV = 0;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    float skill = app->m_helper->GetPlayerData()->GetSkillLevelCoef(static_cast<CDH_PlayerData::skill_type_t>(0));
    float divisor = skill + skill + 1.0f + app->m_helper->GetPlayerData()->m_fieldac;
    m_wanderAngleH = m_wanderAngleH / divisor * 0.05f;
    m_wanderAngleV = m_wanderAngleV / divisor * 0.05f;
}

void DGCamera::HandleZoomEffect(int elapsedMs)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (app->m_helper->m_swerveGame->m_slowMode) {
        if (!m_zoomEffect) {
            m_savedFov = m_fov;
            m_zoomEffect = true;
            m_effectFov = m_fov;
        }
        m_effectFov += elapsedMs * -100.0f / 1000.0f;
        if (m_effectFov < m_headShotFov) {
            m_effectFov = m_headShotFov;
        }
        SetFOV(m_effectFov);
    } else if (m_zoomEffect) {
        m_zoomEffect = false;
        SetFOV(m_savedFov);
    }
}

void DGCamera::CheckConstraints()
{
    bool unchanged = true;
    if (m_field0c > m_horizontalLimit) {
        m_field0c = m_horizontalLimit;
        unchanged = false;
    } else if (m_field0c < -m_horizontalLimit) {
        m_field0c = -m_horizontalLimit;
        unchanged = false;
    }
    if (m_field1c > m_verticalLimit) {
        m_field1c = m_verticalLimit;
    } else if (m_field1c < -m_verticalLimit) {
        m_field1c = -m_verticalLimit;
    } else if (unchanged) {
        return;
    }
    updateTransform(false);
}

void DGCamera::SetTargetPoint(float x, float y)
{
    vec3 nearPoint = DGHelper::GetPointOnNear(x, y, m_camera);
    if (nearPoint.m_z == 0.0f) {
        return;
    }
    float horizontalSlope = nearPoint.m_x / nearPoint.m_z;
    float verticalSlope = nearPoint.m_y / nearPoint.m_z;
    if (!isfinite(horizontalSlope) || !isfinite(verticalSlope)) {
        return;
    }
    float horizontal = AimAngleFromSlope(horizontalSlope);
    float vertical = AimAngleFromSlope(verticalSlope);
    if (horizontal >= 0) {
        rotateLeft(horizontal, false);
    } else {
        rotateRight(-horizontal, false);
    }
    if (vertical >= 0) {
        rotateUp(vertical, false);
    } else {
        rotateDown(-vertical, false);
    }
}

void DGCamera::SetViewPortWidthAndHeight(float width, float height)
{
    m_viewportWidth = width;
    m_viewportHeight = height;
}

bool DGCamera::IsShotEffectInProgress() const
{
    return m_shotEffect;
}

void DGCamera::SetWanderngByActiveWeapon()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_Weapon* weapon = app->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
    if (weapon != NULL) {
        SetWandering(weapon->wanderAmount(), weapon->wanderAmount(), weapon->m_wanderTimeX, weapon->m_wanderTimeY);
    }
}

void DGCamera::SetAngleH(float angle, bool unconstrained)
{
    if (!m_returningToCenter && !m_returningToSavedView) {
        m_field0c = angle;
        if (!unconstrained) {
            CheckConstraints();
        }
    }
}

void DGCamera::SetAngleV(float angle, bool unconstrained)
{
    if (!m_returningToCenter && !m_returningToSavedView) {
        m_field1c = angle;
        if (!unconstrained) {
            CheckConstraints();
        }
    }
}

vec3 DGCamera::GetViewDirection()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    Transform transform(CSwerve::GetInstance()->m_swerve);
    m_camera.getTransformTo(app->m_helper->m_swerveGame->m_world, transform);
    float matrix[16];
    transform.get(matrix);
    return vec3(matrix[2], matrix[6], matrix[10]);
}

typedef char AssertDGCameraSize[sizeof(DGCamera) == 0x168 ? 1 : -1];
