#ifndef CTUTORIALMANAGER_H
#define CTUTORIALMANAGER_H

#include "WindowBase.h"
#include "CShopWindow.h"

class CTutorialManager : public Window {
public:
    CTutorialManager();
    virtual void OnCommand(Event& event);
    virtual void Update();

    static bool IsPlayerInTutorial();
    static bool IsAutoStartMission();
    static bool IsShowShop();
    bool IsTabLocked(CShopWindow::ETab tab);
    static bool IsShowHome();
    static int GetCurrentShopTab();
    static bool CanCloseShop();
    static bool CanCloseNetwork();
    static bool CanCloseWeaponChoice();
    static bool CanChooseWeapon();
    static bool IsHauntVisible(const XString& location);
    static bool IsCameraVisible(int camera);
    static bool IsInstantInformantVisible(int informant);
    static bool IsMissionInformantVisible(int informant);
    static bool IsHomeVisible();
    static bool IsShopCloseBlink();
    static bool IsNetworkCloseBlink();
    static bool IsWeaponChoiceCloseBlink();
    static bool IsWeaponChoicePressBlink();
    static bool IsAvatarChoiceCloseBlink();
    static bool IsAvatarChoicePressBlink();
    static bool IsMissionInformantBlink(int informant);
    bool IsInstantInformantBlink(int informant);
    static bool IsMapShopBlink();
    static bool IsMapNetworkBlink();
    static bool IsAmmoBlink(int ammo);
    bool IsWeaponBlink(int weapon);
    bool IsWeaponButtonBlink(int button);
    bool IsTutorialInstantInformant(int informant);
    static bool IsMenuBarBlocked();
    static bool IsShowObjective();

    void NextTutorialStep();
    void ShowStateInitMessage();
    void ShowStateBaseMessage();
    void ShowMessage(const char* title, const char* message, bool once);

private:
    const char* m_lastMessage;
};

#endif
