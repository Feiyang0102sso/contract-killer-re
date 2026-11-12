#include "CTutorialManager.h"

#include "App.h"
#include "CBH_Player.h"
#include "CDialogWindow.h"

namespace {
enum TutorialState {
    FirstMission = 0, BuyBullets, BulletsBought, FirstMap, SecondMission, BuyWeapon,
    WeaponBought, GoHome, CustomizeAvatar, OpenWeaponChoice, ChooseRifle, BackToMap,
    ShowRandomInformants, TutorialFinished
};

int TutorialStep()
{
    return CBH_Player::GetInstance()->GetTutorialState();
}

const char TutorialTitle[] = "IDS_TUTORIAL_TITLE";
const char BulletsBlocked[] = "IDS_TUTORIAL_BUY_BULLETS_BLOCKED_ELEMENT";
}

CTutorialManager::CTutorialManager() : Window(false), m_lastMessage(NULL)
{
}

bool CTutorialManager::IsPlayerInTutorial()
{
    return CBH_Player::GetInstance()->GetTutorialState() < TutorialFinished;
}

bool CTutorialManager::IsAutoStartMission()
{
    return CBH_Player::GetInstance()->GetTutorialState() == FirstMission;
}

void CTutorialManager::NextTutorialStep()
{
    CBH_Player* player = CBH_Player::GetInstance();
    int current = player->GetTutorialState();
    if (current == TutorialFinished) {
        return;
    }
    int next = current + 1;
    if (current == SecondMission && player->m_gameStats.m_weaponsEarned > 1) {
        next = WeaponBought;
    } else if (current == ShowRandomInformants) {
        WindowApp::HandleTunnelCommand(0x3da61159, 0, 0, 0);
    }
    if (next == CustomizeAvatar) {
        next = OpenWeaponChoice;
    }
    player->SetTutorialState(next);
    ShowStateInitMessage();
    WindowApp::HandleTunnelCommand(-0x5b2f2edf, 0, 0, 0);
}

void CTutorialManager::ShowStateInitMessage()
{
    const char* message = NULL;
    switch (TutorialStep()) {
    case BulletsBought: message = "IDS_TUTORIAL_BUY_BULLETS_COMPLETE"; break;
    case FirstMap: message = "IDS_TUTORIAL_CITY_MAP_FIRST_SEEN"; break;
    case GoHome: message = "IDS_TUTORIAL_GO_HOME"; break;
    case CustomizeAvatar: message = "IDS_TUTORIAL_CUSTOMIZE_AVATAR"; break;
    case ChooseRifle: message = "IDS_TUTORIAL_CHOOSE_RIFLE"; break;
    default: return;
    }
    ShowMessage(TutorialTitle, message, false);
}

void CTutorialManager::ShowMessage(const char* title, const char* message, bool once)
{
    if (once && m_lastMessage == message) {
        return;
    }
    m_lastMessage = message;
    CDialogWindow* dialog = new CDialogWindow(true);
    if (TutorialStep() < CustomizeAvatar) {
        dialog->SetExtraImageFromImageID("IDB_DIALOG_TUTORIAL");
    }
    dialog->SetTitleText(ResString(title));
    dialog->SetMessageText(ResString(message));
    dialog->SetButtons(CDialogWindow::BUTTON_OK);
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_mainLayer->AddModal(dialog);
}

void CTutorialManager::ShowStateBaseMessage()
{
    switch (TutorialStep()) {
    case BuyWeapon:
        ShowMessage(TutorialTitle, "IDS_TUTORIAL_BUY_NEW_WEAPON", false);
        break;
    case OpenWeaponChoice:
        ShowMessage(TutorialTitle, "IDS_TUTORIAL_PRESS_ON_WEAPON", true);
        break;
    case BackToMap:
        ShowMessage(TutorialTitle, "IDS_TUTORIAL_GO_BACK_TO_MAP", true);
        break;
    default:
        ShowStateInitMessage();
        break;
    }
}

void CTutorialManager::OnCommand(Event& event)
{
    int state = TutorialStep();
    switch (event.m_id) {
    case 0x45f9bf40:
        if (state == FirstMission || state == SecondMission) {
            NextTutorialStep();
        }
        break;
    case -0x44b7c071:
        if (state == BuyBullets) {
            NextTutorialStep();
        }
        break;
    case -0x65bedd6e:
        if (state == BuyWeapon && event.m_param1 == 0) {
            NextTutorialStep();
        }
        return;
    case -0x64191a07:
        if (state == CustomizeAvatar) {
            ShowStateInitMessage();
        } else if (state == OpenWeaponChoice) {
            m_lastMessage = NULL;
            ShowStateBaseMessage();
        }
        // The home command also runs the close-shop tutorial checks.
    case 0x4d5de318:
        if (state == BulletsBought || state == WeaponBought) {
            NextTutorialStep();
        } else if (state == BuyBullets) {
            ShowMessage(TutorialTitle, BulletsBlocked, false);
        } else if (state == BuyWeapon) {
            ShowStateBaseMessage();
        }
        break;
    case -0x21e08bf9:
        if (state == BuyBullets) {
            ShowMessage(TutorialTitle, BulletsBlocked, false);
        } else if (state == BuyWeapon) {
            ShowStateBaseMessage();
        }
        break;
    case -0x7b79c896:
        if (state == BuyBullets) {
            ShowMessage(TutorialTitle, "IDS_TUTORIAL_BUY_BULLETS", false);
        } else if (state == BuyWeapon) {
            ShowStateBaseMessage();
        }
        break;
    case -0x2e800a2c:
        m_lastMessage = NULL;
        ShowStateBaseMessage();
        return;
    case -0x2051ee21:
        if (state == FirstMap) {
            NextTutorialStep();
        }
        break;
    case -0x4e79c923:
        if (state == GoHome) {
            NextTutorialStep();
        }
        break;
    case -0x4b9aa0d3:
        if (state == OpenWeaponChoice) {
            ShowStateBaseMessage();
        }
        break;
    case 0x33e070cb:
        if (state == CustomizeAvatar) {
            NextTutorialStep();
        }
        break;
    case 0x21d908a0:
        if (state == OpenWeaponChoice) {
            NextTutorialStep();
        }
        break;
    case 0x0df6b618:
        if (state == ChooseRifle) {
            NextTutorialStep();
        }
        break;
    case 0x6e6446f5:
        if (state == ChooseRifle) {
            ShowMessage(TutorialTitle, "IDS_TUTORIAL_CHOOSE_RIFLE", false);
        }
        break;
    case -0x72ce08c6:
        if (state == BackToMap) {
            NextTutorialStep();
        }
        break;
    case 0x40067282:
        if (state == BackToMap) {
            ShowMessage(TutorialTitle, "IDS_TUTORIAL_MAP_RANDOM_INFORMANTS", true);
        } else if (state == ShowRandomInformants) {
            ShowMessage(TutorialTitle, "IDS_TUTORIAL_FINISH", true);
            NextTutorialStep();
        }
        break;
    default:
        return;
    }
    event.Clear();
}

