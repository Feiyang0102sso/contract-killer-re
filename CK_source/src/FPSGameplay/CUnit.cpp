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
#include "GamePlayHUD.h"

static int g_unitGUID;

CUnit::CUnit(const CBH_PersonType* type, CPathCalculator* calculator, const CGameAIMap* map)
    : m_body(type, map), m_mind(calculator)
{
    m_type = UnitCivilian;
    m_weaponAnimType = PERSON_ANIM_TYPE_DEFAULT;
    m_pickType = HitBody;
    m_squad = NULL;
    m_field1f0 = 0;
    m_tagged = false;
    m_removed = false;
    m_runningAway = false;
    m_picked = false;
    m_inCover = false;
    m_wounded = false;
    m_dead = false;
    m_tranquillized = false;
    m_invulnerable = false;
    m_healthFraction = 0.0f;
    m_guid = g_unitGUID++;

    CalculateHP(type);
    if (type->m_weapon != NULL) {
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
    // Original 0xd9834: dead units stop here; tranquillized or removed units tick neither body nor mind.
    if (static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->m_slowMode
        && !CBH_Player::GetInstance()->IsSpeedMode()) {
        elapsed /= 3;
    }
    if (m_dead) {
        return;
    }
    if (!m_tranquillized && !m_removed) {
        if (m_maxHp > 0) {
            float fraction = static_cast<float>(m_hp) / static_cast<float>(m_maxHp);
            m_healthFraction = fraction;
            m_wounded = fraction < 0.01f;
        }
        m_body.Tick(elapsed);
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
    m_damagePosition.m_x = 0;
    m_damagePosition.m_y = 0;
    m_damagePosition.m_z = 0;
    m_hitWeapon = NULL;
    m_damage = 0;
    m_damageType = HitBody;
    m_fatalDamage = false;
    if (weapon == NULL) {
        return;
    }
    m_hitWeapon = weapon;
    if (m_invulnerable || weapon->m_tranquillizer) {
        return;
    }
    int damage = weapon->damage();
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
    m_damage = damage;
    m_fatalDamage = m_hp <= damage;
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

// Original 0xd9a60. Notes: no ShowUnitInfo for tranquillizer hits; civilians die from any damage;
// a boss headshot outside the scope enables slow motion only when the camera time is negative (as in
// the original); m_tagged (+0x1ec) marks a unit that has been hit before.
void CUnit::ReceivedDamage(bool suppressEffects)
{
    if (m_invulnerable) {
        return;
    }
    if (m_hitWeapon != NULL && m_hitWeapon->m_tranquillizer) {
        m_mind.OnTranquillize(m_hitWeapon->m_tranquillizingHPSpeed);
        return;
    }
    WindowApp::HandleTunnelCommand(-0x2e9b0c33, m_damage, 0, 0);
    int hp = m_hp - m_damage;
    if (hp < 0) {
        hp = 0;
    }
    m_hp = hp;
    if (m_type == UnitCivilian) {
        m_hp = 0;
        hp = 0;
    }
    if (m_maxHp > 0) {
        float fraction = static_cast<float>(hp) / static_cast<float>(m_maxHp);
        m_healthFraction = fraction;
        m_wounded = fraction < 0.01f;
    }
    bool headshot = false;
    if (m_damageType == HitHead) {
        WindowApp::HandleTunnelCommand(-0x1c8ca178, 0, 0, 0);
        static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_headshots++;
        if (m_hitWeapon != NULL) {
            CBH_Player* player = CBH_Player::GetInstance();
            headshot = true;
            player->m_gameStats.RegisterHeadshot(m_hitWeapon);
        }
        if (m_type == UnitBoss) {
            App* app = static_cast<App*>(WindowApp::m_instance);
            CSwerveGame* game = app->m_helper->m_swerveGame;
            if (!game->m_hud->m_scoped && app->m_huntingInfo->m_assignedData.m_cameraDurationMs < 0) {
                game->SetSlowMode(true);
            }
        }
        if (m_body.IsRunning() && m_type != UnitCivilian) {
            static_cast<App*>(WindowApp::m_instance)->m_trickShotManager->RegisterEvent(CTrickShotManager::BullsEye);
        }
    }
    if (m_hitWeapon != NULL) {
        if (XString(m_hitWeapon->GetActiveAmmo()->m_ammoType) == XString("incendiary")) {
            m_mind.OnBurnt(7000, m_damage / 2);
        }
        if (m_hitWeapon != NULL && XString(m_hitWeapon->GetActiveAmmo()->m_ammoType) == XString("explosive")) {
            m_mind.OnHit(true);
        }
    }
    m_mind.OnReceiveDamage();
    if (m_hitWeapon != NULL) {
        CBH_Player::GetInstance()->m_gameStats.RegisterHit(this, m_hitWeapon);
        CDH_Weapon* primary = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon01(true);
        CHuntingInfo* hunting = static_cast<App*>(WindowApp::m_instance)->m_huntingInfo;
        if (primary == m_hitWeapon) {
            hunting->m_primaryHits++;
        } else {
            hunting->m_secondaryHits++;
        }
    }
    bool alive = !headshot && m_hp != 0;
    if (m_type != UnitCivilian && !alive && !m_inCover) {
        CBH_Player::GetInstance()->m_challengeStats->m_killsBeforePlayerCover++;
    }
    if (m_hp == 0) {
        if (m_hitWeapon == NULL && m_type == UnitBoss) {
            CBH_Player::GetInstance()->m_challengeStats->m_bossKillsByExplosiveObject++;
        }
        if (m_fatalDamage) {
            CDH_Weapon* primary = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon01(true);
            CHuntingInfo* hunting = static_cast<App*>(WindowApp::m_instance)->m_huntingInfo;
            if (primary == m_hitWeapon) {
                hunting->m_primaryKills++;
            } else {
                hunting->m_secondaryKills++;
            }
        }
        float distance = static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->GetDistanceToUnit(this);
        if (m_type == UnitCivilian) {
            CBH_Player::GetInstance()->m_gameStats.AddCiviliansKilled(1);
            CBH_Player::GetInstance()->m_challengeStats->RegisterKill(this, m_hitWeapon, headshot, !m_tagged,
                                                                      static_cast<int>(distance));
        } else {
            if (m_type == UnitBoss) {
                CBH_Player::GetInstance()->m_gameStats.AddTargetsKilled(1);
                if (m_runningAway) {
                    CBH_Player::GetInstance()->m_gameStats.RegisterEscapingKill();
                }
            } else if (m_type == UnitWarrior) {
                CBH_Player::GetInstance()->m_gameStats.AddEnemyKilled(1);
            }
            // weapon 36 is the minigun
            if (static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetActiveWeapon()->GetId() == 36) {
                CBH_Player::GetInstance()->m_gameStats.m_minigunKills++;
            }
            CBH_Player::GetInstance()->m_challengeStats->RegisterKill(this, m_hitWeapon, headshot, !m_tagged,
                                                                      static_cast<int>(distance));
            CBH_Player::GetInstance()->m_gameStats.RegisterEnemyKill();
        }
        const CAmmo* ammo = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetActiveWeapon()
            ->GetActiveAmmo();
        if (ammo != NULL) {
            if (XString(ammo->m_ammoType) == XString("incendiary")) {
                CBH_Player::GetInstance()->m_gameStats.m_incendiaryAmmoKills++;
            }
            if (XString(ammo->m_ammoType) == XString("explosive")) {
                CBH_Player::GetInstance()->m_gameStats.m_explosiveAmmoKills++;
            }
        }
        int experienceBefore = static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_experienceEarned;
        m_mind.OnDeath(static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->GetCurrentCameraPos());
        m_field1f0 = static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_experienceEarned - experienceBefore;
    }
    if (!suppressEffects) {
        static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->ShowUnitInfo(this, m_damage);
        if (m_damageType == HitHead) {
            static_cast<App*>(WindowApp::m_instance)->m_soundManager->PlaySound("IDM_HEADSHOT", false, false);
        } else {
            PlayWoundingSound();
        }
        m_body.DrawHitEffect();
    }
    m_tagged = true;
}

void CUnit::PlayWoundingSound()
{
    bool fullHealth = false;
    if (m_maxHp != 0) {
        fullHealth = static_cast<float>(m_hp * 100) / m_maxHp > 50.0f;
    }
    const char* sound;
    if (m_body.m_type->m_visualCharacter->m_isMan) {
        if (fullHealth) {
            sound = "KEYSET_SOUND_MEN_HIT_FULL_HEALTH";
        } else {
            sound = "KEYSET_SOUND_MEN_HIT_LOW_HEALTH";
        }
    } else if (fullHealth) {
        sound = "KEYSET_SOUND_WOMEN_HIT_FULL_HEALTH";
    } else {
        sound = "KEYSET_SOUND_WOMEN_HIT_LOW_HEALTH";
    }
    static_cast<App*>(WindowApp::m_instance)->m_soundManager->PlayRandomSound(sound, false);
}

void CUnit::GetMovementAnimParam(float* speed, EPersonAnimType* animation, bool run)
{
    int type;
    if (m_hp < m_maxHp) {
        *speed = CVisualCharacterType::m_runWoundSpeed;
        type = 7;
    } else {
        bool isMan = m_body.m_type->m_visualCharacter->m_isMan;
        if (run) {
            if (isMan) {
                *speed = CVisualCharacterType::m_runSpeedMan;
                type = 4;
            } else {
                *speed = CVisualCharacterType::m_runSpeedWoman;
                type = 5;
            }
        } else if (isMan) {
            *speed = CVisualCharacterType::m_walkSpeedMan;
            type = 2;
        } else {
            *speed = CVisualCharacterType::m_walkSpeedWoman;
            type = 3;
        }
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
