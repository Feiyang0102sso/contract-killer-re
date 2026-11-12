#include "CGPSBounty.h"

#include "App.h"
#include "CBH_PersonType.h"
#include "CBH_PersonTypeManager.h"
#include "CBH_Player.h"
#include "CDH_Weapon.h"
#include "CDH_WeaponsManager.h"
#include "CFactionType.h"
#include "CFactionTypeManager.h"
#include "CGPSHauntCamera.h"
#include "CGPSInstantInformant.h"
#include "CNationalityType.h"
#include "CProgressSystem.h"
#include "CRandGen.h"
#include "CSquadType.h"
#include "CSquadTypeManager.h"
#include "CVisualCharacterType.h"
#include "CXmlHelper.h"
#include "DGHelper.h"

CGPSBounty::CGPSBounty(CGPSHauntCamera* camera, int type, const CGPSInstantInformant* informant)
    : CGPSHuntingItem(camera), m_bountyType(type), m_messageType(BountyMessageDefault)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    const CFactionType* faction = app->InitFactionTypeManager()->GetRandomFaction();
    if (faction == NULL) {
        return;
    }
    const XString& boss = faction->GetBoss();
    m_boss = app->InitPersonTypeManager()->GetType(boss);
    if (m_boss == NULL) {
        return;
    }
    m_squadName = boss;
    if (m_boss->m_visualCharacter != NULL) {
        m_bossIsMan = m_boss->m_visualCharacter->m_isMan;
    }
    if (m_boss->m_nationality != NULL) {
        m_bossName = m_boss->m_nationality->GetRandomFullName(m_bossIsMan);
    }
    if (informant != NULL) {
        if (informant->HasSpecialSquad()) {
            m_squadName = informant->GetSquadName().Trim(true, true);
        }
        m_weaponGated = informant->IsWeaponGated();
        if (m_weaponGated && camera != NULL && CRandGen::GetInstance()->GetRandRange(0, 1) != 0) {
            m_gatedWeapon = camera->GetGatedWeaponID();
        }
    }
    if (m_weaponGated) {
        m_messageType = BountyMessageWeaponGated;
    } else if (camera != NULL && camera->m_durationMs > 0) {
        m_messageType = BountyMessageMoving;
    }
    SetInfoFromBountyType(informant);
}

CGPSBounty::CGPSBounty(CGPSHauntCamera* camera, TiXmlElement* element)
    : CGPSHuntingItem(camera), m_bountyType(0), m_messageType(BountyMessageDefault)
{
    Load(element);
}

void CGPSBounty::SetInfoFromBountyType(const CGPSInstantInformant* informant)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    const CBountyManager::SBountyGeneralInfo* info = app->m_bountyManager->GetInfo(m_bountyType);
    if (info == NULL) {
        return;
    }
    m_alertSeconds = info->m_alertSeconds;
    m_escapeSeconds = info->m_escapeSeconds;
    m_warTimer = true;
    m_bossFirst = info->m_bossFirst;
    m_bossWins = info->m_bossWins;
    m_tranquillize = info->m_tranquillize;
    m_bossSpawnsLast = info->m_bossSpawnsLast;
    m_silentKill = info->m_silentKill;
    m_minWeaponRange = info->m_minWeaponRange;
    m_energyCost = info->m_energy;
    int level = CBH_Player::GetInstance()->GetLevel();
    m_civilianCount = app->InitProgressSystem()->GetCivilianCount(level);
    m_moneyPenalty = info->m_moneyPenalty;
    m_experiencePenalty = info->m_xpPenalty;
    ConfigReader& config = app->m_helper->m_config;
    float moneyPerLevel = config.GetFloatValueByToken("BOUNTY_MONEY_REWARD_COEFF", true);
    m_moneyReward = static_cast<int>(info->m_money + info->m_money * moneyPerLevel * (level - 1));
    m_experienceReward = info->m_xp + config.GetIntegerValueByToken("ADD_MISSION_XP", true) * (level - 1);
    if (informant != NULL) {
        m_moneyReward = static_cast<int>(m_moneyReward * informant->GetMoneyMultiplier());
        m_experienceReward = static_cast<int>(m_experienceReward * informant->GetXpMultiplier());
    }
    const CSquadType* squad = app->InitSquadTypeManager()->GetType(m_squadName);
    if (squad != NULL && squad->m_field18 > 1.0f) {
        m_moneyReward *= 2;
        m_experienceReward *= 2;
    }
}

