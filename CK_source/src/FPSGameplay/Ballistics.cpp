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
    : m_capacity(capacity), m_next(0), m_activeCount(0), m_activeIndices(new int[capacity]),
      m_projectiles(new Projectile[capacity]), m_maxAge(maxAge), m_gravity(gravity),
      m_collision(collision), m_normalBounce(0.5f), m_tangentRetention(0.5f)
{
    for (int index = 0; index < m_capacity; ++index) {
        m_projectiles[index].m_active = false;
    }
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
            m_activeIndices[m_activeCount++] = index;
            projectile.m_previous = projectile.m_position;
        }
    }
    while (elapsed > 0.0f) {
        float step = 0.05f;
        if (elapsed < step) {
            step = elapsed;
        }
        for (int index = 0; index < m_activeCount; ++index) {
            Projectile& projectile = m_projectiles[m_activeIndices[index]];
            if (!projectile.m_active) {
                continue;
            }
            projectile.m_age += step;
            if (projectile.m_age > m_maxAge) {
                projectile.m_active = false;
                OnActivate(&projectile);
            } else {
                projectile.m_position = projectile.m_position + projectile.m_velocity * step;
                projectile.m_velocity = projectile.m_velocity + m_gravity * step;
            }
        }
        elapsed -= step;
    }
    if (m_collision == NULL) {
        return;
    }
    for (int index = 0; index < m_activeCount; ++index) {
        Projectile& projectile = m_projectiles[m_activeIndices[index]];
        vec3 displacement = projectile.m_position - projectile.m_previous;
        if (!projectile.m_active || displacement.LengthSqr() <= 0.0f) {
            continue;
        }
        vec3 hit;
        vec3 normal;
        CCollision::Triangle triangle;
        CCollision::Line line = {projectile.m_previous, projectile.m_position};
        if (!m_collision->IntersectAny(line, hit, normal, triangle)) {
            continue;
        }
        float fraction = Length(hit - projectile.m_previous) / Length(displacement);
        if (fraction > 1.0f) {
            continue;
        }
        vec3 point = projectile.m_previous + displacement * fraction;
        OnCollision(&projectile, &point, &normal);
        if (projectile.m_active) {
            vec3 normalVelocity = normal * projectile.m_velocity.Dot(normal);
            vec3 tangentVelocity = projectile.m_velocity - normalVelocity;
            projectile.m_velocity = tangentVelocity * m_tangentRetention - normalVelocity * m_normalBounce;
            projectile.m_position = point;
        } else {
            OnActivate(&projectile);
        }
    }
}

int Ballistics::AddProjectile(const vec3& position, const vec3& velocity, float spread, void* userData)
{
    int index = m_next;
    Projectile& projectile = m_projectiles[index];
    ++m_next;
    if (m_next == m_capacity) {
        m_next = 0;
    }
    if (projectile.m_active) {
        projectile.m_active = false;
        OnActivate(&projectile);
    }
    projectile.m_active = true;
    projectile.m_age = 0.0f;
    projectile.m_position = position;
    projectile.m_previous = position;
    projectile.m_velocity = velocity;
    if (spread != 0.0f && velocity.LengthSqr() > 0.0f) {
        vec3 forward = Unit(velocity);
        vec3 right = Unit(vec3(forward.m_y, forward.m_z, forward.m_x).Cross(forward));
        vec3 up = Unit(forward.Cross(right));
        Angles angles(forward, up);
        Mat4x4 matrix(1.0f);
        matrix.SetAngles(angles);
        Quaternion orientation;
        memset(&orientation, 0, sizeof(orientation));
        matrix.ToQuaternion(orientation);

        CRandGen* random = CRandGen::GetInstance();
        float yaw = (10000 - static_cast<int>(random->GetRand(20000))) * 0.0001f * spread;
        float pitch = (10000 - static_cast<int>(random->GetRand(20000))) * 0.0001f * spread;
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
        projectile.m_velocity = Unit(angles.Forward()) * Length(velocity);
    }
    projectile.m_userData = userData;
    OnActivate(&projectile);
    return index;
}

SmokeTrail::SmokeTrail(int capacity, float age, CCollision* collision)
    : Ballistics(capacity, age, vec3(), collision), m_rotationNext(0),
      m_rotations(new Rotation[capacity]), m_basis(CSwerve::GetInstance()->m_swerve),
      m_scratch(CSwerve::GetInstance()->m_swerve)
{
    m_scene = World(App::LoadObject3D("BIN_M3G_EFFECT_SMOKE_SYSTEM"));
    if (m_scene == NULL) {
        return;
    }
    m_root = Group(m_scene.find(DGHelper::getSwerveID("RootNode")));
    if (m_root != NULL) {
        for (int index = 0; index < 16; ++index) {
            XString name = XString::Format(L"Particle%02dTransform", index + 1);
            XString::AnsiString ansiName(name.c_str());
            m_variants[index] = Group(m_root.find(DGHelper::getSwerveID(ansiName)));
        }
    }
}

SmokeTrail::~SmokeTrail()
{
    delete[] m_rotations;
}

