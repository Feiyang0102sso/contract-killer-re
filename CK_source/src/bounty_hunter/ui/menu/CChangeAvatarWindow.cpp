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
    : CBaseScreen(false, true, true, false, true, false),
      m_selectedId(-1), m_selectedFrame(NULL), m_selectedButton(NULL)
{
    SetTitleText(ResString("IDS_CHANGE_AVATAR_TITLE"));
    App* app = static_cast<App*>(WindowApp::m_instance);
    CAvatarManager* manager = app->InitAvatarManager();
    CBH_Player* player = CBH_Player::GetInstance();
    if (player->m_avatar != NULL) {
        m_selectedId = player->m_avatar->m_id;
    }

    int pageWidth = m_content->m_width;
    int pageHeight = app->m_screenHeight - 20 - GetTitleBarHeight();
    CScrollWindow* scroll = new CScrollWindow(false);
    scroll->SetSize(pageWidth, m_content->m_height);
    m_content->AddToFront(scroll);
    CAutoArrangedWindow* page = new CAutoArrangedWindow(pageWidth, pageHeight);
    scroll->Add(page);

    CFont* font = CFontMgr::GetInstance()->GetFont(11);
    for (int index = 0; index < manager->m_avatars.size(); ++index) {
        const CAvatar* avatar = manager->GetAvatar(index);
        if (avatar == NULL || avatar->m_imageResId == NULL) {
            continue;
        }

        WindowTransparent* cell = new WindowTransparent(false);
        CImageButton* button = new CImageButton();
        button->m_command = 0;
        button->m_commandParameter1 = 0;
        button->m_commandParameter2 = 0;
        button->m_commandParameter3 = 0;
        button->AddImage(avatar->m_imageResId, 0, 0);

        ImageWindow* frame = new ImageWindow();
        frame->SetImageById("IDB_FRAME_UNLOCKED");
        button->AddToFront(frame);
        button->SetHeightByContent(0, 0);
        button->SetWidthByContent(0, 0);
        if (player->GetLevel() < avatar->m_unlockLvl) {
            button->AddImage("IDB_FRAME_LOCKED", 0, 0);
        } else {
            button->m_command = SelectAvatar;
            button->m_commandParameter1 = index;
            button->m_commandParameter2 = reinterpret_cast<int>(button);
            if (avatar->m_id == m_selectedId) {
                frame->ClearFlags(FLAG_VISIBLE | FLAG_ENABLED);
                m_selectedFrame = frame;
                m_selectedButton = button;
            }
        }
        cell->AddToFront(button);

        TextWindow* level = new TextWindow(XString::Format(L"LVL %d", avatar->m_unlockLvl), font);
        level->SetAlign(ALIGN_HCENTER | ALIGN_BOTTOM);
        level->SetSize(0, 0);
        cell->AddToFront(level);
        int cellWidth = button->m_width;
        if (level->m_width > cellWidth) {
            cellWidth = level->m_width;
        }
        cell->SetSize(cellWidth, button->m_height + level->m_height);
        if (!page->Add(cell)) {
            scroll->Add(page);
            page = new CAutoArrangedWindow(pageWidth, pageHeight);
            page->Add(cell);
        }
    }
    scroll->Add(page);
}

void CChangeAvatarWindow::OnCommand(Event& event)
{
    if (event.m_id == SelectAvatar) {
        App* app = static_cast<App*>(WindowApp::m_instance);
        CAvatar* avatar = app->InitAvatarManager()->GetAvatar(event.m_param1);
        CBH_Player* player = CBH_Player::GetInstance();
        CImageButton* button = reinterpret_cast<CImageButton*>(event.m_param2);
        if (avatar != NULL && button != NULL && player->GetLevel() >= avatar->m_unlockLvl
            && button != m_selectedButton) {
            if (m_selectedFrame != NULL) {
                m_selectedFrame->SetFlags(FLAG_VISIBLE | FLAG_ENABLED);
            }
            m_selectedId = avatar->m_id;
            m_selectedButton = button;
            m_selectedFrame = static_cast<ImageWindow*>(button->m_firstChild);
            m_selectedFrame->ClearFlags(FLAG_VISIBLE | FLAG_ENABLED);
            player->SetAvatar(avatar);
            WindowApp::HandleTunnelCommand(AvatarChanged, 0, 0, 0);
        }
        event.Clear();
        return;
    }
    CBaseScreen::OnCommand(event);
}

void CChangeAvatarWindow::Update()
{
    if (CTutorialManager::IsPlayerInTutorial() && CTutorialManager::IsAvatarChoiceCloseBlink()) {
        m_back->SetBlinking(true);
    }
}

typedef char CKChangeAvatarWindowSize[sizeof(CChangeAvatarWindow) == 0xe4 ? 1 : -1];
