// CShopItem.cpp - item data, resource ownership and manager iteration.
#include "CShopItem.h"
#include "App.h"
#include "CBH_Player.h"
#include "CBH_SurfaceManager.h"
#include "CDailyDealManager.h"
#include "COverrideManager.h"
#include "CXmlHelper.h"
#include "ResMap.h"
#include <string.h>

CItem::CItem(int id)
    : m_overrideMark(0), m_id(id)
{
    memset(&m_priceCredits, 0, 0x36);
    m_canEquipInSafehouse = false;
}

CItem::~CItem()
{
    releaseResources();
}

void CItem::init(const TiXmlElement& element)
{
    Parse(element);
}

void CItem::Parse(const TiXmlElement& element)
{
    const char* id = CXmlHelper::GetCharAttributeValueOptional(element, "id");
    if (id != NULL) {
        m_id = XString(id).ToInt();
    }
    m_canEquipInSafehouse = CXmlHelper::GetAttributeValueOrUseDefault(
        element, "can_be_equipped_in_safehouse", XString("0")).ToBool();
    const TiXmlElement* texts = element.FirstChildElement("texts");
    if (texts == NULL) {
        texts = &element;
    }
    ParseTexts(*texts);
    const TiXmlElement* stats = element.FirstChildElement("stats");
    if (stats == NULL) {
        stats = &element;
    }
    ParseStats(*stats);
    const TiXmlElement* graphics = element.FirstChildElement("graphics");
    if (graphics != NULL) {
        ParseGraphics(*graphics);
    }
}

void CItem::ParseTexts(const TiXmlElement& element)
{
    const char* name = CXmlHelper::GetCharAttributeValueOptional(element, "name");
    if (name != NULL) {
        setName(Window::ResString(name));
    }
    XString description = CXmlHelper::GetAttributeValueOrUseDefault(element, "desc", XString(""));
    XString::AnsiString descriptionId(description.c_str());
    setDescription(Window::ResString(descriptionId));
}

void CItem::ParseStats(const TiXmlElement& element)
{
    m_hidden = CXmlHelper::GetAttributeValueOrUseDefault(element, "hidden", XString("0")).ToBool();
    const char* value = CXmlHelper::GetCharAttributeValueOptional(element, "price_credits");
    if (value != NULL) {
        setPriceCredits(XString(value).ToInt());
    }
    value = CXmlHelper::GetCharAttributeValueOptional(element, "price_points");
    if (value != NULL) {
        setPricePoints(XString(value).ToInt());
    }
    value = CXmlHelper::GetCharAttributeValueOptional(element, "unlock_level");
    if (value != NULL) {
        setUnlockLvl(XString(value).ToInt());
    }
    value = CXmlHelper::GetCharAttributeValueOptional(element, "price_iap");
    if (value != NULL) {
        setPriceIAP(XString(value).ToInt());
    }

    XString extra = CXmlHelper::GetAttributeValueOrUseDefault(element, "extra_text", XString(""));
    if (extra.ByteSize() >= sizeof(wchar_t)) {
        XString::AnsiString extraId(extra.c_str());
        setExtraText(Window::ResString(extraId));
    }
}

void CItem::ParseGraphics(const TiXmlElement& element)
{
    const char* name = CXmlHelper::GetCharAttributeValueOptional(element, "shop_button");
    if (name != NULL) {
        ResMap* map = static_cast<App*>(WindowApp::m_instance)->InitResMap();
        setShopImageResId(map->getResourceID(XString(name)));
    }
    name = CXmlHelper::GetCharAttributeValueOptional(element, "big_image");
    if (name != NULL) {
        ResMap* map = static_cast<App*>(WindowApp::m_instance)->InitResMap();
        setBigImageResId(map->getResourceID(XString(name)));
    }
}

ICRenderSurface* CItem::getBigImage()
{
    if (m_bigImage == NULL && m_bigImageId != NULL) {
        CBH_SurfaceManager* manager = static_cast<App*>(WindowApp::m_instance)->m_surfaceManager;
        m_bigImage = manager->CreateSurface(m_bigImageId, true);
    }
    return m_bigImage;
}

void CItem::loadResources()
{
    if (m_shopImageId != NULL && m_shopImage == NULL) {
        CBH_SurfaceManager* manager = static_cast<App*>(WindowApp::m_instance)->m_surfaceManager;
        m_shopImage = manager->CreateSurface(m_shopImageId, true);
    }
}

void CItem::releaseResources()
{
    ReleaseBigShopImage();
    if (m_shopImageId != NULL && m_shopImage != NULL) {
        CBH_SurfaceManager* manager = static_cast<App*>(WindowApp::m_instance)->m_surfaceManager;
        manager->ReleaseSurface(m_shopImageId);
        m_shopImage = NULL;
    }
}

void CItem::ReleaseBigShopImage()
{
    if (m_bigImageId != NULL && m_bigImage != NULL) {
        CBH_SurfaceManager* manager = static_cast<App*>(WindowApp::m_instance)->m_surfaceManager;
        manager->ReleaseSurface(m_bigImageId);
        m_bigImage = NULL;
    }
}

