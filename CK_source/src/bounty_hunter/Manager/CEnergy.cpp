#include "CEnergy.h"

#include "CBH_Player.h"
#include "CXmlHelper.h"

CEnergy::CEnergy()
    : CItem(0)
{
    // the original leaves m_energy for ParseStats()
}

void CEnergy::setAvailable(bool available)
{
    // the original only acts on true and caps the energy at the maximum
    if (available) {
        CBH_Player::GetInstance()->AddEnergy(m_energy, false);
        CBH_Player::GetInstance()->m_gameStats.AddEnergyBought(m_energy);
    }
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
