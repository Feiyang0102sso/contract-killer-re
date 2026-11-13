#include "CGPSSubContract.h"

#include <math.h>
#include "App.h"
#include "CBH_GPSMap.h"
#include "CDH_SoundManager.h"
#include "CDialogWindow.h"
#include "CFont.h"
#include "CFontMgr.h"
#include "CBH_Player.h"
#include "CGameAnalytics.h"
#include "CGPSHauntCamera.h"
#include "CGPSHuntingItem.h"
#include "CTimerWindow.h"
#include "CXmlHelper.h"
#include "SScreenAdaptingConstant.h"
#include "DGHelper.h"
#include "MathLib.h"
#include "ServiceWindows.h"

typedef char CKGPSSubContractSize[sizeof(CGPSSubContract) == 0x7c ? 1 : -1];

// the original leaves the screen, activation and flight fields to Init/Update
CGPSSubContract::CGPSSubContract(CGPSHauntCamera* camera)
    : m_camera(camera), m_disappearTime(0), m_rewardFactor(0.1f), m_flying(false)
{
    m_index = static_cast<unsigned int>(-1);
}

CGPSSubContract::CGPSSubContract(CGPSHauntCamera* camera, unsigned int index)
    : m_camera(camera), m_disappearTime(0), m_rewardFactor(0.1f), m_index(index), m_flying(false)
{
    CGameAnalytics::logSubContractAppear(index + 1);
}

CGPSSubContract::~CGPSSubContract()
{
}

void CGPSSubContract::Init()
{
    SetImage("IDB_GPS_ICON_SUBCONTRACT");
}

void CGPSSubContract::Init(unsigned int activate, unsigned int disappear, float rewardFactor)
{
    Init();
    m_activateTime = activate;
    m_disappearTime = disappear;
    m_rewardFactor = rewardFactor;
}

CGPSHauntCamera* CGPSSubContract::GetHauntCamera() const
{
    return m_camera;
}

unsigned int CGPSSubContract::GetActivateTime() const
{
    return m_activateTime;
}

unsigned int CGPSSubContract::GetDisappearTime() const
{
    return m_disappearTime;
}

void CGPSSubContract::Update(long elapsed)
{
    // Original 0x12d4dc
    if (!m_flying) {
        if (!GetPressed()) {
            if (m_disappearTime >= App::GetTime()) {
                return;
            }
            // Both camera callbacks can release this contract.
            if (m_disappearTime != 0) {
                CGameAnalytics::logSubContractDisappear(m_index + 1);
                m_camera->DisappearSubContract();
                return;
            }
            static_cast<App*>(WindowApp::m_instance)->m_soundManager->PlaySound("IDM_SUBCONTRACTING_COLLECT", false, false);
            m_camera->StartNextSubContract();
        }
    } else {
        float step = elapsed * 0.001f;
        m_flyTime -= step;
        if (m_flyTime < step) {
            m_flying = false;
            return;
        }
        m_flyX += m_flyVelocityX * step;
        m_flyY += m_flyVelocityY * step;
        SetScaledPos(static_cast<int>(m_flyX), static_cast<int>(m_flyY));
    }
}

void CGPSSubContract::Paint(ICGraphics2d& graphics, int mapX, int mapY)
{
    // Original 0x12d5c0: no visibility / NULL checks; inactive contracts are drawn at 0.6 opacity.
    m_screenX = GetX() + mapX;
    m_screenY = GetY() + mapY;
    static_cast<App*>(WindowApp::m_instance)->m_helper->DrawImage(
        GetImage(), m_screenX, m_screenY, App::GetTime() < m_activateTime ? 0.6f : 1.0f, 0, 0, 0.0f, 0);
    CGPSItem::Paint(graphics, mapX, mapY);
}

void CGPSSubContract::OnClick()
{
    // the original does not check the disappear time here
    unsigned int now = App::GetTime();
    if (now < m_activateTime) {
        ShowInactiveInfo();
    } else {
        Collect();
    }
}

