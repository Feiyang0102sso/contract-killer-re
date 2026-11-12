// CPeaceIdleExecutor.cpp - peace-time idle animation.
#include "CPeaceIdleExecutor.h"
#include "CUnitMind.h"
#include "CUnit.h"

void CPeaceIdleExecutor::Tick(int deltaTime)
{
    if (m_idle < 0) {
        Finish(EXECUTOR_RESULT_SUCCESS);
    }
}

void CPeaceIdleExecutor::OnAnimationComplete(EPersonAnimType animType, const char* animName)
{
    if (IsAnimationTheSame(animName) == true) {
        Finish(EXECUTOR_RESULT_SUCCESS);
    }
}

void CPeaceIdleExecutor::OnStart()
{
    if (m_idle < 0) {
        return;
    }
    CUnitBody* body = m_mind->GetUnit()->GetBody();
    if (body == NULL) {
        Finish(EXECUTOR_RESULT_NO_BODY);
    } else if (m_idle == 0) {
        m_activeAnimation = body->StartAnimation(PERSON_ANIM_TYPE_DEFAULT, false);
    } else {
        m_activeAnimation = body->PlayIdleAnimation(m_idle - 1);
    }
}

void AddPeaceIdleExecutor(CUnitMind* mind)
{
    if (mind == NULL) {
        return;
    }
    CPeaceIdleExecutor* executor = new CPeaceIdleExecutor(mind);
    mind->AddExecutor(executor);
    executor->OnStart();
}

void AddCurrentPeaceIdleExecutor(CUnitMind* mind, int idle)
{
    if (idle == 0) {
        AddPeaceIdleExecutor(mind);
        return;
    }
    if (mind == NULL) {
        return;
    }
    CPeaceIdleExecutor* executor = new CPeaceIdleExecutor(mind);
    mind->AddExecutor(executor);
    executor->SetIdle(idle);
    executor->OnStart();
}
