// CAutoArrangedWindow.h - lays out child windows in equal-sized cells.
#ifndef CAUTOARRANGEDWINDOW_H
#define CAUTOARRANGEDWINDOW_H

#include "Vector_ss.h"
#include "WindowBase.h"

class CAutoArrangedWindow : public Window {
public:
    CAutoArrangedWindow(int width, int height);
    virtual ~CAutoArrangedWindow();

    bool Add(Window* window);
    bool UpdateContentLayout();
    void SetOneLineMode(bool oneLine);
    void SetItemSpacing(int spacing);
    friend class CRegularShop;  // added by layout: CreateItemsContainer sets m_maxRows (0xb8)
    friend class CIngameShop;   // added by layout: CreateItemsContainer sets m_maxColumns (0xbc), CreateAmmoTab reads m_items

private:
    Vector<Window*> m_items;
    bool m_oneLine;
    int m_maxRows;
    int m_maxColumns;
    bool m_fixedSpacing;
    int m_spacing;
};

typedef char AutoArrangedWindowSizeCheck[sizeof(CAutoArrangedWindow) == 0xc8 ? 1 : -1];

#endif
