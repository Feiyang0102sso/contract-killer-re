#include "CTutorialManager.h"

#include "App.h"
#include "CBH_Player.h"
#include "CDialogWindow.h"
#include "CGameAnalytics.h"

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

const char* const TutorialTitle = "IDS_TUTORIAL_TITLE";
const char* const BulletsBlocked = "IDS_TUTORIAL_BUY_BULLETS_BLOCKED_ELEMENT";
}

CTutorialManager::CTutorialManager() : WindowTransparent(false)
{
    // the original constructor does not set m_lastMessage
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
    int current = TutorialStep();
    if (current == TutorialFinished) {
        return;
    }
    int next = current + 1;
    switch (current) {
    case ShowRandomInformants:
        WindowApp::HandleTunnelCommand(0x3da61159, 0, 0, 0);
        CGameAnalytics::logTutorialEvents(CGameAnalytics::TUTORIAL_EVENT_FINISHED);
        next = TutorialFinished;
        break;
    case SecondMission:
        if (CBH_Player::GetInstance()->m_gameStats.m_weaponsEarned > 1) {
            next = current + 2;  // skip BuyWeapon
        }
        break;
    }
    if (next == CustomizeAvatar) {
        next++;  // skip CustomizeAvatar
    }
    CBH_Player* player = CBH_Player::GetInstance();
    player->m_tutorialState = next;
    player->Save();
    ShowStateInitMessage();
    WindowApp::HandleTunnelCommand(-0x5b2f2edf, 0, 0, 0);
}

void CTutorialManager::ShowStateInitMessage()
{
    switch (TutorialStep()) {
    case BulletsBought:
        ShowMessage(TutorialTitle, "IDS_TUTORIAL_BUY_BULLETS_COMPLETE", false);
        break;
    case FirstMap:
        ShowMessage(TutorialTitle, "IDS_TUTORIAL_CITY_MAP_FIRST_SEEN", false);
        break;
    case GoHome:
        ShowMessage(TutorialTitle, "IDS_TUTORIAL_GO_HOME", false);
        break;
    case CustomizeAvatar:
        ShowMessage(TutorialTitle, "IDS_TUTORIAL_CUSTOMIZE_AVATAR", false);
        break;
    case ChooseRifle:
        ShowMessage(TutorialTitle, "IDS_TUTORIAL_CHOOSE_RIFLE", false);
        break;
    }
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
    case BulletsBought:
        ShowMessage(TutorialTitle, "IDS_TUTORIAL_BUY_BULLETS_COMPLETE", false);
        break;
    case FirstMap:
        ShowMessage(TutorialTitle, "IDS_TUTORIAL_CITY_MAP_FIRST_SEEN", false);
        break;
    case BuyWeapon:
        ShowMessage(TutorialTitle, "IDS_TUTORIAL_BUY_NEW_WEAPON", false);
        break;
    case GoHome:
        ShowMessage(TutorialTitle, "IDS_TUTORIAL_GO_HOME", false);
        break;
    case CustomizeAvatar:
        ShowMessage(TutorialTitle, "IDS_TUTORIAL_CUSTOMIZE_AVATAR", false);
        break;
    case OpenWeaponChoice:
        ShowMessage(TutorialTitle, "IDS_TUTORIAL_PRESS_ON_WEAPON", true);
        break;
    case ChooseRifle:
        ShowMessage(TutorialTitle, "IDS_TUTORIAL_CHOOSE_RIFLE", false);
        break;
    case BackToMap:
        ShowMessage(TutorialTitle, "IDS_TUTORIAL_GO_BACK_TO_MAP", true);
        break;
    }
}

