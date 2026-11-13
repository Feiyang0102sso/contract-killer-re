#include "CGPSHuntingItem.h"

#include "App.h"
#include "CApplet.h"
#include "CBH_Player.h"
#include "CNGS.h"
#include "CNGS_Platform.h"
#include "CNGSLocalUser.h"
#include "CNGSRemoteUser.h"
#include "CNGSRemoteUserList.h"
#include "CNGSSocialInterface.h"
#include "CRandGen.h"
#include "CXmlHelper.h"

CGPSHuntingItem::CGPSHuntingItem(CGPSHauntCamera* camera)
    : m_minWeaponRange(-1.0f), m_gatedWeapon(-1), m_weaponGated(false),
      m_randomWeapon(-1), m_hasBonus(false), m_camera(camera)
{
    m_targetFriendId = -1;
    m_revenge = false;
    m_seed = CRandGen::GetInstance()->GetRand(0x0fffffff);
    m_playerLevel = CBH_Player::GetInstance()->GetLevel();
    m_creationTime = App::GetTime();
}

void CGPSHuntingItem::Load(TiXmlElement* element)
{
    m_moneyReward = CXmlHelper::GetAttributeValue(element, "awardMoney").ToInt();
    m_experienceReward = CXmlHelper::GetAttributeValue(element, "awardXp").ToInt();
    m_moneyPenalty = CXmlHelper::GetAttributeValue(element, "punishMoney").ToInt();
    m_experiencePenalty = CXmlHelper::GetAttributeValue(element, "punishXp").ToInt();
    m_bossName = CXmlHelper::GetAttributeValue(element, "bossScreenName");
    m_squadName = CXmlHelper::GetAttributeValue(element, "squadName");
    m_energyCost = CXmlHelper::GetAttributeValue(element, "energyReduce").ToInt();
    m_civilianCount = CXmlHelper::GetAttributeValue(element, "civiliansCount").ToInt();
    m_tranquillize = CXmlHelper::GetAttributeValue(element, "tranquillizeGoal").ToBool();
    m_alertSeconds = CXmlHelper::GetAttributeValue(element, "timeSecAlert").ToInt();
    m_escapeSeconds = CXmlHelper::GetAttributeValue(element, "timeSecRunAway").ToInt();
    m_warTimer = CXmlHelper::GetAttributeValue(element, "isWarTimerEnabled").ToBool();
    m_bossFirst = CXmlHelper::GetAttributeValue(element, "isBossKillingFirst").ToBool();
    m_bossWins = CXmlHelper::GetAttributeValue(element, "isBossKillingWin").ToBool();
    m_bossSpawnsLast = CXmlHelper::GetAttributeValue(element, "isBossSpawnWhenAllDead").ToBool();
    m_silentKill = CXmlHelper::GetAttributeValueOptional(element, "silentKill").ToBool();
    m_seed = CXmlHelper::GetAttributeValue(element, "seed").ToInt();
    m_playerLevel = CXmlHelper::GetAttributeValue(element, "playerLevel").ToInt();
    m_targetFriendId = CXmlHelper::GetAttributeValueOrUseDefault(element, "targetFriendId", XString("-1")).ToInt();
    // Original: a loaded target is applied again through the setters.
    if (m_targetFriendId >= 0) {
        SetTargetFriendId(m_targetFriendId);
    }
    m_targetFBUser = CXmlHelper::GetAttributeValueOrUseDefault(element, "targetFBUser", XString(""));
    if (m_targetFBUser.Length() > 0) {
        SetTargetFBUser(CStrWChar(m_targetFBUser.c_str()));
    }
    m_revenge = CXmlHelper::GetAttributeValueOrUseDefault(element, "isRevenge", XString("false")).ToBool();
    m_gatedWeapon = CXmlHelper::GetAttributeValueOrUseDefault(element, "gatedWeaponID", XString("-1")).ToInt();
    m_weaponGated = CXmlHelper::GetAttributeValueOrUseDefault(element, "isWeaponGated", XString("false")).ToBool();
    m_randomWeapon = CXmlHelper::GetAttributeValueOrUseDefault(element, "randomWeapon", XString("-1")).ToInt();
    m_hasBonus = CXmlHelper::GetAttributeValueOrUseDefault(element, "hasBonus", XString("false")).ToBool();
    m_minWeaponRange = CXmlHelper::GetAttributeValueOrUseDefault(element, "minimalWeaponRange", XString("-1.0")).ToFloat();
    if (CXmlHelper::HasAttribute(element, "creationTime") == true) {
        m_creationTime = CXmlHelper::GetAttributeValue(element, "creationTime").ToInt();
    } else {
        m_creationTime = App::GetTime();
    }
}

