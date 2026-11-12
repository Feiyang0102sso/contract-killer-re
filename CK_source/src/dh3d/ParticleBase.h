// ParticleBase.h - base of the particle effects driven by ParticleSystem.
#ifndef PARTICLEBASE_H
#define PARTICLEBASE_H

#include "tws.h"
#include "MathLib.h"

/** One particle effect; the concrete effect implements the virtual hooks. */
class ParticleBase {
public:
    friend class ParticleSystem;
    virtual ~ParticleBase() {}

    /** Start the effect at `position` moving along `direction` inside `group`. */
    void Init(const vec3& position, const vec3& direction, Group group, bool attached);
    /** Mark the effect dead and let the concrete effect release its nodes. */
    void Kill();
    /** Advance the effect unless it is dead. */
    void Update(float deltaTime);

protected:
    // vtable slot 2: effect specific start
    virtual void OnInit(const vec3& position, const vec3& direction, Group group, bool attached) = 0;
    // vtable slot 3: effect specific shutdown
    virtual void OnKill() = 0;
    // vtable slot 4: not called by this unit
    virtual bool IsFinished() = 0;
    // vtable slot 5: effect specific update
    virtual void OnUpdate(float deltaTime) = 0;

    bool m_dead;    // +0x04
};

#endif
