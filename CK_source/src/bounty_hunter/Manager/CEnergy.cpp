#include "CEnergy.h"

#include "CBH_Player.h"
#include "CXmlHelper.h"

CEnergy::CEnergy()
    : CItem(0), m_energy(0)
{
}

void CEnergy::setAvailable(bool)
{
    CBH_Player* player = CBH_Player::GetInstance();
    player->AddEnergy(m_energy, true);
    player->m_gameStats.AddEnergyBought(m_energy);
}

void CEnergy::ParseStats(const TiXmlElement& stats)
{
    CItem::ParseStats(stats);
    m_energy = CXmlHelper::GetAttributeValue(stats, "energy_restore").ToInt();
}

XString CEnergy::getUIDString() const
{
    return XString::Format(L"energy_%d", m_id);
}

typedef char AssertCEnergySize[sizeof(CEnergy) == 0x60 ? 1 : -1];
