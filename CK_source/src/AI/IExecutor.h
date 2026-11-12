// IExecutor.h - one behaviour step of a unit mind (the executors form a stack in CUnitMind).
#ifndef IEXECUTOR_H
#define IEXECUTOR_H

#include <stddef.h>
#include "CEnemyWeaponType.h"

class CUnit;
class CUnitMind;
class CAuxRender;
class vec3;

/** How an executor ended (the original enum name keeps its typo). */
enum EExecutosResult {
    EXECUTOR_RESULT_SUCCESS = 0,
    EXECUTOR_RESULT_FAILED = 1,
    EXECUTOR_RESULT_NO_BODY = 2,        // OnStart found no unit body
    EXECUTOR_RESULT_STUN_OVER = 3       // CStunExecutor: animation ended or timed out
};

class IExecutor {
public:
    IExecutor(CUnitMind* mind);
    virtual ~IExecutor();

    virtual void Tick(int deltaTime) {}
    virtual void OnAnimationComplete(EPersonAnimType animType, const char* animName) {}
    virtual void OnPathComplete() {}
    virtual void OnBurnt(int duration, int damage) {}
    virtual void OnHit(bool explosive) {}
    virtual void OnReceiveDamage() {}
    virtual void OnTranquillize() {}
    virtual void OnDeath(const vec3& hitDirection) {}
    virtual void OnSeeUnit(const CUnit* unit) {}
    virtual void OnCancel() {}
    virtual void OnChildFinished(EExecutosResult result) {}
    /** Name used by CUnitMind::GetExecutor / HasExecutor. */
    virtual const wchar_t* GetName() const = 0;
    virtual void DebugRender(CAuxRender* render) {}

    /** Stop the animation this executor started on the unit body. */
    void StopActiveAnimation();
    /** Drop the child executors and leave the mind's stack. */
    void Finish(EExecutosResult result);
    /** Drop the executors pushed after this one. */
    void CancelChildren();
    /** True when `animName` is the animation this executor is playing. */
    bool IsAnimationTheSame(const char* animName) const;

protected:
    CUnitMind* m_mind;              // 0x04
    const char* m_activeAnimation;  // 0x08 animation started by this executor, NULL when none
};

#endif
