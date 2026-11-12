#ifndef CBH_NETWORKWINDOW_H
#define CBH_NETWORKWINDOW_H

#include "CBaseScreen.h"

class CBH_WeaponButton;
class CDH_Weapon;
class CImageButton;
class ICRenderSurface;

class CBH_NetworkWindow : public CBaseScreen {
public:
    CBH_NetworkWindow();
    virtual ~CBH_NetworkWindow();
    virtual void Init();
    virtual void Update();
    virtual void Paint(ICGraphics2d& graphics);
    virtual void OnGetFocus();
    virtual void OnCommand(Event& event);
    virtual void ReleaseResources();
    virtual void OnPointerEvent(Event& event);
    virtual void OnEasyStarted();
    virtual void Shutdown();
    virtual void OnLostFocus();
    virtual void OnEnterForeground();
    virtual void OnStateInterrupt(int state, Window* window);
    virtual void OnStartupEasy();

    static CBH_NetworkWindow* GetInstance();
    void SwapWeapons();
    void UpdateWeaponNames();
    void UpdateOtherText();

private:
    void CreateAvatarWindow();
    void CreateInfoWindow();
    void CreateBarsWindow();
    void CreateWeaponWindow(int slot, const char* title, unsigned int command);
    void CreateMenuBar();
    CImageButton* CreateSwapWeaponsWindow(unsigned int command);

    static CBH_NetworkWindow* m_pInstance;

    ICRenderSurface* m_upperBackground; // 0xd8
    CBH_MenuButton* m_avatar;
    CBH_ProgressBar* m_experience;
    CBH_ProgressBar* m_energy;
    CBH_ProgressBar* m_money;
    CBH_ProgressBar* m_creditsBar;
    CBH_WeaponButton* m_weaponButtons[2];
    TextWindow* m_profileInfo;
    TextWindow* m_inventory;
    TextWindow* m_weaponLabels[2];
    CDH_Weapon* m_weapons[2];
    bool m_showingShop;
    unsigned char m_reserved111[0xb];
    Window* m_notificationLayer;
    CBH_MenuButton* m_swapButton;
};

#endif
