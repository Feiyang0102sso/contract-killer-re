#ifndef CTUTORIALMANAGER_H
#define CTUTORIALMANAGER_H

#include "WindowBase.h"
#include "CShopWindow.h"

class CTutorialManager : public WindowTransparent {
public:
    CTutorialManager();
    virtual void OnCommand(Event& event);
    virtual void Update();

    bool IsPlayerInTutorial();
    bool IsAutoStartMission();
    bool IsShowShop();
    bool IsTabLocked(CShopWindow::ETab tab);
    bool IsShowHome();
    int GetCurrentShopTab();
    bool CanCloseShop();
    bool CanCloseNetwork();
    bool CanCloseWeaponChoice();
    bool CanChooseWeapon();
    bool IsHauntVisible(const XString& location);
    bool IsCameraVisible(int camera);
    bool IsInstantInformantVisible(int informant);
    bool IsMissionInformantVisible(int informant);
    bool IsHomeVisible();
    bool IsShopCloseBlink();
    int IsNetworkCloseBlink();
    bool IsWeaponChoiceCloseBlink();
    int IsWeaponChoicePressBlink();
    bool IsAvatarChoiceCloseBlink();
    int IsAvatarChoicePressBlink();
    int IsMissionInformantBlink(int informant);
    bool IsInstantInformantBlink(int informant);
    bool IsMapShopBlink();
    int IsMapNetworkBlink();
    int IsAmmoBlink(int ammo);
    bool IsWeaponBlink(int weapon);
    bool IsWeaponButtonBlink(int button);
    bool IsTutorialInstantInformant(int informant);
    bool IsMenuBarBlocked();
    bool IsShowObjective();

    void NextTutorialStep();
    void ShowStateInitMessage();
    void ShowStateBaseMessage();
    void ShowMessage(const char* title, const char* message, bool once);

private:
    const char* m_lastMessage;
};

#endif
