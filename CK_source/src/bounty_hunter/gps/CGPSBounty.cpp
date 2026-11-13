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
    const CFactionType* faction = static_cast<App*>(WindowApp::m_instance)->InitFactionTypeManager()->GetRandomFaction();
    const XString& boss = faction->GetBoss();
    m_boss = static_cast<App*>(WindowApp::m_instance)->InitPersonTypeManager()->GetType(boss);
    m_bossIsMan = m_boss->m_visualCharacter->m_isMan;

    bool noCamera;
    if (informant != NULL) {
        if (!informant->HasSpecialSquad()) {
            m_squadName = boss;
        } else {
            m_squadName = informant->GetSquadName();
        }
        noCamera = camera == NULL;
        unsigned char gated = informant->IsWeaponGated();
        m_weaponGated = gated;
        if (camera != NULL && gated == 1) {
            int weapon;
            if (CRandGen::GetInstance()->GetRandRange(0, 1) != 0) {
                weapon = camera->GetGatedWeaponID();
            } else {
                weapon = -1;
            }
            m_gatedWeapon = weapon;
        }
    } else {
        m_squadName = boss;
        noCamera = camera == NULL;
        m_weaponGated = false;
    }

    if (m_boss == NULL || m_boss->m_visualCharacter == NULL) {
        return;
    }
    m_bossName = m_boss->m_nationality->GetRandomFullName(m_boss->m_visualCharacter->m_isMan);
    m_messageType = BountyMessageDefault;
    if (m_weaponGated) {
        m_messageType = BountyMessageWeaponGated;
    } else if (!noCamera && camera->m_durationMs >= 1) {
        m_messageType = BountyMessageMoving;
    }
    if (m_randomWeapon != -1) {
        m_messageType = BountyMessageRandomWeapon;
    }
    SetInfoFromBountyType(informant);
}

CGPSBounty::CGPSBounty(CGPSHauntCamera* camera, TiXmlElement* element)
    : CGPSHuntingItem(camera), m_messageType(BountyMessageDefault)
{
    // the original leaves m_bountyType for Load()
    Load(element);
}

void CGPSBounty::SetInfoFromBountyType(const CGPSInstantInformant* informant)
{
    int level = CBH_Player::GetInstance()->GetLevel();
    const CBountyManager::SBountyGeneralInfo* info =
        static_cast<App*>(WindowApp::m_instance)->m_bountyManager->GetInfo(m_bountyType);
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
    CProgressSystem* progress = static_cast<App*>(WindowApp::m_instance)->InitProgressSystem();
    m_civilianCount = progress->GetCivilianCount(CBH_Player::GetInstance()->GetLevel());
    m_experienceReward = info->m_xp;
    m_moneyPenalty = info->m_moneyPenalty;
    m_experiencePenalty = info->m_xpPenalty;
    int baseMoney = info->m_money;
    float moneyPerLevel = static_cast<App*>(WindowApp::m_instance)->m_helper->m_config.GetFloatValueByToken(
        "BOUNTY_MONEY_REWARD_COEFF", true);
    m_moneyReward = static_cast<int>(baseMoney + baseMoney * moneyPerLevel * (level - 1));
    int xpPerLevel = static_cast<App*>(WindowApp::m_instance)->m_helper->m_config.GetIntegerValueByToken(
        "ADD_MISSION_XP", true);
    m_experienceReward += xpPerLevel * (level - 1);
    if (informant != NULL) {
        float xpMultiplier = informant->GetXpMultiplier();
        m_experienceReward = static_cast<int>(m_experienceReward * xpMultiplier);
        float moneyMultiplier = informant->GetMoneyMultiplier();
        m_moneyReward = static_cast<int>(m_moneyReward * moneyMultiplier);
    }
    const CSquadType* squad = static_cast<App*>(WindowApp::m_instance)->InitSquadTypeManager()->GetType(m_squadName);
    if (squad->m_field18 > 1.0f) {
        m_experienceReward *= 2;
        m_moneyReward *= 2;
    }
}

void CGPSBounty::Load(TiXmlElement* element)
{
    CGPSHuntingItem::Load(element);
    XString bossName = CXmlHelper::GetAttributeValue(element, "bossName");
    m_boss = static_cast<App*>(WindowApp::m_instance)->InitPersonTypeManager()->GetType(bossName);
    m_bossIsMan = m_boss->m_visualCharacter->m_isMan;
    m_bountyType = CXmlHelper::GetAttributeValue(element, "bountyType").ToInt();
    int messageType = CXmlHelper::GetAttributeValueOrUseDefault(element, "bountyMessageType", XString("0")).ToInt();
    m_messageType = static_cast<EBoutnyMessageType>(messageType);
}

TiXmlElement* CGPSBounty::Save(TiXmlElement* parent)
{
    TiXmlElement* element = CGPSHuntingItem::Save(parent);
    element->SetAttribute("bossName", XString::AnsiString(m_boss->m_name.c_str()));
    element->SetAttribute("bountyType", m_bountyType);
    element->SetAttribute("itemType", 1);
    element->SetAttribute("bountyMessageType", m_messageType);
    return element;
}

