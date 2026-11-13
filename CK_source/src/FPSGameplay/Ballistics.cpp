// CK ARMv7 Ballistics.cpp (u418). The pool and smoke rendering reuse the shared CKZ code shape.
#include "Ballistics.h"

#include "App.h"
#include "CRandGen.h"
#include "CSwerve.h"
#include "CSwerveGame.h"
#include "DGHelper.h"
#include "dgCamera.h"

#include <math.h>
#include <string.h>

typedef ::Group Group;
typedef ::World World;
typedef ::Transform Transform;

namespace {
float Length(const vec3& value)
{
    return MathLib::Sqrt(value.LengthSqr());
}

vec3 Unit(const vec3& value)
{
    return value * MathLib::InvSqrt(value.LengthSqr());
}

float Sign(float value)
{
    if (value < 0.0f) {
        return -1.0f;
    }
    if (value > 0.0f) {
        return 1.0f;
    }
    return 0.0f;
}

void WrapSmokeAngle(float& angle)
{
    if (angle < -180.0f) {
        angle += 360.0f;
    }
    if (angle > 180.0f) {
        angle = 360.0f - angle;
    }
}
}

Ballistics::Ballistics(int capacity, float maxAge, vec3 gravity, CCollision* collision)
    : m_gravity()
{
    m_collision = collision;
    m_capacity = capacity;
    m_next = 0;
    m_activeCount = 0;
    m_activeIndices = new int[capacity];
    m_projectiles = new Projectile[m_capacity];
    m_maxAge = maxAge;
    m_gravity = gravity;
    for (int index = 0; index < m_capacity; ++index) {
        m_projectiles[index].m_active = false;
    }
    m_normalBounce = 0.5f;
    m_tangentRetention = 0.5f;
}

Ballistics::~Ballistics()
{
    delete[] m_activeIndices;
    delete[] m_projectiles;
}

void Ballistics::Simulate(float elapsed)
{
    m_activeCount = 0;
    for (int index = 0; index < m_capacity; ++index) {
        Projectile& projectile = m_projectiles[index];
        if (projectile.m_active) {
            m_activeIndices[m_activeCount] = index;
            m_activeCount++;
            projectile.m_previous = projectile.m_position;
        }
    }
    while (elapsed > 0.0f) {
        float step = 0.05f < elapsed ? 0.05f : elapsed;
        for (int index = 0; index < m_activeCount; ++index) {
            Projectile& projectile = m_projectiles[m_activeIndices[index]];
            if (projectile.m_active) {
                projectile.m_age += step;
                if (projectile.m_age <= m_maxAge) {
                    projectile.m_position = projectile.m_position + projectile.m_velocity * step;
                    projectile.m_velocity = projectile.m_velocity + m_gravity * step;
                } else {
                    projectile.m_active = false;
                    OnActivate(&projectile);
                }
            }
        }
        elapsed -= step;
    }
    if (m_collision == NULL || m_activeCount == 0) {
        return;
    }
    vec3 point;
    vec3 normal;
    for (int index = 0; index < m_activeCount; ++index) {
        Projectile& projectile = m_projectiles[m_activeIndices[index]];
        if (!projectile.m_active) {
            continue;
        }
        if (Length(projectile.m_position - projectile.m_previous) <= 0.0f) {
            continue;
        }
        vec3 hit;
        vec3 hitNormal;
        CCollision::Triangle triangle;
        CCollision::Line line = {projectile.m_previous, projectile.m_position};
        bool intersected = m_collision->IntersectAny(line, hit, hitNormal, triangle);
        float fraction = Length(hit - projectile.m_previous) / Length(projectile.m_position - projectile.m_previous);
        if (fraction > 1.0f || !intersected) {
            continue;
        }
        point = projectile.m_previous + (projectile.m_position - projectile.m_previous) * fraction;
        normal = hitNormal;
        OnCollision(&projectile, &point, &normal);
        // Shipped order: deactivation callback first, then re-test the flag before bouncing.
        if (!projectile.m_active) {
            OnActivate(&projectile);
        }
        if (projectile.m_active) {
            vec3 normalVelocity = normal * projectile.m_velocity.Dot(normal);
            vec3 tangentVelocity = projectile.m_velocity - normalVelocity;
            projectile.m_velocity = tangentVelocity * m_tangentRetention - normalVelocity * m_normalBounce;
            projectile.m_position = point;
        }
    }
}

