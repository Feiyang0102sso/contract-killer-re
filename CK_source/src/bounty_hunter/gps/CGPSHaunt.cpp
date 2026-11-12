#include "CGPSHaunt.h"

#include "App.h"
#include "CBH_Player.h"
#include "CFont.h"
#include "CFontMgr.h"
#include "CGPSHauntCamera.h"
#include "CXmlHelper.h"
#include "DGHelper.h"

CGPSHaunt::CGPSHaunt(TiXmlNode* node) : m_listener(NULL)
{
    ParseNode(node);
    m_unlockLevel = 0xffff;
    for (int index = 0; index < m_cameras.size(); ++index) {
        int level = m_cameras[index]->m_unlockLevel;
        if (level < m_unlockLevel) {
            m_unlockLevel = level;
        }
    }
    m_font = CFontMgr::GetInstance()->GetFont(GAME_FONT_DEFAULT);
    m_label = m_screenName;
    m_labelWidth = 0;
    m_nameWidth = 0;
    if (m_font != NULL) {
        m_labelWidth = m_font->MeasureTextWidth(m_label.c_str(), -1, -1, NULL);
        m_nameWidth = m_font->MeasureTextWidth(m_levelText.c_str(), -1, -1, NULL);
    }
    SetPressImage("IDB_GPS_ROUND_ICON_ANIM");
    UpdateState();
}

CGPSHaunt::~CGPSHaunt()
{
    SAFE_DELETE_VECTOR(m_cameras);
}

void CGPSHaunt::ParseNode(TiXmlNode* node)
{
    m_name = CXmlHelper::GetAttributeValue(node, "name");
    m_screenName = CXmlHelper::GetResString(node, "screenName");
    ParseInfo(node->FirstChild("info"));
    for (TiXmlNode* child = node->FirstChild("camera"); child != NULL; child = child->NextSibling("camera")) {
        m_cameras.addElement(new CGPSHauntCamera(m_name, child));
    }
}

void CGPSHaunt::ParseInfo(TiXmlNode* node)
{
    XString image = CXmlHelper::GetAttributeValue(node, "image");
    SetImage(XString::AnsiString(image.c_str()));
    int x = CXmlHelper::GetAttributeValue(node, "posX").ToInt();
    int y = CXmlHelper::GetAttributeValue(node, "posY").ToInt();
    m_description = CXmlHelper::GetResString(node, "description");
    SetPos(x, y);
}

void CGPSHaunt::Update(long elapsed)
{
    for (int index = 0; index < m_cameras.size(); ++index) {
        m_cameras[index]->Update(elapsed);
    }
}

void CGPSHaunt::UpdateState()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    int level = CBH_Player::GetInstance()->GetLevel();
    m_visible = true;
    m_state = 2;
    if (level < m_unlockLevel) {
        int threshold = app->m_helper->m_config.GetIntegerValueByToken("LEVEL_DIFF_HAUNT_BECOME_VISIBLE", true);
        m_visible = m_unlockLevel - level <= threshold;
        m_state = 1;
        if (!m_visible) {
            m_state = 0;
        }
    }
    for (int index = 0; index < m_cameras.size(); ++index) {
        m_cameras[index]->UpdateState();
    }
}

void CGPSHaunt::Paint(ICGraphics2d& graphics, int mapX, int mapY)
{
    if (m_visible) {
        App* app = static_cast<App*>(WindowApp::m_instance);
        int x = mapX + GetX();
        int y = mapY + GetY();
        if (m_image != NULL) {
            app->m_helper->DrawImage(m_image, x, y, 1.0f, 0, 0, 0.0f, 0);
        }
        if (m_font != NULL) {
            m_font->PaintText(m_label.c_str(), -1, x + (m_width - m_labelWidth) / 2,
                              y + m_height, m_labelWidth, m_font->GetFontHeight());
        }
        CGPSItem::Paint(graphics, mapX, mapY);
    }
    for (int index = 0; index < m_cameras.size(); ++index) {
        m_cameras[index]->Paint(graphics, mapX, mapY);
    }
}

void CGPSHaunt::OnClick()
{
    if (m_listener != NULL) {
        m_listener->OnHauntSelected(this);
    }
}

bool CGPSHaunt::CheckHit(int x, int y, CGPSItem** item)
{
    for (int index = 0; index < m_cameras.size(); ++index) {
        if (m_cameras[index]->CheckHit(x, y, item)) {
            return true;
        }
    }
    return m_state != 0 && CGPSItem::CheckHit(x, y, item);
}

void CGPSHaunt::SetHauntListener(CGPSHauntListener* listener)
{
    m_listener = listener;
}

void CGPSHaunt::Save(TiXmlElement* parent)
{
    TiXmlElement* savedHaunt = parent->FirstChildElement("gps_haunt");
    while (savedHaunt != NULL && !(CXmlHelper::GetAttributeValue(savedHaunt, "name") == m_name)) {
        savedHaunt = savedHaunt->NextSiblingElement("gps_haunt");
    }
    if (savedHaunt == NULL) {
        savedHaunt = new TiXmlElement("gps_haunt");
        savedHaunt->SetAttribute("name", XString::AnsiString(m_name.c_str()));
        parent->LinkEndChild(savedHaunt);
    }
    for (int index = 0; index < m_cameras.size(); ++index) {
        m_cameras[index]->Save(savedHaunt);
    }
}

void CGPSHaunt::Load(TiXmlElement* parent)
{
    for (TiXmlElement* haunt = parent->FirstChildElement("gps_haunt"); haunt != NULL;
         haunt = haunt->NextSiblingElement("gps_haunt")) {
        if (CXmlHelper::GetAttributeValue(haunt, "name") == m_name) {
            LoadCameras(haunt);
            break;
        }
    }
    UpdateState();
}

void CGPSHaunt::LoadCameras(TiXmlElement* parent)
{
    for (int index = 0; index < m_cameras.size(); ++index) {
        m_cameras[index]->Load(parent);
    }
}

CGPSHauntCamera* CGPSHaunt::GetCameraById(int id)
{
    for (int index = 0; index < m_cameras.size(); ++index) {
        CGPSHauntCamera* camera = m_cameras[index];
        if (camera->m_id == id) {
            return camera;
        }
    }
    return NULL;
}

typedef char AssertGPSHauntSize[sizeof(CGPSHaunt) == 0x84 ? 1 : -1];
