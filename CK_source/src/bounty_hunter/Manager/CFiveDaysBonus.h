// CFiveDaysBonus.h - the daily login bonus of res/common/configs/five_days_bonus.xml: cash
// for the first four days, a random prize (cash, credits, medkits or a weapon) on the fifth.
#ifndef CFIVEDAYSBONUS_H
#define CFIVEDAYSBONUS_H

#include <stddef.h>
#include "Vector_ss.h"
#include "xString.h"

class TiXmlNode;

class CFiveDaysBonus {
public:
    enum { CASH_DAY_COUNT = 4, DAY_COUNT = 5 };

    /** One possible prize of the last day. */
    struct SFifthDay {
        int m_probability;  // 0x00 percent
        int m_cash;         // 0x04
        int m_credits;      // 0x08
        int m_medkits;      // 0x0c
        int m_weaponId;     // 0x10 -1: no weapon
    };

    CFiveDaysBonus();

    void ParseFourDay(TiXmlNode* node);
    void ParseFifthDay(TiXmlNode* node);
    /** Give the bonus of `day` (0..4; 4 rolls one of the prizes). */
    void GiveBonus(int day);
    /** Give the last-day prize `index`. */
    void GiveChance(int index);
    int GetDay() const;
    const char* GetDayBonusIcon(int day) const;
    XString GetDayBonusNameForIcon(int day) const;
    XString GetDayBonusNameForMessage(int day) const;

    int m_cash[CASH_DAY_COUNT];         // 0x00 bonus of each of the first four days
    Vector<SFifthDay> m_fifthDays;      // 0x10
    XString m_chanceText;               // 0x20 text of the last-day prize
    int m_day;                          // 0x24 -1: none given yet
    int m_chance;                       // 0x28 -1: none given yet
};

#endif