int Ballistics::AddProjectile(const vec3& position, const vec3& velocity, float spread, void* userData)
{
    // Shipped: the oldest slot is overwritten without notifying its owner.
    int index = m_next;
    Projectile& projectile = m_projectiles[index];
    int next = index + 1;
    if (next == m_capacity) {
        next = 0;
    }
    m_next = next;
    projectile.m_active = true;
    projectile.m_age = 0.0f;
    projectile.m_position = position;
    projectile.m_previous = position;
    projectile.m_velocity = velocity;
    if (spread != 0.0f) {
        vec3 forward = Unit(projectile.m_velocity);
        vec3 right = Unit(vec3(forward.m_y, forward.m_z, forward.m_x).Cross(forward));
        vec3 up = Unit(forward.Cross(right));
        Angles angles(forward, up);
        Mat4x4 matrix(1.0f);
        matrix.SetAngles(angles);
        Quaternion orientation;
        memset(&orientation, 0, sizeof(orientation));
        matrix.ToQuaternion(orientation);

        float yaw = (10000 - static_cast<int>(CRandGen::GetInstance()->GetRand(20000))) * 0.0001f * spread;
        float pitch = (10000 - static_cast<int>(CRandGen::GetInstance()->GetRand(20000))) * 0.0001f * spread;
        Quaternion horizontal;
        horizontal.m_x = right.m_x;
        horizontal.m_y = right.m_y;
        horizontal.m_z = right.m_z;
        horizontal.m_w = yaw * 3.1415925f / 360.0f;
        Quaternion vertical;
        vertical.m_x = up.m_x;
        vertical.m_y = up.m_y;
        vertical.m_z = up.m_z;
        vertical.m_w = pitch * 3.1415925f / 360.0f;
        orientation = orientation.Mul(horizontal.Mul(vertical));
        orientation.ToAngles_ByKashtur(angles);
        vec3 direction = Unit(angles.Forward());
        float speedSquared = projectile.m_velocity.LengthSqr();
        float speed = speedSquared * MathLib::InvSqrt(speedSquared);
        projectile.m_velocity = direction * speed;
    }
    projectile.m_userData = userData;
    OnActivate(&projectile);
    return index;
}

SmokeTrail::SmokeTrail(int capacity, float age, CCollision* collision)
    : Ballistics(capacity, age, vec3(), collision)
{
    m_scene = World(App::LoadObject3D("BIN_M3G_EFFECT_SMOKE_SYSTEM"));
    m_root = Group(m_scene.find(DGHelper::getSwerveID("RootNode")));
    for (int index = 0; index < 16; ++index) {
        XString name = XString::Format(L"Particle%02dTransform", index + 1);
        XString::AnsiString ansiName(name.c_str());
        m_variants[index] = Group(m_root.find(DGHelper::getSwerveID(ansiName)));
    }
    m_rotationNext = 0;
    m_rotations = new Rotation[capacity];
    m_basis = Transform(CSwerve::GetInstance()->m_swerve);
    m_scratch = Transform(CSwerve::GetInstance()->m_swerve);
}

SmokeTrail::~SmokeTrail()
{
    delete[] m_rotations;
}

int SmokeTrail::AddProjectile(const vec3& position, const vec3& velocity, float spread)
{
    float angle = static_cast<float>(CRandGen::GetInstance()->GetRand(720)) - 360.0f;
    m_rotations[m_rotationNext].m_angle = angle;
    float speed = static_cast<float>(CRandGen::GetInstance()->GetRand(180)) - 90.0f;
    m_rotations[m_rotationNext].m_speed = speed;

    // The shipped add path stores the angle after each of the two bounds checks.
    float& initialAngle = m_rotations[m_rotationNext].m_angle;
    float wrapped = initialAngle;
    if (initialAngle < -180.0f) {
        wrapped = initialAngle + 360.0f;
    }
    initialAngle = wrapped;
    float& storedAngle = m_rotations[m_rotationNext].m_angle;
    wrapped = storedAngle;
    if (storedAngle > 180.0f) {
        wrapped = 360.0f - storedAngle;
    }
    storedAngle = wrapped;

    int index = Ballistics::AddProjectile(position, velocity, spread, &m_rotations[m_rotationNext]);
    ++m_rotationNext;
    if (m_rotationNext == m_capacity) {
        m_rotationNext = 0;
    }
    return index;
}

void SmokeTrail::Simulate(float elapsed)
{
    Ballistics::Simulate(elapsed);
    // The original spins every listed projectile (no m_active test) up to the count read once.
    int count = m_activeCount;
    for (int index = 0; index < count; ++index) {
        Rotation* spin = static_cast<Rotation*>(m_projectiles[m_activeIndices[index]].m_userData);
        spin->m_angle += spin->m_speed * elapsed;
        WrapSmokeAngle(spin->m_angle);
    }
}

