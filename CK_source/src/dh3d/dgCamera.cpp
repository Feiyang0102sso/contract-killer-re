// Camera motion, projection, recoil and zoom for the CK mission world.
#include "dgCamera.h"

#include "CDH_BasicGameData.h"
#include "CDH_Weapon.h"
#include "CRandGen.h"
#include "CSwerveGame.h"
#include "DGHelper.h"
#include "Fixed.h"
#include "GamePlayHUD.h"

namespace {
typedef ::Group Group;
typedef int (*SetChild)(IQueryInterface*, IQueryInterface*);
typedef int (*Align)(IQueryInterface*, IQueryInterface*, int, IQueryInterface*, int);
typedef int (*GetTransformTo)(IQueryInterface*, INodeInterface*, ITransformInterface*, bool*);
}

DGCamera::DGCamera(Camera camera, float horizontalLimit, float verticalLimit)
    : m_camera(camera), m_baseTransform(CSwerve::GetInstance()->m_swerve),
      m_defaultFov(-1.0f), m_horizontalLimit(horizontalLimit), m_verticalLimit(verticalLimit + 15.0f),
      m_zoomEffect(false), m_savedFov(m_fov), m_recoilAngle(0),
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
    m_viewport.m_x = background.getCropX();
    m_viewport.m_y = background.getCropY();
    m_viewport.m_width = background.getCropWidth();
    m_viewport.m_height = background.getCropHeight();
    Transform projection = static_cast<App*>(WindowApp::m_instance)->m_helper->m_transform;
    m_camera.getProjection(projection);
    projection.invert();
    m_initialProjectionPoint.x = -1.0f;
    m_initialProjectionPoint.y = -1.0f;
    m_initialProjectionPoint.z = 0;
    DGHelper::TransformPoint(projection, m_initialProjectionPoint);
    m_projectionPoint = m_initialProjectionPoint;
    m_frustum.UpdatePlanes();
    background.setColorClearEnable(false);
    m_headShotFov = static_cast<App*>(WindowApp::m_instance)->m_helper->m_config.GetFloatValueByToken("HEAD_SHOT_EFFECT_FOV", true);
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
    bool connected;
    App* app = static_cast<App*>(WindowApp::m_instance);
    Transform transform = app->m_helper->m_transform;
    m_camera.setTransform(m_baseTransform);
    {
        CSwerveGame* game = static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame;
        Transform output = transform;
        IQueryInterface* world = game->m_world.m_interface;
        CKMethod<GetTransformTo>(world, 0xa4)(world, m_camera.node(), output.transformInterface(), &connected);
    }
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
    if (m_recoilElapsed >= 0) {
        float rotation[4];
        if (m_recoilElapsed < m_recoilDuration * 0.1) {
            rotation[0] = m_recoilAngle * static_cast<float>(m_recoilElapsed) / m_recoilDuration;
        } else {
            float duration = m_recoilDuration * 0.9f;
            rotation[0] = m_recoilAngle * (1.0f - (m_recoilElapsed - duration) / duration);
        }
        rotation[1] = 1.0f;
        rotation[2] = 0.0f;
        rotation[3] = 0.0f;
        transform.postRotate(rotation);
    } else if (!m_wanderDisabled && (m_wanderLimitH > 0 || m_wanderLimitV > 0)) {
        float rotation[] = {m_wanderAngleH, 0.0f, 1.0f, 0.0f};
        transform.postRotate(rotation);
        rotation[0] = m_wanderAngleV;
        rotation[1] = 1.0f;
        rotation[2] = 0.0f;
        rotation[3] = 0.0f;
        transform.postRotate(rotation);
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
    if (m_shotEffect) {
        if (m_shotElapsed >= m_shotDuration) {
            m_shotEffect = false;
            App* app = static_cast<App*>(WindowApp::m_instance);
            Group bullet(app->m_helper->m_swerveGame->m_world.find(DGHelper::getSwerveID("bulletTransform")));
            app = static_cast<App*>(WindowApp::m_instance);
            Group cylinder(app->m_helper->m_swerveGame->m_world.find(DGHelper::getSwerveID("Cylinder03Transform")));
            bullet.setRenderingEnable(false);
            cylinder.setRenderingEnable(true);
        } else {
            int duration = m_shotDuration;
            int elapsed = m_shotElapsed + elapsedMs;
            if (elapsed >= duration) {
                elapsed = duration;
            }
            m_shotElapsed = elapsed;
            float phase = static_cast<float>(elapsed) / duration;
            float camera[] = {
                (m_shotDisplacement.x - 3.0f) * phase + m_shotStartPosition.x,
                (m_shotDisplacement.y + 3.0f) * phase + m_shotStartPosition.y,
                (m_shotDisplacement.z + 3.0f) * phase + (m_shotStartPosition.z + 3.0f)
            };
            float bulletPhase = phase + 0.1f;
            if (bulletPhase > 1.0f) {
                bulletPhase = 1.0f;
            }
            float position[] = {
                (m_bulletEndPosition.x - m_bulletStartPosition.x) * bulletPhase + m_bulletStartPosition.x,
                (m_bulletEndPosition.y - m_bulletStartPosition.y) * bulletPhase + m_bulletStartPosition.y,
                (m_bulletEndPosition.z - m_bulletStartPosition.z) * bulletPhase + m_bulletStartPosition.z
            };
            m_camera.setTranslation(camera);
            App* app = static_cast<App*>(WindowApp::m_instance);
            Group bullet(app->m_helper->m_swerveGame->m_world.find(DGHelper::getSwerveID("bulletTransform")));
            bullet.setTranslation(position);
            app = static_cast<App*>(WindowApp::m_instance);
            CSwerveGame::World world = app->m_helper->m_swerveGame->m_world;
            float scale[] = {-1.0f, 1.0f, -1.0f};
            Group target(CSwerve::GetInstance()->m_swerve);
            target.setTranslation(&m_bulletEndPosition.x);
            world.addChild(target);
            m_camera.setScale(scale);
            CKMethod<Align>(m_camera.m_interface, 0xa8)(m_camera.m_interface, target.m_interface,
                                                       0x91, world.m_interface, 0x94);
            CKMethod<SetChild>(m_camera.m_interface, 0xb4)(m_camera.m_interface, bullet.m_interface);
            CKMethod<SetChild>(world.m_interface, 0xc4)(world.m_interface, target.m_interface);
        }
    } else {
        if (m_recoilElapsed >= 0) {
            m_recoilElapsed += elapsedMs;
            updateTransform(false);
            if (m_recoilElapsed > m_recoilDuration) {
                m_recoilElapsed = -1;
            }
        } else {
            updateWander(elapsedMs);
            updateTransform(false);
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
    if (!(m_wanderLimitH > 0) && !(m_wanderLimitV > 0)) {
        return;
    }
    m_wanderElapsedH += elapsedMs;
    if (m_wanderElapsedH > m_wanderPeriodH) {
        m_wanderElapsedH = 0;
        m_wanderAmplitudeH = static_cast<unsigned int>(CRandGen::GetInstance()->GetRand(10) + 1) * m_wanderLimitH / 10.0f;
    }
    float phaseH = static_cast<float>(m_wanderElapsedH) / m_wanderPeriodH;
    m_wanderAngleH = Fixed::toFloat(Fixed::sin(Fixed::fromFloat(phaseH * 360.0f))) * m_wanderAmplitudeH;
    m_wanderElapsedV += elapsedMs;
    if (m_wanderElapsedV > m_wanderPeriodV) {
        m_wanderElapsedV = 0;
        m_wanderAmplitudeV = static_cast<unsigned int>(CRandGen::GetInstance()->GetRand(10) + 1) * m_wanderLimitV / 10.0f;
    }
    float phaseV = static_cast<float>(m_wanderElapsedV) / m_wanderPeriodV;
    m_wanderAngleV = Fixed::toFloat(Fixed::sin(Fixed::fromFloat(phaseV * 360.0f))) * m_wanderAmplitudeV;
    App* app = static_cast<App*>(WindowApp::m_instance);
    float skill = app->m_helper->m_gamePlayManager.m_playerData.GetSkillLevelCoef(static_cast<CDH_PlayerData::skill_type_t>(0));
    app = static_cast<App*>(WindowApp::m_instance);
    float divisor = skill + skill + 1.0f + app->m_helper->GetPlayerData()->m_fieldac;
    m_wanderAngleH = m_wanderAngleH / divisor * 0.05f;
    m_wanderAngleV = m_wanderAngleV / divisor * 0.05f;
    app = static_cast<App*>(WindowApp::m_instance);
    if (app->m_helper->m_swerveGame->m_hud->IsSteadyAimActive()) {
        float coefficient = app->m_helper->m_config.GetFloatValueByToken("STEADY_REDUCE_WANDER_COEF", true);
        m_wanderAngleH *= coefficient;
        m_wanderAngleV *= coefficient;
    }
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
    App* app = static_cast<App*>(WindowApp::m_instance);
    vec3 nearPoint = app->m_helper->GetPointOnNear(x, y, m_camera);
    int horizontalFixed = Fixed::rad2grad(Fixed::atan(Fixed::fromFloat(nearPoint.m_x / nearPoint.m_z)));
    int verticalFixed = Fixed::rad2grad(Fixed::atan(Fixed::fromFloat(nearPoint.m_y / nearPoint.m_z)));
    float horizontal = Fixed::toFloat(horizontalFixed);
    float vertical = Fixed::toFloat(verticalFixed);
    float down = Fixed::toFloat(verticalFixed) * -1.0f;
    if (horizontal < -0.0f) {
        rotateRight(-horizontal, false);
    } else {
        rotateLeft(horizontal, false);
    }
    if (vertical < -0.0f) {
        rotateDown(down, false);
    } else {
        rotateUp(vertical, false);
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
    Transform transform(CSwerve::GetInstance()->m_swerve);
    bool connected;
    App* app = static_cast<App*>(WindowApp::m_instance);
    CSwerveGame* game = app->m_helper->m_swerveGame;
    {
        Transform output = transform;
        CKMethod<GetTransformTo>(m_camera.m_interface, 0xa4)(m_camera.m_interface, game->m_world.node(),
                                                            output.transformInterface(), &connected);
    }
    float matrix[16];
    transform.get(matrix);
    vec3 direction;
    direction.m_x = matrix[2];
    direction.m_y = matrix[6];
    direction.m_z = matrix[10];
    return direction;
}

typedef char AssertDGCameraSize[sizeof(DGCamera) == 0x168 ? 1 : -1];
