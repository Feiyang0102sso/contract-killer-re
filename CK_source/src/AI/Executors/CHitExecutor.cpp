// CHitExecutor.cpp - choose a hit reaction and trigger its sound and visual effect.
#include "CHitExecutor.h"

#include "App.h"
#include "CDH_SoundManager.h"
#include "CRandGen.h"
#include "CSwerveGame.h"
#include "CUnit.h"
#include "DGHelper.h"

namespace {
const EPersonAnimType HitFirst = static_cast<EPersonAnimType>(21);
const EPersonAnimType HitSecond = static_cast<EPersonAnimType>(22);
}

typedef char HitExecutorSizeCheck[sizeof(CHitExecutor) == 0x10 ? 1 : -1];

void CHitExecutor::OnAnimationComplete(EPersonAnimType, const char* name)
{
    if (IsAnimationTheSame(name)) {
        Finish(EXECUTOR_RESULT_STUN_OVER);
    }
}

void CHitExecutor::OnStart()
{
    if (m_mind == NULL) {
        return;
    }
    EPersonAnimType animation = HitFirst;
    if (CRandGen::GetInstance()->GetRandRange(0, 1) != 0) {
        animation = HitSecond;
    }
    CUnitBody* body = m_mind->GetUnit()->GetBody();
    m_activeAnimation = body->StartAnimation(animation, false);
    if (m_explosive) {
        vec3 normal(0.0f, 1.0f, 0.0f);
        vec3 position = m_mind->GetUnit()->GetBody()->m_position;
        position.m_z += 0.5f;
        static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->PlayInternalExplosion(position, normal, 0.3f);
        static_cast<App*>(WindowApp::m_instance)->m_soundManager->PlayRandomSound("KEYSET_SOUND_MAN_EXPLODE", true);
    }
}

void AddHitExecutor(CUnitMind* mind, bool explosive)
{
    if (mind != NULL) {
        CHitExecutor* executor = new CHitExecutor(mind, explosive);
        mind->AddExecutor(executor);
        executor->OnStart();
    }
}
