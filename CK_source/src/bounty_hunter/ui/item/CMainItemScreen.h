#ifndef CMAINITEMSCREEN_H
#define CMAINITEMSCREEN_H

#include "CBaseScreen.h"
#include "Vector_ss.h"

class CAmmo;
class CBH_WeaponButton;
class CDH_Weapon;
class CEquipItemButton;
class CEquipAmmoButton;
class CScrollWindow;

class CMainItemScreen : public CBaseScreen {
public:
    CMainItemScreen(int type, int slot);
    // CK: implicit virtual destructor (D1 holds the whole body, as in the original).
    virtual void Paint(ICGraphics2d& graphics);
    virtual void Update();
    virtual void OnCommand(Event& event);
    virtual void OnLostFocus();
    virtual void OnEasyStarted();
    virtual void OnEnterForeground();

    int GetCurrentItemInSlotId();
    int GetAnotherSlotItemId();
    bool ItemAvailable(int id);
    void SetCurrentItem(int id);
    void SetCurrentItemToSlot();

private:
    Window* CreateChoiseScrolling();
    void AddItems(CScrollWindow* chooser, const Vector<CDH_Weapon*>& weapons);
    Window* CreateStatsButton();
    Window* CreatePlayerStatsWindow();
    void UpdatePlayerStats();
    void HandleStatsButtonPressed();
    Window* CreateItemButtons();
    CEquipItemButton* CreateSilencerButton();
    CEquipItemButton* CreateHitechVisorButton();
    CEquipItemButton* CreateCyberScope();
    void UpdateAmmoButtons();
    void UpdateSilencerButton();
    void UpdateHitechVisorButton();
    void UpdateCyberScopeButton();
    void HandleBulletButtonPressed(const CAmmo* ammo);
    void HandleSilencerButtonPressed();
    void HandleHitechVisorButtonPressed();
    void HandleCyberScopeButtonPressed();
    void ShowBuyWeaponWindow(int id);
    void SetImageItem(int id);
    static Window* createWeaponImage(ImageWindow*& weaponImage);
    void SetNameItem(int id);
    Window* CreateButton(const CAmmo* ammo);

    ImageWindow* m_weaponImage;          // 0xd8
    TextWindow* m_weaponName;
    ImageRes m_background;
    Window* m_chooser;                   // 0xe8
    Window* m_itemButtons;
    Window* m_equipmentPanel;
    CEquipItemButton* m_silencer;
    CEquipItemButton* m_hitechVisor;
    CEquipItemButton* m_cyberScope;
    CDH_Weapon* m_weapon;                // 0x100
    Vector<CEquipAmmoButton*> m_ammoButtons; // 0x104
    int m_type;                          // 0x114
    int m_slot;
    int m_item;
};

#endif
