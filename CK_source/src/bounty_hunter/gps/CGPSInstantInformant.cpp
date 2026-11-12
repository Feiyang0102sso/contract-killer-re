#include "CGPSInstantInformant.h"

#include "App.h"
#include "CBH_GPSMap.h"
#include "CBH_Player.h"
#include "CFont.h"
#include "CFontMgr.h"
#include "CGameAnalytics.h"
#include "CRandGen.h"
#include "CStdUtil_Cocoa.h"
#include "CTutorialManager.h"
#include "CXmlHelper.h"
#include "DGHelper.h"

CGPSInstantInformant::CGPSInstantInformant(TiXmlNode* node)
    : m_lastBountyCompleted(0), m_bought(false), m_listener(NULL), m_cameraId(-1), m_state(Hidden)
{
    ParseNode(node);
    m_timerFont = CFontMgr::GetInstance()->GetFont(8);
    m_nameFont = CFontMgr::GetInstance()->GetFont(15);
    UpdateIcon();
    SetBlinkingImage("IDB_GPS_ROUND_ICON_ANIM");
}

CGPSInstantInformant::~CGPSInstantInformant()
{
}

void CGPSInstantInformant::ParseNode(TiXmlNode* node)
{
    m_id = CXmlHelper::GetAttributeValue(node, "id").ToInt();
    ParseInfo(node->FirstChild("info"));
}

void CGPSInstantInformant::ParseInfo(TiXmlNode* node)
{
    m_type = static_cast<Type>(CXmlHelper::GetAttributeValue(node, "type").ToInt());
    m_icon = CXmlHelper::GetAttributeValue(node, "image");
    m_cameraImage = CXmlHelper::GetAttributeValue(node, "imageCamera");
    m_unlockLevel = CXmlHelper::GetAttributeValue(node, "unlock_lvl").ToInt();
    m_cooldown = CXmlHelper::GetAttributeValue(node, "cooldown_time").ToInt();
    m_resetCost = CXmlHelper::GetAttributeValue(node, "reset_timer_credits_cost").ToInt();
    m_name = CXmlHelper::GetResString(node, "name");
    m_startText = CXmlHelper::GetResString(node, "start_text");
    m_comeLaterText = CXmlHelper::GetResString(node, "come_later_text");
    m_cashCost = CXmlHelper::GetAttributeValue(node, "money_cost").ToInt();
    m_creditsCost = CXmlHelper::GetAttributeValue(node, "credits_cost").ToInt();
    m_xpMultiplier = CXmlHelper::GetAttributeValue(node, "bounty_xp_multiplier").ToFloat();
    m_moneyMultiplier = CXmlHelper::GetAttributeValue(node, "bounty_money_multiplier").ToFloat();
    m_weaponGated = CXmlHelper::GetAttributeValue(node, "is_weapon_gated").ToBool();
    ParseBountuies(node);
    ParseSquad(node);
    int x = CXmlHelper::GetAttributeValue(node, "posX").ToInt();
    int y = CXmlHelper::GetAttributeValue(node, "posY").ToInt();
    SetPos(x, y);
}

void CGPSInstantInformant::ParseBountuies(TiXmlNode* node)
{
    Vector<XString> types;
    CXmlHelper::GetAttributeValue(node, "bounty_types").Split(types, L",");
    for (int index = 0; index < types.size(); ++index) {
        XString value = types[index].Trim(true, true);
        if (value.Length() != 0) {
            m_bountyTypes.addElement(value.ToInt());
        }
    }
}

void CGPSInstantInformant::ParseSquad(TiXmlNode* node)
{
    if (CXmlHelper::HasAttribute(node, "squads")) {
        CXmlHelper::GetAttributeValue(node, "squads").Split(m_squads, L",");
    }
}

void CGPSInstantInformant::SetListener(CGPSInstantInformantListener* listener)
{
    m_listener = listener;
}

void CGPSInstantInformant::SetBlinking(bool enabled)
{
    if (m_blinking != enabled) {
        m_blinking = enabled;
        if (enabled) {
            SetBlinkingImage("IDB_GPS_ROUND_ICON_ANIM_TUTORIAL");
        }
    }
}

