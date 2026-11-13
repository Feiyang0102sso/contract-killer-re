#include "CGPSHaunt.h"

#include "App.h"
#include "CBH_Player.h"
#include "CFont.h"
#include "CFontMgr.h"
#include "CGPSHauntCamera.h"
#include "CTutorialManager.h"
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
    // Original 0x127964: both widths come from DGHelper::GetTextWidth.
    m_font = CFontMgr::GetInstance()->GetFont(GAME_FONT_DEFAULT);
    m_nameWidth = static_cast<App*>(WindowApp::m_instance)->m_helper->GetTextWidth(
        XString::AnsiString(m_levelText.c_str()), m_font);
    m_label = m_screenName;
    m_labelWidth = static_cast<App*>(WindowApp::m_instance)->m_helper->GetTextWidth(
        XString::AnsiString(m_label.c_str()), m_font);
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
    if (m_cameras.size() == 0) {
        return;
    }
    unsigned int index = 0;
    do {
        m_cameras[index++]->Update(elapsed);
    } while (index < static_cast<unsigned int>(m_cameras.size()));
}

void CGPSHaunt::UpdateState()
{
    // Original 0x128050
    if (static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial() == true
        && !static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsHauntVisible(m_name)) {
        m_state = 0;
        m_visible = false;
        return;
    }
    m_visible = true;
    int levelDiff = CBH_Player::GetInstance()->GetLevel() - m_unlockLevel;
    if (levelDiff >= 0) {
        m_state = 2;
    } else if (levelDiff >= -static_cast<App*>(WindowApp::m_instance)->m_helper->m_config.GetIntegerValueByToken(
                   "LEVEL_DIFF_HAUNT_BECOME_VISIBLE", true)) {
        m_state = 1;
    } else {
        m_state = 0;
        m_visible = false;
    }
    for (int index = 0; index < m_cameras.size(); ++index) {
        m_cameras[index]->UpdateState();
    }
}

void CGPSHaunt::Paint(ICGraphics2d& graphics, int mapX, int mapY)
{
    // Original 0x127bd8: hidden haunts (state 0) skip the icon; states 1 and 2 draw the icon and
    // the label. The label y uses GetWidth() (original; the icons are square).
    if (m_state != 0) {
        if (m_state == 1 || m_state == 2) {
            static_cast<App*>(WindowApp::m_instance)->m_helper->DrawImage(
                GetImage(), GetX() + mapX, GetY() + mapY, 1.0f, 0, 0, 0.0f, 0);
            m_font->PaintText(m_label.c_str(), -1, GetX() + mapX + GetWidth() / 2 - m_labelWidth / 2,
                              GetY() + mapY + GetWidth(), -1, -1);
            CRectangle label(GetX() + GetWidth() / 2 - m_labelWidth / 2, GetY() + GetHeight(),
                             m_labelWidth, m_font->GetFontHeight());
            setSubItem(0, label, mapX, mapY);
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
    // Original 0x127f10: a fresh element is filled, copied into the parent and deleted
    // (CBH_GPSMap::Save clears the map element first).
    TiXmlElement* savedHaunt = new TiXmlElement("gps_haunt");
    savedHaunt->SetAttribute("name", XString::AnsiString(m_name.c_str()));
    for (int index = 0; index < m_cameras.size(); ++index) {
        m_cameras[index]->Save(savedHaunt);
    }
    parent->InsertEndChild(*savedHaunt);
    delete savedHaunt;
}

void CGPSHaunt::Load(TiXmlElement* parent)
{
    // Original 0x127f94: every matching element is loaded (no break).
    TiXmlElement* haunt = parent->FirstChildElement("gps_haunt");
    while (haunt != NULL) {
        XString name = CXmlHelper::GetAttributeValue(haunt, "name");
        if (name == m_name) {
            LoadCameras(haunt);
        }
        haunt = haunt->NextSiblingElement("gps_haunt");
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
