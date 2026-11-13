// CProgressSystem.cpp - read the compact CK progress.xml level table.
#include "CProgressSystem.h"

#include <stdlib.h>
#include "App.h"
#include "CBH_PersonType.h"
#include "CBH_Player.h"
#include "CDH_Weapon.h"
#include "CHuntingInfo.h"
#include "CRandGen.h"
#include "CSwerveGame.h"
#include "CTrickShotManager.h"
#include "CUnit.h"
#include "CXmlHelper.h"
#include "DGHelper.h"
#include "ResMap.h"

typedef char CProgressSystem_ARMv7_Size[(sizeof(CProgressSystem) == 0x14) ? 1 : -1];

CProgressSystem::CProgressSystem()
{
    ResMap* resources = static_cast<App*>(WindowApp::m_instance)->InitResMap();
    char* text = resources->loadText(XString(L"res/common/configs/progress.xml"));
    TiXmlDocument document;
    document.Parse(text, NULL, TIXML_ENCODING_UNKNOWN);
    if (text != NULL) {
        free(text);
    }
    ParseLevels(document.FirstChild("progress"));
    m_field10 = 0.5f;
}

void CProgressSystem::ParseLevels(TiXmlNode* root)
{
    const int count = CXmlHelper::GetAttributeValue(root, "level_count").ToInt();
    m_levels.ensureCapacity(count);
    // The shipped table supplies dmgCoeff for every level.
    SLevelInfo defaults;
    defaults.m_xpThreshold = 0;
    defaults.m_maxEnergy = 0;
    defaults.m_civilianCount = 0;
    for (int remaining = count; remaining > 0; --remaining) {
        m_levels.addElement(defaults);
    }
    for (TiXmlNode* node = root->FirstChild("level"); node != NULL; node = node->NextSibling("level")) {
        const int index = CXmlHelper::GetAttributeValue(node, "num").ToInt();
        int xp = CXmlHelper::GetAttributeValue(node, "xp").ToInt();
        int energy = CXmlHelper::GetAttributeValue(node, "energy").ToInt();
        int civilians = CXmlHelper::GetAttributeValue(node, "civilianCount").ToInt();
        float damageCoefficient = CXmlHelper::GetAttributeValue(node, "dmgCoeff").ToFloat();
        if (index >= 0 && index < count) {
            m_levels[index].m_xpThreshold = xp;
            m_levels[index].m_maxEnergy = energy;
            m_levels[index].m_civilianCount = civilians;
            m_levels[index].m_damageCoefficient = damageCoefficient;
        }
    }
}

int CProgressSystem::GetMaxLevel() const
{
    return m_levels.size() - 1;
}

int CProgressSystem::GetMaxXPForLevel(int level) const
{
    if (level > 0 && level - 1 < m_levels.size()) {
        return m_levels[level - 1].m_xpThreshold;
    }
    return 0;
}

int CProgressSystem::GetMaxXP() const
{
    int level = m_levels.size();
    if (level > 0) {
        --level;
    }
    return GetMaxXPForLevel(level);
}

int CProgressSystem::GetMaxEnergyForLevel(int level) const
{
    if (level >= 0 && level < m_levels.size()) {
        return m_levels[level].m_maxEnergy;
    }
    return 0;
}

int CProgressSystem::GetLevel(int experience) const
{
    if (experience <= 0) {
        return 0;
    }
    for (int index = 0; index < m_levels.size(); ++index) {
        if (experience < m_levels[index].m_xpThreshold) {
            return index;
        }
    }
    return m_levels.size() - 1;
}

int CProgressSystem::GetCivilianCount(int level) const
{
    if (level >= 0 && level < m_levels.size()) {
        return m_levels[level].m_civilianCount;
    }
    return 0;
}

float CProgressSystem::GetRandValue() const
{
    unsigned int randomValue = CRandGen::GetInstance()->GetRandRange(0, 0x7fffffff);
    return randomValue * (1.0f / 2147483648.0f);
}

