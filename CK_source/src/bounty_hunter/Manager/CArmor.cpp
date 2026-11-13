#include "CArmor.h"

#include "CBH_Player.h"
#include "CXmlHelper.h"
#include "App.h"
#include "ResMap.h"

CArmor::CArmor()
    : CItem(0), m_damageProof(0)
{
}

void CArmor::setAvailable(bool)
{
    CBH_Player::GetInstance()->SetArmor(m_id, m_damageProof);
}

void CArmor::ParseStats(const TiXmlElement& stats)
{
    CItem::ParseStats(stats);
    m_damageProof = CXmlHelper::GetAttributeValue(stats, "damage_proof").ToInt();
}

XString CArmor::getUIDString() const
{
    return XString::Format(L"armor_%d", m_id);
}

int CArmor::getType()
{
    return 11;
}

void CArmor::ParseGraphics(const TiXmlElement& graphics)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    ResMap* resources = app->InitResMap();
    XString::AnsiString button(CXmlHelper::GetAttributeValue(graphics, "shop_button").c_str());
    setShopImageResId(resources->getResourceID(resources->getFilename(button)));
    XString::AnsiString image(CXmlHelper::GetAttributeValue(graphics, "big_image").c_str());
    setBigImageResId(resources->getResourceID(resources->getFilename(image)));
}

XString CArmor::GetStatsString() const
{
    XString result;
    if (m_damageProof > 0) {
        result.Append(XString::Format(Window::ResString("IDS_SHOP_ARMOR_STATS").c_str(), m_damageProof));
        result.Append(XString(L"\n"));
    }
    result.Append(CItem::GetStatsString());
    return result;
}

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AssertCArmorSize[sizeof(CArmor) == 0x60 ? 1 : -1];
#endif
