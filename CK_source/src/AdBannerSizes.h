// PROVISIONAL (created by weapon): the original header of these tables is not known.
// AdBannerSizes.h - per-unit ad banner tables of CK (phone, phone retina, pad, pad half; -1: unused).
// 21 CK units (App.cpp, CGameApp.cpp, CDH_Weapon.cpp, CSkyBox.cpp, CSaveManager.cpp, ...) carry a private
// 64 byte copy that their static initializer fills with 320, 640, 768, -1 / 50, 100, 90, -1 /
// 20, 40, -1, -1 / 20, 40, -1, -1. CKZ (src/AdBannerSizes.h) has only the first two rows (512 / 80
// instead of -1). Four 16 byte statics, combined by GlobalMerge into one __MergedGlobals block.
// Names of the last two rows are guesses.
#ifndef ADBANNERSIZES_H
#define ADBANNERSIZES_H

/** Ad banner widths: phone, phone retina, pad, pad half. */
struct AdBannerWidths {
    AdBannerWidths()
    {
        m_value[0] = 320;
        m_value[1] = 640;
        m_value[2] = 768;
        m_value[3] = -1;
    }

    int m_value[4];
};

/** Ad banner heights, same order as the widths. */
struct AdBannerHeights {
    AdBannerHeights()
    {
        m_value[0] = 50;
        m_value[1] = 100;
        m_value[2] = 90;
        m_value[3] = -1;
    }

    int m_value[4];
};

/** Ad banner horizontal margins, same order as the widths. */
struct AdBannerMarginsX {
    AdBannerMarginsX()
    {
        m_value[0] = 20;
        m_value[1] = 40;
        m_value[2] = -1;
        m_value[3] = -1;
    }

    int m_value[4];
};

/** Ad banner vertical margins, same order as the widths. */
struct AdBannerMarginsY {
    AdBannerMarginsY()
    {
        m_value[0] = 20;
        m_value[1] = 40;
        m_value[2] = -1;
        m_value[3] = -1;
    }

    int m_value[4];
};

/** This unit's copies (filled by the static initializer). */
static AdBannerWidths s_adBannerWidths;
static AdBannerHeights s_adBannerHeights;
// Some units (CSaveManager.cpp) carry only the first two tables (32 bytes): they define
// AD_BANNER_SIZES_NO_MARGINS before the include. (added by gserve2)
#ifndef AD_BANNER_SIZES_NO_MARGINS
static AdBannerMarginsX s_adBannerMarginsX;
static AdBannerMarginsY s_adBannerMarginsY;
#endif

#endif
