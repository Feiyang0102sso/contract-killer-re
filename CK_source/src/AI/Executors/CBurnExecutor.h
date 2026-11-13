// CBurnExecutor.h - unit animation and periodic damage for the burn effect.
#ifndef CBURNEXECUTOR_H
#define CBURNEXECUTOR_H

#include "IExecutor.h"
#include "SwerveHelper.h"

class CDH_Animation;

class CBurnExecutor : public IExecutor {
public:
    CBurnExecutor(CUnitMind* mind, int duration, int damage)
        : IExecutor(mind), m_bodyPhase(Start), m_firePhase(Start),
          m_duration(duration), m_elapsed(0), m_damageElapsed(0), m_damage(damage), m_expired(false)
    {
        // The original clears the model handles and the animation pointer after the counters.
        m_model.Clear();
        m_fireRoot.Clear();
        m_fireAnimation = NULL;
    }
    virtual ~CBurnExecutor();

    void SetDamageAndTime(int duration, int damage);
    virtual void Tick(int elapsed);
    virtual void OnAnimationComplete(EPersonAnimType type, const char* name);
    virtual void OnCancel();
    virtual const wchar_t* GetName() const { return L"CBurnExecutor"; }
    void OnStart();

private:
    enum Phase { Start, Loop, End };
    void UnbindFireFX();
    void Cleanup();
    void TickFireFX();
    void CreateFireFX();
    void StartFireFX();

    SwerveHelper::Group m_model;       // 0x0c
    SwerveHelper::Node m_fireRoot;     // 0x10
    CDH_Animation* m_fireAnimation;    // 0x14
    Phase m_bodyPhase;                 // 0x18
    Phase m_firePhase;                 // 0x1c
    int m_duration;                    // 0x20
    int m_elapsed;                     // 0x24
    int m_damageElapsed;               // 0x28
    int m_damage;                      // 0x2c
    bool m_expired;                    // 0x30
};

void AddBurnExecutor(CUnitMind* mind, int duration, int damage);

#endif
