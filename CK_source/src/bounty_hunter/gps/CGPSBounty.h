#ifndef CGPSBOUNTY_H
#define CGPSBOUNTY_H

#include "CBountyManager.h"
#include "CGPSHuntingItem.h"

class CGPSInstantInformant;

class CGPSBounty : public CGPSHuntingItem {
public:
    CGPSBounty(CGPSHauntCamera* camera, int type, const CGPSInstantInformant* informant);
    CGPSBounty(CGPSHauntCamera* camera, TiXmlElement* element);
    virtual ~CGPSBounty() {}

    virtual int GetAAId() const { return m_bountyType + 200; }
    virtual XString GetObjectiveTitle() const;
    virtual XString GetObjectiveGoal() const;
    virtual float GenerateDiffRandomizer() const;
    virtual int GetMissionId() const { return -1; }
    virtual XString GetScriptName(CGPSHauntCamera* camera) const;
    virtual int GetType() const { return 1; }
    virtual TiXmlElement* Save(TiXmlElement* parent);
    virtual bool IsNeedToBeDeleted() const;

    void Load(TiXmlElement* element);
    void SetInfoFromBountyType(const CGPSInstantInformant* informant);
    XString GetBountyMessage() const;

    int m_bountyType;                       // 0x6c
    EBoutnyMessageType m_messageType;

private:
    const CBountyManager::SMessage* GetMessage() const;
    XString ReplaceMissionTerms(XString text, bool title) const;
};

#endif
