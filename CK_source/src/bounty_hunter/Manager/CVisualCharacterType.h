// CVisualCharacterType.h - models and animation tables of CK 1.5.2 characters.
#ifndef CVISUALCHARACTERTYPE_H
#define CVISUALCHARACTERTYPE_H

#include "CEnemyWeaponType.h"
#include "HashTable.h"
#include "SwerveHelper.h"

class AnimationImpl;
class TiXmlNode;

enum EPersonAnimShotAttitudeType { PERSON_SHOT_STAND, PERSON_SHOT_CROUCH, PERSON_SHOT_WALL_RIGHT, PERSON_SHOT_WALL_LEFT };
enum EPersonAnimShotDirectionType { PERSON_SHOT_UP, PERSON_SHOT_FRONT };
enum EPersonAnimWeaponType { PERSON_SHOT_PISTOL, PERSON_SHOT_RIFLE, PERSON_SHOT_SHOTGUN, PERSON_SHOT_MACHINEGUN };

class CVisualCharacterType {
public:
    typedef SwerveHelper::Object3D Object3D;
    typedef SwerveHelper::Group Group;
    typedef ::Image2D Image2D;

    explicit CVisualCharacterType(TiXmlNode* node);
    ~CVisualCharacterType();

    void Parse(TiXmlNode* node);
    void ParseInfo(TiXmlNode* node);
    void ParseModel(TiXmlNode* node);
    void CleanupResources() const;
    static void LoadCommonInfo(TiXmlNode* node);
    static void LoadCommonAnimation(TiXmlNode* node);
    static void DestroyCommonAnimation();

    Group GetCharacterModelCopy() const;
    void ReleaseCharacterModelCopy() const;
    void LoadModel() const;
    void ChangeImageRecursively(Object3D object, Image2D image) const;
    void ReplaceTexture(Object3D body, Object3D head) const;

    AnimationImpl* GetAnimation(EPersonAnimType type) const;
    AnimationImpl* GetIdleAnimation(int index) const;
    AnimationImpl* GetBossAfraidAnimation() const;
    int GetEnumerateAnimationCount(const Vector<AnimationImpl*>* animations) const;
    AnimationImpl* GetShotAnimation(EPersonAnimShotAttitudeType attitude,
                                   EPersonAnimShotDirectionType direction, EPersonAnimWeaponType weapon) const;
    AnimationImpl* GetHideAnimation(EPersonAnimShotAttitudeType attitude, EPersonAnimWeaponType weapon) const;
    AnimationImpl* GetUnhideAnimation(EPersonAnimShotAttitudeType attitude, EPersonAnimWeaponType weapon) const;
    AnimationImpl* GetReloadAnimation(EPersonAnimShotAttitudeType attitude, EPersonAnimWeaponType weapon) const;
    AnimationImpl* GetHideAndReloadAnimationImpl(const Vector<AnimationImpl*>* animations,
                                                EPersonAnimShotAttitudeType attitude, EPersonAnimWeaponType weapon) const;
    AnimationImpl* GetTakeOutWeaponAnimation(EPersonAnimWeaponType weapon) const;
    EPersonAnimType GetAnimationTypeByName(const char* name) const;
    void ParseAnimation() const;
    void ParseEnumerateAnimation(Vector<AnimationImpl*>* animations, const char* prefix, EPersonAnimType type) const;
    void ParseShotAnimation() const;
    void ParseHideUnhideAndReloadAnimation() const;
    void ParseGetWeaponAnimation() const;

    static const char* m_pBodyDummyName;
    static float m_walkSpeedMan;
    static float m_walkSpeedWoman;
    static float m_runSpeedMan;
    static float m_runSpeedWoman;
    static float m_runWoundSpeed;
    static Vector<AnimationImpl*> m_commonAnimation;
    static const char* m_commonMapResourceID;
    static const char* m_commonAnmResourceID;

    mutable Object3D m_model;                        // 0x000
    mutable int m_modelCopies;
    XString m_name;
    XString m_bodyResource;
    XString m_headResource;
    XString m_bodyTexture;
    XString m_headTexture;
    const char* m_screenImage;
    mutable AnimationImpl* m_animations[23];        // 0x020
    mutable Vector<AnimationImpl*> m_idle;           // 0x07c
    mutable Vector<AnimationImpl*> m_afraid;
    mutable Vector<AnimationImpl*> m_walk;
    mutable Vector<AnimationImpl*> m_wound;
    mutable Vector<AnimationImpl*> m_shot;
    mutable Vector<AnimationImpl*> m_hide;
    mutable Vector<AnimationImpl*> m_unhide;
    mutable Vector<AnimationImpl*> m_reload;
    mutable Vector<AnimationImpl*> m_getWeapon;
    mutable Vector<AnimationImpl*> m_deathFront;
    mutable Vector<AnimationImpl*> m_deathBack;
    mutable Vector<AnimationImpl*> m_stun;
    mutable HashTable<XString, EPersonAnimType> m_animationTypes; // 0x13c
    bool m_isWoman;                                   // 0x148
    bool m_isMan;
};

#endif
