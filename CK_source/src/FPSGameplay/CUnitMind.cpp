// CUnitMind.cpp - the executor stack of a unit and the event fan-out to it.
#include <wchar.h>
#include "CUnitMind.h"
#include "CUnit.h"

CUnitMind::CUnitMind(CPathCalculator* pathCalculator)
    : m_unit(NULL),
      m_pathCalculator(pathCalculator)
{
    m_tranquillizeDose = 0;
    m_tranquillizeTime = 0;
    m_executors.ensureCapacity(10);
}

CUnitMind::~CUnitMind()
{
    SAFE_DELETE_VECTOR(m_executors);
}

void CUnitMind::Init(CUnit* unit)
{
    m_unit = unit;
}

void CUnitMind::Tick(int deltaTime)
{
    if (m_tranquillizeDose > 0) {
        m_tranquillizeTime += (float)(m_tranquillizeDose * deltaTime) * 0.001f;
        if (m_tranquillizeTime >= (float)m_unit->m_hp) {
            m_tranquillizeDose = 0;
            for (int i = 0; i < m_executors.size(); i++) {
                IExecutor* executor = m_executors[i];
                if (executor != NULL) {
                    executor->OnTranquillize();
                }
            }
        }
    }
    for (int i = 0; i < m_executors.size(); i++) {
        IExecutor* executor = m_executors[i];
        if (executor != NULL) {
            executor->Tick(deltaTime);
        }
    }
}

void CUnitMind::AddExecutor(IExecutor* executor)
{
    m_executors.addElement(executor);
}

void CUnitMind::OnAnimationComplete(EPersonAnimType animType, const char* animName)
{
    for (int i = 0; i < m_executors.size(); i++) {
        IExecutor* executor = m_executors[i];
        if (executor != NULL) {
            executor->OnAnimationComplete(animType, animName);
        }
    }
}

void CUnitMind::OnPathComplete()
{
    for (int i = 0; i < m_executors.size(); i++) {
        IExecutor* executor = m_executors[i];
        if (executor != NULL) {
            executor->OnPathComplete();
        }
    }
}

void CUnitMind::OnBurnt(int duration, int damage)
{
    for (int i = 0; i < m_executors.size(); i++) {
        IExecutor* executor = m_executors[i];
        if (executor != NULL) {
            executor->OnBurnt(duration, damage);
        }
    }
}

void CUnitMind::OnHit(bool explosive)
{
    for (int i = 0; i < m_executors.size(); i++) {
        IExecutor* executor = m_executors[i];
        if (executor != NULL) {
            executor->OnHit(explosive);
        }
    }
}

void CUnitMind::OnReceiveDamage()
{
    for (int i = 0; i < m_executors.size(); i++) {
        IExecutor* executor = m_executors[i];
        if (executor != NULL) {
            executor->OnReceiveDamage();
        }
    }
}

void CUnitMind::OnTranquillize(int dose)
{
    m_tranquillizeDose += dose;
    if (m_tranquillizeDose < 0) {
        m_tranquillizeDose = 0;
    }
}

void CUnitMind::OnDeath(const vec3& hitDirection)
{
    for (int i = 0; i < m_executors.size(); i++) {
        IExecutor* executor = m_executors[i];
        if (executor != NULL) {
            executor->OnDeath(hitDirection);
        }
    }
}

void CUnitMind::OnSeeUnit(const CUnit* unit)
{
    for (int i = 0; i < m_executors.size(); i++) {
        IExecutor* executor = m_executors[i];
        if (executor != NULL) {
            executor->OnSeeUnit(unit);
        }
    }
}

void CUnitMind::CancelAllExecutors()
{
    for (int i = 0; i < m_executors.size(); i++) {
        IExecutor* executor = m_executors[i];
        if (executor != NULL) {
            executor->OnCancel();
            delete executor;
        }
    }
    m_executors.setSize(0);
}

void CUnitMind::FinishExecutor(const IExecutor* executor, EExecutosResult result)
{
    int index = GetExecutorIndex(executor);
    if (index < 0) {
        return;
    }
    // only the top executor may finish
    int lastIndex = m_executors.size() - 1;
    if (lastIndex != index) {
        return;
    }
    delete executor;
    m_executors.setSize(lastIndex);
    if (index > 0) {
        m_executors[index - 1]->OnChildFinished(result);
    }
}

void CUnitMind::CancelChildrenExecutors(const IExecutor* executor)
{
    int index = GetExecutorIndex(executor);
    if (index < 0) {
        return;
    }
    int count = m_executors.size();
    int firstChild = index + 1;
    for (int i = firstChild; i < count; i++) {
        IExecutor* child = m_executors[i];
        if (child != NULL) {
            child->OnCancel();
            delete child;
        }
    }
    m_executors.setSize(firstChild);
}

IExecutor* CUnitMind::GetExecutor(const wchar_t* name)
{
    if (name != NULL) {
        for (int i = 0; i < m_executors.size(); i++) {
            IExecutor* executor = m_executors[i];
            if (executor != NULL && wcscmp(name, executor->GetName()) == 0) {
                return executor;
            }
        }
    }
    return NULL;
}

bool CUnitMind::HasExecutor(const wchar_t* name)
{
    if (name == NULL) {
        return false;
    }
    for (int i = 0; i < m_executors.size(); i++) {
        IExecutor* executor = m_executors[i];
        if (executor != NULL && wcscmp(name, executor->GetName()) == 0) {
            return true;
        }
    }
    return false;
}
