#ifndef CDAILYDEALWINDOW_H
#define CDAILYDEALWINDOW_H

#include "CDialogWindow.h"

class CItem;
class CFont;

class CDailyDealWindow : public CDialogWindow {
public:
    CDailyDealWindow();
    virtual void OnCommand(Event& event);

private:
    void CreateUI();
    void CreateItemPriceWindow();
    Window* CreatePriceCountWindow(int price, CFont* font);
    void CreateTimerWindow();
    void HandleBuyButtonPressed();

    CItem* m_item;
};

#endif
