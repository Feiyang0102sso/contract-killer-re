#include "CFiveDaysBonusDialog.h"

#include "App.h"
#include "BaseDialog.h"
#include "CBH_MenuButton.h"
#include "CFiveDaysBonus.h"
#include "CFiveDaysIncentiveBonus.h"
#include "SScreenAdaptingConstant.h"

namespace {
const int DayCount = 5;
const SScreenAdaptingConstant s_okTop(210, 420, 420);
const SScreenAdaptingConstant s_messageMargin(-5, -10);
const SScreenAdaptingConstant s_firstLineHeight(35, 70);
const SScreenAdaptingConstant s_priceLineHeight(10, 20);
const SScreenAdaptingConstant s_iconMargin(-5, -10, -10);
const SScreenAdaptingConstant s_dayTagMargin(-7, -15);
}

CFiveDaysBonusDialog::CFiveDaysBonusDialog(int day, EBonusType type)
    : CTextDialog(), m_bonusContent(new Window(false)), m_icons(NULL), m_type(type)
{
    m_bonusContent->SetLayoutType(LAYOUT_SIMPLE);
    m_bonusContent->m_debugFrame = true;
    m_bonusContent->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    m_bonusContent->SetPercentWidth(80, 0, 0);
    m_bonusContent->SetPercentHeight(50, 0, 0);
    AddToFront(m_bonusContent);

    CreateTitle();
    CreateMessage(day);
    CreateIcons(day);
    CreateOKButton();
    m_title->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
    m_content->SetWidthByContent(0, 0);
    m_content->SetHeightByContent(0, 0);
    CBH_MenuButton* button = m_buttons[0];
    button->SetCorner((m_width - button->m_width) / 2, s_okTop());
}

void CFiveDaysBonusDialog::CreateTitle()
{
    m_title->SetAlign(ALIGN_LEFT);
    if (m_type == INCENTIVE_BONUS) {
        SetTitleText("IDS_DAILY_BONUS_INCENTIVED_TITLE");
    } else if (m_type == DAILY_BONUS) {
        SetTitleText("IDS_DAILY_BONUS_TITLE");
    }
}

void CFiveDaysBonusDialog::CreateMessage(int day)
{
    Window* message = new Window(false);
    message->SetPercentWidth(100, 0, 0);
    message->SetHeightByContent(0, 0);
    message->SetOutsetSpacing(s_messageMargin(), 0, 0, 0);
    XString price = GetMessagePriceString(day);
    XString firstLine = GetFirstString(day);
    XString format = ResString("IDS_DAILY_BONUS_BONUS");
    XString secondLine = XString::Format(format.c_str(), price.c_str());
    CFontMgr* fonts = CFontMgr::GetInstance();
    TextWindow* first = new TextWindow(firstLine, fonts->GetFont(15));
    TextWindow* second = new TextWindow(secondLine, fonts->GetFont(17));
    first->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    second->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    first->SetPercentWidth(80, 0, 0);
    first->SetDesiredHeight(s_firstLineHeight());
    second->SetWidthByContent(0, 0);
    second->SetDesiredHeight(s_priceLineHeight());
    message->AddToFront(first, 0, 0);
    message->AddToFront(second, 0, 1);
    second->SetHeightByContent(0, 0);
    second->SetOutsetSpacing(-5, 0, 0, 0);
    m_bonusContent->AddToFront(message, 0, 0);
}

void CFiveDaysBonusDialog::CreateIcons(int day)
{
    m_icons = new Window(false);
    m_icons->SetLayoutType(LAYOUT_SIMPLE);
    m_icons->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    m_icons->SetWidthByContent(0, 0);
    m_icons->SetHeightByContent(0, 0);
    m_icons->SetOutsetSpacing(-5, 0, 0, 0);
    m_bonusContent->AddToFront(m_icons, 0, 1);
    for (int iconDay = 0; iconDay < DayCount; ++iconDay) {
        CreateIcon(iconDay, day);
    }
    m_downText->SetWidthByContent(0, 0);
    m_downText->SetHeightByContent(0, 0);
}

void CFiveDaysBonusDialog::CreateOKButton()
{
    SetButtons(BUTTON_OK);
    if (m_type == INCENTIVE_BONUS) {
        SetButtonsCommands(INCENTIVE_BONUS_ACCEPTED, -1, -1);
    }
}

