// CBurnExecutor.cpp - drive the burn animation, sound and unit health.
#include "CBurnExecutor.h"

#include "App.h"
#include "CDH_Animation.h"
#include "CDH_SoundManager.h"
#include "CSwerveGame.h"
#include "CUnit.h"
#include "DGHelper.h"

namespace {
const EPersonAnimType BurnStart = static_cast<EPersonAnimType>(18);
const EPersonAnimType BurnLoop = static_cast<EPersonAnimType>(19);
const EPersonAnimType BurnEnd = static_cast<EPersonAnimType>(20);
const char FireSound[] = "IDM_ENEMY_FIRE_01";
}

typedef char BurnExecutorSizeCheck[sizeof(CBurnExecutor) == 0x34 ? 1 : -1];

void CBurnExecutor::SetDamageAndTime(int duration, int damage)
{
    m_duration = duration;
    m_elapsed = 0;
    m_damageElapsed = 0;
    m_damage = damage;
    m_expired = false;
    if (m_bodyPhase == End) {
        m_bodyPhase = Loop;
    }
}

CBurnExecutor::~CBurnExecutor()
{
    UnbindFireFX();
    Cleanup();
}

void CBurnExecutor::UnbindFireFX()
{
    if (m_model != NULL) {
        SwerveHelper::Group mount(m_model.find(DGHelper::getSwerveID("dummy_guyTransform")));
        if (mount != NULL && m_fireRoot != NULL) {
            mount.removeChild(m_fireRoot);
        }
    }
}

void CBurnExecutor::Cleanup()
{
    delete m_fireAnimation;
    m_fireAnimation = NULL;
    m_fireRoot.Clear();
}

void CBurnExecutor::Tick(int elapsed)
{
    TickFireFX();
    if (m_expired || m_bodyPhase != Loop) {
        return;
    }
    m_elapsed += elapsed;
    m_damageElapsed += elapsed;
    if (m_duration < m_elapsed) {
        m_expired = true;
    }
    if (m_damageElapsed >= 1000 && m_mind != NULL) {
        float damage = static_cast<float>(m_damage) / m_duration * m_damageElapsed;
        m_damageElapsed %= 1000;
        CUnit* unit = m_mind->GetUnit();
        if (unit != NULL) {
            int amount = static_cast<int>(damage);
            if (amount < 1) {
                amount = 1;
            }
            unit->CalcDamageAndKillingPosibility(amount);
            unit->ReceivedDamage(true);
        }
    }
}

void CBurnExecutor::TickFireFX()
{
    if (m_fireAnimation == NULL) {
        return;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    int time = app->m_helper->m_swerveGame->m_missionElapsedMs;
    if (m_fireAnimation->isAnimationComplete(time) && !m_expired) {
        if (m_firePhase == End) {
            return;
        }
        m_fireAnimation->setCurrentController(time, 1, false);
        if (m_firePhase == Start) {
            m_firePhase = Loop;
        }
    }
    m_fireAnimation->animate(time);
}

void CBurnExecutor::OnAnimationComplete(EPersonAnimType, const char* name)
{
    if (!IsAnimationTheSame(name)) {
        return;
    }
    StopActiveAnimation();
    if (m_bodyPhase == End) {
        App* app = static_cast<App*>(WindowApp::m_instance);
        app->m_soundManager->Stop(FireSound);
        Finish(EXECUTOR_RESULT_STUN_OVER);
    } else {
        CUnitBody* body = m_mind->GetUnit()->GetBody();
        if (m_bodyPhase == Loop && m_expired) {
            m_activeAnimation = body->StartAnimation(BurnEnd, false);
            m_bodyPhase = End;
        } else {
            m_activeAnimation = body->StartAnimation(BurnLoop, false);
            m_bodyPhase = Loop;
        }
    }
}

void CBurnExecutor::OnCancel()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_soundManager->Stop(FireSound);
}

void CBurnExecutor::OnStart()
{
    if (m_mind != NULL) {
        CreateFireFX();
        m_activeAnimation = m_mind->GetUnit()->GetBody()->StartAnimation(BurnStart, false);
        StartFireFX();
        App* app = static_cast<App*>(WindowApp::m_instance);
        app->m_soundManager->PlaySound(FireSound, false, false);
    }
}

void CBurnExecutor::CreateFireFX()
{
    m_model = m_mind->GetUnit()->GetBody()->m_model;
    SwerveHelper::Object3D effect = App::LoadObject3D("BIN_M3G_EFFECT_FIRE_SYSTEM");
    if (m_model == NULL || effect == NULL) {
        return;
    }
    SwerveHelper::Group mount(m_model.find(DGHelper::getSwerveID("dummy_guyTransform")));
    if (mount != NULL) {
        m_fireRoot = SwerveHelper::Node(effect.find(SwerveHelper::Id("fx_fire_rootTransform")));
        if (m_fireRoot != NULL) {
            SwerveHelper::Node parent = m_fireRoot.getParent();
            mount.addChild(m_fireRoot);
            m_fireAnimation = new CDH_Animation(m_fireRoot);
            m_fireAnimation->addController(XString("start_burn"));
            m_fireAnimation->addController(XString("cyckle_burn"));
            m_fireAnimation->addController(XString("end_burn"));
        }
    }
}

void CBurnExecutor::StartFireFX()
{
    if (m_fireAnimation != NULL) {
        App* app = static_cast<App*>(WindowApp::m_instance);
        m_fireAnimation->setCurrentController(app->m_helper->m_swerveGame->m_missionElapsedMs, 0, false);
    }
}

void AddBurnExecutor(CUnitMind* mind, int duration, int damage)
{
    if (mind != NULL) {
        CBurnExecutor* executor = new CBurnExecutor(mind, duration, damage);
        mind->AddExecutor(executor);
        executor->OnStart();
    }
}