bool CTutorialManager::IsShowShop()
{
    unsigned int state = CBH_Player::GetInstance()->GetTutorialState();
    if (state < 7) {
        return ((1 << state) & 0x66) != 0;
    }
    return false;
}

bool CTutorialManager::IsTabLocked(CShopWindow::ETab tab)
{
    int state = TutorialStep();
    if (state == BuyWeapon) {
        return tab != CShopWindow::TAB_WEAPONS;
    }
    if (state == BuyBullets) {
        return tab != CShopWindow::TAB_AMMO;
    }
    return false;
}

bool CTutorialManager::IsShowHome()
{
    return (unsigned int)(TutorialStep() - CustomizeAvatar) < 3;
}

int CTutorialManager::GetCurrentShopTab()
{
    if ((unsigned int)(TutorialStep() - BuyBullets) < 4) {
        return 3;
    }
    return 1;
}

bool CTutorialManager::CanCloseShop()
{
    int state = TutorialStep();
    return state != BuyBullets && state != BuyWeapon;
}

bool CTutorialManager::CanCloseNetwork()
{
    return (unsigned int)(TutorialStep() - CustomizeAvatar) > 2;
}

bool CTutorialManager::CanCloseWeaponChoice()
{
    return TutorialStep() != ChooseRifle;
}

bool CTutorialManager::CanChooseWeapon()
{
    return TutorialStep() > CustomizeAvatar;
}

bool CTutorialManager::IsHauntVisible(const XString&)
{
    return true;
}

bool CTutorialManager::IsCameraVisible(int)
{
    return true;
}

bool CTutorialManager::IsInstantInformantVisible(int)
{
    return TutorialStep() > ChooseRifle;
}

bool CTutorialManager::IsMissionInformantVisible(int)
{
    return true;
}

bool CTutorialManager::IsHomeVisible()
{
    return TutorialStep() > SecondMission;
}

void CTutorialManager::Update()
{
    CBH_Player::GetInstance();
}

bool CTutorialManager::IsShopCloseBlink()
{
    return (TutorialStep() | 4) == 6;
}

bool CTutorialManager::IsNetworkCloseBlink()
{
    return TutorialStep() == BackToMap;
}

bool CTutorialManager::IsWeaponChoiceCloseBlink()
{
    return TutorialStep() == BackToMap;
}

bool CTutorialManager::IsWeaponChoicePressBlink()
{
    return (unsigned int)(TutorialStep() - OpenWeaponChoice) < 2;
}

bool CTutorialManager::IsAvatarChoiceCloseBlink()
{
    return TutorialStep() == OpenWeaponChoice;
}

bool CTutorialManager::IsAvatarChoicePressBlink()
{
    return TutorialStep() == CustomizeAvatar;
}

bool CTutorialManager::IsMissionInformantBlink(int)
{
    return TutorialStep() == FirstMap;
}

bool CTutorialManager::IsInstantInformantBlink(int informant)
{
    return TutorialStep() == BackToMap && informant == 0;
}

bool CTutorialManager::IsMapShopBlink()
{
    return false;
}

bool CTutorialManager::IsMapNetworkBlink()
{
    return TutorialStep() == GoHome;
}

bool CTutorialManager::IsAmmoBlink(int)
{
    return TutorialStep() == BuyBullets;
}

bool CTutorialManager::IsWeaponBlink(int weapon)
{
    return TutorialStep() == BuyWeapon && weapon == 0;
}

bool CTutorialManager::IsWeaponButtonBlink(int button)
{
    return TutorialStep() == ChooseRifle && button == 0;
}

bool CTutorialManager::IsTutorialInstantInformant(int informant)
{
    return informant == 0;
}

bool CTutorialManager::IsMenuBarBlocked()
{
    return TutorialStep() < TutorialFinished;
}

bool CTutorialManager::IsShowObjective()
{
    return TutorialStep() != FirstMission;
}

typedef char AssertTutorialManagerSize[sizeof(CTutorialManager) == 0xa8 ? 1 : -1];
