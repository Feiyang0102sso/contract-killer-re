#include "CSquadType.h"

#include "App.h"
#include "CBH_PersonType.h"
#include "CBH_PersonTypeManager.h"
#include "CEnemyWeaponType.h"
#include "CRandGen.h"
#include "CXmlHelper.h"

CSquadType::CSquadType(TiXmlNode* node) : m_damage(0.0f), m_field18(1.0f)
{
    Parse(node);
}

CSquadType::~CSquadType()
{
}

void CSquadType::Parse(TiXmlNode* node)
{
    m_name = CXmlHelper::GetAttributeValue(node, "name");
    ParseMinion(node);
    CalcDPM();
}

void CSquadType::ParseMinion(TiXmlNode* node)
{
    for (TiXmlNode* minion = node->FirstChild("minion"); minion != NULL; minion = minion->NextSibling("minion")) {
        m_minions.addElement(CXmlHelper::GetAttributeValue(minion, "name"));
    }
}

void CSquadType::CalcDPM()
{
    m_damage = 0.0f;
    App* app = static_cast<App*>(WindowApp::m_instance);
    for (int index = 0; index < m_minions.size(); ++index) {
        CBH_PersonType* type = app->InitPersonTypeManager()->GetType(m_minions[index]);
        if (type != NULL && type->m_weapon != NULL) {
            m_damage += type->m_weapon->m_baseDamage;
        }
    }
}

const XString& CSquadType::GetMiniomName() const
{
    return m_minions[CRandGen::GetInstance()->GetRandRange(0, m_minions.size() - 1)];
}

const XString& CSquadType::GetMiniomNameByIndex(int index) const
{
    return m_minions[index];
}

typedef char AssertSquadTypeSize[sizeof(CSquadType) == 0x1c ? 1 : -1];