void SmokeTrail::Paint3D(Graphics3D& graphics)
{
    int count = m_activeCount;
    App* app = static_cast<App*>(WindowApp::m_instance);
    vec3 camera = app->m_helper->m_swerveGame->GetCurrentCameraPos();
    app = static_cast<App*>(WindowApp::m_instance);
    vec3 viewDirection = app->m_helper->m_swerveGame->m_camera->GetViewDirection();
    float values[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    Transform renderTransform(CSwerve::GetInstance()->m_swerve);
    renderTransform.setIdentity();
    int batch = 0;
    for (int index = 0; index < count; ++index) {
        Projectile& projectile = m_projectiles[m_activeIndices[index]];
        if (!projectile.m_active) {
            continue;
        }
        Rotation* spin = static_cast<Rotation*>(projectile.m_userData);
        float age = projectile.m_age / m_maxAge;
        if (age > 1.0f) {
            age = 1.0f;
        }
        float scale = MathLib::Pow(age, 0.5f);
        if (scale < 0.01f) {
            scale = 0.01f;
        }
        vec3 center = projectile.m_previous + (projectile.m_position - projectile.m_previous) * 0.5f;
        vec3 front = Unit(center - camera);
        vec3 up = Unit(vec3(0.0f, 0.0f, 1.0f));
        vec3 side = Unit(up.Cross(front));
        up = up * scale * 100.0f;
        side = side * scale * 100.0f;
        vec3 depth = front * scale * 100.0f;
        values[0] = up.m_x;
        values[1] = side.m_x;
        values[2] = depth.m_x;
        values[3] = 0.0f;
        values[4] = up.m_y;
        values[5] = side.m_y;
        values[6] = depth.m_y;
        values[7] = 0.0f;
        values[8] = up.m_z;
        values[9] = side.m_z;
        values[10] = depth.m_z;
        values[11] = 0.0f;
        m_basis.set(values);
        m_scratch.setIdentity();
        float rotation[4] = {spin->m_angle, depth.m_x, depth.m_y, depth.m_z};
        m_scratch.postRotate(rotation);
        m_scratch.postMultiply(m_basis);
        m_basis.multiply(m_scratch);
        m_scratch.setIdentity();
        m_scratch.postTranslate(&center.m_x);
        m_scratch.postMultiply(m_basis);
        m_variants[batch].setTransform(m_scratch);
        m_variants[batch].setAlphaFactor(1.0f - projectile.m_age / m_maxAge);
        ++batch;
        if (batch == 16) {
            graphics.render(m_root, renderTransform);
            batch = 0;
        }
    }
    if (batch > 0) {
        vec3 hidden = camera - viewDirection * 10000.0f;
        values[3] = hidden.m_x;
        values[7] = hidden.m_y;
        values[11] = hidden.m_z;
        m_basis.set(values);
        for (int index = batch; index < 16; ++index) {
            m_variants[index].setTransform(m_basis);
        }
        graphics.render(m_root, renderTransform);
    }
}

GrenadeLauncher::GrenadeLauncher(CCollision* collision, const Vector<CUnit*>& units, const char* model)
    : Ballistics(32, 10.0f, vec3(), collision), m_units(units)
{
    m_model = World(App::LoadObject3D(model));
    m_smoke = new SmokeTrail(128, 0.5f, NULL);
    m_flightNext = 0;
    m_flights = new Flight[32];
}

GrenadeLauncher::~GrenadeLauncher()
{
    delete m_smoke;
    delete[] m_flights;
}

void GrenadeLauncher::OnCollision(Projectile* projectile, vec3* point, vec3* normal)
{
    projectile->m_active = false;
    static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->BlastGrenade(
        *point, *normal, *static_cast<Flight*>(projectile->m_userData)->m_weapon);
    *point = *point + *normal * 0.1f;
}

void GrenadeLauncher::AddProjectile(const vec3& position, const vec3& velocity, float spread,
                                    const CDH_Weapon& weapon)
{
    Flight& flight = m_flights[m_flightNext];
    flight.m_weapon = &weapon;
    flight.m_distance = 0.0f;
    flight.m_trailDistance = 0.0f;
    Ballistics::AddProjectile(position, velocity, spread, &m_flights[m_flightNext]);
    if (++m_flightNext == m_capacity) {
        m_flightNext = 0;
    }
}

void GrenadeLauncher::Simulate(float elapsed)
{
    Ballistics::Simulate(elapsed);
    m_smoke->Simulate(elapsed);
    vec3 up(0.0f, 1.0f, 0.0f);
    unsigned int count = m_activeCount;
    for (unsigned int index = 0; index < count; ++index) {
        Projectile& projectile = m_projectiles[m_activeIndices[index]];
        if (!projectile.m_active) {
            continue;
        }
        // Units: accept a box hit only when it lies within this frame's segment.
        for (unsigned int unitIndex = 0; unitIndex < (unsigned int)m_units.size(); ++unitIndex) {
            CUnit* unit = m_units[unitIndex];
            if (unit->m_dead) {
                continue;
            }
            float height = unit->m_body.GetHeight();
            vec3 center = unit->m_body.m_position;
            CCollision::AABB bounds = {
                center - vec3(0.4f, 0.4f, 0.0f),
                center + vec3(0.4f, 0.4f, height)
            };
            vec3 point;
            CCollision::Line line = {projectile.m_previous, projectile.m_position};
            if (CCollision::Intersect(line, bounds, point)) {
                if (Length(point - projectile.m_previous) <= Length(projectile.m_position - projectile.m_previous)) {
                    OnCollision(&projectile, &point, &up);
                    if (!projectile.m_active) {
                        break;
                    }
                }
            }
        }
        if (!projectile.m_active) {
            continue;
        }
        // Smoke puffs every 0.25 units along the flight path.
        Flight* flight = static_cast<Flight*>(projectile.m_userData);
        vec3 position = projectile.m_previous;
        vec3 direction = Unit(projectile.m_position - projectile.m_previous);
        float distance = Length(projectile.m_position - projectile.m_previous);
        if (distance + flight->m_trailDistance < 0.25f) {
            flight->m_trailDistance = flight->m_trailDistance + distance;
        } else {
            float first = 0.25f - flight->m_trailDistance;
            position = position + direction * first;
            distance = distance - first;
            m_smoke->AddProjectile(position, direction * -0.25f, 0.0f);
            flight->m_trailDistance = 0.0f;
            while (distance > 0.25f) {
                position = position + direction * 0.25f;
                m_smoke->AddProjectile(position, direction * -0.25f, 0.0f);
                distance = distance - 0.25f;
            }
            flight->m_trailDistance = distance;
        }
        flight->m_distance = flight->m_distance + distance;
    }
}

namespace {
/** Places the grenade model at (x, y, z) with its +X axis along `velocity`. */
void PlaceGrenade(Transform& transform, float x, float y, float z, const vec3& velocity)
{
    float horizontal[4];
    float vertical[4];
    float translation[3];
    translation[0] = x;
    translation[1] = y;
    translation[2] = z;
    transform.postTranslate(translation);
    // No zero-length guard, as shipped.
    vec3 direction = Unit(velocity);
    vec3 back(-direction.m_x, -direction.m_y, -direction.m_z);
    vec3 planar(back.m_x, back.m_y, 0.0f);
    float pitch = Sign(back.m_z) * MathLib::ArcCos(Length(planar) / Length(back));
    float base = 0.0f;
    float factor = 1.0f;
    if (back.m_x < 0.0f) {
        base = 180.0f;
        factor = -1.0f;
    }
    float yaw = Sign(back.m_y) * (base + factor * MathLib::ArcCos(Length(vec3(back.m_x, 0.0f, 0.0f)) / Length(planar))) + 180.0f;
    horizontal[0] = yaw;
    horizontal[1] = 0.0f;
    horizontal[2] = 0.0f;
    horizontal[3] = 1.0f;
    transform.postRotate(horizontal);
    vertical[0] = pitch;
    vertical[1] = 0.0f;
    vertical[2] = 1.0f;
    vertical[3] = 0.0f;
    transform.postRotate(vertical);
}
}

void GrenadeLauncher::Paint3D(Graphics3D& graphics)
{
    int index = m_activeCount;
    Transform transform(CSwerve::GetInstance()->m_swerve);
    while (index > 0) {
        index--;
        const Projectile& projectile = m_projectiles[m_activeIndices[index]];
        if (projectile.m_active) {
            transform.setIdentity();
            const vec3& position = projectile.m_position;
            PlaceGrenade(transform, position.m_x, position.m_y, position.m_z, projectile.m_velocity);
            graphics.render(m_model, transform);
        }
    }
    m_smoke->Paint3D(graphics);
}

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AssertProjectileSize[sizeof(Ballistics::Projectile) == 0x30 ? 1 : -1];
#endif
#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AssertBallisticsSize[sizeof(Ballistics) == 0x34 ? 1 : -1];
#endif
#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AssertSmokeTrailSize[sizeof(SmokeTrail) == 0x8c ? 1 : -1];
#endif
#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AssertGrenadeLauncherSize[sizeof(GrenadeLauncher) == 0x48 ? 1 : -1];
#endif
