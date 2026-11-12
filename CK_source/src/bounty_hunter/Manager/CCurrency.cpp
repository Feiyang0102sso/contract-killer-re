// CCurrency.cpp - cash pack item: parsing and the purchase reward.
#include "CCurrency.h"
#include "CBH_Player.h"
#include "CGameAnalytics.h"
#include "CXmlHelper.h"
#include "WindowBase.h"

CCurrency::CCurrency()
    : CItem(0)
{
}

void CCurrency::ParseStats(const TiXmlElement& element)
{
    CItem::ParseStats(element);
    m_cashGain = CXmlHelper::GetAttributeValue(element, "cash_gain").ToInt();
}

void CCurrency::ParseTexts(const TiXmlElement& element)
{
    setName(Window::ResString(XString::AnsiString(CXmlHelper::GetAttributeValueOptional(element, "name").c_str())));
    setDescription(Window::ResString(XString::AnsiString(CXmlHelper::GetAttributeValueOptional(element, "desc").c_str())));
}

XString CCurrency::getUIDString() const
{
    return XString::Format(L"currency_%d", m_id);
}

void CCurrency::setAvailable(bool available)
{
    // buying the pack: the cash is added immediately
    CBH_Player::GetInstance()->AddMoney(m_cashGain, GAIN_SOURCE_IAP);
    CGameAnalytics::logCurrencyChange(XString::Format(L"%02d_currency", m_id), m_cashGain, CGameAnalytics::CURRENCY_SOFT);
}
