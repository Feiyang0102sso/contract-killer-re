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

CFactionType::CFactionType(TiXmlNode* node) : m_legal(false), m_smallIcon(NULL)
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
    XString enemies = CXmlHelper::GetAttributeValue(node, "enemies");
    if (friends.Length() != 0) {
        friends.Split(m_friends, L",");
    }
    if (enemies.Length() != 0) {
        enemies.Split(m_enemies, L",");
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
    static const XString empty;
    App* app = static_cast<App*>(WindowApp::m_instance);
    int level = CBH_Player::GetInstance()->GetLevel();
    m_eligibleBosses.removeAllElements();
    for (int index = 0; index < m_bosses.size(); ++index) {
        XString* name = &m_bosses.m_items[index];
        CBH_PersonType* person = app->InitPersonTypeManager()->GetType(*name);
        if (person != NULL && person->IsSuitableForPlayerLevel(level)) {
            m_eligibleBosses.addElement(name);
        }
    }
    int count = m_eligibleBosses.size();
    if (count < 1) {
        return empty;
    }
    CSquadTypeManager* squads = app->InitSquadTypeManager();
    float totalDamage = 0.0f;
    for (int index = 0; index < count; ++index) {
        const CSquadType* squad = squads->GetType(*m_eligibleBosses[index]);
        if (squad != NULL) {
            totalDamage += squad->m_damage;
        }
    }
    int selected = CRandGen::GetInstance()->GetRandRange(0, count - 1);
    const XString& name = *m_eligibleBosses[selected];
    CSquadType* squad = squads->GetType(name);
    if (squad != NULL && totalDamage > 0.0f) {
        squad->m_field18 = squad->m_damage / (totalDamage / count);
    }
    return name;
}

typedef char AssertFactionSize[sizeof(CFactionType) == 0x64 ? 1 : -1];
