#include "CGPSSubContract.h"

#include <math.h>
#include "App.h"
#include "CBH_GPSMap.h"
#include "CDialogWindow.h"
#include "CFont.h"
#include "CFontMgr.h"
#include "CGPSHauntCamera.h"
#include "CGPSHuntingItem.h"
#include "CXmlHelper.h"
#include "DGHelper.h"
#include "ServiceWindows.h"

typedef char CKGPSSubContractSize[sizeof(CGPSSubContract) == 0x7c ? 1 : -1];

CGPSSubContract::CGPSSubContract(CGPSHauntCamera* camera)
    : m_screenX(0), m_screenY(0), m_camera(camera), m_activateTime(0), m_disappearTime(0),
      m_flyTime(0.0f), m_flyX(0.0f), m_flyY(0.0f), m_flyVelocityX(0.0f), m_flyVelocityY(0.0f),
      m_rewardFactor(0.1f), m_index(static_cast<unsigned int>(-1)), m_flying(false)
{
}

CGPSSubContract::CGPSSubContract(CGPSHauntCamera* camera, unsigned int index)
    : m_screenX(0), m_screenY(0), m_camera(camera), m_activateTime(0), m_disappearTime(0),
      m_flyTime(0.0f), m_flyX(0.0f), m_flyY(0.0f), m_flyVelocityX(0.0f), m_flyVelocityY(0.0f),
      m_rewardFactor(0.1f), m_index(index), m_flying(false)
{
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
    if (m_flying) {
        float step = elapsed * 0.001f;
        m_flyTime -= step;
        if (m_flyTime <= 0.0f) {
            m_flying = false;
        } else {
            m_flyX += m_flyVelocityX * step;
            m_flyY += m_flyVelocityY * step;
            SetScaledPos(static_cast<int>(m_flyX), static_cast<int>(m_flyY));
        }
    }
}

void CGPSSubContract::Paint(ICGraphics2d& graphics, int mapX, int mapY)
{
    if (!m_visible || m_image == NULL) {
        return;
    }
    m_screenX = mapX + GetX();
    m_screenY = mapY + GetY();
    float opacity = 1.0f;
    if (App::GetTime() < m_activateTime) {
        opacity = 0.5f;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_helper->DrawImage(m_image, m_screenX, m_screenY, opacity, 0, 0, 0.0f, 0);
    CGPSItem::Paint(graphics, mapX, mapY);
}

bool CGPSSubContract::CheckHit(int x, int y, CGPSItem** item)
{
    return !m_flying && m_disappearTime != 0 && CGPSItem::CheckHit(x, y, item);
}

void CGPSSubContract::OnClick()
{
    unsigned int now = App::GetTime();
    if (m_disappearTime == 0 || now > m_disappearTime) {
        return;
    }
    if (now < m_activateTime) {
        ShowInactiveInfo();
    } else {
        Collect();
    }
}

void CGPSSubContract::ShowInactiveInfo()
{
    CGPSHuntingItem* item = m_camera->GetGPSHuntingItem();
    if (item == NULL) {
        return;
    }
    int money = static_cast<int>(ceilf(item->m_moneyReward * m_rewardFactor));
    int xp = static_cast<int>(ceilf(item->m_experienceReward * m_rewardFactor));
    unsigned int now = App::GetTime();
    unsigned int remaining = 0;
    if (now < m_activateTime) {
        remaining = m_activateTime - now;
    }
    CDialogWindow* dialog = new CDialogWindow(true);
    dialog->SetTitleText("IDS_DIALOG_SUB_CONTRACT_INACTIVE_TITLE");
    dialog->SetExtraImageFromImageID("IDB_GPS_ICON_SUBCONTRACT");
    XString message = Window::ResString("IDS_DIALOG_SUB_CONTRACT_INACTIVE_MESSAGE");
    XString appears = Window::ResString("IDS_DIALOG_SUB_CONTRACT_APPEARS");
    dialog->SetMessageText(XString::Format(L"%ls\n+$%d   +%d XP\n%ls %02u:%02u:%02u",
        message.c_str(), money, xp, appears.c_str(), remaining / 3600, remaining / 60 % 60, remaining % 60));
    dialog->SetButtons(CDialogWindow::BUTTON_OK);
    static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddModal(dialog);
}

void CGPSSubContract::Collect()
{
    CGPSHuntingItem* item = m_camera->GetGPSHuntingItem();
    if (item == NULL || m_disappearTime == 0) {
        return;
    }
    int money = static_cast<int>(ceilf(item->m_moneyReward * m_rewardFactor));
    int xp = static_cast<int>(ceilf(item->m_experienceReward * m_rewardFactor));
    item->m_moneyReward += money;
    item->m_experienceReward += xp;
    item->m_hasBonus = true;
    m_disappearTime = 0;
    StartFly();
    ShowReward(money, xp);
    if (CBH_GPSMap::m_instance != NULL) {
        CBH_GPSMap::m_instance->Save();
    }
}

void CGPSSubContract::ShowReward(int money, int xp) const
{
    CDialogWindow* dialog = new CDialogWindow(true);
    dialog->SetTitleText("IDS_DIALOG_SUB_CONTRACT_INACTIVE_TITLE");
    dialog->SetExtraImageFromImageID("IDB_GPS_ICON_SUBCONTRACT");
    dialog->SetMessageText(XString::Format(L"+$%d   +%d XP", money, xp));
    dialog->SetButtons(CDialogWindow::BUTTON_OK);
    static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddModal(dialog);
}

void CGPSSubContract::StartFly()
{
    m_flyX = static_cast<float>(GetX());
    m_flyY = static_cast<float>(GetY());
    float dx = m_camera->GetX() - m_flyX;
    float dy = m_camera->GetY() - m_flyY;
    m_flyTime = sqrtf(dx * dx + dy * dy) / 512.0f;
    if (m_flyTime > 0.0f) {
        m_flyVelocityX = dx / m_flyTime;
        m_flyVelocityY = dy / m_flyTime;
        m_flying = true;
    }
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
    element->SetDoubleAttribute("activateTime", m_activateTime);
    element->SetDoubleAttribute("disappearTime", m_disappearTime);
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
