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
    m_pills = CXmlHelper::GetAttributeValueOptional(stats, "pills_gain").ToInt();
    m_seconds = CXmlHelper::GetAttributeValueOptional(stats, "seconds_gain").ToInt();
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
    XString result;
    if (m_pills > 0) {
        result.Append(XString::Format(Window::ResString("IDS_SHOP_PILLS_STATS").c_str(), m_pills));
        result.Append(XString(L"\n"));
    }
    if (m_seconds > 0) {
        result.Append(XString::Format(Window::ResString("IDS_SHOP_REACTIX_CONTROL_STATS").c_str(), m_seconds));
        result.Append(XString(L"\n"));
    }
    result.Append(CItem::GetStatsString());
    return result;
}

typedef char AssertCGoPillSize[sizeof(CGoPill) == 0x64 ? 1 : -1];
