// CPushNotification_Cocoa.h - remote notifications: Apple registration, then the provider
// (a custom callback or Urban Airship). One instance per application (singleton).
#ifndef CPUSHNOTIFICATION_COCOA_H
#define CPUSHNOTIFICATION_COCOA_H

#include <wchar.h>
#include "CSingleton.h"
#include "CStrWChar.h"

// WCharToNSString() is file-static in CPushNotification_Cocoa_mm.mm (internal linkage in the original)

class CPushNotification_Cocoa : public CSingleton {
public:
    /** Singleton id in CApp::m_singletons. */
    enum { SingletonId = 0x82c89f37 };

    /** Registration progress (m_state). */
    enum State {
        State_Idle = 0,
        State_RegisteringWithApple = 1,
        State_Registered = 2,
        State_Failed = 3
    };

    /** Provider hook: (context, device token), returns 1 on success. */
    typedef unsigned char (*Callback)(void* context, const CStrWChar& token);

    /** The application's push notification object, created on first use. */
    static inline CPushNotification_Cocoa* GetInstance()
    {
        void* found = NULL;
        CApplet::m_pApp->m_singletons->Find(SingletonId, &found);
        CPushNotification_Cocoa* instance = static_cast<CPushNotification_Cocoa*>(found);
        if (instance == NULL) {
            instance = new CPushNotification_Cocoa();
        }
        return instance;
    }

    CPushNotification_Cocoa();
    virtual ~CPushNotification_Cocoa();

    /** Start registration (only from State_Idle); appKey / appSecret are the provider credentials. */
    virtual unsigned char Register(const wchar_t* appKey, const wchar_t* appSecret, void* context, Callback callback);
    /** Registered and a device token is known. */
    virtual unsigned char IsEnabled();
    /** The device token. */
    virtual unsigned char GetIdentity(CStrWChar& identity);
    unsigned char Cancel();
    unsigned char SetBadgeNumber(unsigned int badgeNumber);
    unsigned char GetBadgeNumber(unsigned int& badgeNumber);

    void RegisterWithApple();
    /** Answer to registerForRemoteNotificationTypes: (deviceToken is the hex token text). */
    void RegisterWithAppleCallback(unsigned char success, const char* deviceToken);
    void RegisterWithProvider();
    /** A remote notification arrived (`message` is its text). */
    void NotificationCallback(unsigned char launchedApplication, const wchar_t* message);
    void RegisterWithProviderUrbanAirship();

private:
    int m_state;                                                // 0x08 State
    void* m_context;                                            // 0x0c
    Callback m_callback;                                        // 0x10 NULL: Urban Airship
    CStrWChar m_appKey;         // 0x14
    CStrWChar m_appSecret;      // 0x24
    CStrWChar m_deviceToken;    // 0x34
    CStrWChar m_message;        // 0x44 last received notification text
};

#endif
