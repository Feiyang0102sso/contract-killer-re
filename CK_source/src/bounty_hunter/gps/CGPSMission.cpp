#include "CGPSMission.h"

#include "App.h"
#include "CBH_PersonType.h"
#include "CBH_PersonTypeManager.h"
#include "CMissionManager.h"
#include "CRandGen.h"
#include "CVisualCharacterType.h"
#include "CXmlHelper.h"

namespace {
CMission* FindMission(int id)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    return app->m_missionManager->GetMissionById(id);
}
}

CGPSMission::CGPSMission(CGPSHauntCamera* camera, int missionId)
    : CGPSHuntingItem(camera)
{
    Init(missionId);
}

CGPSMission::CGPSMission(CGPSHauntCamera* camera, TiXmlElement* element)
    : CGPSHuntingItem(camera)
{
    Load(element);
}

void CGPSMission::ResolveBoss(const CMission& mission)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    m_boss = app->InitPersonTypeManager()->GetType(mission.m_boss);
    if (m_boss != NULL && m_boss->m_visualCharacter != NULL) {
        m_bossIsMan = m_boss->m_visualCharacter->m_isMan;
    }
}

void CGPSMission::Init(int missionId)
{
    m_missionId = missionId;
    CMission* mission = FindMission(missionId);
    if (mission == NULL) {
        return;
    }
    ResolveBoss(*mission);
    m_moneyReward = mission->m_moneyReward;
    m_experienceReward = mission->m_experienceReward;
    m_squadName = XString::Format(L"Mission%d", missionId);
    m_energyCost = mission->m_energyCost;
    m_civilianCount = mission->m_civiliansCount;
    m_alertSeconds = mission->m_alertSeconds;
    m_escapeSeconds = mission->m_escapeSeconds;
    m_warTimer = mission->m_warTimer;
    m_bossFirst = mission->m_bossFirst;
    m_bossWins = mission->m_bossWins;
    m_tranquillize = mission->m_tranquillize;
    m_bossSpawnsLast = mission->m_bossSpawnsLast;
    m_silentKill = mission->m_silentKill;
    m_bossName = mission->m_bossName;
    m_minWeaponRange = mission->m_minWeaponRange;
}

void CGPSMission::Load(TiXmlElement* element)
{
    CGPSHuntingItem::Load(element);
    m_missionId = CXmlHelper::GetAttributeValue(element, "missionId").ToInt();
    CMission* mission = FindMission(m_missionId);
    if (mission != NULL) {
        ResolveBoss(*mission);
    }
}

TiXmlElement* CGPSMission::Save(TiXmlElement* parent)
{
    TiXmlElement* element = CGPSHuntingItem::Save(parent);
    element->SetAttribute("missionId", m_missionId);
    element->SetAttribute("itemType", 0);
    return element;
}

const XString& CGPSMission::GetObjectiveBossScreenName() const
{
    CMission* mission = FindMission(m_missionId);
    if (mission != NULL && mission->m_fakeBossName.Length() != 0) {
        return mission->m_fakeBossName;
    }
    return m_bossName;
}

CBH_PersonType* CGPSMission::GetObjectiveBossType() const
{
    CMission* mission = FindMission(m_missionId);
    if (mission != NULL && mission->m_fakeBoss.Length() != 0) {
        App* app = static_cast<App*>(WindowApp::m_instance);
        return app->InitPersonTypeManager()->GetType(mission->m_fakeBoss);
    }
    return m_boss;
}

float CGPSMission::GenerateDiffRandomizer() const
{
    CMission* mission = FindMission(m_missionId);
    if (mission == NULL) {
        return 0.5f;
    }
    int minimum = static_cast<int>(mission->m_minDifficulty * 1000.0f);
    int maximum = static_cast<int>(mission->m_maxDifficulty * 1000.0f);
    return CRandGen::GetInstance()->GetRandRange(minimum, maximum) * 0.001f;
}

int CGPSMission::GetEstimatedWeaponID() const
{
    CMission* mission = FindMission(m_missionId);
    if (mission != NULL) {
        return mission->m_estimatedWeaponId;
    }
    return -1;
}

int CGPSMission::GetEstimatedAmmonCount() const
{
    CMission* mission = FindMission(m_missionId);
    if (mission != NULL) {
        return mission->m_estimatedWeaponAmmoCount;
    }
    return 0;
}

XString CGPSMission::GetScriptName(CGPSHauntCamera*) const
{
    CMission* mission = FindMission(m_missionId);
    if (mission != NULL) {
        return mission->m_scriptName;
    }
    return XString();
}

XString CGPSMission::GetObjectiveTitle() const
{
    CMission* mission = FindMission(m_missionId);
    if (mission != NULL) {
        return mission->m_objectiveTitle;
    }
    return XString();
}

XString CGPSMission::GetObjectiveGoal() const
{
    CMission* mission = FindMission(m_missionId);
    if (mission != NULL) {
        return ParametrizeText(mission->m_objectiveGoal);
    }
    return XString();
}

typedef char AssertGPSMissionSize[sizeof(CGPSMission) == 0x70 ? 1 : -1];