// Original 0x12d684: text dialog with the money and xp rewards as icon labels, the "appears in"
// text, a progress bar frame and a countdown to the activation time.
void CGPSSubContract::ShowInactiveInfo()
{
    CTextDialog* dialog = new CTextDialog();
    dialog->SetTitleText(Window::ResString("IDS_DIALOG_SUB_CONTRACT_INACTIVE_TITLE"));
    dialog->SetMessageText(Window::ResString("IDS_DIALOG_SUB_CONTRACT_INACTIVE_MESSAGE"));
    CGPSHuntingItem* item = m_camera->GetGPSHuntingItem();
    float money = ceilf(item->m_moneyReward * m_rewardFactor);
    float xp = ceilf(item->m_experienceReward * m_rewardFactor);

    LabelWithIcon* moneyLabel = new LabelWithIcon("IDB_HUD_ICON_MONEY", XString("+") + XString(static_cast<int>(money)),
                                                  false, CFontMgr::GetInstance()->GetFont(15), 0);
    moneyLabel->SetLayoutType(Window::LAYOUT_SIMPLE);
    moneyLabel->SetAlign(0x14);
    int moneyRight = SScreenAdaptingConstant(65, 130)();
    int moneyBottom = SScreenAdaptingConstant(165, 330)();
    moneyLabel->SetOutsetSpacing(0, moneyRight, moneyBottom, 0);
    dialog->AddToFront(moneyLabel);

    LabelWithIcon* xpLabel = new LabelWithIcon("IDB_HUD_ICON_EXP", XString("+") + XString(static_cast<int>(xp)),
                                               false, CFontMgr::GetInstance()->GetFont(15), 0);
    xpLabel->SetLayoutType(Window::LAYOUT_SIMPLE);
    xpLabel->SetAlign(0x14);
    int xpBottom = SScreenAdaptingConstant(165, 330)();
    int xpLeft = SScreenAdaptingConstant(65, 130)();
    xpLabel->SetOutsetSpacing(0, 0, xpBottom, xpLeft);
    dialog->AddToFront(xpLabel);

    TextWindow* appears = new TextWindow(Window::ResString("IDS_DIALOG_SUB_CONTRACT_APPEARS"),
                                         CFontMgr::GetInstance()->GetFont(15));
    appears->SetLayoutType(Window::LAYOUT_SIMPLE);
    appears->SetAlign(0x14);
    appears->SetOutsetSpacing(0, 0, SScreenAdaptingConstant(150, 300)(), 0);
    dialog->AddToFront(appears);

    CFont* timerFont = CFontMgr::GetInstance()->GetFont(8);
    HorizImageWindow* bar = new HorizImageWindow(ImageRes("SUR_HUD_PROGRESSBAR_LEFT"), ImageRes("SUR_HUD_PROGRESSBAR"),
                                                 ImageRes("SUR_HUD_PROGRESSBAR_RIGHT"), false);
    bar->SetLayoutType(Window::LAYOUT_SIMPLE);
    bar->SetAlign(0x14);
    bar->SetOutsetSpacing(0, 0, SScreenAdaptingConstant(130, 260)(), 0);
    int textWidth = static_cast<App*>(WindowApp::m_instance)->m_helper->GetTextWidth("00:00:00", timerFont);
    bar->SetDesiredWidth(SScreenAdaptingConstant(16, 32)() + textWidth);
    dialog->AddToFront(bar);

    CTimerWindow* timer = new CTimerWindow(m_activateTime, timerFont);
    timer->SetLayoutType(Window::LAYOUT_SIMPLE);
    timer->SetAlign(0x14);
    timer->SetOutsetSpacing(0, 0, SScreenAdaptingConstant(130, 260)(), 0);
    dialog->AddToFront(timer);

    dialog->SetButtons(CDialogWindow::BUTTON_OK);
    static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddModal(dialog);
}

void CGPSSubContract::Collect()
{
    // the original checks neither the item nor the disappear time and does not save the map here
    CGPSHuntingItem* item = m_camera->GetGPSHuntingItem();
    CBH_Player::GetInstance();  // result unused in the original
    int money = static_cast<int>(ceilf(item->m_moneyReward * m_rewardFactor));
    item->m_moneyReward += money;
    int xp = static_cast<int>(ceilf(item->m_experienceReward * m_rewardFactor));
    item->m_experienceReward += xp;
    item->m_hasBonus = true;
    ShowReward(money, xp);
    StartFly();
    m_disappearTime = 0;
    CGameAnalytics::logSubContractCollected(m_index);
}

