// CIAPCurrency.cpp - parse and award a configured currency pack.
#include "CIAPCurrency.h"
#include "CBH_Player.h"
#include "CGameAnalytics.h"
#include "CXmlHelper.h"

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char CIAPCurrencySizeCheck[sizeof(CIAPCurrency) == 0x6c ? 1 : -1];
#endif

CIAPCurrency::CIAPCurrency()
    : CItem(0)
{
    // set after the string members, as in the original
    m_available = false;
}

void CIAPCurrency::init(const TiXmlElement& element)
{
    CItem::init(element);
    setAvailable(m_type == L"tapjoy");
}

void CIAPCurrency::Parse(const TiXmlElement& element)
{
    CItem::Parse(element);
    m_type = CXmlHelper::GetAttributeValue(element, "type");
}

void CIAPCurrency::ParseTexts(const TiXmlElement& element)
{
    CItem::ParseTexts(element);
    m_appleId = CXmlHelper::GetAttributeValue(element, "appleid");
}

void CIAPCurrency::ParseStats(const TiXmlElement& element)
{
    CItem::ParseStats(element);
    m_creditsGain = CXmlHelper::GetAttributeValue(element, "credits_gain").ToInt();
    m_cashGain = CXmlHelper::GetAttributeValue(element, "cash_gain").ToInt();
}

XString CIAPCurrency::getUIDString() const
{
    return XString::Format(L"iap_currency_%d", m_id);
}

void CIAPCurrency::setAvailable(bool available)
{
    m_available = available;
}

void CIAPCurrency::Buy()
{
    if (m_type == L"tapjoy") {
        return;
    }
    XString source = XString::Format(L"%02d_iap", m_id);
    if (m_creditsGain > 0) {
        CBH_Player::GetInstance()->AddCredits(m_creditsGain);
        CGameAnalytics::logCurrencyChange(source, m_creditsGain, CGameAnalytics::CURRENCY_HARD);
    }
    if (m_cashGain > 0) {
        CBH_Player::GetInstance()->AddMoney(m_cashGain, GAIN_SOURCE_IAP);
        CGameAnalytics::logCurrencyChange(source, m_cashGain, CGameAnalytics::CURRENCY_SOFT);
    }
}
