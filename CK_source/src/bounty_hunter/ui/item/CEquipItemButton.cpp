// CEquipItemButton.cpp - show the equipped and owned states of an item.
#include "CEquipItemButton.h"

#include "BaseDialog.h"
#include "CAmmo.h"
#include "CBH_Player.h"
#include "CDH_Weapon.h"
#include "CFontMgr.h"
#include "ServiceWindows.h"

namespace {
const int UnlimitedAmmo = 0x0fffffff;
}

CEquipItemButton::CEquipItemButton()
    : m_clickable(true), m_available(true), m_image(NULL), m_bottomText(NULL), m_count(NULL)
{
}

CEquipAmmoButton::CEquipAmmoButton(const CDH_Weapon* weapon, const CAmmo* ammo)
    : m_weapon(weapon), m_ammo(ammo)
{
    XString image = ammo->m_equipButton;
    XString::AnsiString imageId(image.c_str());
    XString label("");
    XString icon = ammo->m_bulletIcon;
    XString::AnsiString iconId(icon.c_str());
    int count = CBH_Player::GetInstance()->GetAvailableAmmo(ammo, true);
    CreateUI(imageId, label, NULL, iconId, count);
    UpdateStatus();
}

Window* CEquipItemButton::CreateUI(const char* image, const XString&, CFont*, const char* countIcon, int count)
{
    Window* contents = new Window(false);
    m_image = new ImageWindow();
    if (image != NULL) {
        m_image->SetFillType(ImageWindow::FILL_NONE);
        m_image->SetImageById(image);
        m_image->SetLayoutType(LAYOUT_SIMPLE);
        m_image->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
    }
    contents->AddToFront(m_image);
    m_bottomText = new TextWindow();
    contents->AddToFront(m_bottomText);
    if (count != -1) {
        m_count = new LabelWithIcon(countIcon, XString(count), true, NULL, 0);
        m_count->SetLayoutType(LAYOUT_SIMPLE);
        m_count->SetAlign(ALIGN_RIGHT | ALIGN_TOP);
        m_image->AddToFront(m_count);
    }
    AddToFront(contents);
    SetAvailable(m_available);
    return contents;
}

void CEquipAmmoButton::UpdateStatus()
{
    if (m_weapon->GetActiveAmmo() == m_ammo) {
        XString equipped = ResString("IDS_EQUIPED");
        SetBottomText(equipped, CFontMgr::GetInstance()->GetFont(14));
        SetItemCount(CBH_Player::GetInstance()->GetAvailableAmmo(m_ammo, true));
        SetClickable(false);
        SetAvailable(true);
        return;
    }
    int count = CBH_Player::GetInstance()->GetAvailableAmmo(m_ammo, true);
    if (count < 1) {
        SetAvailable(false);
        SetClickable(false);
        SetBottomText(XString(""), NULL);
        return;
    }
    XString owned = ResString("IDS_OWNED");
    SetBottomText(owned, CFontMgr::GetInstance()->GetFont(13));
    SetItemCount(CBH_Player::GetInstance()->GetAvailableAmmo(m_ammo, true));
    SetClickable(true);
    SetAvailable(false);
}

void CEquipAmmoButton::OnPointerReleased(Event& event)
{
    CEquipItemButton::OnPointerReleased(event);
}

void CEquipItemButton::OnPointerReleased(Event&)
{
    if (m_clickable) {
        SetAvailable(!m_available);
    }
}

CEquipItemButton::CEquipItemButton(const char* image, const XString& text, CFont* font, const char* countIcon, int count)
    : m_clickable(true), m_available(true), m_image(NULL), m_bottomText(NULL), m_count(NULL)
{
    CreateUI(image, text, font, countIcon, count);
}

void CEquipItemButton::SetClickable(bool clickable)
{
    m_clickable = clickable;
}

void CEquipItemButton::SetAvailable(bool available)
{
    m_available = available;
    if (m_image != NULL) {
        float opacity = 0.5f;
        if (available) {
            opacity = 1.0f;
        }
        m_image->SetOpacity(opacity);
    }
}

void CEquipItemButton::SetBottomText(const XString& text, CFont* font)
{
    if (font != NULL) {
        m_bottomText->SetFont(font);
    }
    m_bottomText->SetWidthByContent(0, 0);
    m_bottomText->SetHeightByContent(0, 0);
    m_bottomText->SetLayoutType(LAYOUT_SIMPLE);
    m_bottomText->SetAlign(ALIGN_HCENTER | ALIGN_BOTTOM);
    m_bottomText->SetText(text);
}

void CEquipItemButton::SetItemCount(int count)
{
    if (count == UnlimitedAmmo) {
        m_count->SetText(XString(L"\u221e"));
    } else {
        m_count->SetText(XString(count));
    }
}

int CEquipItemButton::GetIconHeight() const
{
    return ImageHeight(m_image->m_surface);
}
