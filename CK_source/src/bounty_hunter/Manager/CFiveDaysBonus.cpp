// CFiveDaysBonus.cpp - the daily login bonus (five_days_bonus.xml).
#include "CFiveDaysBonus.h"
#include <wchar.h>
#include "CApplet.h"
#include "CApp.h"
#include "CHash.h"
#include "CRandGen.h"
#include "App.h"
#include "ResMap.h"
#include "tinyxml.h"
#include "CXmlHelper.h"
#include "CShopItem.h"
#include "CDH_WeaponsManager.h"
#include "CBH_Player.h"
#include "CGameAnalytics.h"
typedef char CFiveDaysBonus_ARMv7_Size[(sizeof(CFiveDaysBonus) == 0x2c) ? 1 : -1];

CFiveDaysBonus::CFiveDaysBonus()
    : m_cash(), m_day(-1), m_chance(-1)
{
    ResMap* resMap = ((App*)WindowApp::m_instance)->m_resMap;
    char* text = resMap->loadText(XString(L"res/common/configs/five_days_bonus.xml"));
    TiXmlDocument document;
    document.Parse(text);
    if (text != NULL) {
        delete[] text;
    }
    TiXmlNode* root = document.FirstChild("fiveDaysBonus");
    ParseFourDay(root);
    ParseFifthDay(root);
}

void CFiveDaysBonus::ParseFourDay(TiXmlNode* node)
{
    if (node == NULL) {
        return;
    }
    TiXmlNode* child = node->FirstChild("day");
    for (int day = 0; child != NULL && day < CASH_DAY_COUNT; ++day) {
        m_cash[day] = CXmlHelper::GetAttributeValue(child, "cash").ToInt();
        child = child->NextSibling("day");
    }
}

void CFiveDaysBonus::ParseFifthDay(TiXmlNode* node)
{
    if (node == NULL) {
        return;
    }
    m_fifthDays.ensureCapacity(5);
    for (TiXmlNode* child = node->FirstChild("fifth_day"); child != NULL; child = child->NextSibling("fifth_day")) {
        SFifthDay prize;
        prize.m_probability = CXmlHelper::GetAttributeValue(child, "probability").ToInt();
        prize.m_cash = CXmlHelper::GetAttributeValue(child, "cash").ToInt();
        prize.m_credits = CXmlHelper::GetAttributeValue(child, "credits").ToInt();
        prize.m_medkits = CXmlHelper::GetAttributeValue(child, "medkits").ToInt();
        prize.m_weaponId = CXmlHelper::GetAttributeValue(child, "weaponID").ToInt();
        m_fifthDays.addElement(prize);
    }
}

void CFiveDaysBonus::GiveBonus(int day)
{
    if ((unsigned int)day >= DAY_COUNT) {
        return;
    }
    m_day = day;
    if (day < CASH_DAY_COUNT) {
        XString bonus = XString::Format(L"Cash %d", m_cash[day]);
        CGameAnalytics::logNewDailyBonus(bonus, m_day);
        CBH_Player::GetInstance()->AddMoney(m_cash[m_day], GAIN_SOURCE_IAP);
        return;
    }
    // the last day: roll one of the prizes by their probabilities
    int roll = CRandGen::GetInstance()->GetRandRange(0, 100);
    int sum = 0;
    for (int index = 0; index < m_fifthDays.size(); index++) {
        sum += m_fifthDays[index].m_probability;
        if (roll < sum) {
            GiveChance(index);
            return;
        }
    }
}

void CFiveDaysBonus::GiveChance(int index)
{
    m_chance = index;
    SFifthDay& prize = m_fifthDays[index];
    // b147: original lays out the weapon branch first (cmp #0; blt to the cash/credits branch).
    if (prize.m_weaponId >= 0) {
        App* app = static_cast<App*>(WindowApp::m_instance);
        CItem* weapon = app->InitWeaponManager()->getItemById(prize.m_weaponId);
        if (weapon == NULL) {
            return;
        }
        if (weapon->isAvailable() == true) {
            // already owned: the cash of the prize instead
            int cash = m_fifthDays[index].m_cash;
            m_chanceText = XString::Format(Window::ResString("IDS_FIVE_DAYS_BONUS_MONEY").c_str(), cash);
            CBH_Player::GetInstance()->AddMoney(cash, GAIN_SOURCE_IAP);
            XString bonus = XString::Format(L"Chance: cash for weapon %d", cash);
            CGameAnalytics::logDailyBonus(bonus, m_day);
        } else {
            weapon->setAvailable(true);
            m_chanceText = weapon->GetName();
            XString bonus = XString::Format(L"Chance: weapon %s", m_chanceText.c_str());
            CGameAnalytics::logDailyBonus(bonus, m_day);
        }
    } else {
        if (prize.m_cash > 0) {
            CBH_Player::GetInstance()->AddMoney(m_fifthDays[index].m_cash, GAIN_SOURCE_IAP);
            m_chanceText = XString::Format(Window::ResString("IDS_FIVE_DAYS_BONUS_MONEY").c_str(), m_fifthDays[index].m_cash);
            XString bonus = XString::Format(L"Chance: cash %d", m_fifthDays[index].m_cash);
            CGameAnalytics::logDailyBonus(bonus, m_day);
        } else if (prize.m_credits > 0) {
            CBH_Player::GetInstance()->AddCredits(m_fifthDays[index].m_credits);
            m_chanceText = XString::Format(Window::ResString("IDS_FIVE_DAYS_BONUS_CREDITS").c_str(), m_fifthDays[index].m_credits);
            XString bonus = XString::Format(L"Chance: credits %d", m_fifthDays[index].m_credits);
            CGameAnalytics::logDailyBonus(bonus, m_day);
        } else if (prize.m_medkits > 0) {
            m_chanceText = XString::Format(Window::ResString("IDS_FIVE_DAYS_BONUS_MEDKITS").c_str(), m_fifthDays[index].m_medkits);
            CBH_Player::GetInstance()->AddMedkits(m_fifthDays[index].m_medkits);
            XString bonus = XString::Format(L"Chance: medkit(s) %d", m_fifthDays[index].m_medkits);
            CGameAnalytics::logDailyBonus(bonus, m_day);
        }
    }
}

int CFiveDaysBonus::GetDay() const
{
    return m_day;
}

const char* CFiveDaysBonus::GetDayBonusIcon(int) const
{
    return "IDB_SOFT_1_PNG";
}

XString CFiveDaysBonus::GetDayBonusNameForIcon(int day) const
{
    if (day < CASH_DAY_COUNT) {  // 原版为有符号比较（bgt）
        return XString::Format(Window::ResString("IDS_FIVE_DAYS_BONUS_MONEY").c_str(), m_cash[day]);
    }
    return Window::ResString("IDS_FIVE_DAYS_BONUS_CHANCE");
}

XString CFiveDaysBonus::GetDayBonusNameForMessage(int day) const
{
    if (day < CASH_DAY_COUNT) {  // 原版为有符号比较（bgt）
        return XString::Format(Window::ResString("IDS_FIVE_DAYS_BONUS_MONEY").c_str(), m_cash[day]);
    }
    return m_chanceText;
}
