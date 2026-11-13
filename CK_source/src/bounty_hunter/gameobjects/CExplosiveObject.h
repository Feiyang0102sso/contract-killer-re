#ifndef CEXPLOSIVEOBJECT_H
#define CEXPLOSIVEOBJECT_H

#include "SwerveHelper.h"
#include "MathLib.h"
#include "xString.h"

class CDH_Animation;
class TiXmlNode;

class IGameObject {
public:
    typedef ::Camera Camera;
    typedef SwerveHelper::Group Group;

    virtual ~IGameObject() {}
    virtual void ReceivedDamage(int damage, int delayMs) = 0;
    virtual void Tick(int elapsed) = 0;
    virtual void SetToPos(Group& point) = 0;
    virtual void Bind() = 0;
    virtual bool Pick(float x, float y, float distance, const Camera& camera) = 0;
    virtual bool CanBeHandled() = 0;
    virtual const XString& GetName() = 0;
    virtual const vec3& GetWorldPosition() = 0;
};

class CExplosiveObject : public IGameObject {
public:
    explicit CExplosiveObject(const TiXmlNode* config);
    virtual ~CExplosiveObject();
    virtual void ReceivedDamage(int damage, int delayMs);
    virtual void Tick(int elapsed);
    virtual void SetToPos(Group& point);
    virtual void Bind();
    virtual bool Pick(float x, float y, float distance, const Camera& camera);
    virtual bool CanBeHandled();
    virtual const XString& GetName();
    virtual const vec3& GetWorldPosition() { return m_position; }

    void LoadModel(const char* const& resource, const XString& root);
    void Unbind();
    void Explode();

    int m_maxHp;                   // 0x04
    int m_hp;                      // 0x08
    int m_damage;                  // 0x0c
    int m_damageRadius;            // 0x10
    vec3 m_position;               // 0x14
    unsigned int m_reserved20;     // 0x20 initialized to zero; no observed use.
    Group m_pickable;              // 0x24
    Group m_model;                 // 0x28
    XString m_name;                // 0x2c
    XString m_pickableName;        // 0x30
    XString m_resource;            // 0x34
    int m_explosionDelay;          // 0x38
    bool m_removeAfterExplosion;  // 0x3c
    bool m_exploded;               // 0x3d
    bool m_pendingExplosion;      // 0x3e
    bool m_animationComplete;     // 0x3f
    CDH_Animation* m_animation;    // 0x40
    RayIntersection m_intersection; // 0x44 reserved handle; Pick creates a local one.
};

#endif
