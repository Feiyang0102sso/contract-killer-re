// CK ARMv7 projectile and smoke pools, original Ballistics.cpp (u418).
#ifndef BALLISTICS_H
#define BALLISTICS_H

#include "CCollision.h"
#include "Vector_ss.h"

class CDH_Weapon;
class CUnit;

class Ballistics {
public:
    struct Projectile {
        bool m_active;         // 0x00; other scalar fields are initialized when activated
        float m_age;           // 0x04
        vec3 m_position;       // 0x08
        vec3 m_previous;       // 0x14
        vec3 m_velocity;       // 0x20
        void* m_userData;      // 0x2c
    };

    Ballistics(int capacity, float maxAge, vec3 gravity, CCollision* collision);
    virtual ~Ballistics();
    virtual void OnActivate(Projectile* projectile) = 0;
    virtual void OnCollision(Projectile* projectile, vec3* point, vec3* normal) = 0;
    void Simulate(float elapsed);
    int AddProjectile(const vec3& position, const vec3& velocity, float spread, void* userData);

    int m_capacity;              // 0x04
    int m_next;                  // 0x08
    int m_activeCount;           // 0x0c snapshot from Simulate
    int* m_activeIndices;        // 0x10
    Projectile* m_projectiles;   // 0x14
    float m_maxAge;              // 0x18
    vec3 m_gravity;              // 0x1c
    CCollision* m_collision;     // 0x28 borrowed
    float m_normalBounce;        // 0x2c
    float m_tangentRetention;    // 0x30
};

class SmokeTrail : public Ballistics {
public:
    struct Rotation { float m_angle; float m_speed; };

    SmokeTrail(int capacity, float age, CCollision* collision);
    virtual ~SmokeTrail();
    virtual void OnActivate(Projectile*) {} // CK 0xd710c.
    virtual void OnCollision(Projectile*, vec3*, vec3*) {} // CK 0xd7110.
    int AddProjectile(const vec3& position, const vec3& velocity, float spread);
    void Simulate(float elapsed);
    void Paint3D(Graphics3D& graphics);

    World m_scene; // 0x34
    Group m_root;  // 0x38
    Group m_variants[16]; // 0x3c
    int m_rotationNext;          // 0x7c
    Rotation* m_rotations;       // 0x80
    Transform m_basis;   // 0x84
    Transform m_scratch; // 0x88
};

class GrenadeLauncher : public Ballistics {
public:
    struct Flight {
        const CDH_Weapon* m_weapon;
        float m_distance;
        float m_trailDistance;
    };

    GrenadeLauncher(CCollision* collision, const Vector<CUnit*>& units, const char* model);
    virtual ~GrenadeLauncher();
    virtual void OnActivate(Projectile*) {} // CK 0xd7114.
    virtual void OnCollision(Projectile* projectile, vec3* point, vec3* normal);
    void AddProjectile(const vec3& position, const vec3& velocity, float spread, const CDH_Weapon& weapon);
    void Simulate(float elapsed);
    void Paint3D(Graphics3D& graphics);

    World m_model; // 0x34
    SmokeTrail* m_smoke;          // 0x38
    int m_flightNext;             // 0x3c
    Flight* m_flights;            // 0x40
    const Vector<CUnit*>& m_units; // 0x44 borrowed
};

#endif
