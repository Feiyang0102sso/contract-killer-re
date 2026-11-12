#include "CUnit.h"

#include "App.h"
#include "CAmmo.h"
#include "CBossLogicExecutor.h"
#include "CBH_PersonType.h"
#include "CBH_Player.h"
#include "CCivilianLogicExecutor.h"
#include "CChallengeStats.h"
#include "CDH_SoundManager.h"
#include "CDH_Weapon.h"
#include "CHuntingInfo.h"
#include "CProgressSystem.h"
#include "CSwerveGame.h"
#include "CTrickShotManager.h"
#include "CVisualCharacterType.h"
#include "CWarriorLogicExecutor.h"
#include "DGHelper.h"

static int g_unitGUID;

CUnit::CUnit(const CBH_PersonType* type, CPathCalculator* calculator, const CGameAIMap* map)
    : m_body(type, map), m_mind(calculator), m_hitWeapon(NULL), m_damage(0), m_damageType(HitBody),
      m_fatalDamage(false), m_type(UnitCivilian), m_weaponAnimType(PERSON_ANIM_TYPE_DEFAULT),
      m_pickType(HitBody), m_squad(NULL), m_hp(0), m_maxHp(0), m_healthFraction(0.0f), m_wounded(false),
      m_dead(false), m_tranquillized(false), m_invulnerable(false), m_removed(false), m_runningAway(false),
      m_picked(false), m_inCover(false), m_tagged(false), m_field1f0(0), m_guid(g_unitGUID++)
{
    CalculateHP(type);
    if (type != NULL && type->m_weapon != NULL) {
        m_weaponAnimType = type->m_weapon->GetCharacterAnimationType();
    }
}

CUnit::~CUnit()
{
}

void CUnit::CalculateHP(const CBH_PersonType* type)
{
    if (type != NULL) {
        App* app = static_cast<App*>(WindowApp::m_instance);
        m_maxHp = static_cast<int>(type->m_baseHp * app->InitProgressSystem()->GetLevelForkFormulaCoef(true));
        m_hp = m_maxHp;
        if (m_hp < 0) {
            m_hp = 0;
        }
    }
}

void CUnit::Init(EUnitMindType type, int point, bool scripted, bool squad)
{
    m_type = type;
    m_mind.Init(this);
    m_body.Init(this, point);
    InitBehaviour(scripted, squad);
}

void CUnit::InitBehaviour(bool scripted, bool squad)
{
    switch (m_type) {
    case UnitBoss:
        AddBossLogicExecutor(&m_mind, scripted);
        break;
    case UnitWarrior:
        AddWarriorLogicExecutor(&m_mind, squad);
        break;
    case UnitCivilian:
        AddCivilianLogicExecutor(&m_mind);
        break;
    }
}

void CUnit::Tick(int elapsed)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CSwerveGame* game = app->m_helper->m_swerveGame;
    if (game != NULL && game->m_slowMode && !CBH_Player::GetInstance()->IsSpeedMode()) {
        elapsed /= 3;
    }
    if (m_removed) {
        return;
    }
    if (m_maxHp > 0) {
        m_healthFraction = static_cast<float>(m_hp) / m_maxHp;
        m_wounded = m_healthFraction < 0.01f;
    }
    m_body.Tick(elapsed);
    if (CanBeHandled()) {
        m_mind.Tick(elapsed);
    }
}

bool CUnit::CanBeHandled() const
{
    return !m_dead && !m_tranquillized && !m_removed;
}

bool CUnit::Pick(float x, float y, float distance, CUnitBody::Camera& camera)
{
    EHitType hitType;
    m_picked = m_body.Pick(x, y, distance, camera, hitType);
    m_pickType = hitType;
    return m_picked;
}

bool CUnit::SpreadPick(CUnitBody::Camera& camera, CUnitBody::Group& collision)
{
    EHitType hitType;
    m_picked = m_body.SpreadPick(camera, collision, hitType);
    m_pickType = hitType;
    return m_picked;
}

