#include "CGPSInformantMisson.h"

#include "App.h"
#include "CBH_Player.h"
#include "CFont.h"
#include "CFontMgr.h"
#include "CTutorialManager.h"
#include "CXmlHelper.h"
#include "DGHelper.h"
#include "tinyxml.h"

CGPSMissionInformant::CGPSMissionInformant(TiXmlNode* node)
    : m_listener(NULL)
{
    // Original 0x12ad48: hidden until the map unhides it (m_visible keeps its default).
    ParseNode(node);
    m_hidden = 1;
    m_font = CFontMgr::GetInstance()->GetFont(15);
    SetPressImage("IDB_GPS_ROUND_ICON_ANIM");
}

CGPSMissionInformant::~CGPSMissionInformant()
{
}

void CGPSMissionInformant::ParseNode(TiXmlNode* node)
{
    TiXmlNode* info = node->FirstChild("info");
    m_id = CXmlHelper::GetAttributeValue(node, "id").ToInt();
    ParseInfo(info);
}

void CGPSMissionInformant::ParseInfo(TiXmlNode* node)
{
    m_missionId = CXmlHelper::GetAttributeValue(node, "missionId").ToInt();
    m_cameraId = CXmlHelper::GetAttributeValue(node, "camera_id").ToInt();
    m_unlockLevel = CXmlHelper::GetAttributeValue(node, "unlock_lvl").ToInt();
    m_name = CXmlHelper::GetResString(node, "name");
    XString image = CXmlHelper::GetAttributeValue(node, "image");
    m_cameraImage = CXmlHelper::GetAttributeValue(node, "imageCamera");
    SetImage(XString::AnsiString(image.c_str()));
    SetPos(CXmlHelper::GetAttributeValue(node, "posX").ToInt(),
           CXmlHelper::GetAttributeValue(node, "posY").ToInt());
    m_startText = CXmlHelper::GetAttributeValueOptional(node, "start_text");
}

void CGPSMissionInformant::SetListener(IGPSMissionInformantListener* listener)
{
    m_listener = listener;
}

void CGPSMissionInformant::Save(TiXmlElement*)
{
}

void CGPSMissionInformant::Load(TiXmlElement*)
{
}

bool CGPSMissionInformant::IsAvailable()
{
    // the original asks CBH_Player::GetInstance() for each check
    if (CBH_Player::GetInstance()->GetLevel() < m_unlockLevel) {
        return false;
    }
    return !CBH_Player::GetInstance()->IsMissionCompleted(m_missionId);
}

void CGPSMissionInformant::SetVisible(bool visible)
{
    if (static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial() && !static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsMissionInformantVisible(m_id)) {
        visible = false;
    }
    m_hidden = !visible;
    m_visible = visible;
}

void CGPSMissionInformant::SetBlinking(bool blinking)
{
    if (m_blinking != blinking) {
        m_blinking = blinking;
        if (blinking) {
            SetBlinkingImage("IDB_GPS_ROUND_ICON_ANIM_TUTORIAL");
        }
    }
}

void CGPSMissionInformant::Update(long)
{
    if (static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial()) {
        SetBlinking(static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsMissionInformantBlink(m_id));
    }
}

void CGPSMissionInformant::OnClick()
{
    if (m_hidden != 1 && m_listener != NULL) {
        m_listener->OnGpsMissionInformantSelected(this);
    }
}

void CGPSMissionInformant::Paint(ICGraphics2d& graphics, int mapX, int mapY)
{
    // Original 0x12af14: the text y uses GetWidth() (original; the icons are square).
    if (m_hidden == 0) {
        static_cast<App*>(WindowApp::m_instance)->m_helper->DrawImage(
            GetImage(), GetX() + mapX, GetY() + mapY, 1.0f, 0, 0, 0.0f, 0);
        int width = static_cast<App*>(WindowApp::m_instance)->m_helper->GetTextWidth(
            XString::AnsiString(m_name.c_str()), m_font);
        m_font->PaintText(m_name.c_str(), -1, GetX() + mapX + GetWidth() / 2 - width / 2,
                          mapY + GetY() + GetWidth(), -1, -1);
        CRectangle label(GetX() - width / 2 + GetWidth() / 2, GetY() + GetHeight(), width,
                         m_font->GetFontHeight());
        setSubItem(0, label, mapX, mapY);
        CGPSItem::Paint(graphics, mapX, mapY);
    }
}

typedef char AssertGPSMissionInformantSize[sizeof(CGPSMissionInformant) == 0x74 ? 1 : -1];
