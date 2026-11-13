#include "CRevive.h"

#include "CBH_Player.h"
#include "CXmlHelper.h"
#include "App.h"

CRevive::CRevive(const TiXmlElement& config)
    : CItem(0), m_count(0)
{
    CItem::Parse(config);
}

void CRevive::setAvailable(bool available)
{
    // the original only acts on true
    if (available) {
        CBH_Player::GetInstance()->AddRevive(m_count);
    }
}

void CRevive::ParseStats(const TiXmlElement& stats)
{
    CItem::ParseStats(stats);
    m_count = CXmlHelper::GetAttributeValue(stats, "count").ToInt();
}

XString CRevive::getUIDString() const
{
    return XString::Format(L"misc_%d", m_id);
}

int CRevive::getCurrentAmount()
{
    return CBH_Player::GetInstance()->m_reviveCount;
}

// Shop stats: credits price line and cash price line, each followed by a newline.
XString CRevive::GetStatsString() const
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

typedef char AssertCReviveSize[sizeof(CRevive) == 0x60 ? 1 : -1];