XString CFiveDaysBonusDialog::GetMessagePriceString(int day)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (m_type == INCENTIVE_BONUS) {
        return app->m_fiveDaysIncentiveBonus->GetDayBonusName(day);
    }
    return app->m_fiveDaysBonus->GetDayBonusNameForMessage(day);
}

XString CFiveDaysBonusDialog::GetFirstString(int day)
{
    if (m_type == INCENTIVE_BONUS) {
        return ResString("IDS_DAILY_BONUS_TEXT_INCENTIVED_DAYS");
    }
    if (m_type == DAILY_BONUS && day != 0) {
        XString format = ResString("IDS_DAILY_BONUS_TEXT_DAYS");
        return XString::Format(format.c_str(), day + 1);
    }
    return ResString("IDS_FIVE_DAYS_BONUS_FIRST_DAY");
}

void CFiveDaysBonusDialog::CreateIcon(int iconDay, int today)
{
    CFontMgr* fonts = CFontMgr::GetInstance();
    CFont* active = fonts->GetFont(17);
    CFont* future = fonts->GetFont(18);
    CFont* font = active;
    if (iconDay > today) {
        font = future;
    }
    TextWindow* day = new TextWindow(XString::Format(L"Day %d", iconDay + 1), font);
    day->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    ImageWindow* image = new ImageWindow();
    SetIcon(&image, iconDay, today);
    image->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    image->SetOutsetSpacing(-5, s_iconMargin(), 0, s_iconMargin());
    Window* icon = new Window(false);
    icon->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    icon->SetWidthByContent(0, 0);
    icon->SetHeightByContent(0, 0);
    icon->AddToFront(image);
    if (today <= iconDay) {
        TextWindow* price = new TextWindow(GetIconPriceString(iconDay), font);
        price->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
        price->SetWidthByContent(0, 0);
        price->SetHeightByContent(0, 0);
        icon->AddToFront(price);
    }
    m_icons->AddToFront(day, iconDay, 0);
    m_icons->AddToFront(icon, iconDay, 1);

    TextWindow* marker = NULL;
    if (iconDay == today) {
        marker = new TextWindow(ResString("IDS_FIVE_DAYS_BONUS_TODAY"), active);
    } else if (iconDay == today + 1) {
        marker = new TextWindow(ResString("IDS_FIVE_DAYS_BONUS_TOMORROW"), future);
    }
    if (marker != NULL) {
        marker->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
        marker->SetOutsetSpacing(s_dayTagMargin(), 0, 0, 0);
        m_icons->AddToFront(marker, iconDay, 2);
    }
}

void CFiveDaysBonusDialog::SetIcon(ImageWindow** image, int iconDay, int today)
{
    if (image == NULL || *image == NULL) {
        return;
    }
    const char* resource = NULL;
    if (m_type == INCENTIVE_BONUS) {
        static const char* const rewards[] = {
            "IDB_SHOP_ICON_12000_CASH", "IDB_SHOP_ICON_CLASSIC_CARD", "IDB_SHOP_ICON_PREMIUM_CARD",
            "IDB_SHOP_ICON_GOLD_CARD", "IDB_SHOP_ICON_PLATINUM_CARD"
        };
        if (iconDay < today) {
            resource = "IDB_DAILY_BONUS_LAST_DAY_ICON";
        } else if (iconDay >= 0 && iconDay < DayCount) {
            resource = rewards[iconDay];
        }
    } else if (m_type == DAILY_BONUS) {
        if (iconDay == DayCount - 1) {
            resource = "IDB_DAILY_BONUS_CHANCE_ICON";
        } else if (iconDay < today) {
            resource = "IDB_DAILY_BONUS_LAST_DAY_ICON";
        } else {
            resource = "IDB_SHOP_ICON_12000_CASH";
        }
    }
    if (resource != NULL) {
        (*image)->SetImageById(resource);
    }
}

XString CFiveDaysBonusDialog::GetIconPriceString(int day)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (m_type == INCENTIVE_BONUS) {
        return app->m_fiveDaysIncentiveBonus->GetDayBonusName(day);
    }
    return app->m_fiveDaysBonus->GetDayBonusNameForIcon(day);
}
