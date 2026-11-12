// ICNotification.h - registration and identity of platform push notifications.
#ifndef ICNOTIFICATION_H
#define ICNOTIFICATION_H

#include <wchar.h>
#include "CSingleton.h"

                                                                           

class CStrWChar;

class ICNotification : public CSingleton {
public:
    enum { SingletonId = 0x0581a7b1 };

    /** Kind of notification to register (CGameApp registers push with 1). */
    enum Type {
        Type_Local = 0,
        Type_Push = 1
    };

    /**
     * Registration hook: called with the caller context and the platform object
     * (push: device token). Returns 1 on success.
     */
    typedef unsigned char (*Callback)(void* context, const CStrWChar& token);

    /** Platform singleton (CNotification_Cocoa). */
    static ICNotification* GetInstance();

    ICNotification() : CSingleton(SingletonId) {}
    virtual ~ICNotification() {}

    /** Push: register with the provider using its application key / secret. */
    virtual unsigned char Register(Type type, const wchar_t* appKey, const wchar_t* appSecret,
                                   void* context, Callback callback) = 0;
    virtual unsigned char IsEnabled(Type type) = 0;
    /** Push device token. */
    virtual unsigned char GetIdentity(Type type, CStrWChar& identity) = 0;
};

       

#endif