bool CItem::checkOverride()
{
    COverrideManager* manager = static_cast<App*>(WindowApp::m_instance)->m_overrideManager;
    if (manager != NULL && manager->changesMark() != m_overrideMark) {
        m_overrideMark = manager->changesMark();
        return doOverride(*manager, getUIDString());
    }
    return false;
}

bool CItem::doOverride(COverrideManager& manager, const XString& id)
{
    CStrWChar path(id.c_str());
    unsigned long long value;
    bool changed = false;

    m_priceCredits = m_originalPriceCredits;
    if (manager.FindIntAttribute(path, CStrWChar(L"price_credits"), &value)) {
        m_priceCredits = value;
        changed = true;
    }
    m_pricePoints = m_originalPricePoints;
    if (manager.FindIntAttribute(path, CStrWChar(L"price_points"), &value)) {
        m_pricePoints = value;
        changed = true;
    }
    m_priceIAP = m_originalPriceIAP;
    if (manager.FindIntAttribute(path, CStrWChar(L"price_iap"), &value)) {
        m_priceIAP = value;
        changed = true;
    }
    m_unlockLevel = m_originalUnlockLevel;
    if (manager.FindIntAttribute(path, CStrWChar(L"unlock_level"), &value)) {
        m_unlockLevel = value;
        changed = true;
    }

    bool hasNew = manager.FindIntAttribute(path, CStrWChar(L"new_marked"), &value);
    m_new = hasNew && value != 0;
    bool hasSale = manager.FindIntAttribute(path, CStrWChar(L"sale_marked"), &value);
    m_sale = hasSale && value != 0;
    bool hasLimited = manager.FindIntAttribute(path, CStrWChar(L"limited_marked"), &value);
    m_limited = hasLimited && value != 0;
    bool hasFeatured = manager.FindIntAttribute(path, CStrWChar(L"featured_marked"), &value);
    m_featured = hasFeatured && value != 0;

    CStrWChar text;
    m_name = m_originalName;
    if (manager.FindStringAttribute(path, CStrWChar(L"name"), &text)) {
        m_name = text.GetWCharPtr();
        changed = true;
    }
    m_description = m_originalDescription;
    if (manager.FindStringAttribute(path, CStrWChar(L"desc"), &text)) {
        m_description = text.GetWCharPtr();
        changed = true;
    }
    m_extraText = m_originalExtraText;
    if (manager.FindStringAttribute(path, CStrWChar(L"extra_text"), &text)) {
        m_extraText = text.GetWCharPtr();
        changed = true;
    }
    return changed || hasNew || hasSale || hasLimited || hasFeatured;
}

XString CItem::GetStatsString() const
{
    XString result("");
    if (getPriceCredits() > 0) {
        XString label = Window::ResString("IDS_SHOP_AMMO_STATS_PRICE_CREDITS");
        result.Append(XString::Format(label.c_str(), getPriceCredits()));
        result.Append(XString(L"\n"));
    }
    if (m_pricePoints > 0) {
        XString label = Window::ResString("IDS_SHOP_AMMO_STATS_PRICE_CASH");
        result.Append(XString::Format(label.c_str(), m_pricePoints));
        result.Append(XString(L"\n"));
    }
    return result;
}

int CItem::getPriceCredits() const
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (app->InitDailyDealManager()->GetCurrentDealItem() == this) {
        SDailyDeal* deal = app->InitDailyDealManager()->GetCurrentDailyDeal();
        if (deal != NULL) {
            return deal->m_priceCredits;
        }
    }
    return m_priceCredits;
}

bool CItem::TryPurchase()
{
    CBH_Player* player = CBH_Player::GetInstance();
    int credits = getPriceCredits();
    int cash = m_pricePoints;
    if (credits > 0) {
        cash = 0;
    } else {
        credits = 0;
    }
    if (player->m_credits < credits || player->m_cash < cash || cash < 0
        || player->GetLevel() < m_unlockLevel) {
        return false;
    }
    if (credits > 0) {
        player->ReduceCredits(credits);
    } else if (cash > 0) {
        player->ReduceMoney(cash);
    }
    setAvailable(true);
    return true;
}

CItem* CItemManager::getItemByName(XString name)
{
    for (int index = 0; ; ++index) {
        CItem* item = getItemByInternalId(index);
        if (item == NULL) {
            return NULL;
        }
        if (item->m_name == name) {
            item->checkOverride();
            return item;
        }
    }
}

CItem* CItemManager::firstItem()
{
    m_iterator = 0;
    CItem* item = getItemByInternalId(0);
    if (item != NULL) {
        item->checkOverride();
    }
    return item;
}

CItem* CItemManager::nextItem()
{
    CItem* item = getItemByInternalId(++m_iterator);
    if (item != NULL) {
        item->checkOverride();
    }
    return item;
}

CItem* CItemManager::getItemById(int id)
{
    for (int index = 0; ; ++index) {
        CItem* item = getItemByInternalId(index);
        if (item == NULL) {
            return NULL;
        }
        if (item->m_id == id) {
            item->checkOverride();
            return item;
        }
    }
}

CItem* CItemManager::getItemByStringUId(const XString& id)
{
    for (int index = 0; ; ++index) {
        CItem* item = getItemByInternalId(index);
        if (item == NULL) {
            return NULL;
        }
        if (item->getUIDString() == id) {
            item->checkOverride();
            return item;
        }
    }
}
