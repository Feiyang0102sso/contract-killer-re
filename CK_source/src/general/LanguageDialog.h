// LanguageDialog.h - first start dialog that lets the player choose the game language.
#ifndef LANGUAGEDIALOG_H
#define LANGUAGEDIALOG_H

#include "SimpleDialog.h"

class SelectLanguageDialog : public SimpleDialog {
public:
    SelectLanguageDialog();

    virtual ~SelectLanguageDialog()
    {
    }

    virtual void OnStartupEasy();
    virtual void Init();
    virtual void Shutdown();
    virtual ItemFonts ItemFontsByIndex(int index) const;
    virtual void ClickItem(int index, bool byPointer);
    virtual void AdjustByDefRect();
    virtual void InitCenter();

    CFont* m_itemFont;            // 0x170
    CFont* m_selectedItemFont;    // 0x174
    int m_selectedLanguage;                                     // 0x178 -1: none chosen
};

#endif
