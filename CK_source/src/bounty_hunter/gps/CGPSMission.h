// Story missions placed on CK camera markers.
#ifndef CGPSMISSION_H
#define CGPSMISSION_H

#include "CGPSHuntingItem.h"

class CMission;

class CGPSMission : public CGPSHuntingItem {
public:
    CGPSMission(CGPSHauntCamera* camera, int missionId);
    CGPSMission(CGPSHauntCamera* camera, TiXmlElement* element);

    virtual int GetAAId() const { return m_missionId; }
    virtual XString GetObjectiveTitle() const;
    virtual XString GetObjectiveGoal() const;
    virtual const XString& GetObjectiveBossScreenName() const;
    virtual CBH_PersonType* GetObjectiveBossType() const;
    virtual float GenerateDiffRandomizer() const;
    virtual int GetMissionId() const { return m_missionId; }
    virtual XString GetScriptName(CGPSHauntCamera* camera) const;
    virtual int GetType() const { return 0; }
    virtual TiXmlElement* Save(TiXmlElement* parent);
    virtual int GetEstimatedWeaponID() const;
    virtual int GetEstimatedAmmonCount() const;

    void Init(int missionId);
    void Load(TiXmlElement* element);

private:
    int m_missionId;
};

#endif