TiXmlElement* CGPSHuntingItem::Save(TiXmlElement* parent)
{
    TiXmlElement* element = new TiXmlElement("huntingItem");
    element->SetAttribute("awardMoney", m_moneyReward);
    element->SetAttribute("awardXp", m_experienceReward);
    element->SetAttribute("punishMoney", m_moneyPenalty);
    element->SetAttribute("punishXp", m_experiencePenalty);
    element->SetAttribute("bossScreenName", XString::AnsiString(m_bossName.c_str()));
    element->SetAttribute("squadName", XString::AnsiString(m_squadName.c_str()));
    element->SetAttribute("energyReduce", m_energyCost);
    element->SetAttribute("civiliansCount", m_civilianCount);
    element->SetAttribute("tranquillizeGoal", m_tranquillize);
    element->SetAttribute("timeSecAlert", m_alertSeconds);
    element->SetAttribute("timeSecRunAway", m_escapeSeconds);
    element->SetAttribute("isWarTimerEnabled", m_warTimer);
    element->SetAttribute("isBossKillingFirst", m_bossFirst);
    element->SetAttribute("isBossKillingWin", m_bossWins);
    element->SetAttribute("isBossSpawnWhenAllDead", m_bossSpawnsLast);
    element->SetAttribute("silentKill", m_silentKill);
    element->SetAttribute("seed", m_seed);
    element->SetAttribute("playerLevel", m_playerLevel);
    element->SetAttribute("targetFriendId", m_targetFriendId);
    element->SetAttribute("targetFBUser", XString::AnsiString(m_targetFBUser.c_str()));
    element->SetAttribute("isRevenge", m_revenge);
    element->SetAttribute("gatedWeaponID", m_gatedWeapon);
    element->SetAttribute("isWeaponGated", m_weaponGated);
    element->SetAttribute("randomWeapon", m_randomWeapon);
    element->SetAttribute("hasBonus", m_hasBonus);
    element->SetAttribute("minimalWeaponRange", static_cast<int>(m_minWeaponRange));
    element->SetAttribute("creationTime", static_cast<int>(m_creationTime));
    TiXmlElement* copy = parent->InsertEndChild(*element)->ToElement();
    delete element;
    return copy;
}

XString CGPSHuntingItem::ParametrizeText(const XString& text) const
{
    return ParametrizeTextWithParams(text, m_bossName, m_bossIsMan);
}

void CGPSHuntingItem::SetBossScreenName(const XString& name)
{
    m_bossName.Assign(name.Clone());
}

bool CGPSHuntingItem::SetTargetFriendId(int friendId)
{
    m_targetFriendId = friendId;
    XString name("win32");
    CNGSLocalUser* local = CNGS::GetInstance()->GetLocalUser();
    CNGSRemoteUser* target = local->m_remoteUserLists[0]->findRemoteUserByID(friendId);
    bool found;
    if (target != NULL) {
        {
            CStrWChar nickname = target->GetNickName();
            name = nickname.GetWCharPtr();
        }
        SetBossScreenName(name);
        found = true;
    } else {
        m_targetFriendId = -1;
        found = false;
    }
    return found;
}

bool CGPSHuntingItem::SetTargetFBUser(CStrWChar identity)
{
    m_targetFBUser = identity.GetWCharPtr();
    XString name("unknown");
    CNGSSocialInterface* facebook = CNGS_Platform::GetInstance()->getSocialInterface(SOCIAL_INTERFACE_FACEBOOK);
    {
        CStrWChar alias = facebook->getFriendNameFromSortedListWithId(identity);
        name = alias.GetWCharPtr();
    }
    int comparison = XString::CmpData(name.c_str(), name.ByteSize(), L"unknown",
                                     wcslen(L"unknown") * sizeof(wchar_t), 0x7fffffff);
    bool found = false;
    if (comparison != 0) {
        found = true;
        SetBossScreenName(name);
    }
    return found;
}

XString CGPSHuntingItem::ParametrizeTextWithParams(const XString& text, XString bossName, bool isMan)
{
    XString result = text.Replace(L"[BossName]", bossName.c_str(), true);
    {
        XString playerName = CBH_Player::GetInstance()->m_playerName;
        result = result.Replace(L"[PlayerName]", playerName.c_str(), true);
    }
    if (isMan == true) {
        result = result.Replace(L"[he]", L"he", true);
        result = result.Replace(L"[He]", L"He", true);
        result = result.Replace(L"[his]", L"his", true);
        result = result.Replace(L"[His]", L"His", true);
        result = result.Replace(L"[himself]", L"himself", true);
    } else {
        result = result.Replace(L"[he]", L"she", true);
        result = result.Replace(L"[He]", L"She", true);
        result = result.Replace(L"[his]", L"her", true);
        result = result.Replace(L"[His]", L"Her", true);
        result = result.Replace(L"[himself]", L"herself", true);
    }
    return result;
}

typedef char AssertGPSHuntingItemSize[sizeof(CGPSHuntingItem) == 0x6c ? 1 : -1];
