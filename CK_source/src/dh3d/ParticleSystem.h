// ParticleSystem.h - collection of particle emitters active in a mission.
#ifndef PARTICLESYSTEM_H
#define PARTICLESYSTEM_H

#include "ParticleBase.h"
#include "Vector_ss.h"

class ParticleSystem {
public:
    typedef ::Group Group;

    struct ParticlesArea {
        vec3 m_position;                 // 0x00
        vec3 m_direction;                // 0x0c
        Vector<ParticleBase*> m_effects; // 0x18
        float m_lifetime;                // 0x28, negative for an indefinite lifetime
        float m_elapsed;                 // 0x2c
        Group m_group;                   // 0x30
    };

    ParticleSystem();
    ~ParticleSystem();
    void Free();
    void Update(float elapsed);

    Group m_resourceGroup;              // 0x00
    Vector<ParticlesArea*> m_areas;       // 0x04
};

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char ParticleSystemSizeCheck[sizeof(ParticleSystem) == 0x14 ? 1 : -1];
#endif
#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char ParticlesAreaSizeCheck[sizeof(ParticleSystem::ParticlesArea) == 0x34 ? 1 : -1];
#endif

#endif
