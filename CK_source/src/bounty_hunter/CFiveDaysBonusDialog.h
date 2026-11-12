#ifndef CFIVEDAYSBONUSDIALOG_H
#define CFIVEDAYSBONUSDIALOG_H

#include "CDialogWindow.h"

class CFiveDaysBonusDialog : public CTextDialog {
public:
    enum EBonusType { DAILY_BONUS = 0, INCENTIVE_BONUS = 1 };
    enum { INCENTIVE_BONUS_ACCEPTED = -0x5821d14c };

    CFiveDaysBonusDialog(int day, EBonusType type);

private:
    void CreateTitle();
    void CreateMessage(int day);
    void CreateIcons(int day);
    void CreateOKButton();
    XString GetMessagePriceString(int day);
    XString GetFirstString(int day);
    void CreateIcon(int iconDay, int today);
    void SetIcon(ImageWindow** image, int iconDay, int today);
    XString GetIconPriceString(int day);

    Window* m_bonusContent;
    Window* m_icons;
    EBonusType m_type;
};

typedef char FiveDaysBonusDialogSizeCheck[sizeof(CFiveDaysBonusDialog) == 0x12c ? 1 : -1];

#endif
