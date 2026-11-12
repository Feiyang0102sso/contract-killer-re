// PROVISIONAL (created by game3): owner src/FPSGameplay/CUnitBody.cpp not written yet
// CUnitBody.h - model, animations and picking of the unit body at CUnit+0x04.
#ifndef CUNITBODY_H
#define CUNITBODY_H

#include <stddef.h>
#include "CVisualCharacterType.h"
#include "AnimationInstance.h"
#include "FrustumCulling.h"
#include "MathLib.h"
#include "SwerveHelper.h"

class AnimationImpl;
class AnimationTransition;
class AnimatableSwerveObject;
class CBH_PersonType;
class CGameAIMap;
class CUnit;

enum EHitType { HitBody, HitHead, HitVital, HitArmor };

class CUnitBody : public AnimationListener {
public:
    typedef SwerveHelper::Group Group;
    typedef SwerveHelper::Node Node;
    typedef ::Camera Camera;
    typedef ::RayIntersection RayIntersection;

    CUnitBody(const CBH_PersonType* type, const CGameAIMap* map);
    virtual ~CUnitBody();
    virtual void eventListener(const AnimationEvent& event, AnimationInstance* instance);

    void Init(CUnit* unit, int point);
    void Cleanup();
    void LoadSkin();
    void BindToWorld();
    void BindShadow();
    void UnbindFromWorld();
    void InitBodyZones();
    void InitHealthBar();
    void UpdateHealthBar();
    void OnTagged(bool known);
    void SetAsKnown();
    void BindKnownIcon();
    void BindUnknownIcon();
    void UnbindAllIcons();
    void ShowDistanceTag(bool show);
    void InitDistanceTag();
    void UpdateDistanceTag();
    void SetDistanceTag(int distance);
    void CombineCharImages(unsigned char** pixels, int* width, int* height, const XString& text);
    static ICRenderSurface* CreateDistanceTagSurface();
    void InitWeapon();
    void BindWeaponToHand();
    void UnbindWeapon();
    void ShowWeapon(bool show);
    void ShowBulletMuzzleFlash();
    void UnsetDistanceTag();
    void SetIconTransparencyParameters();
    void PlayWeaponSound();
    void Tick(int elapsed);
    void UpdateAnimation(int elapsed);
    void UpdateAngle(int elapsed);
    void UpdatePosition(int elapsed);
    void CalculateStep(int elapsed);
    void AvoidCollision(CUnitBody* other);
    void AvoidObstacles();
    void AddStaticObstacle(const vec3& start, const vec3& end);
    void AdditionalRotate(float degrees);
    void CheckVisibility(const FrustumCulling& frustum);
    void SetToPoint(int point);
    void SetExactlyToPoint();
    void SetMovementParam(const vec3& target, float speed);
    void SetOrientationToPoint(const vec3& target, bool reverse);
    void DefineCameraPitch(const vec3& direction);
    float GetYawAngle(const vec3& direction) const;
    float GetRadius() const;
    float GetHeight() const;
    void OnPathComplete();
    void OnNeutralization();
    vec3 GetAutoAimPointPosition();
    void DrawHitEffect();
    bool Pick(float x, float y, float maximumDistance, Camera& camera, EHitType& type);
    bool SpreadPick(Camera& camera, Group& collision, EHitType& type);
    bool PickWithAngleAndCollisionGeometry(Camera& camera, Group& collision, float angle, float axisX,
                                           float axisY, float axisZ, RayIntersection intersection, EHitType& type);
    void DefineBodyZones(EHitType* type, float x, float y, const Camera& camera);

    /** Stop the animation `animName` if it is playing. */
    void StopAnimation(const char* animName);
    /** Start an animation of `animType`; the name of the started animation. */
    const char* StartAnimation(EPersonAnimType animType, bool loop);
    /** Start a random afraid animation; its name. */
    const char* PlayAfraidAnimation();
    /** Start idle animation `index`; its name. */
    const char* PlayIdleAnimation(int index);
    const char* PlayShotAnimation();
    const char* PlayHideAnimation();
    const char* PlayUnhideAnimation();
    const char* PlayReloadAnimation(bool* available);
    const char* PlayGetWeaponAnimation();
    const char* PlayDeathAnimation(const vec3& from);
    const char* PlayTranquillizeAnimation();
    bool IsRunning() const;
    void SetAttitude(EPersonAnimShotAttitudeType attitude);

    // 0x24 bytes: Vector<SStaticObstacle>::construct_items value-initializes (memset) and then runs
    // the two-element vec3 array constructor and a third vec3 constructor (weapon, from the original).
    struct SStaticObstacle { vec3 m_points[2]; vec3 m_normal; };
    struct SDynamicObstacle { vec3 m_position; float m_radius; };

    Vector<SStaticObstacle> m_staticObstacles;    // 0x04
    Vector<SDynamicObstacle> m_dynamicObstacles; // 0x14
    FrustumCulling::CullSphere* m_cullSphere;     // 0x24
    CUnit* m_unit;                               // 0x28
    Group m_model;
    Group m_barMount;
    Group m_healthBar;
    Group m_headZone;
    Group m_headArmorZone;
    Group m_vitalZone;
    Group m_bodyArmorZone;
    Group m_weapon;
    Group m_knownIcon;
    Group m_groundShadow;
    Node m_distanceTag;
    RayIntersection m_zoneIntersection;         // 0x58
    RayIntersection m_spreadIntersection;
    AnimatableSwerveObject* m_skin;
    AnimationTransition* m_transition;
    const CBH_PersonType* m_type;                // 0x68
    const CGameAIMap* m_map;
    unsigned char m_reserved70[0x5c];
    SwerveAnimations m_barAnimations;           // 0xcc
    SwerveAnimations::Animation* m_pulsar;      // 0xe0
    SwerveAnimations::Animation* m_health;      // 0xe4
    SwerveAnimations m_weaponAnimations;       // 0xe8 animations of the enemy weapon model
    SwerveAnimations::Animation* m_shotFlash;   // 0xfc "shotflash"
    EPersonAnimType m_animationType;            // 0x100
    int m_cellId;
    int m_pointId;
    bool m_moved;
    vec3 m_position;                             // 0x110
    vec3 m_target;
    vec3 m_direction;
    float m_speed;
    float m_yaw;
    EPersonAnimShotDirectionType m_shotDirection; // 0x13c
    EPersonAnimShotAttitudeType m_attitude;      // 0x140
    bool m_hasTag;                              // 0x144
    bool m_hasKnownTag;                         // 0x145
    bool m_visible;                             // 0x146
    bool m_showDistanceTag;                     // 0x147
    int m_bloodTimeMs;                          // 0x148
    SwerveHelper::Transform m_hitEffectTransform;
    Node m_blood;
    bool m_showBlood;
    float m_pickRay[6];                         // 0x158
    float m_hitDistance;
    float m_pickDistance;
    int m_field178;                             // 0x178 frame counter of the low-end animation skip
    int m_distanceTagValue;                     // 0x17c distance shown by the tag
    unsigned int m_distanceTagTime;             // 0x180 GetUpTimeMS of the last tag update

private:
    void BindMark(const char* resource);
    const char* PlayAnimation(AnimationImpl* animation, EPersonAnimType type, bool loop, int fadeTime);
};

#endif
