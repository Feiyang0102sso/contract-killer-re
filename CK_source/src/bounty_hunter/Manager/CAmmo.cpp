// CAmmo.cpp - ammo pack data and the award on purchase.
#include "CAmmo.h"

#include "App.h"
#include "CBH_Player.h"
#include "CXmlHelper.h"
#include "ResMap.h"

typedef char CAmmo_ARMv7_Size[(sizeof(CAmmo) == 0x74) ? 1 : -1];

CAmmo::CAmmo()
    : CItem(0)
    , m_universal(false)
{
    // set after the string members, as in the original
    m_owned = false;
}

void CAmmo::init(const TiXmlElement& element)
{
    setUnlockLvl(1);
    Parse(element);
}

void CAmmo::ParseStats(const TiXmlElement& element)
{
    CItem::ParseStats(element);
    m_extraDamage = CXmlHelper::GetAttributeValue(element, "extra_damage").ToInt();
    m_countInPack = CXmlHelper::GetAttributeValue(element, "count_in_pack").ToInt();
    m_universal = CXmlHelper::GetAttributeValue(element, "is_universal").ToBool();
}

void CAmmo::Parse(const TiXmlElement& element)
{
    m_ammoType = CXmlHelper::GetAttributeValue(element, "type");
    CItem::Parse(element);
}

XString CAmmo::getUIDString() const
{
    return XString::Format(L"ammo_%d", m_id);
}

void CAmmo::setAvailable(bool available)
{
    CBH_Player::GetInstance()->IncAmmo(this, 1);
    m_owned = available;
}

void CAmmo::ParseGraphics(const TiXmlElement& element)
{
    CItem::ParseGraphics(element);
    App* app = static_cast<App*>(WindowApp::m_instance);
    XString name = CXmlHelper::GetAttributeValue(element, "equip_button");
    m_equipButton = XString(app->InitResMap()->getResourceID(name));
    name = CXmlHelper::GetAttributeValue(element, "bullet_icon");
    m_bulletIcon = XString(app->InitResMap()->getResourceID(name));
}

XString CAmmo::GetStatsString() const
{
    XString result = XString::Format(Window::ResString("IDS_SHOP_AMMO_STATS_COUNT").c_str(), m_countInPack) + L"\n";
    if (getPriceCredits() > 0) {
        result.Append(XString::Format(Window::ResString("IDS_SHOP_AMMO_STATS_PRICE_CREDITS").c_str(), getPriceCredits()) + L"\n");
    }
    if (m_pricePoints > 0) {
        result.Append(XString::Format(Window::ResString("IDS_SHOP_AMMO_STATS_PRICE_CASH").c_str(), m_pricePoints) + L"\n");
    }
    return result;
}
