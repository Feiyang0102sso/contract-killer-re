// CNGSPushNotificationDetails.cpp - notification display data for message envelopes.
#include "CNGSPushNotificationDetails.h"
#include "CObjectMapObject.h"

CNGSPushNotificationDetails::~CNGSPushNotificationDetails()
{
}

CObjectMapObject* CNGSPushNotificationDetails::GetObjectMapRepresentation() const
{
    CObjectMapObject* details = new CObjectMapObject();
    if (m_buttonText.GetLength() != 0) {
        details->addEntry(CStrWChar("buttonText"), new CObjectMapString(m_buttonText));
    }
    if (m_alertText.GetLength() != 0) {
        details->addEntry(CStrWChar("alertText"), new CObjectMapString(m_alertText));
    }
    return details;
}