int SmokeTrail::AddProjectile(const vec3& position, const vec3& velocity, float spread)
{
    CRandGen* random = CRandGen::GetInstance();
    Rotation& spin = m_rotations[m_rotationNext];
    spin.m_angle = static_cast<float>(random->GetRand(720)) - 360.0f;
    spin.m_speed = static_cast<float>(random->GetRand(180)) - 90.0f;
    WrapSmokeAngle(spin.m_angle);
    int index = Ballistics::AddProjectile(position, velocity, spread, &spin);
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
    if (m_activeCount == 0 || m_root == NULL) {
        return;
    }
    for (int index = 0; index < 16; ++index) {
        if (m_variants[index] == NULL) {
            return;
        }
    }
    CSwerveGame* game = CSwerveGame::m_pInstance;
    vec3 camera = game->GetCurrentCameraPos();
    vec3 viewDirection = game->m_camera->GetViewDirection();
    float values[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    Transform renderTransform(CSwerve::GetInstance()->m_swerve);
    renderTransform.setIdentity();
    int batch = 0;
    for (int index = 0; index < m_activeCount; ++index) {
        Projectile& projectile = m_projectiles[m_activeIndices[index]];
        if (!projectile.m_active) {
            continue;
        }
        Rotation* spin = static_cast<Rotation*>(projectile.m_userData);
        float age = projectile.m_age / m_maxAge;
        float scale = MathLib::Pow(age, 0.5f);
        if (scale < 0.01f) {
            scale = 0.01f;
        }
        vec3 center = projectile.m_previous + (projectile.m_position - projectile.m_previous) * 0.5f;
        vec3 front = Unit(center - camera);
        vec3 up(0.0f, 0.0f, 1.0f);
        vec3 side = Unit(up.Cross(front));
        up = up * (scale * 75.0f);
        side = side * (scale * 75.0f);
        vec3 depth = front * (scale * 75.0f);
        values[0] = up.m_x;
        values[1] = side.m_x;
        values[2] = depth.m_x;
        values[4] = up.m_y;
        values[5] = side.m_y;
        values[6] = depth.m_y;
        values[8] = up.m_z;
        values[9] = side.m_z;
        values[10] = depth.m_z;
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
        m_variants[batch].setAlphaFactor(1.0f - age);
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
    : Ballistics(32, 10.0f, vec3(), collision), m_model(App::LoadObject3D(model)),
      m_smoke(new SmokeTrail(128, 0.5f, NULL)), m_flightNext(0),
      m_flights(new Flight[32]), m_units(units)
{
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
    for (int index = 0; index < m_activeCount; ++index) {
        Projectile& projectile = m_projectiles[m_activeIndices[index]];
        if (!projectile.m_active) {
            continue;
        }
        CCollision::Line line = {projectile.m_previous, projectile.m_position};
        for (int unitIndex = 0; unitIndex < m_units.size(); ++unitIndex) {
            CUnit* unit = m_units[unitIndex];
            if (unit->m_dead) {
                continue;
            }
            vec3 center = unit->m_body.m_position;
            CCollision::AABB bounds = {
                center - vec3(0.4f, 0.4f, 0.0f),
                center + vec3(0.4f, 0.4f, unit->m_body.GetHeight())
            };
            vec3 point;
            if (CCollision::Intersect(line, bounds, point)) {
                vec3 normal(0.0f, 1.0f, 0.0f);
                OnCollision(&projectile, &point, &normal);
                break;
            }
        }
        vec3 movement = projectile.m_position - projectile.m_previous;
        float distance = Length(movement);
        if (!projectile.m_active || distance <= 0.0f) {
            continue;
        }
        Flight* flight = static_cast<Flight*>(projectile.m_userData);
        vec3 direction = Unit(movement);
        float spacing = 0.25f;
        float next = spacing - flight->m_trailDistance;
        while (next <= distance) {
            vec3 position = projectile.m_previous + direction * next;
            m_smoke->AddProjectile(position, direction * -0.25f, 0.0f);
            next += spacing;
        }
        flight->m_trailDistance = spacing + distance - next;
        flight->m_distance += distance;
    }
}

void GrenadeLauncher::Paint3D(Graphics3D& graphics)
{
    if (m_model != NULL && m_activeCount > 0) {
        Transform transform(CSwerve::GetInstance()->m_swerve);
        for (int index = m_activeCount - 1; index >= 0; --index) {
            const Projectile& projectile = m_projectiles[m_activeIndices[index]];
            if (!projectile.m_active) {
                continue;
            }
            vec3 velocity = projectile.m_velocity;
            float yaw = MathLib::ArcTan(velocity.m_y, velocity.m_x);
            float planar = sqrtf(velocity.m_x * velocity.m_x + velocity.m_y * velocity.m_y);
            float pitch = MathLib::ArcTan(velocity.m_z, planar);
            float horizontal[4] = {yaw, 0.0f, 0.0f, 1.0f};
            float vertical[4] = {-pitch - 90.0f, 0.0f, 1.0f, 0.0f};
            transform.setIdentity();
            transform.postTranslate(&projectile.m_position.m_x);
            transform.postRotate(horizontal);
            transform.postRotate(vertical);
            graphics.render(m_model, transform);
        }
    }
    m_smoke->Paint3D(graphics);
}

typedef char AssertProjectileSize[sizeof(Ballistics::Projectile) == 0x30 ? 1 : -1];
typedef char AssertBallisticsSize[sizeof(Ballistics) == 0x34 ? 1 : -1];
typedef char AssertSmokeTrailSize[sizeof(SmokeTrail) == 0x8c ? 1 : -1];
typedef char AssertGrenadeLauncherSize[sizeof(GrenadeLauncher) == 0x48 ? 1 : -1];
