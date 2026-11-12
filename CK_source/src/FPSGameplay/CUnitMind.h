// CUnitMind.h - the executor stack driving one unit (owned by CUnit at +0x188).
#ifndef CUNITMIND_H
#define CUNITMIND_H

#include <stddef.h>
#include "Vector_ss.h"
#include "IExecutor.h"

class CUnit;
class CPathCalculator;
class vec3;

class CUnitMind {
public:
    CUnitMind(CPathCalculator* pathCalculator);
    ~CUnitMind();

    void Init(CUnit* unit);
    /** Wear off the tranquillizer, then tick every executor. */
    void Tick(int deltaTime);
    /** Push `executor` on top of the stack (the mind owns it). */
    void AddExecutor(IExecutor* executor);

    // Events forwarded to every executor.
    void OnAnimationComplete(EPersonAnimType animType, const char* animName);
    void OnPathComplete();
    void OnBurnt(int duration, int damage);
    void OnHit(bool explosive);
    void OnReceiveDamage();
    /** Add tranquillizer dose (never below 0). */
    void OnTranquillize(int dose);
    void OnDeath(const vec3& hitDirection);
    void OnSeeUnit(const CUnit* unit);

    /** Cancel and delete every executor. */
    void CancelAllExecutors();
    /** Remove `executor` when it is the top one and notify the executor below. */
    void FinishExecutor(const IExecutor* executor, EExecutosResult result);
    /** Cancel and delete the executors above `executor`. */
    void CancelChildrenExecutors(const IExecutor* executor);
    IExecutor* GetExecutor(const wchar_t* name);
    bool HasExecutor(const wchar_t* name);

    CUnit* GetUnit() const
    {
        return m_unit;
    }

    /** Stack index of `executor`, -1 when absent or NULL. */
    int GetExecutorIndex(const IExecutor* executor) const
    {
        if (executor != NULL) {
            for (int i = 0; i < m_executors.size(); i++) {
                if (m_executors[i] == executor) {
                    return i;
                }
            }
        }
        return -1;
    }

    CUnit* m_unit;                          // 0x00
    CPathCalculator* m_pathCalculator;      // 0x04
    Vector<IExecutor*> m_executors;         // 0x08 bottom first
    int m_tranquillizeDose;                 // 0x18
    float m_tranquillizeTime;               // 0x1c
};

#endif
