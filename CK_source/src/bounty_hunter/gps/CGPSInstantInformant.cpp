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
    : m_lastBountyCompleted(0), m_bought(false), m_listener(NULL), m_weaponGated(false), m_cameraId(-1),
      m_state(Hidden)
{
    // Original 0x12b7fc: the round animation is the press overlay.
    ParseNode(node);
    m_timerFont = CFontMgr::GetInstance()->GetFont(8);
    m_nameFont = CFontMgr::GetInstance()->GetFont(15);
    UpdateIcon();
    SetPressImage("IDB_GPS_ROUND_ICON_ANIM");
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
    int type = CXmlHelper::GetAttributeValue(node, "type").ToInt();
    m_type = static_cast<Type>(type);
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
    if (node != NULL) {
        XString source = CXmlHelper::GetAttributeValue(node, "bounty_types");
        Vector<XString> types;
        source.Split(types, L",");
        for (int index = 0; index < types.size(); ++index) {
            XString value = types[index].Trim(true, true);
            if (value.GetData()->m_byteSize != 0) {
                int type = value.ToInt();
                m_bountyTypes.addElement(type);
            }
        }
    }
}

void CGPSInstantInformant::ParseSquad(TiXmlNode* node)
{
    if (node != NULL && CXmlHelper::HasAttribute(node, "squads") == true) {
        XString source = CXmlHelper::GetAttributeValue(node, "squads");
        Vector<XString> squads;
        source.Split(m_squads, L",");
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
    if (static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial() == true) {
        App* app = static_cast<App*>(WindowApp::m_instance);
        bool blink = app->m_tutorialManager->IsInstantInformantBlink(m_id);
        SetBlinking(blink == true);
    }
}

// Original 0x12b7a0: both states load the same icon.
void CGPSInstantInformant::UpdateIcon()
{
    if (m_state == Locked) {
        SetImage(XString::AnsiString(m_icon.c_str()));
    } else {
        SetImage(XString::AnsiString(m_icon.c_str()));
    }
}

// Original 0x12bdc4: the moving informant keeps its state (Hidden -> Available once the moving
// camera missions are finished) and never becomes Searching; the listener is not NULL checked.
void CGPSInstantInformant::UpdateState()
{
    if (static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial() == true
        && !static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsInstantInformantVisible(m_id)) {
        m_state = Hidden;
        m_visible = false;
        return;
    }
    unsigned int now = CStdUtil_Cocoa::GetTimeSeconds();
    int levelDiff = CBH_Player::GetInstance()->GetLevel() - m_unlockLevel;
    Type type = m_type;
    if (type == Moving) {
        State& state = m_state;
        bool visible = false;
        if (levelDiff >= 0) {
            m_listener->OnVehicleMissionsVisible();
            visible = CBH_GPSMap::m_instance->IsMovingCameraMissionsFinished();
        }
        if (visible) {
            if (state == Hidden && visible == true) {
                state = Available;
            }
        } else {
            state = Hidden;
        }
        m_visible = visible;
        if (state == Hidden) {
            return;
        }
        if (m_cameraId >= 0) {
            state = Assigned;
            return;
        }
        if (levelDiff < 0) {
            return;
        }
        if (now - m_lastBountyCompleted >= m_cooldown) {
            state = Available;
        }
        return;
    }
    m_visible = true;
    if (type == Targeting) {
        m_listener->OnTargetMissionsVisible();
    }
    if (m_cameraId >= 0) {
        m_state = Assigned;
    } else if (levelDiff < 0) {
        m_state = Hidden;
        m_visible = false;
    } else if (!m_bought) {
        m_state = Locked;
    } else {
        m_state = now - m_lastBountyCompleted < m_cooldown ? Searching : Available;
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
    // Original 0x12b3b8: weapon gated, targeting and hidden informants draw nothing. Both texts use
    // the name font; the text y uses GetWidth() (original; the icons are square).
    if (!m_weaponGated) {
        if (m_type != Targeting && m_state != Hidden) {
            static_cast<App*>(WindowApp::m_instance)->m_helper->DrawImage(
                GetImage(), GetX() + mapX, GetY() + mapY, 1.0f, 0, 0, 0.0f, 0);
            int nameWidth = static_cast<App*>(WindowApp::m_instance)->m_helper->GetTextWidth(
                XString::AnsiString(m_name.c_str()), m_nameFont);
            m_nameFont->PaintText(m_name.c_str(), -1, GetX() + mapX + GetWidth() / 2 - nameWidth / 2,
                                  mapY + GetY() + GetWidth(), -1, -1);
            CRectangle nameLabel(GetX() - nameWidth / 2 + GetWidth() / 2, GetY() + GetHeight(),
                                 nameWidth, m_nameFont->GetFontHeight());
            setSubItem(0, nameLabel, mapX, mapY);
            int seconds = GetCooldownTimeLeft();
            if (m_state == Searching && m_type != Blue && seconds > 0) {
                CStdUtil_Cocoa::GetTimeSeconds();  // result unused (original)
                XString time = DGHelper::ClockStringFromSeconds(seconds);
                int timeWidth = static_cast<App*>(WindowApp::m_instance)->m_helper->GetTextWidth(
                    XString::AnsiString(time.c_str()), m_nameFont);
                m_nameFont->PaintText(time.c_str(), -1, GetX() + mapX + GetWidth() / 2 - timeWidth / 2,
                                      mapY + GetY() + GetWidth() + m_nameFont->GetFontHeight(), -1, -1);
                CRectangle timeLabel(GetX() - timeWidth / 2 + GetWidth() / 2,
                                     GetY() + GetHeight() + m_nameFont->GetFontHeight(),
                                     timeWidth, m_nameFont->GetFontHeight());
                setSubItem(1, timeLabel, mapX, mapY);
            }
            CGPSItem::Paint(graphics, mapX, mapY);
        }
    }
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
    if (m_cooldown > elapsed) {
        return m_cooldown - elapsed;
    }
    return 0;
}

void CGPSInstantInformant::Save(TiXmlElement* parent)
{
    // Original 0x12b2f4: a fresh element is copied into the parent and deleted.
    TiXmlElement* item = new TiXmlElement("instant_informant");
    item->SetAttribute("id", m_id);
    item->SetAttribute("lastBountyCompletedTime", m_lastBountyCompleted);
    item->SetAttribute("bountyCameraId", m_cameraId);
    item->SetAttribute("bought", m_bought);
    parent->InsertEndChild(*item);
    delete item;
}

void CGPSInstantInformant::Load(TiXmlElement* parent)
{
    for (TiXmlElement* item = parent->FirstChildElement("instant_informant"); item != NULL;
         item = item->NextSiblingElement("instant_informant")) {
        // Original 0x12b6a8: plain attribute reads, every matching element (no break).
        int id = CXmlHelper::GetAttributeValue(item, "id").ToInt();
        if (id == m_id) {
            m_lastBountyCompleted = CXmlHelper::GetAttributeValue(item, "lastBountyCompletedTime").ToInt();
            m_cameraId = CXmlHelper::GetAttributeValue(item, "bountyCameraId").ToInt();
            m_bought = CXmlHelper::GetAttributeValue(item, "bought").ToBool();
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
    int count = m_bountyTypes.size();
    if (count > 0) {
        return m_bountyTypes[CRandGen::GetInstance()->GetRand(count)];
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
    int count = m_squads.size();
    int index = CRandGen::GetInstance()->GetRandRange(0, count - 1);
    if (index < 0 || index >= count) {
        return XString("");
    }
    return m_squads[index];
}

bool CGPSInstantInformant::Buy()
{
    if (m_bought) {
        return false;
    }
    CBH_Player* player = CBH_Player::GetInstance();
    if (player->m_cash < m_cashCost || player->m_credits < m_creditsCost) {
        return false;
    }
    CBH_Player::GetInstance()->ReduceMoney(m_cashCost);
    CBH_Player::GetInstance()->ReduceCredits(m_creditsCost);
    XString source = XString::Format(L"%02d_informant", m_id);
    CGameAnalytics::logCurrencyChange(source, -m_cashCost, CGameAnalytics::CURRENCY_SOFT);
    CGameAnalytics::logCurrencyChange(source, -m_creditsCost, CGameAnalytics::CURRENCY_HARD);
    m_bought = true;
    UpdateState();
    UpdateIcon();
    CGameAnalytics::logInformantUnlocked(m_id);
    CBH_Player::GetInstance()->m_gameStats.AddInformantsUnlocked(1);
    return true;
}

bool CGPSInstantInformant::BuyFree()
{
    // the original does not test m_bought first and logs the unlock
    m_bought = true;
    UpdateState();
    UpdateIcon();
    CGameAnalytics::logInformantUnlocked(m_id);
    CBH_Player::GetInstance()->m_gameStats.AddInformantsUnlocked(1);
    return true;
}

bool CGPSInstantInformant::BuyInfoFaster()
{
    CBH_Player* player = CBH_Player::GetInstance();
    if (player->m_cash < 0 || player->m_credits < m_resetCost) {
        return false;
    }
    CBH_Player::GetInstance()->ReduceCredits(m_resetCost);
    XString source("informant timer");
    CGameAnalytics::logCurrencyChange(source, -m_resetCost, CGameAnalytics::CURRENCY_HARD);
    m_lastBountyCompleted = 0;
    UpdateState();
    UpdateIcon();
    return true;
}

typedef char AssertInstantInformantSize[sizeof(CGPSInstantInformant) == 0xc0 ? 1 : -1];
