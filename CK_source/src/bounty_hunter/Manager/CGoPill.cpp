#include "CGoPill.h"

#include "CBH_Player.h"
#include "CXmlHelper.h"
#include "App.h"
#include "ResMap.h"

CGoPill::CGoPill()
    : CItem(0), m_pills(0), m_seconds(0)
{
}

void CGoPill::setAvailable(bool)
{
    if (m_pills > 0) {
        CBH_Player::GetInstance()->AddGoPills(m_pills);
    }
    if (m_seconds != 0) {
        CBH_Player::GetInstance()->AddReactixControlTimeSec(m_seconds);
    }
}

void CGoPill::ParseStats(const TiXmlElement& stats)
{
    CItem::ParseStats(stats);
    m_pills = CXmlHelper::GetAttributeValueOrUseDefault(stats, "pills_gain", XString("0")).ToInt();
    m_seconds = CXmlHelper::GetAttributeValueOrUseDefault(stats, "seconds_gain", XString("0")).ToInt();
}

XString CGoPill::getUIDString() const
{
    return XString::Format(L"go_pill_%d", m_id);
}

int CGoPill::getType()
{
    return 10;
}

void CGoPill::ParseGraphics(const TiXmlElement& graphics)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    ResMap* resources = app->InitResMap();
    XString::AnsiString button(CXmlHelper::GetAttributeValue(graphics, "shop_button").c_str());
    setShopImageResId(resources->getResourceID(resources->getFilename(button)));
    XString::AnsiString image(CXmlHelper::GetAttributeValue(graphics, "big_image").c_str());
    setBigImageResId(resources->getResourceID(resources->getFilename(image)));
}

XString CGoPill::GetStatsString() const
{
    // Original: pills show the plain pill text, otherwise the Reactix minutes; then the prices.
    XString result;
    if (m_pills > 0) {
        result = Window::ResString("IDS_SHOP_PILLS_STATS") + L"\n";
    } else {
        result = XString::Format(Window::ResString("IDS_SHOP_REACTIX_CONTROL_STATS").c_str(), m_seconds / 60) + L"\n";
    }
    if (getPriceCredits() > 0) {
        result.Append(XString::Format(Window::ResString("IDS_SHOP_AMMO_STATS_PRICE_CREDITS").c_str(), getPriceCredits()) + L"\n");
    }
    if (m_pricePoints > 0) {
        result.Append(XString::Format(Window::ResString("IDS_SHOP_AMMO_STATS_PRICE_CASH").c_str(), m_pricePoints) + L"\n");
    }
    return result;
}

typedef char AssertCGoPillSize[sizeof(CGoPill) == 0x64 ? 1 : -1];
