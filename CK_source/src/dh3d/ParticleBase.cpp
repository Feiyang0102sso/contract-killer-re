// ParticleBase.cpp - common start / kill / update of a particle effect.
#include "ParticleBase.h"

void ParticleBase::Init(const vec3& position, const vec3& direction, Group group, bool attached)
{
    m_dead = false;
    OnInit(position, direction, group, attached);
}

void ParticleBase::Kill()
{
    m_dead = true;
    OnKill();
}

void ParticleBase::Update(float deltaTime)
{
    if (m_dead) {
        return;
    }
    OnUpdate(deltaTime);
}