void CGPSInstantInformant::Update(long)
{
    UpdateState();
    if (CTutorialManager::IsPlayerInTutorial()) {
        App* app = static_cast<App*>(WindowApp::m_instance);
        SetBlinking(app->m_tutorialManager->IsInstantInformantBlink(m_id));
    }
}

void CGPSInstantInformant::UpdateIcon()
{
    SetImage(XString::AnsiString(m_icon.c_str()));
}

void CGPSInstantInformant::UpdateState()
{
    CBH_Player* player = CBH_Player::GetInstance();
    m_visible = CTutorialManager::IsInstantInformantVisible(m_id)
        && (player->GetLevel() >= m_unlockLevel || m_cameraId >= 0);
    m_state = Hidden;
    if (!m_visible) {
        return;
    }
    if (m_type == Moving) {
        CBH_GPSMap* map = CBH_GPSMap::m_instance;
        m_visible = player->GetLevel() >= m_unlockLevel && map != NULL && map->IsMovingCameraMissionsFinished();
        if (!m_visible) {
            return;
        }
        if (m_cameraId >= 0) {
            m_state = Assigned;
        } else if (CStdUtil_Cocoa::GetTimeSeconds() - m_lastBountyCompleted >= m_cooldown) {
            m_state = Available;
        } else {
            m_state = Searching;
        }
        return;
    }
    if (m_cameraId >= 0) {
        m_state = Assigned;
    } else if (!m_bought) {
        m_state = Locked;
    } else if (CStdUtil_Cocoa::GetTimeSeconds() - m_lastBountyCompleted < m_cooldown) {
        m_state = Searching;
    } else {
        m_state = Available;
    }
}

void CGPSInstantInformant::OnClick()
{
    if (!m_weaponGated && m_type != Targeting && m_listener != NULL && m_state != Hidden) {
        m_listener->OnGPSInstantInformantSelected(this);
    }
}

