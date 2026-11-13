#include "CMedicant.h"

#include "CBH_Player.h"
#include "CXmlHelper.h"
#include "App.h"

CMedicant::CMedicant()
    : CItem(0), m_available(false)
{
    // the original leaves m_medkits for ParseStats()
}

void CMedicant::setAvailable(bool)
{
    CBH_Player::GetInstance()->AddMedkits(m_medkits);
}

void CMedicant::ParseStats(const TiXmlElement& stats)
{
    CItem::ParseStats(stats);
    m_medkits = CXmlHelper::GetAttributeValue(stats, "medkits_gain").ToInt();
}

XString CMedicant::getUIDString() const
{
    return XString::Format(L"medicant_%d", m_id);
}

// Shop stats: credits price line and cash price line, each followed by a newline.
XString CMedicant::GetStatsString() const
{
    XString result("");
    if (getPriceCredits() > 0) {
        result.Append(XString::Format(Window::ResString("IDS_SHOP_AMMO_STATS_PRICE_CREDITS").c_str(), getPriceCredits()) + L"\n");
    }
    if (m_pricePoints > 0) {
        result.Append(XString::Format(Window::ResString("IDS_SHOP_AMMO_STATS_PRICE_CASH").c_str(), m_pricePoints) + L"\n");
    }
    return result;
}

// CMedicantManager allocates 0x64 bytes per item in the original
typedef char AssertCMedicantSize[sizeof(CMedicant) == 0x64 ? 1 : -1];
