#ifndef CSHOPWINDOW_H
#define CSHOPWINDOW_H

#include "BaseDialog.h"
#include "CBaseScreen.h"

class ScrollingContainer;
class CIAPCurrency;

class CTabWindow;

/** Original 1.5.2 layout: CBaseScreen + tab strip and honeycomb images, size 0x104. */
class CShopWindow : public CBaseScreen {
public:
    enum ETab { TAB_BANK, TAB_WEAPONS, TAB_ENERGY, TAB_AMMO, TAB_OTHER, TAB_COUNT };

    CShopWindow();
    virtual ~CShopWindow();
    virtual void OnEasyStarted();
    virtual void OnClosed();
    virtual void OnGetFocus();
    virtual void OnLostFocus();
    virtual void Paint(ICGraphics2d& graphics);
    virtual void OnEnterForeground();
    virtual void AddModal(Window* window);  // vtable slot 48 (+0xb8)
    void SetTab(int tab);
    int GetTab();
    void SetTabLock(int tab, bool locked);
    int AddTab(XString name, Window* window, const char* image, bool locked, int padding);
    int GetTabLeft();
    int GetTabTop();
    static Window* AddShopButton(Window* parent, const XString& text, int x, int y,
                                 int width, int height, int command, int argument);
    enum EIAPCommand {
        IAPPurchaseComplete = -0x41cbb99a,
        IAPPurchaseCancelled = 0x5f9d3f54,
        IAPPurchaseFailed = 0x659570dc
    };
    static void AddCurrencyRows(Window* parent, int y, int command);
    static CIAPCurrency* FindCurrency(int catalogIndex);
    static void ShowCurrencyResult(int command, CIAPCurrency* item);

protected:
    CTabWindow* m_tabs;             // 0xd8
    ImageRes m_honeycombTop;        // 0xdc
    ImageRes m_honeycombBottom;     // 0xe4
    bool m_unknownEc;               // 0xec cleared by OnClosed / AddModal
    int m_tabIndices[TAB_COUNT]; // 0xf0 shop tab -> CTabWindow index (-1: none)
};

#endif
