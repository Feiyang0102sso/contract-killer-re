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
const char* const FireSound = "IDM_ENEMY_FIRE_01";
}

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char BurnExecutorSizeCheck[sizeof(CBurnExecutor) == 0x34 ? 1 : -1];
#endif

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
    // the original only clears the pointer when there was an animation
    if (m_fireAnimation != NULL) {
        delete m_fireAnimation;
        m_fireAnimation = NULL;
    }
    m_fireRoot.Clear();
}

void CBurnExecutor::Tick(int elapsed)
{
    TickFireFX();
    if (m_expired || m_bodyPhase != Loop) {
        return;
    }
    int totalElapsed = m_elapsed + elapsed;
    m_elapsed = totalElapsed;
    int damageElapsed = m_damageElapsed + elapsed;
    m_damageElapsed = damageElapsed;
    if (totalElapsed > m_duration) {
        m_expired = true;
    }
    if (damageElapsed >= 1000) {
        if (m_mind == NULL) {
            return;
        }
        float damage = static_cast<float>(m_damage) / m_duration * damageElapsed;
        m_damageElapsed = damageElapsed % 1000;
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
    if (m_fireAnimation->isAnimationComplete(time) == 1 && !m_expired) {
        if (m_firePhase == End) {
            return;
        }
        switch (m_firePhase) {
        case Start:
            m_fireAnimation->setCurrentController(time, 1, false);
            m_firePhase = Loop;
            break;
        case Loop:
            m_fireAnimation->setCurrentController(time, 1, false);
            break;
        default:
            break;
        }
    }
    m_fireAnimation->animate(time);
}

void CBurnExecutor::OnAnimationComplete(EPersonAnimType, const char* name)
{
    if (IsAnimationTheSame(name) != 1) {
        return;
    }
    StopActiveAnimation();
    switch (m_bodyPhase) {
    case Start:
        m_activeAnimation = m_mind->GetUnit()->GetBody()->StartAnimation(BurnLoop, false);
        m_bodyPhase = Loop;
        break;
    case Loop:
        if (m_expired) {
            m_activeAnimation = m_mind->GetUnit()->GetBody()->StartAnimation(BurnEnd, false);
            m_bodyPhase = End;
        } else {
            m_activeAnimation = m_mind->GetUnit()->GetBody()->StartAnimation(BurnLoop, false);
        }
        break;
    case End:
        static_cast<App*>(WindowApp::m_instance)->m_soundManager->Stop(FireSound);
        Finish(EXECUTOR_RESULT_STUN_OVER);
        break;
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
    if (m_mind == NULL) {
        return;
    }
    {
        SwerveHelper::Group model = m_mind->GetUnit()->GetBody()->m_model;
        static_cast<QueryInterface&>(m_model) = model.m_interface;
    }
    SwerveHelper::Object3D effect = App::LoadObject3D("BIN_M3G_EFFECT_FIRE_SYSTEM");
    SwerveHelper::Group mount(m_model.find(DGHelper::getSwerveID("dummy_guyTransform")));
    if (mount == NULL) {
        return;
    }
    SwerveHelper::Object3D fireObject = effect.find(SwerveHelper::Id("fx_fire_rootTransform"));
    IQueryInterface* fireInterface = fireObject.m_interface;
    if (fireInterface != NULL) {
        m_fireRoot = SwerveHelper::Node(fireObject);
        if (m_fireRoot != NULL) {
            SwerveHelper::Node parent = m_fireRoot.getParent();
            mount.addChild(m_fireRoot);
            m_fireAnimation = new CDH_Animation(m_fireRoot);
            m_fireAnimation->addController(XString("start_burn"));
            m_fireAnimation->addController(XString("cyckle_burn"));
            m_fireAnimation->addController(XString("end_burn"));
        }
        fireObject.m_interface = NULL;
        fireInterface->Release();
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
