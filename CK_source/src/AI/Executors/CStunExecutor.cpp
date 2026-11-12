// CStunExecutor.cpp - stunned person: stun animation, finished after it or a timeout.
#include "CStunExecutor.h"
#include "CUnitMind.h"
#include "CUnit.h"

void CStunExecutor::Tick(int deltaTime)
{
    m_elapsed += deltaTime;
    if (m_elapsed > 12000) {
        Finish(EXECUTOR_RESULT_STUN_OVER);
    }
}

void CStunExecutor::OnAnimationComplete(EPersonAnimType animType, const char* animName)
{
    if (IsAnimationTheSame(animName) == true) {
        Finish(EXECUTOR_RESULT_STUN_OVER);
    }
}

void CStunExecutor::OnStart()
{
    CUnitBody* body = m_mind->GetUnit()->GetBody();
    if (body == NULL) {
        Finish(EXECUTOR_RESULT_NO_BODY);
    } else {
        m_activeAnimation = body->StartAnimation(PERSON_ANIM_STUN, false);
    }
}

void AddStunExecutor(CUnitMind* mind)
{
    if (mind == NULL) {
        return;
    }
    CStunExecutor* executor = new CStunExecutor(mind);
    mind->AddExecutor(executor);
    executor->OnStart();
}