void CGPSInstantInformant::Paint(ICGraphics2d& graphics, int mapX, int mapY)
{
    if (!m_visible) {
        return;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    int x = GetX();
    int y = GetY();
    app->m_helper->DrawImage(GetImage(), x + mapX, y + mapY, 1.0f, 0, 0, 0.0f, 0);
    if (m_nameFont != NULL) {
        int width = m_nameFont->MeasureTextWidth(m_name.c_str(), -1, -1, NULL);
        CRectangle label;
        label.m_x = x + (GetWidth() - width) / 2;
        label.m_y = y + GetHeight();
        label.m_width = width;
        label.m_height = m_nameFont->GetFontHeight();
        m_nameFont->PaintText(m_name.c_str(), -1, label.m_x + mapX, label.m_y + mapY, -1, -1);
        setSubItem(0, label, mapX, mapY);
        int seconds = GetCooldownTimeLeft();
        if (m_type != Blue && seconds > 0 && m_timerFont != NULL) {
            XString time = DGHelper::ClockStringFromSeconds(seconds);
            label.m_width = m_timerFont->MeasureTextWidth(time.c_str(), -1, -1, NULL);
            label.m_x = x + (GetWidth() - label.m_width) / 2;
            label.m_y += label.m_height;
            label.m_height = m_timerFont->GetFontHeight();
            m_timerFont->PaintText(time.c_str(), -1, label.m_x + mapX, label.m_y + mapY, -1, -1);
            setSubItem(1, label, mapX, mapY);
        }
    }
    CGPSItem::Paint(graphics, mapX, mapY);
}

int CGPSInstantInformant::GetCooldownTimeLeft()
{
    if (m_state != Searching) {
        return 0;
    }
    unsigned int now = CStdUtil_Cocoa::GetTimeSeconds();
    if (now < m_lastBountyCompleted) {
        return 0;
    }
    unsigned int elapsed = now - m_lastBountyCompleted;
    if (elapsed >= m_cooldown) {
        return 0;
    }
    return m_cooldown - elapsed;
}

void CGPSInstantInformant::Save(TiXmlElement* parent)
{
    TiXmlElement* item = parent->FirstChildElement("instant_informant");
    while (item != NULL && CXmlHelper::GetAttributeValue(item, "id").ToInt() != m_id) {
        item = item->NextSiblingElement("instant_informant");
    }
    if (item == NULL) {
        item = new TiXmlElement("instant_informant");
        item->SetAttribute("id", m_id);
        parent->LinkEndChild(item);
    }
    item->SetAttribute("lastBountyCompletedTime", m_lastBountyCompleted);
    item->SetAttribute("bountyCameraId", m_cameraId);
    item->SetAttribute("bought", m_bought);
}

void CGPSInstantInformant::Load(TiXmlElement* parent)
{
    for (TiXmlElement* item = parent->FirstChildElement("instant_informant"); item != NULL;
         item = item->NextSiblingElement("instant_informant")) {
        if (CXmlHelper::GetAttributeValue(item, "id").ToInt() == m_id) {
            m_lastBountyCompleted = CXmlHelper::GetIntAttributeValueOrUseDefault(item, "lastBountyCompletedTime", 0);
            m_cameraId = CXmlHelper::GetIntAttributeValueOrUseDefault(item, "bountyCameraId", -1);
            m_bought = CXmlHelper::GetBoolAttributeValueOrUseDefault(item, "bought", false);
            break;
        }
    }
    UpdateState();
    UpdateIcon();
}

bool CGPSInstantInformant::IsWeaponGated() const
{
    return m_weaponGated;
}

bool CGPSInstantInformant::IsBountyAvailable()
{
    return m_state == Available;
}

bool CGPSInstantInformant::IsSearching()
{
    return m_state == Searching;
}

void CGPSInstantInformant::OnBountyGiven(int cameraId)
{
    m_cameraId = cameraId;
    UpdateState();
}

void CGPSInstantInformant::OnBountyCompleted()
{
    m_lastBountyCompleted = CStdUtil_Cocoa::GetTimeSeconds();
    m_cameraId = -1;
    UpdateState();
}

void CGPSInstantInformant::OnBountyCanceled()
{
    m_cameraId = -1;
    UpdateState();
}

int CGPSInstantInformant::GetRandomBountyType() const
{
    if (m_bountyTypes.size() > 0) {
        return m_bountyTypes[CRandGen::GetInstance()->GetRand(m_bountyTypes.size())];
    }
    return 0;
}

float CGPSInstantInformant::GetXpMultiplier() const
{
    return m_xpMultiplier;
}

float CGPSInstantInformant::GetMoneyMultiplier() const
{
    return m_moneyMultiplier;
}

bool CGPSInstantInformant::HasSpecialSquad() const
{
    return m_squads.size() > 0;
}

XString CGPSInstantInformant::GetSquadName() const
{
    if (m_squads.size() > 0) {
        int index = CRandGen::GetInstance()->GetRandRange(0, m_squads.size() - 1);
        return m_squads[index];
    }
    return XString();
}

bool CGPSInstantInformant::Buy()
{
    CBH_Player* player = CBH_Player::GetInstance();
    if (m_bought || m_cashCost > player->m_cash || m_creditsCost > player->m_credits) {
        return false;
    }
    player->ReduceMoney(m_cashCost);
    player->ReduceCredits(m_creditsCost);
    XString source = XString::Format(L"%02d_informant", m_id);
    CGameAnalytics::logCurrencyChange(source, -m_cashCost, CGameAnalytics::CURRENCY_SOFT);
    CGameAnalytics::logCurrencyChange(source, -m_creditsCost, CGameAnalytics::CURRENCY_HARD);
    return BuyFree();
}

bool CGPSInstantInformant::BuyFree()
{
    if (m_bought) {
        return false;
    }
    m_bought = true;
    UpdateState();
    UpdateIcon();
    CBH_Player::GetInstance()->m_gameStats.AddInformantsUnlocked(1);
    return true;
}

bool CGPSInstantInformant::BuyInfoFaster()
{
    CBH_Player* player = CBH_Player::GetInstance();
    if (player->m_cash < 0 || m_resetCost > player->m_credits) {
        return false;
    }
    player->ReduceCredits(m_resetCost);
    CGameAnalytics::logCurrencyChange(XString("informant timer"), -m_resetCost, CGameAnalytics::CURRENCY_HARD);
    m_lastBountyCompleted = 0;
    UpdateState();
    UpdateIcon();
    return true;
}

typedef char AssertInstantInformantSize[sizeof(CGPSInstantInformant) == 0xc0 ? 1 : -1];
