// CShopItemButton.h - a shop item with its stock, price and offer state.
#ifndef CSHOPITEMBUTTON_H
#define CSHOPITEMBUTTON_H

#include "CBH_Button.h"

class CItem;
class CImageButton;
class CItemManager;
class TextWindow;

class CShopItemButton : public CBH_Button {
public:
    CShopItemButton(CItemManager* manager, int type, const XString& name, int unlockLevel,
                    int context, int itemId, const char* image);
    virtual ~CShopItemButton();
    virtual void Paint(ICGraphics2d& graphics);
    virtual void Update();
    virtual void OnCommand(Event& event);

    bool IsItemPurchased();
    int GetUnlockLevel();
    void SetAlwaysActive(bool active);
    void SetActiveImage(const char* image);
    void UpdateLabel();

private:
    friend class CIngameShop;  // added by layout: original reads m_type (0xd8) and m_itemId (0xe4)
    friend class CRegularShop;  // added by layout: original OnCommand reads m_type/m_itemId/m_disabled
    class Label;

    Label* CreateLabel();
    void UpdateGoPills();
    void UpdateMisc();

    Label* m_label;                     // 0xd4
    int m_type;
    int m_unlockLevel;
    int m_price;
    int m_itemId;
    bool m_disabled;
    bool m_alwaysActive;
    XString m_name;
    XString m_priceText;
    TextWindow* m_title;
    CFont* m_priceFont;
    CFont* m_titleFont;
    ICRenderSurface* m_activeImage;
    ICRenderSurface* m_itemImage;
    ImageRes m_background;
    ImageRes m_currencyIcon;
    CItemManager* m_manager;
    CItem* m_item;
    CImageButton* m_saleMarker;
    Window* m_dealTimer;
    bool m_blink;
};

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char ShopItemButtonSizeCheck[sizeof(CShopItemButton) == 0x12c ? 1 : -1];
#endif

#endif
