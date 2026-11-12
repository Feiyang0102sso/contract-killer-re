// CFiveDaysIncentiveBonus.h - five days login bonus (five_days_incentive_bonus.xml).
#ifndef CFIVEDAYSINCENTIVEBONUS_H
#define CFIVEDAYSINCENTIVEBONUS_H

#include "xString.h"

class TiXmlNode;

class CFiveDaysIncentiveBonus {
public:
    CFiveDaysIncentiveBonus();

    void Parse(TiXmlNode* node);
    void GiveBonus(int day);
    int GetDay() const;
    XString GetDayBonusName(int day) const;

    int m_bonus[5];         // 0x00 day 1 is cash, days 2..5 are credits
    int m_day;              // 0x14 last rewarded day
    int m_field18;          // 0x18 not read in this unit
};

#endif