void CGPSSubContract::ShowReward(int money, int xp) const
{
    int centerX = m_screenX + GetWidth() / 2;
    int centerY = m_screenY + GetWidth() / 2;
    App* app = static_cast<App*>(WindowApp::m_instance);

    AnimatedWindow* moneyLabel = MakeRewardWindow(XString::Format(L"+$%d", money));
    int moneyY = centerY - moneyLabel->m_height / 2;
    moneyLabel->SetRect(centerX - moneyLabel->m_width / 2, moneyY,
                        moneyLabel->m_width, moneyLabel->m_height, false);
    app->m_mainLayer->AddToFront(moneyLabel);

    AnimatedWindow* xpLabel = MakeRewardWindow(XString::Format(L"+%d XP", xp));
    xpLabel->SetRect(centerX - xpLabel->m_width / 2, moneyY - xpLabel->m_height,
                     xpLabel->m_width, xpLabel->m_height, false);
    app->m_mainLayer->AddToFront(xpLabel);
}

void CGPSSubContract::StartFly()
{
    // Original 0x12ddac: flies to the camera at 512 px/s (no zero distance check).
    m_flyX = GetX();
    m_flyVelocityX = m_camera->GetX() - m_flyX;
    m_flyY = GetY();
    m_flyVelocityY = m_camera->GetY() - m_flyY;
    m_flyTime = MathLib::Sqrt(m_flyVelocityX * m_flyVelocityX + m_flyVelocityY * m_flyVelocityY) / 512.0f;
    m_flyVelocityX = m_flyVelocityX / m_flyTime;
    m_flyVelocityY = m_flyVelocityY / m_flyTime;
    m_flying = true;
}

AnimatedWindow* CGPSSubContract::MakeRewardWindow(const XString& text)
{
    CFont* font = CFontMgr::GetInstance()->GetFont(15);
    App* app = static_cast<App*>(WindowApp::m_instance);
    int width = app->m_helper->GetTextWidth(XString::AnsiString(text.c_str()), font);
    int height = font->GetFontHeight();
    LabelWithIcon* label = new LabelWithIcon(NULL, text, false, CFontMgr::GetInstance()->GetFont(15), 0);
    AnimatedWindow* window = new AnimatedWindow(static_cast<AnimatedWindow::EMode>(2));
    window->AddToFront(label);
    EasyLib::EMethod linear = static_cast<EasyLib::EMethod>(0);
    window->SetStartEasy(0, 0 * App::Scale(), 0.55f, 0.0f, 0.7f, 0.0f, linear);
    window->SetCloseEasy(0, -70 * App::Scale(), 0.3f, 0.7f, 0.0f, 0.05f, linear);
    window->SetLayoutType(Window::LAYOUT_NONE);
    window->SetSize(width, height);
    return window;
}

void CGPSSubContract::Save(TiXmlElement* element)
{
    element->SetDoubleAttribute("index", m_index);
    element->SetAttribute("x", GetX());
    element->SetAttribute("y", GetY());
    element->SetAttribute("activateTime", m_activateTime);
    element->SetAttribute("disappearTime", m_disappearTime);
    element->SetDoubleAttribute("rewardFactor", m_rewardFactor);
}

void CGPSSubContract::Load(TiXmlElement* element)
{
    if (CXmlHelper::HasAttribute(element, "x")) {
        m_index = CXmlHelper::GetAttributeValueOptional(element, "index").ToInt();
        SetScaledPos(CXmlHelper::GetAttributeValue(element, "x").ToInt(),
                     CXmlHelper::GetAttributeValue(element, "y").ToInt());
        m_activateTime = CXmlHelper::GetAttributeValue(element, "activateTime").ToInt();
        m_disappearTime = CXmlHelper::GetAttributeValue(element, "disappearTime").ToInt();
        m_rewardFactor = CXmlHelper::GetAttributeValue(element, "rewardFactor").ToFloat();
    }
}
