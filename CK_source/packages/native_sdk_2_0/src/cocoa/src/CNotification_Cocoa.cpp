// CNotification_Cocoa.cpp - routes ICNotification calls to the registered push singleton.
#include "CNotification_Cocoa.h"
#include "CPushNotification_Cocoa.h"

ICNotification* ICNotification::GetInstance()
{
    void* found = NULL;
    CApplet::m_pApp->m_singletons->Find(SingletonId, &found);
    ICNotification* instance = static_cast<ICNotification*>(found);
    if (instance == NULL) {
        instance = new CNotification_Cocoa();
    }
    return instance;
}

CNotification_Cocoa::~CNotification_Cocoa()
{
}

unsigned char CNotification_Cocoa::Register(Type type, const wchar_t* appKey, const wchar_t* appSecret,
                                            void* context, Callback callback)
{
    if (type == Type_Push) {
        m_push = CPushNotification_Cocoa::GetInstance();
        if (m_push != NULL) {
            return m_push->Register(appKey, appSecret, context, callback);
        }
    }
    return 0;
}

unsigned char CNotification_Cocoa::IsEnabled(Type type)
{
    if (type == Type_Push && m_push != NULL) {
        return m_push->IsEnabled();
    }
    return 0;
}

unsigned char CNotification_Cocoa::GetIdentity(Type type, CStrWChar& identity)
{
    if (type == Type_Push && m_push != NULL) {
        return m_push->GetIdentity(identity);
    }
    return 0;
}
