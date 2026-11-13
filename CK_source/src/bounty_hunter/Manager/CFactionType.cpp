#include "CFactionType.h"

#include "App.h"
#include "CBH_PersonType.h"
#include "CBH_PersonTypeManager.h"
#include "CBH_Player.h"
#include "CRandGen.h"
#include "CSquadType.h"
#include "CSquadTypeManager.h"
#include "CXmlHelper.h"
#include "ResMap.h"

CFactionType::SReputationChanges::SReputationChanges()
{
    // Five explicit stores (a loop turns into a memset call).
    m_values[0] = 0;
    m_values[1] = 0;
    m_values[2] = 0;
    m_values[3] = 0;
    m_values[4] = 0;
}

CFactionType::CFactionType(TiXmlNode* node)
{
    Parse(node);
}

void CFactionType::Parse(TiXmlNode* node)
{
    m_name = CXmlHelper::GetAttributeValue(node, "name");
    m_screenName = CXmlHelper::GetAttributeValue(node, "screenName");
    ParseInfo(node->FirstChild("info"));
    ParseGraphics(node->FirstChild("graphics"));
    ParseBoss(node);
}

void CFactionType::ParseInfo(TiXmlNode* node)
{
    if (node == NULL) {
        return;
    }
    m_friends.setSize(0);
    m_enemies.setSize(0);
    XString friends = CXmlHelper::GetAttributeValue(node, "friends");
    if (!(XString("") == friends)) {
        m_friends = friends.Split(L",");
    }
    XString enemies = CXmlHelper::GetAttributeValue(node, "enemies");
    if (!(XString("") == enemies)) {
        m_enemies = enemies.Split(L",");
    }
    m_description = CXmlHelper::GetAttributeValue(node, "description");
    m_legal = CXmlHelper::GetAttributeValue(node, "legal").ToInt() == 1;
}

void CFactionType::ParseGraphics(TiXmlNode* node)
{
    // The original reads the attribute first and has no NULL check.
    XString smallIcon = CXmlHelper::GetAttributeValue(node, "small_icon");
    App* app = static_cast<App*>(WindowApp::m_instance);
    m_smallIcon = app->InitResMap()->getResourceID(smallIcon);
}

void CFactionType::ParseBoss(TiXmlNode* node)
{
    for (TiXmlNode* boss = node->FirstChild("boss"); boss != NULL; boss = boss->NextSibling("boss")) {
        m_bosses.addElement(CXmlHelper::GetAttributeValue(boss, "name"));
    }
    if (m_bosses.size() > 0) {
        m_eligibleBosses.ensureCapacity(m_bosses.size());
    }
}

const XString& CFactionType::GetBoss() const
{
    int level = CBH_Player::GetInstance()->GetLevel();
    m_eligibleBosses.removeAllElements();
    for (int index = 0; index < m_bosses.size(); ++index) {
        CBH_PersonTypeManager* people = static_cast<App*>(WindowApp::m_instance)->InitPersonTypeManager();
        CBH_PersonType* person = people->GetType(m_bosses[index]);
        if (person != NULL && person->IsSuitableForPlayerLevel(level)) {
            XString* name = &m_bosses.m_items[index];
            m_eligibleBosses.addElement(name);
        }
    }

    int count = m_eligibleBosses.size();
    float totalDamage = 0.0f;
    int damageCount = count;
    for (int index = 0; index < damageCount; ++index) {
        CSquadTypeManager* squads = static_cast<App*>(WindowApp::m_instance)->InitSquadTypeManager();
        totalDamage += squads->GetType(*m_eligibleBosses[index])->m_damage;
        damageCount = m_eligibleBosses.size();
    }

    CRandGen* random = CRandGen::GetInstance();
    float averageDamage = totalDamage / damageCount;
    int last = count - 1;
    int index = random->GetRandRange(0, last);
    if (index < 0) {
        index = 0;
    } else if (index >= count) {
        index = last;
    }
    // Each boss in the shipped factions table also has a squad entry.
    CSquadTypeManager* squads = static_cast<App*>(WindowApp::m_instance)->InitSquadTypeManager();
    CSquadType* squad = squads->GetType(*m_eligibleBosses[index]);
    squad->m_field18 = squad->m_damage / averageDamage;
    return *m_eligibleBosses[index];
}

typedef char AssertFactionSize[sizeof(CFactionType) == 0x64 ? 1 : -1];
