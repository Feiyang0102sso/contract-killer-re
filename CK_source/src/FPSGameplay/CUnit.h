// PROVISIONAL (created by game3): owner src/FPSGameplay/CUnit.cpp not written yet
// CUnit.h - one person of the level: body (base at +0x04) and mind (+0x188).
#ifndef CUNIT_H
#define CUNIT_H

#include "CUnitBody.h"
#include "CUnitMind.h"

class CDH_Weapon;
class CSquad;

enum EUnitMindType { UnitCivilian, UnitWarrior, UnitBoss };

class CUnit {
public:
    CUnit(const CBH_PersonType* type, CPathCalculator* calculator, const CGameAIMap* map);
    virtual ~CUnit();

    void CalculateHP(const CBH_PersonType* type);
    void Init(EUnitMindType type, int point, bool scripted, bool squad);
    void InitBehaviour(bool scripted, bool squad);
    void Tick(int elapsed);
    bool CanBeHandled() const;
    bool Pick(float x, float y, float distance, CUnitBody::Camera& camera);
    bool SpreadPick(CUnitBody::Camera& camera, CUnitBody::Group& collision);
    void ResetPickInfo();
    bool GetPickInfo(EHitType* type);
    void CalcDamageAndKillingPosibility(EHitType type, const CDH_Weapon* weapon);
    void CalcDamageAndKillingPosibility(int damage, const CDH_Weapon* weapon);
    void CalcDamageAndKillingPosibility(int damage);
    void ReceivedDamage(bool showEffect);
    void PlayWoundingSound();
    void GetMovementAnimParam(float* speed, EPersonAnimType* animation, bool run);
    void MarkAsRemoved();
    void MarkAsRunAway();
    void SetSquad(CSquad* squad);
    void OnTakeCover();

    CUnitBody* GetBody()
    {
        return &m_body;
    }

    CUnitBody m_body;                   // 0x04

    CUnitMind m_mind;                   // 0x188
    XString m_name;                     // 0x1a8
    vec3 m_damagePosition;
    const CDH_Weapon* m_hitWeapon;
    int m_damage;
    EHitType m_damageType;
    bool m_fatalDamage;
    EUnitMindType m_type;               // 0x1c8
    EPersonAnimType m_weaponAnimType;
    EHitType m_pickType;
    CSquad* m_squad;
    int m_hp;                           // 0x1d8
    int m_maxHp;                        // 0x1dc
    float m_healthFraction;
    bool m_wounded;
    bool m_dead;
    bool m_tranquillized;
    bool m_invulnerable;
    bool m_removed;
    bool m_runningAway;
    bool m_picked;
    bool m_inCover;
    bool m_tagged;
    int m_field1f0;
    int m_guid;
};

#endif
