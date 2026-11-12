// TapjoyInterface.h - C++ entry points of the Tapjoy integration (created by b033x).
#ifndef TAPJOYINTERFACE_H
#define TAPJOYINTERFACE_H

#include "CStrWChar.h"

#ifdef __OBJC__
@class TapjoyManager;
#else
typedef struct objc_object TapjoyManager;
#endif

class TapjoyInterface {
public:
    /** Last featured app reported by the SDK (fields follow TJCFeaturedAppModel). */
    struct FeaturedAppInfo {
        CStrWChar m_cost;              // 0x00
        CStrWChar m_storeID;           // 0x10 (not filled by TapjoyManager)
        CStrWChar m_name;              // 0x20
        CStrWChar m_description;       // 0x30
        int m_amount;                                                  // 0x40
        CStrWChar m_iconURL;           // 0x44
        CStrWChar m_redirectURL;       // 0x54
        int m_maxTimesToDisplayThisApp;                                // 0x64
        CStrWChar m_fullScreenAdURL;   // 0x68 (not filled by TapjoyManager)
    };

    /** `currencyId` is not used on iOS (the game passes the app id there). */
    static void initialize(const char* appId, const char* padAppId, const char* secretKey, bool enableVideoAds);
    static void initializeFromBundle();
    static void prepareToDestroy();
    static void destroy();
    static bool getFeaturedAppQueryState();
    static FeaturedAppInfo* getFeaturedApp();
    static void openTapjoyInterface();
    static void tick(int elapsedMillis);
    static unsigned char interfaceIsOpen();
    static unsigned char canDisplayInterface();
    static unsigned int getRemainingTapjoyPoints();
    static unsigned char consumeTapjoyPoints(unsigned int points);
    static unsigned int convertPointsToGameCurrency(unsigned int points);
    static void onResume();
    /** `callback(began, context)`: 1 when a video ad starts, 0 when it closes. */
    static void registerVideoCallback(void (*callback)(unsigned char, void*), void* context);

    static TapjoyManager* m_pTapjoy;
    static FeaturedAppInfo* m_pFeaturedAppInfo;
    static bool m_FeaturedAppQueryInProgress;
};

#endif
