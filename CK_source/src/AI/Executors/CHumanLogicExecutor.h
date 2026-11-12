// CHumanLogicExecutor.h - root logic of a person: picks the peace-time behaviour and reacts to
// burns, hits, tranquillizers and death by pushing the matching executor.
#ifndef CHUMANLOGICEXECUTOR_H
#define CHUMANLOGICEXECUTOR_H

#include "IExecutor.h"

class CHumanLogicExecutor : public IExecutor {
public:
    CHumanLogicExecutor(CUnitMind* mind, bool scripted, bool inSquad);

    /** Push the peace-time executor (squad, scripted or plain). */
    void OnStart();
    virtual void Tick(int elapsed);
    virtual void OnBurnt(int duration, int damage);
    virtual void OnHit(bool explosive);
    virtual void OnTranquillize();
    virtual void OnDeath(const vec3& hitDirection);

    void RegisterAsTranquillized();
    void RegisterAsDead();
    void RegisterAsNeutralized();

protected:
    bool m_reacted;     // 0x0c set by the derived logic once it reacted to a threat
    bool m_scripted;    // 0x0d peace time follows a script
    bool m_inSquad;     // 0x0e peace time inside a squad
};

#endif
