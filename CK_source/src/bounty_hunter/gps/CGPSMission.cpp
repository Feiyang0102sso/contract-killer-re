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

void CGPSMission::Init(int missionId)
{
    m_missionId = missionId;
    CMission* mission = FindMission(missionId);
    if (mission == NULL) {
        return;
    }
    // the original reads the visual character before testing the boss for NULL
    m_boss = static_cast<App*>(WindowApp::m_instance)->InitPersonTypeManager()->GetType(mission->m_boss);
    m_bossIsMan = m_boss->m_visualCharacter->m_isMan;
    if (m_boss == NULL) {
        return;
    }
    m_moneyReward = mission->m_moneyReward;
    m_experienceReward = mission->m_experienceReward;
    m_moneyPenalty = 0;
    m_experiencePenalty = 0;
    m_squadName = XString::Format(L"Mission%d", m_missionId);
    m_energyCost = mission->m_energyCost;
    m_civilianCount = mission->m_civiliansCount;
    m_tranquillize = mission->m_tranquillize;
    m_alertSeconds = mission->m_alertSeconds;
    m_escapeSeconds = mission->m_escapeSeconds;
    m_warTimer = mission->m_warTimer;
    m_bossFirst = mission->m_bossFirst;
    m_bossWins = mission->m_bossWins;
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
        // the original does not check the boss or its visual character
        m_boss = static_cast<App*>(WindowApp::m_instance)->InitPersonTypeManager()->GetType(mission->m_boss);
        m_bossIsMan = m_boss->m_visualCharacter->m_isMan;
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
    // the original does not check for a missing mission
    const XString& fakeName = FindMission(m_missionId)->m_fakeBossName;
    if (fakeName.ByteSize() != 0) {
        return fakeName;
    }
    return m_bossName;
}

CBH_PersonType* CGPSMission::GetObjectiveBossType() const
{
    // the original does not check for a missing mission and tests the byte size
    CMission* mission = FindMission(m_missionId);
    if (mission->m_fakeBoss.ByteSize() == 0) {
        return m_boss;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    return app->InitPersonTypeManager()->GetType(mission->m_fakeBoss);
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
    // the original does not check for a missing mission
    return FindMission(m_missionId)->m_scriptName;
}

XString CGPSMission::GetObjectiveTitle() const
{
    // the original does not check for a missing mission
    return FindMission(m_missionId)->m_objectiveTitle;
}

XString CGPSMission::GetObjectiveGoal() const
{
    // the original does not check for a missing mission
    return ParametrizeText(FindMission(m_missionId)->m_objectiveGoal);
}

typedef char AssertGPSMissionSize[sizeof(CGPSMission) == 0x70 ? 1 : -1];
