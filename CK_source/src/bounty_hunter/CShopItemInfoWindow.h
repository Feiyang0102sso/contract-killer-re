#ifndef CSHOPITEMINFOWINDOW_H
#define CSHOPITEMINFOWINDOW_H

#include "CBloodWindow.h"

class AnimatedWindow;
class CImageButton;
class CItem;
class CItemManager;
class LabelWithIcon;

class CShopItemInfoWindow : public CBloodWindow {
public:
    enum ESource { SOURCE_SHOP = 0, SOURCE_RANDOM_WEAPON_MISSION = 2 };

    CShopItemInfoWindow(CItemManager* manager, int type, int id, const XString& stats,
                        bool hideEquip, ESource source);
    virtual ~CShopItemInfoWindow();
    virtual void Paint(ICGraphics2d& graphics);
    virtual void OnCommand(Event& event);
    virtual void OnClosed();
    virtual void Update();

    void SetExtraButton(const char* image, const XString& text, unsigned int command);

private:
    CItem* GetItem() const;
    CImageButton* CreateButton(const char* image, const XString& text, int align, int command);
    bool CanShowBuyButton();
    bool CanShowEquipButton();
    static Window* CreateTextArea(const XString& text, int type);
    LabelWithIcon* CreatePriceLabel();
    void UpdateAmmoLabel();
    void UpdateAmmoLabel(int quantity, const char* icon);
    void HandleRedurectToMainItemScreen();
    void ShowEquipWindow(int unused);
    bool purchaseForPoints();
    bool purchaseForCredits();
    void ShowArmorDialog();
    void ShowFullEnergyDialog();
    void ShowLowMoneyDialog();
    void Purchase();
    bool PurchaseWithCurrency(CItem* item, int price, bool credits);

    ImageRes m_redBackground;
    ICRenderSurface* m_itemImage;
    CImageButton* m_extraButton;
    AnimatedWindow* m_count;
    AnimatedWindow* m_countIncrease;
    bool m_refreshQuantity;
    int m_displayedCredits;
    CImageButton* m_buyButton;
    XString m_reserved144;
    XString m_reserved148;
    int m_itemId;
    int m_type;
    bool m_hideEquip;
    CItemManager* m_manager;
    int m_quantity;
    unsigned int m_lastPurchaseMs;
    unsigned int m_lastWarningTime;
    bool m_confirmArmor;
    ESource m_source;
};

#endif
