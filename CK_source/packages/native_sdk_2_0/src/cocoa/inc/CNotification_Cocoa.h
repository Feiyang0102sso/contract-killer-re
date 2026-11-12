// CNotification_Cocoa.h - iOS push notification dispatcher.
#ifndef CNOTIFICATION_COCOA_H
#define CNOTIFICATION_COCOA_H

#include "ICNotification.h"

class CPushNotification_Cocoa;

class CNotification_Cocoa : public ICNotification {
public:
    CNotification_Cocoa()
    {
        m_push = NULL;
    }

    virtual ~CNotification_Cocoa();

    virtual unsigned char Register(Type type, const wchar_t* appKey, const wchar_t* appSecret,
                                   void* context, Callback callback);
    virtual unsigned char IsEnabled(Type type);
    virtual unsigned char GetIdentity(Type type, CStrWChar& identity);

private:
    CPushNotification_Cocoa* m_push;        // 0x08 application singleton, not owned
};

#endif
