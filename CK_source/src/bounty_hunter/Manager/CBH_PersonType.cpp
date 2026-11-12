// CBH_PersonType.cpp - one kind of person of persons.xml.
#include "CBH_PersonType.h"
#include "CXmlHelper.h"
#include "App.h"
#include "CVisualCharacterTypeManager.h"
#include "CNationalityTypeManager.h"
#include "CEnemyWeaponTypeManager.h"

CBH_PersonType::CBH_PersonType(TiXmlNode* node)
    : m_visualCharacter()
    , m_nationality()
    , m_weapon()
    , m_levelRangeMin()
    , m_levelRangeMax()
    , m_baseXp()
    , m_baseHp()
    , m_baseAccuracy()
{
    Parse(node);
}

void CBH_PersonType::Parse(TiXmlNode* node)
{
    m_name = CXmlHelper::GetAttributeValue(node, "name");
    ParseInfo(node->FirstChild("info"));
}

bool CBH_PersonType::IsSuitableForPlayerLevel(int level) const
{
    return level >= m_levelRangeMin && level <= m_levelRangeMax;
}

void CBH_PersonType::ParseInfo(TiXmlNode* node)
{
    if (node == NULL) {
        return;
    }
    XString visualCharacter = CXmlHelper::GetAttributeValue(node, "visualCharacter");
    m_visualCharacter = ((App*)WindowApp::m_instance)->InitVisualCharacterTypeManager()->GetType(visualCharacter);
    XString nationality = CXmlHelper::GetAttributeValue(node, "nationality");
    m_nationality = ((App*)WindowApp::m_instance)->InitNationalityTypeManager()->GetType(nationality);
    XString weapon = CXmlHelper::GetAttributeValue(node, "weapon");
    if (weapon.Length() != 0) {
        m_weapon = ((App*)WindowApp::m_instance)->InitEnemyWeaponTypeManager()->GetType(weapon);
    }
    m_levelRangeMin = CXmlHelper::GetAttributeValue(node, "level_range_min").ToInt();
    m_levelRangeMax = CXmlHelper::GetAttributeValue(node, "level_range_max").ToInt();
    m_baseXp = CXmlHelper::GetAttributeValue(node, "base_xp").ToInt();
    m_baseHp = CXmlHelper::GetAttributeValue(node, "base_hp").ToInt();
    m_baseAccuracy = CXmlHelper::GetAttributeValue(node, "base_accuracy").ToFloat();
    m_spawnCivilians = CXmlHelper::GetAttributeValue(node, "spawn_civilians").ToBool();
}
