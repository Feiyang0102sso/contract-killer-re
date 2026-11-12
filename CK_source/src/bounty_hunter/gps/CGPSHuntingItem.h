// Common state and save format for a CK map mission or bounty.
#ifndef CGPSHUNTINGITEM_H
#define CGPSHUNTINGITEM_H

#include "xString.h"

class CBH_PersonType;
class CGPSHauntCamera;
class CStrWChar;
class TiXmlElement;

class CGPSHuntingItem {
public:
    explicit CGPSHuntingItem(CGPSHauntCamera* camera);
    virtual ~CGPSHuntingItem() {}
    virtual int GetAAId() const = 0;
    virtual XString GetObjectiveTitle() const = 0;
    virtual XString GetObjectiveGoal() const = 0;
    virtual const XString& GetObjectiveBossScreenName() const { return m_bossName; }
    virtual CBH_PersonType* GetObjectiveBossType() const { return m_boss; }
    virtual float GenerateDiffRandomizer() const = 0;
    virtual int GetMissionId() const = 0;
    virtual XString GetScriptName(CGPSHauntCamera* camera) const = 0;
    virtual int GetType() const = 0;
    virtual TiXmlElement* Save(TiXmlElement* parent);
    virtual int GetEstimatedWeaponID() const { return -1; }
    virtual int GetEstimatedAmmonCount() const { return 0; }
    virtual bool IsNeedToBeDeleted() const { return false; }

    void Load(TiXmlElement* element);
    XString ParametrizeText(const XString& text) const;
    static XString ParametrizeTextWithParams(const XString& text, XString bossName, bool isMan);
    void SetBossScreenName(const XString& name) { m_bossName.Assign(name.Clone()); }
    bool SetTargetFriendId(int friendId);
    bool SetTargetFBUser(CStrWChar identity);

    int m_experienceReward;
    int m_moneyReward;
    int m_experiencePenalty;
    int m_moneyPenalty;
    CBH_PersonType* m_boss;
    XString m_bossName;
    bool m_bossIsMan;
    XString m_squadName;
    int m_energyCost;
    int m_civilianCount;
    int m_alertSeconds;
    int m_escapeSeconds;
    bool m_warTimer;
    bool m_bossFirst;
    bool m_bossWins;
    bool m_tranquillize;
    bool m_bossSpawnsLast;
    bool m_silentKill;
    int m_targetFriendId;
    XString m_targetFBUser;
    bool m_revenge;
    float m_minWeaponRange;
    int m_seed;
    int m_playerLevel;
    int m_gatedWeapon;
    bool m_weaponGated;
    int m_randomWeapon;
    bool m_hasBonus;
    CGPSHauntCamera* m_camera;
    unsigned int m_creationTime;
};

#endif
