// CEnemyWeaponType.h - one enemy weapon of enemy_weapons.xml (model, damage, animations, sound).
#ifndef CENEMYWEAPONTYPE_H
#define CENEMYWEAPONTYPE_H

#include "xString.h"
#include "Vector_ss.h"
#include "CSwerve.h"
#include "tws.h"

class TiXmlNode;

/** Person animation set used with a weapon (values from characterAnimationTypes). */
enum EPersonAnimType {
    PERSON_ANIM_TYPE_DEFAULT = 0,
    PERSON_ANIM_STUN = 16   // added by game3 (CStunExecutor::OnStart)
};

class CEnemyWeaponType {
public:
    CEnemyWeaponType(TiXmlNode* node);
    ~CEnemyWeaponType();

    /** Read <enemyWeapon>: name and the info / model / sounds children. */
    void Parse(TiXmlNode* node);
    /** A duplicate of the model (loaded on first use); give it back with ReleaseModelCopy. */
    Group GetModelCopy() const;
    /** Load the m3g file and pick the dummy node as the model. */
    void LoadModel() const;
    /** One copy less; the model is dropped with the last one. */
    void ReleaseModelCopy() const;
    void ParseInfo(TiXmlNode* node);
    void ParseModel(TiXmlNode* node);
    void ParseSounds(TiXmlNode* node);
    /** User id given to the hit zone meshes of the weapon models. */
    static int GetWeaponHitZoneUserId();
    /** Animation type configured for this weapon. */
    EPersonAnimType GetCharacterAnimationType() const { return m_animType; }

    mutable Object3D m_model;    // 0x00 loaded on demand
    XString m_name;                         // 0x04
    XString m_dummyName;                    // 0x08 node of the m3g file used as the model
    int m_baseDamage;                       // 0x0c
    int m_effectiveRange;                   // 0x10
    EPersonAnimType m_animType;             // 0x14
    const char* m_modelResource;            // 0x18 m3g resource id
    const char* m_shootSound;               // 0x1c
    mutable int m_modelCopies;              // 0x20
};

#endif
