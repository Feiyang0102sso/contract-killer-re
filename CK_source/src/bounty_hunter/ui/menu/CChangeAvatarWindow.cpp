#include "CChangeAvatarWindow.h"

#include "App.h"
#include "CAutoArrangedWindow.h"
#include "CAvatar.h"
#include "CAvatarManager.h"
#include "CBH_MenuButton.h"
#include "CBH_Player.h"
#include "CFontMgr.h"
#include "CImageButton.h"
#include "CScrollWindow.h"
#include "CTutorialManager.h"

namespace {
const int SelectAvatar = 0x665212fc;
}

CChangeAvatarWindow::CChangeAvatarWindow()
    : CBaseScreen(false, true, true, false, true, false)
{
    SetTitleText(ResString("IDS_CHANGE_AVATAR_TITLE"));
    CScrollWindow* scroll = new CScrollWindow(false);
    m_content->AddToFront(scroll);
    CAutoArrangedWindow* page = new CAutoArrangedWindow(
        scroll->m_width, static_cast<App*>(WindowApp::m_instance)->m_screenHeight - 20 - GetTitleBarHeight());
    scroll->Add(page);

    m_selectedId = CBH_Player::GetInstance()->m_avatar->m_id;
    for (int index = 0;
         index < static_cast<App*>(WindowApp::m_instance)->InitAvatarManager()->m_avatars.size(); ++index) {
        CAvatarManager* manager = static_cast<App*>(WindowApp::m_instance)->InitAvatarManager();
        const CAvatar* avatar = manager->GetAvatar(index);
        WindowTransparent* cell = new WindowTransparent(false);
        CImageButton* button = new CImageButton();
        button->m_command = 0;
        button->m_commandParameter1 = 0;
        button->m_commandParameter2 = 0;
        button->m_commandParameter3 = 0;
        button->AddImage(avatar->m_imageResId, 0, 0);

        m_selectedFrame = new ImageWindow();
        m_selectedFrame->SetImageById("IDB_FRAME_UNLOCKED");
        button->AddToFront(m_selectedFrame);
        button->SetHeightByContent(0, 0);
        button->SetWidthByContent(0, 0);
        int unlockLevel = avatar->m_unlockLvl;
        int playerLevel = CBH_Player::GetInstance()->GetLevel();
        if (unlockLevel > playerLevel) {
            button->AddImage("IDB_FRAME_LOCKED", 0, 0);
        } else {
            int avatarId = avatar->m_id;
            int selectedId = m_selectedId;
            button->m_command = SelectAvatar;
            button->m_commandParameter1 = index;
            button->m_commandParameter2 = reinterpret_cast<int>(button);
            button->m_commandParameter3 = 0;
            if (avatarId == selectedId) {
                m_selectedButton = button;
                button->m_firstChild->ClearFlags(FLAG_VISIBLE | FLAG_ENABLED);
            }
        }
        cell->AddToFront(button);

        XString label = XString::Format(L"LVL %d", avatar->m_unlockLvl);
        TextWindow* level = new TextWindow(label, CFontMgr::GetInstance()->GetFont(11));
        level->SetAlign(ALIGN_HCENTER | ALIGN_BOTTOM);
        level->SetSize(0, 0);
        cell->AddToFront(level);
        int labelWidth = level->m_width;
        int labelHeight = level->m_height;
        int buttonWidth = button->m_width;
        int buttonHeight = button->m_height;
        if (buttonWidth > labelWidth) {
            labelWidth = buttonWidth;
        }
        cell->SetSize(labelWidth, buttonHeight + labelHeight);
        if (!page->Add(cell)) {
            scroll->Add(page);
            page = new CAutoArrangedWindow(
                scroll->m_width, static_cast<App*>(WindowApp::m_instance)->m_screenHeight - 20 - GetTitleBarHeight());
            page->Add(cell);
        }
    }
    scroll->Add(page);
}

void CChangeAvatarWindow::OnCommand(Event& event)
{
    // the original does not check the avatar level, keeps the event and always passes it on
    if (event.m_id == SelectAvatar) {
        m_selectedId = event.m_param1;
        m_selectedButton->m_firstChild->SetFlags(FLAG_VISIBLE | FLAG_ENABLED);
        CImageButton* button = reinterpret_cast<CImageButton*>(event.m_param2);
        button->m_firstChild->ClearFlags(FLAG_VISIBLE | FLAG_ENABLED);
        m_selectedButton = button;
        CAvatar* avatar = static_cast<App*>(WindowApp::m_instance)->InitAvatarManager()->GetAvatar(m_selectedId);
        CBH_Player::GetInstance()->SetAvatar(avatar);
        WindowApp::HandleTunnelCommand(AvatarChanged, 0, 0, 0);
    }
    CBaseScreen::OnCommand(event);
}

void CChangeAvatarWindow::Update()
{
    if (static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial() && static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsAvatarChoiceCloseBlink()) {
        m_back->SetBlinking(true);
    }
}

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char CKChangeAvatarWindowSize[sizeof(CChangeAvatarWindow) == 0xe4 ? 1 : -1];
#endif
