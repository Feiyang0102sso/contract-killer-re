#ifndef CNGSPUSHNOTIFICATIONDETAILS_H
#define CNGSPUSHNOTIFICATIONDETAILS_H

#include "CStrWChar.h"

class CObjectMapObject;

/** Optional display strings sent with a push notification envelope. */
class CNGSPushNotificationDetails {
public:
    virtual ~CNGSPushNotificationDetails();
    CObjectMapObject* GetObjectMapRepresentation() const;

    CStrWChar m_buttonText;  // 0x04
    CStrWChar m_alertText;   // 0x14
};

#endif