float CProgressSystem::GetLevelCoef(bool allowAlternate) const
{
    int level = CBH_Player::GetInstance()->GetLevel();
    ConfigReader& config = static_cast<App*>(WindowApp::m_instance)->m_helper->m_config;
    if (level >= 6 && allowAlternate == 1) {
        return config.GetFloatValueByToken("LEVEL_COEF_02", true);
    }
    return config.GetFloatValueByToken("LEVEL_COEF_01", true);
}

float CProgressSystem::GetDmgCoeff() const
{
    // the original does not range-check the level
    return m_levels[CBH_Player::GetInstance()->GetLevel()].m_damageCoefficient;
}

float CProgressSystem::GetLevelForkFormulaCoef(bool allowAlternate) const
{
    float coefficient = GetLevelCoef(allowAlternate);
    int range = static_cast<App*>(WindowApp::m_instance)->m_helper->m_config.GetIntegerValueByToken("RND_RANGE", true);
    int level = CBH_Player::GetInstance()->GetLevel();
    int minimum = level - range;
    int maximum = level + range;
    if (maximum > GetMaxLevel()) {
        maximum = GetMaxLevel();
    }
    float minimumOffset = minimum - 1;
    if (minimum < 1) {
        minimumOffset = 0.0f;
    }
    coefficient += coefficient;
    float lower = coefficient * minimumOffset + 1.0f;
    float upper = coefficient * (maximum - 1) + 1.0f;
    float random = GetRandValue();
    return lower + random * (upper - lower);
}

int CProgressSystem::CalcXpForKilledUnit(CUnit* unit)
{
    if (unit == NULL) {
        return 0;
    }
    int baseXp = unit->m_body.m_type->m_baseXp;
    CProgressSystem* progress = static_cast<App*>(WindowApp::m_instance)->InitProgressSystem();
    return static_cast<int>(baseXp * progress->GetLevelForkFormulaCoef(false));
}

int CProgressSystem::GetTaggingTimeMS()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    float limit = app->m_helper->m_config.GetFloatValueByToken("TIME_TAGGING_LIMIT_MS", true);
    // the original asks for the player before testing the level table
    CBH_Player* player = CBH_Player::GetInstance();
    if (m_levels.size() < 1) {
        return 0;
    }
    return static_cast<int>(limit) * (player->GetLevel() - 1) / m_levels.size();
}

void CProgressSystem::RegisterGameEvent(EGameEvent event, int experience, int headshot)
{
    switch (event) {
    case EVENT_ENEMY_KILLED:
        AddXPForUnit(experience, headshot == 1);
        CBH_Player::GetInstance()->OnEnemyKilled();
        break;
    case EVENT_ENEMY_TRANQUILLIZED:
        AddXPForUnit(experience, headshot == 1);
        CBH_Player::GetInstance()->OnEnemyTranquillized();
        break;
    case EVENT_ENEMY_ESCAPED:
        CBH_Player::GetInstance()->OnEnemyEscaped();
        break;
    }
}

void CProgressSystem::AddXPForUnit(int experience, bool headshot)
{
    int multiplier = static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->m_multiplier << headshot;
    if (multiplier > 1) {
        WindowApp::HandleTunnelCommand(-0x53245bd8, multiplier, 0, 0);
    }
    // CK queries the trick-shot multiplier here and uses the mission multiplier for the actual award.
    static_cast<App*>(WindowApp::m_instance)->m_trickShotManager->GetMultiplier();
    CDH_Weapon* weapon = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetActiveWeapon();
    int total = experience * multiplier + weapon->m_bonusXp;
    int previousLevel = CBH_Player::GetInstance()->GetLevel();
    CBH_Player::GetInstance()->AddXp(total, GAIN_SOURCE_ENEMY);
    if (CBH_Player::GetInstance()->GetLevel() - previousLevel > 0) {
        WindowApp::HandleTunnelCommand(0x6aa69fd2, 0, 0, 0);
    }
    static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->OnReceivedXP(total);
}
