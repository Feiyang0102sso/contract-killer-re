#ifndef CDAILYDEALWINDOW_H
#define CDAILYDEALWINDOW_H

#include "CDialogWindow.h"

class CFont;

class CDailyDealWindow : public CTextDialog {
public:
    CDailyDealWindow();
    virtual void OnCommand(Event& event);

private:
    void CreateUI();
    Window* CreateItemPriceWindow();
    Window* CreatePriceCountWindow(int price, CFont* font);
    Window* CreateTimerWindow();
    void HandleBuyButtonPressed();
};

#endif
