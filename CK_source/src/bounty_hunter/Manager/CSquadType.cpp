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
    // the original checks neither the person type nor its weapon
    for (int index = 0; index < m_minions.size(); ++index) {
        m_damage += static_cast<App*>(WindowApp::m_instance)->InitPersonTypeManager()->GetType(m_minions[index])->m_weapon->m_baseDamage;
    }
}

const XString& CSquadType::GetMiniomName() const
{
    int last = m_minions.size() - 1;
    return m_minions[CRandGen::GetInstance()->GetRandRange(0, last)];
}

const XString& CSquadType::GetMiniomNameByIndex(int index) const
{
    return m_minions[index];
}

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AssertSquadTypeSize[sizeof(CSquadType) == 0x1c ? 1 : -1];
#endif
