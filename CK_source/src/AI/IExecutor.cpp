// IExecutor.cpp - common executor plumbing (animation bookkeeping, finishing).
#include <string.h>
#include "IExecutor.h"
#include "CUnitMind.h"
#include "CUnit.h"

IExecutor::IExecutor(CUnitMind* mind)
{
    m_mind = mind;
    m_activeAnimation = NULL;
}

IExecutor::~IExecutor()
{
    StopActiveAnimation();
}

void IExecutor::StopActiveAnimation()
{
    if (m_activeAnimation != NULL) {
        CUnitBody* body = m_mind->GetUnit()->GetBody();
        if (body != NULL) {
            body->StopAnimation(m_activeAnimation);
            m_activeAnimation = NULL;
        }
    }
}

void IExecutor::Finish(EExecutosResult result)
{
    m_mind->CancelChildrenExecutors(this);
    m_mind->FinishExecutor(this, result);
}

void IExecutor::CancelChildren()
{
    m_mind->CancelChildrenExecutors(this);
}

bool IExecutor::IsAnimationTheSame(const char* animName) const
{
    return m_activeAnimation != NULL && animName != NULL && strcmp(animName, m_activeAnimation) == 0;
}
