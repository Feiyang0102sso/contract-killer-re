#ifndef CINGAMESHOP_H
#define CINGAMESHOP_H

#include "WindowBase.h"
#include "Vector_ss.h"

class CItem;
class CShopItemButton;
class CImageButton;
class CItemManager;
class CScrollWindow;
class CAutoArrangedWindow;
class CBH_ProgressBar;
class CDialogWindow;
class CDH_Weapon;

/** In-mission shop. Original 1.5.2 layout (operator new 0xe0). */
class CIngameShop : public Window {
public:
    enum EItemTabs { Bank, Ammo, GoPills, Medkits, Armor, TabCount };

    CIngameShop();
    virtual ~CIngameShop();
    virtual void OnCommand(Event& event);
    virtual void OnClosed();
    void SetTab(EItemTabs tab);
    void SetCurrentItemButton(CShopItemButton* button);
    static CImageButton* CreateEquipButton();
    static CImageButton* CreateBuyButton();
    void ReleaseTabResources(EItemTabs tab);
    Window* CreateFilterButton(const char* image, EItemTabs tab);

private:
    void CreateUI();
    Window* CreateStatsWindow();
    void UpdateStatsWindow();
    Window* CreateFilterButtons();
    static CAutoArrangedWindow* CreateItemsContainer();
    CAutoArrangedWindow* CreatePage(CScrollWindow* scroll, CItemManager* manager, CItem* item,
                                    CAutoArrangedWindow* container);
    void CreateTab(CItemManager* manager);
    void CreateBankTab();
    void UpdateBankTab();
    void CreateMedkitsTab();
    void CreateAmmoTab();
    CAutoArrangedWindow* GetAmmoForWeapon(CAutoArrangedWindow* container, const CDH_Weapon* weapon);
    void ShowEquipButton();
    void ShowUneqipButton();
    void UpdateEquipUnequipButtonState();
    void HandleBuyButtonPressed();
    void TryToBuyItem(CItemManager* manager, CItem* item);
    void RegisterPurchasingForAchievement(CItem* item);
    void HandleEquipButtonPressed();
    void HandleUnequipButtonPressed();
    void ShowArmorDialog(CItem* item);

    EItemTabs m_tab;                    // 0xa4
    CScrollWindow* m_scroll;            // 0xa8 current tab page
    Window* m_pageHolder;               // 0xac
    Window* m_buyHolder;                // 0xb0
    CImageButton* m_buyButton;          // 0xb4
    Window* m_equipHolder;              // 0xb8
    CImageButton* m_equipButton;        // 0xbc
    CShopItemButton* m_currentItemButton; // 0xc0
    CBH_ProgressBar* m_cashBar;         // 0xc4
    CBH_ProgressBar* m_creditsBar;      // 0xc8
    CDialogWindow* m_waitDialog;        // 0xcc IAP "loading" dialog
    Vector<Window*> m_filterButtons;    // 0xd0
};

#endif
