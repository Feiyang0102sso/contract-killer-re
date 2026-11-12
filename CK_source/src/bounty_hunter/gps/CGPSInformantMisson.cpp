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
    : m_field64(0), m_font(NULL), m_listener(NULL), m_hidden(1)
{
    ParseNode(node);
    m_font = CFontMgr::GetInstance()->GetFont(15);
    SetPressImage("IDB_GPS_ROUND_ICON_ANIM");
    m_visible = false;
}

CGPSMissionInformant::~CGPSMissionInformant()
{
}

void CGPSMissionInformant::ParseNode(TiXmlNode* node)
{
    m_id = CXmlHelper::GetAttributeValue(node, "id").ToInt();
    ParseInfo(node->FirstChild("info"));
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
    CBH_Player* player = CBH_Player::GetInstance();
    return player->GetLevel() >= m_unlockLevel && !player->IsMissionCompleted(m_missionId);
}

void CGPSMissionInformant::SetVisible(bool visible)
{
    if (CTutorialManager::IsPlayerInTutorial() && !CTutorialManager::IsMissionInformantVisible(m_id)) {
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
    if (CTutorialManager::IsPlayerInTutorial()) {
        SetBlinking(CTutorialManager::IsMissionInformantBlink(m_id));
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
    if (m_hidden != 0) {
        return;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (GetImage() != NULL) {
        app->m_helper->DrawImage(GetImage(), mapX + GetX(), mapY + GetY(), 1.0f, 0, 0, 0.0f, 0);
    }
    if (m_font != NULL) {
        int width = m_font->MeasureTextWidth(m_name.c_str(), -1, -1, NULL);
        CRectangle label;
        label.m_x = GetX() + (GetWidth() - width) / 2;
        label.m_y = GetY() + GetHeight();
        label.m_width = width;
        label.m_height = m_font->GetFontHeight();
        m_font->PaintText(m_name.c_str(), -1, mapX + label.m_x, mapY + label.m_y, -1, -1);
        setSubItem(0, label, mapX, mapY);
    }
    CGPSItem::Paint(graphics, mapX, mapY);
}

bool CGPSMissionInformant::CheckHit(int x, int y, CGPSItem** item)
{
    if (!m_visible) {
        return false;
    }
    if (CGPSItem::CheckHit(x, y, item)) {
        return true;
    }
    if (m_font != NULL) {
        int width = m_font->MeasureTextWidth(m_name.c_str(), -1, -1, NULL);
        int left = GetX() + (GetWidth() - width) / 2;
        int top = GetY() + GetHeight();
        if (x >= left && x < left + width && y >= top && y < top + m_font->GetFontHeight()) {
            *item = this;
            return true;
        }
    }
    return false;
}

typedef char AssertGPSMissionInformantSize[sizeof(CGPSMissionInformant) == 0x74 ? 1 : -1];
