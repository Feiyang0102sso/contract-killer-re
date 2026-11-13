// DGWorld.h - the 3D world of a level: visibility of its nodes per camera angle and the hit effects.
#ifndef DGWORLD_H
#define DGWORLD_H

#include "tws.h"
#include "Vector_ss.h"
#include "FrustumCulling.h"

class DGWorld {
public:
    /** Hit effect kinds (index of m_effectNodes / m_effectDurations). */
    enum {
        EFFECT_GROUND = 0,
        EFFECT_DUST = 1,
        EFFECT_WATER = 2,
        EFFECT_BLAST = 3,
        EFFECT_COUNT = 4
    };

    /** A running hit effect. */
    struct EffectInstance {
        // Original placement copies zero the aggregate before member copy construction.
        int m_type;                                                 // 0x00
        Transform m_transform;  // 0x04
        int m_time;                                                 // 0x08 milliseconds since the start
    };

    DGWorld(World world, const char* name);
    ~DGWorld();

    /** Load the effect scenes and their durations. */
    void loadHitEffects();
    /** Show the nodes visible from the camera angle. */
    void update();
    /** Pick the scene at a screen coordinate and start the CK dust effect. */
    void shot(int x, int y);
    /** Animate and render the running effects (finished ones are removed). */
    void paintHitEffects(Graphics3D& graphics);

    World m_world;                    // 0x00
    bool m_field04;                                                         // 0x04
    Vector<FrustumCulling::CullSphere> m_cullSpheres;                       // 0x08
    unsigned char* m_visibility;                                            // 0x18 per angle row and node
    Node* m_nodes;                    // 0x1c new[]
    int m_nodeCount;                                                        // 0x20
    RayIntersection m_rayIntersection;  // 0x24
    float m_pickRay[6];                                                     // 0x28
    Node m_effectNodes[EFFECT_COUNT];   // 0x40
    int m_effectDurations[EFFECT_COUNT];                                    // 0x50 milliseconds
    Vector<EffectInstance> m_effects;                                       // 0x60
};

#endif