XString CGPSBounty::GetObjectiveTitle() const
{
    const CBountyManager::SMessage* message = static_cast<App*>(WindowApp::m_instance)->m_bountyManager
        ->GetRandomMessageWithSeed(m_bountyType, XString(""), m_seed, m_messageType);
    const XString& title = message->m_title;
    if (m_weaponGated) {
        if (m_gatedWeapon >= 1) {
            if (m_camera != NULL) {
                CDH_Weapon* weapon = static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->GetWeaponById(m_gatedWeapon);
                if (weapon != NULL) {
                    return title.Replace(L"[Weapon]", CDH_Weapon::GetWeaponCategoryName(weapon->m_category).c_str(), true);
                }
            }
        } else if (m_camera != NULL) {
            return title.Replace(L"[Weapon]", m_camera->m_gatedWeaponCategory.Upper().c_str(), true);
        }
    } else if (m_camera != NULL && m_messageType == BountyMessageMoving) {
        XString vehicle = Window::ResString(XString::AnsiString(m_camera->m_vehicle.c_str()));
        return title.Replace(L"[Vehicle]", vehicle.c_str(), true);
    }
    return title;
}

XString CGPSBounty::GetObjectiveGoal() const
{
    const CBountyManager::SMessage* message = static_cast<App*>(WindowApp::m_instance)->m_bountyManager
        ->GetRandomMessageWithSeed(m_bountyType, XString(""), m_seed, m_messageType);
    XString goal = ParametrizeText(message->m_goal);
    if (m_weaponGated) {
        if (m_gatedWeapon >= 1) {
            if (m_camera != NULL) {
                CDH_Weapon* weapon = static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->GetWeaponById(m_gatedWeapon);
                if (weapon != NULL) {
                    return goal.Replace(L"[Weapon]", weapon->GetName().c_str(), true);
                }
            }
        } else if (m_camera != NULL) {
            return goal.Replace(L"[Weapon]", m_camera->m_gatedWeaponCategory.Upper().c_str(), true);
        }
    } else if (m_camera != NULL && m_messageType == BountyMessageMoving) {
        XString vehicle = Window::ResString(XString::AnsiString(m_camera->m_vehicle.c_str()));
        return goal.Replace(L"[Vehicle]", vehicle.c_str(), true);
    }
    return goal;
}

XString CGPSBounty::GetBountyMessage() const
{
    const CBountyManager::SMessage* message = static_cast<App*>(WindowApp::m_instance)->m_bountyManager
        ->GetRandomMessageWithSeed(m_bountyType, XString(""), m_seed, m_messageType);
    XString text = ParametrizeText(message->m_text);
    if (m_weaponGated) {
        if (m_gatedWeapon >= 1) {
            if (m_camera != NULL) {
                CDH_Weapon* weapon = static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->GetWeaponById(m_gatedWeapon);
                if (weapon != NULL) {
                    return text.Replace(L"[Weapon]", weapon->GetName().c_str(), true);
                }
            }
        } else if (m_camera != NULL) {
            return text.Replace(L"[Weapon]", m_camera->m_gatedWeaponCategory.Upper().c_str(), true);
        }
    } else {
        if (m_camera != NULL && m_messageType == BountyMessageMoving) {
            XString vehicle = Window::ResString(XString::AnsiString(m_camera->m_vehicle.c_str()));
            return text.Replace(L"[Vehicle]", vehicle.c_str(), true);
        }
        if (m_randomWeapon != -1) {
            CDH_Weapon* weapon = static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->GetWeaponById(m_randomWeapon);
            return text.Replace(L"[Weapon]", weapon->GetName().c_str(), true);
        }
    }
    return text;
}

float CGPSBounty::GenerateDiffRandomizer() const
{
    CBountyManager::SBountyGeneralInfo info =
        *static_cast<App*>(WindowApp::m_instance)->m_bountyManager->GetInfo(m_bountyType);
    float minimum = info.m_difficultyMin;
    float maximum = info.m_difficultyMax;
    CRandGen* random = CRandGen::GetInstance();
    unsigned int difficulty = random->GetRandRange(
        static_cast<int>(minimum * 1000.0f), static_cast<unsigned int>(maximum * 1000.0f));
    return difficulty * 0.001f;
}

XString CGPSBounty::GetScriptName(CGPSHauntCamera* camera) const
{
    // the original checks neither the bounty info nor the camera
    App* app = static_cast<App*>(WindowApp::m_instance);
    const CBountyManager::SBountyGeneralInfo* info = app->m_bountyManager->GetInfo(m_bountyType);
    return app->m_bountyManager->GetRandomScriptWithSeed(*info, m_bountyType, camera->m_location,
                                                         camera->m_cameraIndex, m_playerLevel, m_seed);
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