void CUnit::ResetPickInfo()
{
    m_picked = false;
}

bool CUnit::GetPickInfo(EHitType* type)
{
    *type = m_pickType;
    return m_picked;
}

void CUnit::CalcDamageAndKillingPosibility(EHitType type, const CDH_Weapon* weapon)
{
    int damage = 0;
    if (weapon != NULL && !weapon->m_tranquillizer) {
        damage = weapon->damage();
        switch (type) {
        case HitHead:
            damage = m_hp;
            break;
        case HitVital:
            damage *= 2;
            break;
        case HitArmor:
            damage /= 2;
            break;
        case HitBody:
            break;
        default:
            damage = 0;
            break;
        }
    }
    CalcDamageAndKillingPosibility(damage, weapon);
    m_damageType = type;
}

void CUnit::CalcDamageAndKillingPosibility(int damage, const CDH_Weapon* weapon)
{
    m_damagePosition = vec3();
    m_hitWeapon = weapon;
    m_damage = 0;
    m_damageType = HitBody;
    m_fatalDamage = false;
    if (!m_invulnerable) {
        m_damage = damage;
        m_fatalDamage = m_hp <= damage;
    }
}

void CUnit::CalcDamageAndKillingPosibility(int damage)
{
    CalcDamageAndKillingPosibility(damage, NULL);
}

void CUnit::ReceivedDamage(bool suppressEffects)
{
    if (m_invulnerable || !CanBeHandled()) {
        return;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    CBH_Player* player = CBH_Player::GetInstance();
    if (m_hitWeapon != NULL && m_hitWeapon->m_tranquillizer) {
        m_mind.OnTranquillize(m_hitWeapon->m_tranquillizingHPSpeed);
        if (!suppressEffects) {
            app->m_helper->m_swerveGame->ShowUnitInfo(this, 0);
        }
        return;
    }
    WindowApp::HandleTunnelCommand(-0x2e9b0c33, m_damage, 0, 0);
    m_hp -= m_damage;
    if (m_hp < 0 || m_type == UnitCivilian) {
        m_hp = 0;
    }
    if (m_maxHp > 0) {
        m_healthFraction = static_cast<float>(m_hp) / m_maxHp;
        m_wounded = m_healthFraction < 0.01f;
    }

    bool headshot = m_damageType == HitHead;
    if (headshot) {
        WindowApp::HandleTunnelCommand(-0x1c8ca178, 0, 0, 0);
        if (m_hitWeapon != NULL) {
            player->m_gameStats.RegisterHeadshot(m_hitWeapon);
        }
        if (m_type != UnitCivilian && m_body.IsRunning()) {
            app->m_trickShotManager->RegisterEvent(CTrickShotManager::BullsEye);
        }
        ++app->m_huntingInfo->m_headshots;
    }
    if (m_hitWeapon != NULL) {
        const CAmmo* ammo = m_hitWeapon->GetActiveAmmo();
        if (ammo != NULL) {
            if (ammo->m_ammoType == L"incendiary") {
                m_mind.OnBurnt(7000, m_damage / 2);
            }
            if (ammo->m_ammoType == L"explosive") {
                m_mind.OnHit(true);
            }
        }
    }
    m_mind.OnReceiveDamage();
    if (m_hitWeapon != NULL) {
        player->m_gameStats.RegisterHit(this, m_hitWeapon);
        if (app->m_helper->m_gamePlayManager.GetGameData()->GetWeapon01(true) == m_hitWeapon) {
            ++app->m_huntingInfo->m_primaryHits;
            if (m_hp == 0 && m_fatalDamage) {
                ++app->m_huntingInfo->m_primaryKills;
            }
        } else {
            ++app->m_huntingInfo->m_secondaryHits;
            if (m_hp == 0 && m_fatalDamage) {
                ++app->m_huntingInfo->m_secondaryKills;
            }
        }
    }
    if (m_hp == 0) {
        CSwerveGame* game = app->m_helper->m_swerveGame;
        player->m_challengeStats->RegisterKill(this, m_hitWeapon, headshot && m_hitWeapon != NULL,
                                               !m_inCover, static_cast<int>(game->GetDistanceToUnit(this)));
        app->m_helper->m_swerveGame->RegisterUnitNeutralization(m_type);
        vec3 shotOrigin(m_body.m_pickRay[0], m_body.m_pickRay[1], m_body.m_pickRay[2]);
        m_mind.OnDeath(shotOrigin);
        m_dead = true;
        if (m_type == UnitCivilian) {
            player->m_gameStats.AddCiviliansKilled(1);
        } else {
            if (m_type == UnitBoss) {
                player->m_gameStats.AddTargetsKilled(1);
            } else {
                player->m_gameStats.AddEnemyKilled(1);
            }
            CProgressSystem* progress = app->InitProgressSystem();
            progress->RegisterGameEvent(CProgressSystem::EVENT_ENEMY_KILLED,
                                        CProgressSystem::CalcXpForKilledUnit(this), headshot);
            player->m_gameStats.RegisterEnemyKill();
        }
    }
    if (!suppressEffects) {
        app->m_helper->m_swerveGame->ShowUnitInfo(this, m_damage);
        if (m_hitWeapon != NULL && m_picked && m_damage > 0) {
            m_body.DrawHitEffect();
        }
    }
    if (!suppressEffects) {
        if (headshot) {
            app->m_soundManager->PlaySound("IDM_HEADSHOT", false, false);
        } else {
            PlayWoundingSound();
        }
    }
}

void CUnit::PlayWoundingSound()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    const char* sound = "KEYSET_SOUND_MEN_HIT_LOW_HEALTH";
    if (m_body.m_type != NULL && m_body.m_type->m_visualCharacter != NULL &&
        !m_body.m_type->m_visualCharacter->m_isMan) {
        if (m_healthFraction > 0.5f) {
            sound = "KEYSET_SOUND_WOMEN_HIT_FULL_HEALTH";
        } else {
            sound = "KEYSET_SOUND_WOMEN_HIT_LOW_HEALTH";
        }
    } else if (m_healthFraction > 0.5f) {
        sound = "KEYSET_SOUND_MEN_HIT_FULL_HEALTH";
    }
    app->m_soundManager->PlayRandomSound(sound, false);
}

