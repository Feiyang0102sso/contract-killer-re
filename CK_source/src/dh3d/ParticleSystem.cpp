// ParticleSystem.cpp - update live particles and release expired emitters.
#include "ParticleSystem.h"

#include "CSwerve.h"

ParticleSystem::ParticleSystem()
{
    IObject3D* resource = CSwerve::GetInstance()->LoadM3GResource("BIN_PARTICLES");
    Object3D model(reinterpret_cast<IQueryInterface*>(resource));
    m_resourceGroup = Group(model);
}

ParticleSystem::~ParticleSystem()
{
    Free();
}

void ParticleSystem::Free()
{
    for (int index = 0; index < m_areas.size(); ++index) {
        ParticlesArea* area = m_areas[index];
        for (int effectIndex = 0; effectIndex < area->m_effects.size(); ++effectIndex) {
            ParticleBase*& effect = area->m_effects[effectIndex];
            effect->Kill();
            delete effect;
            effect = NULL;
        }
        delete area;
        m_areas[index] = NULL;
    }
    m_areas.removeAllElements();
}

void ParticleSystem::Update(float elapsed)
{
    if (elapsed > 100.0f) {
        elapsed = 100.0f;
    }
    for (int index = 0; index < m_areas.size(); ++index) {
        ParticlesArea* area = m_areas[index];
        area->m_elapsed += elapsed;
        if (area->m_lifetime >= 0.0f && area->m_elapsed >= area->m_lifetime) {
            for (int effectIndex = 0; effectIndex < area->m_effects.size(); ++effectIndex) {
                ParticleBase*& effect = area->m_effects[effectIndex];
                delete effect;
                effect = NULL;
            }
            delete area;
            m_areas.removeElementAt(index);
            --index;
        } else {
            for (int effectIndex = 0; effectIndex < area->m_effects.size(); ++effectIndex) {
                ParticleBase* effect = area->m_effects[effectIndex];
                if (effect->m_dead) {
                    effect->Init(area->m_position, area->m_direction, area->m_group, false);
                } else {
                    effect->Update(elapsed);
                }
            }
        }
    }
}
