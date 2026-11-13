#ifndef CREGULARSHOP_H
#define CREGULARSHOP_H

#include "CShopWindow.h"
#include "Vector_ss.h"
#include "CImageButton.h"

class CItem;
class CItemManager;
class CAdColonyButton;
class CShopItemButton;

/** Weapon filter toggle: dimmed when its bit is not set in the shared filter mask. */
class CFilterButton : public CImageButton {
public:
    CFilterButton(const XString& text, const char* image, const int& filterMask);
    virtual ~CFilterButton() {}
    virtual void Update() {}
    virtual void Paint(ICGraphics2d& graphics);

    const int* m_filterMask;        // 0xfc points at the owner's filter mask
};

class CScrollWindow;
class CAutoArrangedWindow;
class CDH_Weapon;
class CDialogWindow;

/** Original 1.5.2 layout: CShopWindow (0x104) + filters and tab pages, operator new 0x124. */
class CRegularShop : public CShopWindow {
public:
    CRegularShop();
    virtual ~CRegularShop();
    virtual void Init();
    virtual void Update();
    virtual void OnCommand(Event& event);
    virtual void OnGetFocus();
    virtual void OnLostFocus();

    static CRegularShop* GetInstance(bool create);
    static void Show(ETab tab);
    void UpdateTabLock();
    void SwitchWeaponFilter(int filter);  // public: CBH_GPSMap::ShowShopWithWeapon calls it
    friend class CBH_GPSMap;  // added by gps: CBH_GPSMap::ShowShop calls TryOpenTapJoy
    friend class CShopItemInfoWindow;

private:
    void UpdateCurrenciesPage();
    void CreateWeaponsPage(CScrollWindow* scroll, CAutoArrangedWindow* container, unsigned int filter);
    CAdColonyButton* CreateCustomButton(Window* parent, bool notification, const char* image,
                                               const char* activeImage, const XString& caption, unsigned int command);
    CAutoArrangedWindow* CreatePage(CScrollWindow* scroll, CItemManager* manager, CAutoArrangedWindow* container,
                                    int maxItems);
    void UpdateAmmoPage();
    void CreateOtherTab();
    CAutoArrangedWindow* CreateItemsContainer(int width);
    void CreateOthersPage();
    CShopItemButton* CreateTapjoyWeaponButton();
    void TryOpenTapJoy();
    Window* CreateWeaponDescription(CDH_Weapon* weapon);
    static Window* CreateWeaponStats(CDH_Weapon* weapon);
    static void AddIconDescription(Window* parent, const char* icon, const XString& text, int column, int row);
    static void AddStatsBar(Window* parent, int value, int maximum, int column, int row, bool infinite);

    static CRegularShop* m_instance;
    int m_weaponFilter;                 // 0x104 bit mask of the weapon filter buttons
    int m_otherFilter;                  // 0x108 bit mask of the "other" filter buttons
    CDialogWindow* m_waitDialog;        // 0x10c IAP "loading" dialog
    CScrollWindow* m_bankPage;          // 0x110
    CScrollWindow* m_ammoPage;          // 0x114
    CScrollWindow* m_weaponsPage;       // 0x118
    CScrollWindow* m_othersPage;        // 0x11c
    CAdColonyButton* m_moreGamesButton; // 0x120
};

#endif