void CGPSBounty::Load(TiXmlElement* element)
{
    CGPSHuntingItem::Load(element);
    App* app = static_cast<App*>(WindowApp::m_instance);
    m_boss = app->InitPersonTypeManager()->GetType(CXmlHelper::GetAttributeValue(element, "bossName"));
    if (m_boss != NULL && m_boss->m_visualCharacter != NULL) {
        m_bossIsMan = m_boss->m_visualCharacter->m_isMan;
    }
    m_bountyType = CXmlHelper::GetAttributeValue(element, "bountyType").ToInt();
    m_messageType = static_cast<EBoutnyMessageType>(
        CXmlHelper::GetIntAttributeValueOrUseDefault(element, "bountyMessageType", 0));
}

TiXmlElement* CGPSBounty::Save(TiXmlElement* parent)
{
    TiXmlElement* element = CGPSHuntingItem::Save(parent);
    if (m_boss != NULL) {
        element->SetAttribute("bossName", XString::AnsiString(m_boss->m_name.c_str()));
    }
    element->SetAttribute("bountyType", m_bountyType);
    element->SetAttribute("itemType", 1);
    element->SetAttribute("bountyMessageType", m_messageType);
    return element;
}

const CBountyManager::SMessage* CGPSBounty::GetMessage() const
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    return app->m_bountyManager->GetRandomMessageWithSeed(m_bountyType, XString(), m_seed, m_messageType);
}

XString CGPSBounty::ReplaceMissionTerms(XString text, bool title) const
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (m_weaponGated && m_camera != NULL) {
        if (m_gatedWeapon < 1) {
            return text.Replace(L"[Weapon]", m_camera->m_gatedWeaponCategory.Upper().c_str(), true);
        }
        CDH_Weapon* weapon = app->InitWeaponManager()->GetWeaponById(m_gatedWeapon);
        if (weapon != NULL) {
            if (title) {
                return text.Replace(L"[Weapon]", CDH_Weapon::GetWeaponCategoryName(weapon->m_category).c_str(), true);
            }
            return text.Replace(L"[Weapon]", weapon->GetName().c_str(), true);
        }
    } else if (m_messageType == BountyMessageMoving && m_camera != NULL) {
        XString name = Window::ResString(XString::AnsiString(m_camera->m_vehicle.c_str()));
        return text.Replace(L"[Vehicle]", name.c_str(), true);
    } else if (!title && m_randomWeapon >= 0) {
        CDH_Weapon* weapon = app->InitWeaponManager()->GetWeaponById(m_randomWeapon);
        if (weapon != NULL) {
            return text.Replace(L"[Weapon]", weapon->GetName().c_str(), true);
        }
    }
    return text;
}

XString CGPSBounty::GetObjectiveTitle() const
{
    const CBountyManager::SMessage* message = GetMessage();
    if (message != NULL) {
        return ReplaceMissionTerms(message->m_title, true);
    }
    return XString();
}

XString CGPSBounty::GetObjectiveGoal() const
{
    const CBountyManager::SMessage* message = GetMessage();
    if (message != NULL) {
        return ReplaceMissionTerms(ParametrizeText(message->m_goal), false);
    }
    return XString();
}

XString CGPSBounty::GetBountyMessage() const
{
    const CBountyManager::SMessage* message = GetMessage();
    if (message != NULL) {
        return ReplaceMissionTerms(ParametrizeText(message->m_text), false);
    }
    return XString();
}

float CGPSBounty::GenerateDiffRandomizer() const
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    const CBountyManager::SBountyGeneralInfo* info = app->m_bountyManager->GetInfo(m_bountyType);
    if (info == NULL) {
        return 0.0f;
    }
    int low = static_cast<int>(info->m_difficultyMin * 1000.0f);
    int high = static_cast<int>(info->m_difficultyMax * 1000.0f);
    return CRandGen::GetInstance()->GetRandRange(low, high) * 0.001f;
}

XString CGPSBounty::GetScriptName(CGPSHauntCamera* camera) const
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    const CBountyManager::SBountyGeneralInfo* info = app->m_bountyManager->GetInfo(m_bountyType);
    if (info != NULL && camera != NULL) {
        return app->m_bountyManager->GetRandomScriptWithSeed(*info, m_bountyType, camera->m_location,
                                                             camera->m_cameraIndex, m_playerLevel, m_seed);
    }
    return XString();
}

bool CGPSBounty::IsNeedToBeDeleted() const
{
    if (m_weaponGated || m_silentKill) {
        unsigned int now = App::GetTime();
        return now >= m_creationTime && now - m_creationTime >= 86400;
    }
    return false;
}

typedef char AssertGPSBountySize[sizeof(CGPSBounty) == 0x74 ? 1 : -1];
