// PROVISIONAL (created by b057x): owner src/CFontMgr.cpp not written yet
// CFontMgr.h - the game's fonts (singleton); only what src/general uses.
// Restored by b063; the preceding PROVISIONAL is historical.
#ifndef CFONTMGR_H
#define CFONTMGR_H

#include "CSingleton.h"

// b074i: named enum from the original mangled symbol of SGButton::SetText; default font 15 is proven; other values to be added from real calls.
enum eGameFont {
    GAME_FONT_DEFAULT = 15
};

                                                                         
class CFont;
       

class CFontMgr : public CSingleton {
public:
    enum { SingletonId = 0x70990b0e };
    enum { FontCount = 24 };

    /** The font manager of the running application, created on first use. */
    static inline CFontMgr* GetInstance()
    {
        void* found = NULL;
        CApplet::m_pApp->m_singletons->Find(SingletonId, &found);
        CFontMgr* instance = (CFontMgr*)found;
        if (instance == NULL) {
            instance = new CFontMgr();
        }
        return instance;
    }

    CFontMgr();
    virtual ~CFontMgr();

    /** Font number `index` of the font table. */
    CFont* GetFont(int index);
    void FreeFont(int index);

    // Former m_data08[0x8c - 8] was the CKZ size. CK clears 0x60 bytes at +8.
    CFont* m_fonts[FontCount];
};

/** Two resource names per font, read through the original global table symbol. */
struct GameFontInfo {
    const char* m_metrics;
    const char* m_image;
};
extern const GameFontInfo kTableGameFontInfo[CFontMgr::FontCount];

#endif
