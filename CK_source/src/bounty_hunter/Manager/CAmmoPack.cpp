#include "CAmmoPack.h"

#include "App.h"
#include "CAmmoManager.h"
#include "CBH_Player.h"
#include "CXmlHelper.h"
#include "tinyxml.h"

CAmmoPack::CAmmoPack() : CItem(0)
{
    // the original leaves m_countInPack for Parse()
}

void CAmmoPack::Parse(const TiXmlElement& element)
{
    setId(CXmlHelper::GetAttributeValue(element, "id").ToInt());
    const TiXmlElement* info = element.FirstChildElement("info");
    if (info != NULL) {
        ParseTexts(*info);
        ParseStats(*info);
        ParseGraphics(*info);
        InitStatsStr();
    }
}

void CAmmoPack::InitStatsStr()
{
    m_stats = XString::Format(Window::ResString("IDS_SHOP_AMMO_STATS_COUNT").c_str(), m_countInPack) + L"\n";
    if (getPriceCredits() > 0) {
        m_stats.Append(XString::Format(Window::ResString("IDS_SHOP_AMMO_STATS_PRICE_CREDITS").c_str(),
            getPriceCredits()) + L"\n");
    }
    if (m_pricePoints > 0) {
        m_stats.Append(XString::Format(Window::ResString("IDS_SHOP_AMMO_STATS_PRICE_CASH").c_str(),
            m_pricePoints) + L"\n");
    }
}

void CAmmoPack::ParseTexts(const TiXmlElement& element)
{
    CItem::ParseTexts(element);
    m_ammoType = CXmlHelper::GetAttributeValue(element, "ammo_type");
}

void CAmmoPack::ParseStats(const TiXmlElement& element)
{
    CItem::ParseStats(element);
    m_countInPack = CXmlHelper::GetAttributeValue(element, "count_in_pack").ToInt();
}

XString CAmmoPack::getUIDString() const
{
    return XString::Format(L"ammo_pack_%d", m_id);
}

void CAmmoPack::setAvailable(bool)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    // the original does not check the ammo type
    CAmmo* ammo = app->InitAmmoManager()->GetAmmoByType(m_ammoType);
    CBH_Player::GetInstance()->IncAmmoByBulletsCount(ammo, m_countInPack);
}

XString CAmmoPack::GetStatsString() const
{
    return m_stats;
}

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AssertAmmoPackSize[sizeof(CAmmoPack) == 0x68 ? 1 : -1];
#endif
