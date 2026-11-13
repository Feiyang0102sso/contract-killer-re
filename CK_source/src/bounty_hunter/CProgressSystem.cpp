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
    if (text != NULL) {
        document.Parse(text, NULL, TIXML_ENCODING_UNKNOWN);
        free(text);
        ParseLevels(document.FirstChild("progress"));
    }
    m_field10 = 0.5f;
}

void CProgressSystem::ParseLevels(TiXmlNode* root)
{
    if (root == NULL) {
        return;
    }
    const int count = CXmlHelper::GetAttributeValue(root, "level_count").ToInt();
    m_levels.ensureCapacity(count);
    const SLevelInfo defaults = { 0, 0, 0, 0.0f };
    for (int index = 0; index < count; ++index) {
        m_levels.addElement(defaults);
    }
    for (TiXmlNode* node = root->FirstChild("level"); node != NULL; node = node->NextSibling("level")) {
        const int index = CXmlHelper::GetAttributeValue(node, "num").ToInt();
        SLevelInfo level;
        level.m_xpThreshold = CXmlHelper::GetAttributeValue(node, "xp").ToInt();
        level.m_maxEnergy = CXmlHelper::GetAttributeValue(node, "energy").ToInt();
        level.m_civilianCount = CXmlHelper::GetAttributeValue(node, "civilianCount").ToInt();
        level.m_damageCoefficient = CXmlHelper::GetAttributeValue(node, "dmgCoeff").ToFloat();
        if (index >= 0 && index < count) {
            m_levels[index] = level;
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
    if (m_levels.size() > 0) {
        return GetMaxXPForLevel(GetMaxLevel());
    }
    return 0;
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
    App* app = static_cast<App*>(WindowApp::m_instance);
    const char* token = "LEVEL_COEF_01";
    if (allowAlternate && CBH_Player::GetInstance()->GetLevel() >= 6) {
        token = "LEVEL_COEF_02";
    }
    return app->m_helper->m_config.GetFloatValueByToken(token, true);
}

float CProgressSystem::GetDmgCoeff() const
{
    // the original does not range-check the level
    return m_levels[CBH_Player::GetInstance()->GetLevel()].m_damageCoefficient;
}

float CProgressSystem::GetLevelForkFormulaCoef(bool allowAlternate) const
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    float coefficient = GetLevelCoef(allowAlternate) * 2.0f;
    int range = app->m_helper->m_config.GetIntegerValueByToken("RND_RANGE", true);
    int level = CBH_Player::GetInstance()->GetLevel();
    int minimum = level - range;
    if (minimum < 1) {
        minimum = 1;
    }
    int maximum = level + range;
    if (maximum > GetMaxLevel()) {
        maximum = GetMaxLevel();
    }
    float lower = 1.0f + coefficient * (minimum - 1);
    float upper = 1.0f + coefficient * (maximum - 1);
    return lower + GetRandValue() * (upper - lower);
}

int CProgressSystem::CalcXpForKilledUnit(CUnit* unit)
{
    if (unit == NULL || unit->m_body.m_type == NULL) {
        return 0;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    return static_cast<int>(unit->m_body.m_type->m_baseXp * app->InitProgressSystem()->GetLevelForkFormulaCoef(false));
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
    App* app = static_cast<App*>(WindowApp::m_instance);
    CSwerveGame* game = app->m_helper->m_swerveGame;
    int multiplier = 1;
    if (game != NULL) {
        multiplier = game->m_multiplier;
    }
    if (headshot) {
        multiplier *= 2;
    }
    if (multiplier > 1) {
        WindowApp::HandleTunnelCommand(-0x53245bd8, multiplier, 0, 0);
    }
    // CK queries the trick-shot multiplier here and uses the mission multiplier for the actual award.
    app->m_trickShotManager->GetMultiplier();
    CDH_Weapon* weapon = app->m_helper->GetGameData()->GetActiveWeapon();
    int total = experience * multiplier;
    if (weapon != NULL) {
        total += weapon->m_bonusXp;
    }
    CBH_Player* player = CBH_Player::GetInstance();
    int previousLevel = player->GetLevel();
    player->AddXp(total, GAIN_SOURCE_ENEMY);
    if (player->GetLevel() > previousLevel) {
        WindowApp::HandleTunnelCommand(0x6aa69fd2, 0, 0, 0);
    }
    app->m_huntingInfo->OnReceivedXP(total);
}
