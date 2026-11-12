#include "CSilencer.h"

#include "CBH_Player.h"
#include "CXmlHelper.h"
#include "App.h"

CSilencer::CSilencer(const TiXmlElement& config)
    : CItem(0), m_shots(0)
{
    init(config);
}

void CSilencer::setAvailable(bool)
{
    CBH_Player::GetInstance()->AddSilentShotsCount(m_shots);
}

void CSilencer::ParseStats(const TiXmlElement& stats)
{
    CItem::ParseStats(stats);
    m_shots = CXmlHelper::GetAttributeValue(stats, "silent_shots_count").ToInt();
}

XString CSilencer::getUIDString() const
{
    return XString::Format(L"misc_%d", m_id);
}

int CSilencer::getCurrentAmount()
{
    return CBH_Player::GetInstance()->m_silentShotsCount;
}

// Shop stats: credits price line and cash price line, each followed by a newline.
XString CSilencer::GetStatsString() const
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

typedef char AssertCSilencerSize[sizeof(CSilencer) == 0x60 ? 1 : -1];
