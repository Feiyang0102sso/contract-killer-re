// CFactionsWindow.h - faction standings and the player's overall rating.
#ifndef CFACTIONSWINDOW_H
#define CFACTIONSWINDOW_H

#include "CBaseScreen.h"
#include "Vector_ss.h"
#include "tinyxml.h"

class CFactionsWindow : public CBaseScreen {
public:
    CFactionsWindow();
    virtual ~CFactionsWindow();
    virtual void Paint(ICGraphics2d& graphics);
    virtual void OnClosed();

private:
    struct TRougueTitle {
        TRougueTitle() : m_min(0), m_max(0) {}

        int m_min;
        int m_max;
        XString m_title;
    };

    void LoadTitleStrings();
    int CalculateOverallRating();
    XString GetTitleForRating(int rating);

    Vector<TRougueTitle*> m_titles;
    ImageRes m_background;
    TiXmlDocument m_titleConfig;
};

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char FactionsWindowSizeCheck[sizeof(CFactionsWindow) == 0x138 ? 1 : -1];
#endif

#endif
