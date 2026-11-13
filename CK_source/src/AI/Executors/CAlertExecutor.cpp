// CAlertExecutor.cpp - alerted person: afraid animation, finished when it ends.
#include "CAlertExecutor.h"
#include "CUnitMind.h"
#include "CUnit.h"

void CAlertExecutor::OnAnimationComplete(EPersonAnimType animType, const char* animName)
{
    if (IsAnimationTheSame(animName) == true) {
        Finish(EXECUTOR_RESULT_SUCCESS);
    }
}

void CAlertExecutor::OnStart()
{
    CUnitBody* body = m_mind->GetUnit()->GetBody();
    if (body == NULL) {
        Finish(EXECUTOR_RESULT_NO_BODY);
    } else {
        m_activeAnimation = body->PlayAfraidAnimation();
    }
}

void AddAlertExecutor(CUnitMind* mind)
{
    if (mind == NULL) {
        return;
    }
    CAlertExecutor* executor = new CAlertExecutor(mind);
    mind->AddExecutor(executor);
    executor->OnStart();
}
