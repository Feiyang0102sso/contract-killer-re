// CFiveDaysIncentiveBonus.cpp - five days login bonus configuration and reward.
#include "CFiveDaysIncentiveBonus.h"
#include "App.h"
#include "CBH_Player.h"
#include "CXmlHelper.h"
#include "ResMap.h"
#include "WindowBase.h"
#include "tinyxml.h"

CFiveDaysIncentiveBonus::CFiveDaysIncentiveBonus()
    : m_day(-1), m_field18(-1)
{
    char* config = static_cast<App*>(WindowApp::m_instance)->InitResMap()->loadText(
        XString(L"res/common/configs/five_days_incentive_bonus.xml"));
    TiXmlDocument document;
    document.Parse(config);
    if (config != NULL) {
        delete[] config;
    }
    Parse(document.FirstChild("fiveDaysIncentiveBonus"));
}

void CFiveDaysIncentiveBonus::Parse(TiXmlNode* node)
{
    if (node == NULL) {
        return;
    }
    m_bonus[0] = CXmlHelper::GetAttributeValue(node, "day1").ToInt();
    m_bonus[1] = CXmlHelper::GetAttributeValue(node, "day2").ToInt();
    m_bonus[2] = CXmlHelper::GetAttributeValue(node, "day3").ToInt();
    m_bonus[3] = CXmlHelper::GetAttributeValue(node, "day4").ToInt();
    m_bonus[4] = CXmlHelper::GetAttributeValue(node, "day5").ToInt();
}

void CFiveDaysIncentiveBonus::GiveBonus(int day)
{
    if ((unsigned int)day > 4) {
        return;
    }
    m_day = day;
    CBH_Player* player = CBH_Player::GetInstance();
    if (day == 0) {
        player->AddMoney(m_bonus[m_day], GAIN_SOURCE_IAP);
        return;
    }
    player->AddCredits(m_bonus[m_day]);
}

int CFiveDaysIncentiveBonus::GetDay() const
{
    return m_day;
}

XString CFiveDaysIncentiveBonus::GetDayBonusName(int day) const
{
    if (day == 0) {
        return XString::Format(Window::ResString("IDS_FIVE_DAYS_BONUS_MONEY").c_str(), m_bonus[0]);
    }
    return XString::Format(Window::ResString("IDS_FIVE_DAYS_BONUS_CREDITS").c_str(), m_bonus[day]);
}