void CTutorialManager::OnCommand(Event& event)
{
    switch (event.m_id) {
    case 0x45f9bf40:
        if (TutorialStep() == FirstMission || TutorialStep() == SecondMission) {
            NextTutorialStep();
        }
        break;
    case -0x44b7c071:
        if (TutorialStep() == BuyBullets) {
            NextTutorialStep();
        }
        break;
    case -0x65bedd6e:
        if (TutorialStep() == BuyWeapon && event.m_param1 == 0) {
            NextTutorialStep();
        }
        return;
    case -0x64191a07:
        if (TutorialStep() == CustomizeAvatar) {
            ShowStateInitMessage();
        } else if (TutorialStep() == OpenWeaponChoice) {
            m_lastMessage = NULL;
            ShowStateBaseMessage();
        }
        // The home command also runs the close-shop tutorial checks.
    case 0x4d5de318:
        if (TutorialStep() == BulletsBought || TutorialStep() == WeaponBought) {
            NextTutorialStep();
        } else if (TutorialStep() == BuyBullets) {
            ShowMessage(TutorialTitle, BulletsBlocked, false);
        } else if (TutorialStep() == BuyWeapon) {
            ShowStateBaseMessage();
        }
        break;
    case -0x21e08bf9:
        if (TutorialStep() == BuyBullets) {
            ShowMessage(TutorialTitle, BulletsBlocked, false);
        } else if (TutorialStep() == BuyWeapon) {
            ShowStateBaseMessage();
        }
        break;
    case -0x7b79c896:
        if (TutorialStep() == BuyBullets) {
            ShowMessage(TutorialTitle, "IDS_TUTORIAL_BUY_BULLETS", false);
        } else if (TutorialStep() == BuyWeapon) {
            ShowStateBaseMessage();
        }
        break;
    case -0x2e800a2c:
        m_lastMessage = NULL;
        ShowStateBaseMessage();
        return;
    case -0x2051ee21:
        if (TutorialStep() == FirstMap) {
            NextTutorialStep();
        }
        break;
    case -0x4e79c923:
        if (TutorialStep() == GoHome) {
            NextTutorialStep();
        }
        break;
    case -0x4b9aa0d3:
        if (TutorialStep() == OpenWeaponChoice) {
            ShowStateBaseMessage();
        }
        break;
    case 0x33e070cb:
        if (TutorialStep() == CustomizeAvatar) {
            NextTutorialStep();
        }
        break;
    case 0x21d908a0:
        if (TutorialStep() == OpenWeaponChoice) {
            NextTutorialStep();
        }
        break;
    case 0x0df6b618:
        if (TutorialStep() == ChooseRifle) {
            NextTutorialStep();
        }
        break;
    case 0x6e6446f5:
        if (TutorialStep() == ChooseRifle) {
            ShowMessage(TutorialTitle, "IDS_TUTORIAL_CHOOSE_RIFLE", false);
        }
        break;
    case -0x72ce08c6:
        if (TutorialStep() == BackToMap) {
            NextTutorialStep();
        }
        break;
    case 0x40067282:
        if (TutorialStep() == BackToMap) {
            ShowMessage(TutorialTitle, "IDS_TUTORIAL_MAP_RANDOM_INFORMANTS", true);
        } else if (TutorialStep() == ShowRandomInformants) {
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
    // the original is a switch (lowered to the 0x66 bit test)
    switch (CBH_Player::GetInstance()->GetTutorialState()) {
    case BuyBullets:
    case BulletsBought:
    case BuyWeapon:
    case WeaponBought:
        return true;
    default:
        return false;
    }
}

bool CTutorialManager::IsTabLocked(CShopWindow::ETab tab)
{
    int state = TutorialStep();
    if (state == BuyBullets) {
        return tab != CShopWindow::TAB_AMMO;
    }
    if (state == BuyWeapon) {
        return tab != CShopWindow::TAB_WEAPONS;
    }
    return false;
}

bool CTutorialManager::IsShowHome()
{
    return (unsigned int)(TutorialStep() - CustomizeAvatar) < 3;
}

int CTutorialManager::GetCurrentShopTab()
{
    // the original is a switch over the shop states
    switch (TutorialStep()) {
    case BuyBullets:
    case BulletsBought:
    case FirstMap:
    case SecondMission:
        return 3;
    case BuyWeapon:
    case WeaponBought:
        return 1;
    default:
        return 1;
    }
}

bool CTutorialManager::CanCloseShop()
{
    // the original lists the shop states one by one (a jump table over 1..6)
    switch (TutorialStep()) {
    case BuyBullets:
        return false;
    case BulletsBought:
        return true;
    case FirstMap:
        return true;
    case SecondMission:
        return true;
    case BuyWeapon:
        return false;
    case WeaponBought:
        return true;
    default:
        return true;
    }
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
    // the original is a switch (lowered to (state | 4) == 6)
    switch (TutorialStep()) {
    case BulletsBought:
    case WeaponBought:
        return true;
    default:
        return false;
    }
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
    if (TutorialStep() == BackToMap && informant == 0) {
        return true;
    }
    return false;
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
    if (TutorialStep() == BuyWeapon && weapon == 0) {
        return true;
    }
    return false;
}

bool CTutorialManager::IsWeaponButtonBlink(int button)
{
    if (TutorialStep() == ChooseRifle && button == 0) {
        return true;
    }
    return false;
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