void CUnit::GetMovementAnimParam(float* speed, EPersonAnimType* animation, bool run)
{
    int type;
    if (m_hp < m_maxHp) {
        *speed = CVisualCharacterType::m_runWoundSpeed;
        type = 7;
    } else if (m_body.m_type->m_visualCharacter->m_isMan) {
        if (run) {
            *speed = CVisualCharacterType::m_runSpeedMan;
            type = 4;
        } else {
            *speed = CVisualCharacterType::m_walkSpeedMan;
            type = 2;
        }
    } else if (run) {
        *speed = CVisualCharacterType::m_runSpeedWoman;
        type = 5;
    } else {
        *speed = CVisualCharacterType::m_walkSpeedWoman;
        type = 3;
    }
    *animation = static_cast<EPersonAnimType>(type);
}

void CUnit::MarkAsRemoved()
{
    m_removed = true;
    if (m_type != UnitCivilian) {
        App* app = static_cast<App*>(WindowApp::m_instance);
        app->InitProgressSystem()->RegisterGameEvent(CProgressSystem::EVENT_ENEMY_ESCAPED, 0, -1);
    }
}

void CUnit::MarkAsRunAway()
{
    m_runningAway = true;
}

void CUnit::SetSquad(CSquad* squad)
{
    if (squad != NULL) {
        m_squad = squad;
    }
}

void CUnit::OnTakeCover()
{
    m_inCover = true;
}

typedef char AssertUnitSize[sizeof(CUnit) == 0x1f8 ? 1 : -1];
